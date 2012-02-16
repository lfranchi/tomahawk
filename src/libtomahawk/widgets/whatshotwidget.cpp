/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "whatshotwidget.h"
#include "whatshotwidget_p.h"
#include "ui_whatshotwidget.h"

#include <QPainter>
#include <QStandardItemModel>
#include <QStandardItem>

#include "viewmanager.h"
#include "sourcelist.h"
#include "tomahawksettings.h"
#include "RecentPlaylistsModel.h"
#include "ChartDataLoader.h"

#include "audio/audioengine.h"
#include "dynamic/GeneratorInterface.h"
#include "playlist/playlistmodel.h"
#include "playlist/treeproxymodel.h"
#include "widgets/overlaywidget.h"
#include "utils/tomahawkutils.h"

#include "libdavros/davros.h"
#include "utils/logger.h"
#include "pipeline.h"

#define HISTORY_TRACK_ITEMS 25
#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_RESOLVING_TIMEOUT 2500

using namespace Tomahawk;

static QString s_whatsHotIdentifier = QString( "WhatsHotWidget" );


WhatsHotWidget::WhatsHotWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WhatsHotWidget )
    , m_sortedProxy( 0 )
    , m_workerThread( 0 )
{
    ui->setupUi( this );

    ui->albumsView->setFrameShape( QFrame::NoFrame );
    ui->albumsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->stackLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->horizontalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->breadCrumbLeft->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );

    m_crumbModelLeft = new QStandardItemModel( this );
    m_sortedProxy = new QSortFilterProxyModel( this );
    m_sortedProxy->setDynamicSortFilter( true );
    m_sortedProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );

    ui->breadCrumbLeft->setRootIcon( QPixmap( RESPATH "images/charts.png" ) );

    connect( ui->breadCrumbLeft, SIGNAL( activateIndex( QModelIndex ) ), SLOT( leftCrumbIndexChanged(QModelIndex) ) );

    ui->tracksViewLeft->setFrameShape( QFrame::NoFrame );
    ui->tracksViewLeft->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->tracksViewLeft->overlay()->setEnabled( false );
    ui->tracksViewLeft->setHeaderHidden( true );
    ui->tracksViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    TreeProxyModel* artistsProxy = new TreeProxyModel( ui->artistsViewLeft );
    artistsProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
    artistsProxy->setDynamicSortFilter( true );

    ui->artistsViewLeft->setProxyModel( artistsProxy );
    ui->artistsViewLeft->setFrameShape( QFrame::NoFrame );
    ui->artistsViewLeft->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    ui->artistsViewLeft->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->artistsViewLeft->header()->setVisible( true );

    m_playlistInterface = Tomahawk::playlistinterface_ptr( new ChartsPlaylistInterface( this ) );

    m_workerThread = new QThread( this );
    m_workerThread->start();

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(), SIGNAL( finished( QString ) ), SLOT( infoSystemFinished( QString ) ) );

    QTimer::singleShot( 0, this, SLOT( fetchData() ) );
}


WhatsHotWidget::~WhatsHotWidget()
{
    m_workerThread->exit(0);
    m_playlistInterface.clear();
    delete ui;
}


Tomahawk::playlistinterface_ptr
WhatsHotWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
WhatsHotWidget::isBeingPlayed() const
{
    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->artistsViewLeft->playlistInterface() )
        return true;

    if ( AudioEngine::instance()->currentTrackPlaylist() == ui->tracksViewLeft->playlistInterface() )
        return true;

    return false;
}


bool
WhatsHotWidget::jumpToCurrentTrack()
{
    if ( ui->artistsViewLeft->jumpToCurrentTrack() )
        return true;

    if ( ui->tracksViewLeft->jumpToCurrentTrack() )
        return true;

    return false;
}


void
WhatsHotWidget::fetchData()
{
    Tomahawk::InfoSystem::InfoStringHash artistInfo;

    Tomahawk::InfoSystem::InfoRequestData requestData;
    requestData.caller = s_whatsHotIdentifier;
    requestData.customData = QVariantMap();
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( artistInfo );
    requestData.type = Tomahawk::InfoSystem::InfoChartCapabilities;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    tDebug( LOGVERBOSE ) << "WhatsHot: requested InfoChartCapabilities";
}


