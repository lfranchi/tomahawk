/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
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

#include "SearchWidget.h"
#include "ui_SearchWidget.h"

#include <QPushButton>
#include <QDialogButtonBox>

#include "SourceList.h"
#include "ViewManager.h"
#include "playlist/AlbumModel.h"
#include "playlist/PlaylistModel.h"
#include "widgets/OverlayWidget.h"
#include "utils/AnimatedSpinner.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


SearchWidget::SearchWidget( const QString& search, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SearchWidget )
    , m_search( search )
{
    ui->setupUi( this );

    ui->resultsView->setGuid( "searchwidget" );
    m_resultsModel = new PlaylistModel( ui->resultsView );
    ui->resultsView->setPlaylistModel( m_resultsModel );
    ui->resultsView->overlay()->setEnabled( false );
    ui->resultsView->sortByColumn( PlaylistModel::Score, Qt::DescendingOrder );

    m_albumsModel = new AlbumModel( ui->albumView );
    ui->albumView->setAlbumModel( m_albumsModel );

    m_artistsModel = new AlbumModel( ui->artistView );
    ui->artistView->setAlbumModel( m_artistsModel );

    ui->artistView->setAutoFitItems( false );
    ui->albumView->setAutoFitItems( false );
    ui->artistView->setSpacing( 8 );
    ui->albumView->setSpacing( 8 );

    ui->artistView->proxyModel()->sort( -1 );
    ui->albumView->proxyModel()->sort( -1 );

    TomahawkUtils::unmarginLayout( ui->verticalLayout );

    ui->artistView->setContentsMargins( 0, 0, 0, 0 );
    ui->artistView->setFrameShape( QFrame::NoFrame );
    ui->artistView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    ui->albumView->setContentsMargins( 0, 0, 0, 0 );
    ui->albumView->setFrameShape( QFrame::NoFrame );
    ui->albumView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    ui->resultsView->setContentsMargins( 0, 0, 0, 0 );
    ui->resultsView->setFrameShape( QFrame::NoFrame );
    ui->resultsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    ui->resultsView->loadingSpinner()->fadeIn();
    m_queries << Tomahawk::Query::get( search, uuid() );

    ui->splitter_2->setStretchFactor( 0, 0 );
    ui->splitter_2->setStretchFactor( 1, 1 );

    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        connect( query.data(), SIGNAL( artistsAdded( QList<Tomahawk::artist_ptr> ) ), SLOT( onArtistsFound( QList<Tomahawk::artist_ptr> ) ) );
        connect( query.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr> ) ), SLOT( onAlbumsFound( QList<Tomahawk::album_ptr> ) ) );
        connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ), SLOT( onResultsFound( QList<Tomahawk::result_ptr> ) ) );
        connect( query.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryFinished() ) );
    }
}


SearchWidget::~SearchWidget()
{
    delete ui;
}


void
SearchWidget::changeEvent( QEvent* e )
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


Tomahawk::playlistinterface_ptr
SearchWidget::playlistInterface() const
{
    return ui->resultsView->playlistInterface();
}


bool
SearchWidget::jumpToCurrentTrack()
{
    return ui->resultsView->jumpToCurrentTrack();
}


void
SearchWidget::onResultsFound( const QList<Tomahawk::result_ptr>& results )
{
    QList<Tomahawk::artist_ptr> artists;
    QList<Tomahawk::album_ptr> albums;

    foreach( const Tomahawk::result_ptr& result, results )
    {
        if ( !result->collection().isNull() && !result->isOnline() )
            continue;

        QList< Tomahawk::result_ptr > rl;
        rl << result;

        Tomahawk::query_ptr q = result->toQuery();
        q->setResolveFinished( true );
        q->addResults( rl );

        m_resultsModel->append( q );

        artists << result->artist();
        albums << result->album();
    }

    m_artistsModel->addArtists( artists );
    m_albumsModel->addAlbums( albums );
}


void
SearchWidget::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums )
{
    m_albumsModel->addAlbums( albums );
}


void
SearchWidget::onArtistsFound( const QList<Tomahawk::artist_ptr>& artists )
{
    m_artistsModel->addArtists( artists );
}


void
SearchWidget::onQueryFinished()
{
    ui->resultsView->loadingSpinner()->fadeOut();
}