void
WhatsHotWidget::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != s_whatsHotIdentifier )
        return;

    if ( !output.canConvert< QVariantMap >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "WhatsHot: Could not parse output";
        return;
    }

    QVariantMap returnedData = output.toMap();
    switch ( requestData.type )
    {
        case InfoSystem::InfoChartCapabilities:
        {
            QStandardItem *rootItem= m_crumbModelLeft->invisibleRootItem();
            QVariantMap defaults;
            if ( returnedData.contains( "defaults" ) )
                defaults = returnedData.take( "defaults" ).toMap();
            QString defaultSource = returnedData.take( "defaultSource" ).toString();

            foreach ( const QString label, returnedData.keys() )
            {
                QStandardItem *childItem = parseNode( rootItem, label, returnedData[label] );
                rootItem->appendRow(childItem);
            }

            // Set the default source
            // Set the default chart for each source
            for ( int i = 0; i < rootItem->rowCount(); i++ )
            {
                QStandardItem* source = rootItem->child( i, 0 );
                if ( defaultSource.toLower() == source->text().toLower() )
                {
                    source->setData( true, Breadcrumb::DefaultRole );
                }

                if ( defaults.contains( source->text().toLower() ) )
                {
                    QStringList defaultIndices = defaults[ source->text().toLower() ].toStringList();
                    QStandardItem* cur = source;

                    foreach( const QString& index, defaultIndices )
                    {
                        // Go through the children of the current item, marking the default one as default
                        for ( int k = 0; k < cur->rowCount(); k++ )
                        {
                            if ( cur->child( k, 0 )->text() == index )
                            {
                                cur = cur->child( k, 0 ); // this is the default, drill down into the default to pick the next default
                                cur->setData( true, Breadcrumb::DefaultRole );
                                break;
                            }
                        }
                    }
                }
            }

            m_sortedProxy->setSourceModel( m_crumbModelLeft );
            m_sortedProxy->sort( 0, Qt::AscendingOrder );
            ui->breadCrumbLeft->setModel( m_sortedProxy );
            break;
        }

        case InfoSystem::InfoChart:
        {
            if( !returnedData.contains("type") )
                break;
            const QString type = returnedData["type"].toString();
            if( !returnedData.contains(type) )
                break;
            const QString side = requestData.customData["whatshot_side"].toString();
            const QString chartId = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >().value( "chart_id" );

            m_queuedFetches.remove( chartId );

            ChartDataLoader* loader = new ChartDataLoader();
            loader->setProperty( "chartid", chartId );
            loader->moveToThread( m_workerThread );

            if ( type == "artists" )
            {
                loader->setType( ChartDataLoader::Artist );
                loader->setData( returnedData[ "artists" ].value< QStringList >() );

                connect( loader, SIGNAL( artists( Tomahawk::ChartDataLoader*, QList< Tomahawk::artist_ptr > ) ), this, SLOT( chartArtistsLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::artist_ptr > ) ) );

                TreeModel* artistsModel = new TreeModel( ui->artistsViewLeft );
                artistsModel->setMode( InfoSystemMode );
                artistsModel->setColumnStyle( TreeModel::AllColumns );

                m_artistModels[ chartId ] = artistsModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewArtists( artistsModel );
            }
            else if ( type == "albums" )
            {

                loader->setType( ChartDataLoader::Album );
                loader->setData( returnedData[ "albums" ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >() );

                connect( loader, SIGNAL( albums( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ), this, SLOT( chartAlbumsLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::album_ptr > ) ) );

                AlbumModel* albumModel = new AlbumModel( ui->albumsView );

                m_albumModels[ chartId ] = albumModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewAlbums( albumModel );
            }
            else if ( type == "tracks" )
            {

                loader->setType( ChartDataLoader::Track );
                loader->setData( returnedData[ "tracks" ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >() );

                connect( loader, SIGNAL( tracks( Tomahawk::ChartDataLoader*, QList< Tomahawk::query_ptr > ) ), this, SLOT( chartTracksLoaded( Tomahawk::ChartDataLoader*, QList< Tomahawk::query_ptr > ) ) );

                PlaylistModel* trackModel = new PlaylistModel( ui->tracksViewLeft );
                trackModel->setStyle( TrackModel::Short );

                m_trackModels[ chartId ] = trackModel;

                if ( m_queueItemToShow == chartId )
                    setLeftViewTracks( trackModel );
            }

            QMetaObject::invokeMethod( loader, "go", Qt::QueuedConnection );

            break;
        }

        default:
            return;
    }
}


void
WhatsHotWidget::infoSystemFinished( QString target )
{
    Q_UNUSED( target );
}


void
WhatsHotWidget::leftCrumbIndexChanged( QModelIndex index )
{
    tDebug( LOGVERBOSE ) << "WhatsHot:: left crumb changed" << index.data();
    QStandardItem* item = m_crumbModelLeft->itemFromIndex( m_sortedProxy->mapToSource( index ) );
    if( !item )
        return;
    if( !item->data( Breadcrumb::ChartIdRole ).isValid() )
        return;


    QList<QModelIndex> indexes;
    while ( index.parent().isValid() )
    {
        indexes.prepend(index);
        index = index.parent();
    }


    const QString chartId = item->data( Breadcrumb::ChartIdRole ).toString();

    if ( m_artistModels.contains( chartId ) )
    {
        setLeftViewArtists( m_artistModels[ chartId ] );
        return;
    }
    else if ( m_albumModels.contains( chartId ) )
    {
        setLeftViewAlbums( m_albumModels[ chartId ] );
        return;
    }
    else if ( m_trackModels.contains( chartId ) )
    {
        setLeftViewTracks( m_trackModels[ chartId ] );
        return;
    }

    if ( m_queuedFetches.contains( chartId ) )
    {
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria.insert( "chart_id", chartId );
    /// Remember to lower the source!
    criteria.insert( "chart_source",  index.data().toString().toLower() );

    Tomahawk::InfoSystem::InfoRequestData requestData;
    QVariantMap customData;
    customData.insert( "whatshot_side", "left" );
    requestData.caller = s_whatsHotIdentifier;
    requestData.customData = customData;
    requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( criteria );
    requestData.type = Tomahawk::InfoSystem::InfoChart;
    requestData.timeoutMillis = 20000;
    requestData.allSources = true;

    Davros::debug() << "Making infosystem request for chart of type:" <<chartId;
    Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );

    m_queuedFetches.insert( chartId );
    m_queueItemToShow = chartId;
}


void
WhatsHotWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


QStandardItem*
WhatsHotWidget::parseNode( QStandardItem* parentItem, const QString &label, const QVariant &data )
{
    Q_UNUSED( parentItem );
//     tDebug( LOGVERBOSE ) << "WhatsHot:: parsing " << label;

    QStandardItem *sourceItem = new QStandardItem(label);

    if ( data.canConvert< QList< Tomahawk::InfoSystem::InfoStringHash > >() )
    {
        QList< Tomahawk::InfoSystem::InfoStringHash > charts = data.value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
        foreach ( Tomahawk::InfoSystem::InfoStringHash chart, charts )
        {
            QStandardItem *childItem= new QStandardItem( chart[ "label" ] );
            childItem->setData( chart[ "id" ], Breadcrumb::ChartIdRole );
            if ( chart.value( "default", "" ) == "true")
            {
                childItem->setData( true, Breadcrumb::DefaultRole );
            }
            sourceItem->appendRow( childItem );
        }
    }
    else if ( data.canConvert<QVariantMap>() )
    {
        QVariantMap dataMap = data.toMap();
        foreach ( const QString childLabel,dataMap.keys() )
        {
            QStandardItem *childItem  = parseNode( sourceItem, childLabel, dataMap[childLabel] );
            sourceItem->appendRow( childItem );
        }
    }
    else if ( data.canConvert<QVariantList>() )
    {
        QVariantList dataList = data.toList();

        foreach ( const QVariant value, dataList )
        {
            QStandardItem *childItem= new QStandardItem(value.toString());
            sourceItem->appendRow(childItem);
        }
    }
    else
    {
        QStandardItem *childItem= new QStandardItem( data.toString() );
        sourceItem->appendRow( childItem );
    }
    return sourceItem;
}


void
WhatsHotWidget::setLeftViewAlbums( AlbumModel* model )
{
    ui->albumsView->setAlbumModel( model );
    ui->albumsView->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
    ui->stackLeft->setCurrentIndex( 2 );
}


void
WhatsHotWidget::setLeftViewArtists( TreeModel* model )
{
    ui->artistsViewLeft->setTreeModel( model );
    ui->artistsViewLeft->proxyModel()->sort( -1 ); // disable sorting, must be called after artistsViewLeft->setTreeModel
    ui->stackLeft->setCurrentIndex( 1 );
}


void
WhatsHotWidget::setLeftViewTracks( PlaylistModel* model )
{
    ui->tracksViewLeft->setPlaylistModel( model );
    ui->tracksViewLeft->proxyModel()->sort( -1 );
    ui->stackLeft->setCurrentIndex( 0 );
}


void
WhatsHotWidget::chartArtistsLoaded( ChartDataLoader* loader, const QList< artist_ptr >& artists )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_artistModels.contains( chartId ) );

    if ( m_artistModels.contains( chartId ) )
    {
        foreach( const artist_ptr& artist, artists )
        {
            m_artistModels[ chartId ]->addArtists( artist );
        }
    }

    loader->deleteLater();
}


void
WhatsHotWidget::chartTracksLoaded( ChartDataLoader* loader, const QList< query_ptr >& tracks )
{

    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_trackModels.contains( chartId ) );

    if ( m_trackModels.contains( chartId ) )
    {
        Pipeline::instance()->resolve( tracks );
        m_trackModels[ chartId ]->append( tracks );
    }

    loader->deleteLater();
}


void
WhatsHotWidget::chartAlbumsLoaded( ChartDataLoader* loader, const QList< album_ptr >& albums )
{
    QString chartId = loader->property( "chartid" ).toString();
    Q_ASSERT( m_albumModels.contains( chartId ) );

    if ( m_albumModels.contains( chartId ) )
        m_albumModels[ chartId ]->addAlbums( albums );

    loader->deleteLater();
}
