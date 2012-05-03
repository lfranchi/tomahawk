/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012,      Leo Franchi            <lfranchi@kde.org>
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

#include "InfoBar.h"
#include "ui_InfoBar.h"

#include <QLabel>
#include <QPixmap>

#include "ViewManager.h"
#include "thirdparty/Qocoa/qsearchfield.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include <QCheckBox>
#include <QPaintEvent>
#include <QPainter>
#include <widgets/QueryLabel.h>

#define ANIMATION_TIME 400
#define IMAGE_HEIGHT 64

using namespace Tomahawk;


InfoBar::InfoBar( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::InfoBar )
    , m_queryLabel( 0 )
{
    ui->setupUi( this );
    TomahawkUtils::unmarginLayout( layout() );
    layout()->setContentsMargins( 8, 4, 8, 4 );

    QFont boldFont = ui->captionLabel->font();
    boldFont.setPixelSize( 18 );
    boldFont.setBold( true );
    ui->captionLabel->setFont( boldFont );
    ui->captionLabel->setElideMode( Qt::ElideRight );

    boldFont.setPixelSize( 12 );
    ui->descriptionLabel->setFont( boldFont );

    QFont regFont = ui->longDescriptionLabel->font();
    regFont.setPixelSize( 11 );
    ui->longDescriptionLabel->setFont( regFont );

    m_whitePal = ui->captionLabel->palette();
    m_whitePal.setColor( QPalette::Foreground, Qt::white );

    ui->captionLabel->setPalette( m_whitePal );
    ui->descriptionLabel->setPalette( m_whitePal );
    ui->longDescriptionLabel->setPalette( m_whitePal );

    ui->captionLabel->setMargin( 6 );
    ui->descriptionLabel->setMargin( 6 );
    ui->longDescriptionLabel->setMargin( 4 );

    ui->captionLabel->setText( QString() );
    ui->descriptionLabel->setText( QString() );
    ui->longDescriptionLabel->setText( QString() );
    ui->imageLabel->setText( QString() );

    m_queryLabel = new QueryLabel( this );
    m_queryLabel->setType( QueryLabel::Artist );
    m_queryLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_queryLabel->setTextPen( palette().brightText().color() );
    m_queryLabel->setFont( boldFont );
    m_queryLabel->hide();
    connect( m_queryLabel, SIGNAL( clickedArtist() ), this, SLOT( artistClicked() ) );

    m_searchWidget = new QSearchField( this );
    m_searchWidget->setPlaceholderText( tr( "Filter..." ) );
    m_searchWidget->setMinimumWidth( 180 );
    connect( m_searchWidget, SIGNAL( textChanged( QString ) ), this, SLOT( onFilterEdited() ) );

    ui->horizontalLayout->addWidget( m_searchWidget );

    setAutoFillBackground( true );

    setMinimumHeight( geometry().height() );
    setMaximumHeight( geometry().height() );

    createTile();

    connect( ViewManager::instance(), SIGNAL( filterAvailable( bool ) ), SLOT( setFilterAvailable( bool ) ) );
}


InfoBar::~InfoBar()
{
    delete ui;
}


void
InfoBar::setCaption( const QString& s )
{
    ui->captionLabel->setText( s );
}


void
InfoBar::setDescription( const QString& s )
{
    if ( m_queryLabel->isVisible() )
    {
        ui->verticalLayout->removeWidget( m_queryLabel );
        m_queryLabel->hide();

        ui->verticalLayout->addWidget( ui->descriptionLabel );
        ui->verticalLayout->setContentsMargins( 0, 0, 0, 0 );
        ui->descriptionLabel->show();
    }
    ui->descriptionLabel->setText( s );
}

void
InfoBar::setDescription( const artist_ptr& artist )
{
    m_queryLabel->setQuery( Query::get( artist->name(), QString(), QString() ) );
    m_queryLabel->setExtraContentsMargins( 4, 0, 0, 0 );

    if ( !m_queryLabel->isVisible() )
    {
        ui->verticalLayout->removeWidget( ui->descriptionLabel );
        ui->descriptionLabel->hide();

        m_queryLabel->show();
        ui->verticalLayout->addWidget( m_queryLabel );
        ui->verticalLayout->setContentsMargins( 0, 0, 0, 15 );
    }

}

void
InfoBar::setDescription( const album_ptr&  )
{
    // TODO
}

void
InfoBar::artistClicked()
{
    if ( m_queryLabel && !m_queryLabel->query().isNull() )
        ViewManager::instance()->show( Artist::get( m_queryLabel->artist() ) );
}


void
InfoBar::setLongDescription( const QString& s )
{
    ui->longDescriptionLabel->setText( s );

    if ( s.isEmpty() )
    {
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 1 );
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout_2, 0 );
    } else
    {
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 0 );
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout_2, 99 );
    }
}


void
InfoBar::setPixmap( const QPixmap& p )
{
    ui->imageLabel->setPixmap( p.scaledToHeight( IMAGE_HEIGHT, Qt::SmoothTransformation ) );
}


void
InfoBar::setFilter( const QString& filter )
{
    m_searchWidget->setText( filter );
}


void
InfoBar::setFilterAvailable( bool b )
{
    m_searchWidget->setVisible( b );
}


void
InfoBar::setUpdaters( const QList<PlaylistUpdaterInterface*>& updaters )
{
    QList< QWidget* > newUpdaterWidgets;
    foreach ( PlaylistUpdaterInterface* updater, updaters )
    {
        if ( updater->configurationWidget() )
            newUpdaterWidgets << updater->configurationWidget();
    }


    foreach ( QWidget* updaterWidget, m_updaterConfigurations )
    {
        updaterWidget->hide();

        if ( !newUpdaterWidgets.contains( updaterWidget ) )
        {
            // Old config widget no longer present, remove it
            ui->horizontalLayout->removeWidget( updaterWidget );
        }
    }

    m_updaters = updaters;
    m_updaterConfigurations = newUpdaterWidgets;

    // Display each new widget in the proper place
    int insertIdx = -1; // Ugh, no indexOf for QSpacerItem*
    for ( int i = 0; i < ui->horizontalLayout->count(); i++ )
    {
        if ( ui->horizontalLayout->itemAt( i )->spacerItem() == ui->horizontalSpacer_4 )
        {
            insertIdx = i;
            break;
        }
    }
    insertIdx++;

    foreach ( QWidget* updaterWidget, m_updaterConfigurations )
    {
        updaterWidget->setPalette( m_whitePal );
        ui->horizontalLayout->insertWidget( insertIdx, updaterWidget );
        updaterWidget->show();
    }

//     if ( m_updaterConfiguration )
//         m_updaterConfiguration->hide();
//
//     if ( m_updaterConfiguration && ( interface ? (m_updaterConfiguration != interface->configurationWidget()) : true ) )
//         ui->horizontalLayout->removeWidget( m_updaterConfiguration );
//
//     m_updaterInterface = interface;
//     m_updaterConfiguration = interface ? interface->configurationWidget() : 0;
//
//     if ( !m_updaterInterface || !m_updaterConfiguration )
//         return;
//
//     m_updaterConfiguration->setPalette( m_whitePal );
//     int insertIdx = -1; // Ugh, no indexOf for QSpacerItem*
//     for ( int i = 0; i < ui->horizontalLayout->count(); i++ )
//     {
//         if ( ui->horizontalLayout->itemAt( i )->spacerItem() == ui->horizontalSpacer_4 )
//         {
//             insertIdx = i;
//             break;
//         }
//     }
//     insertIdx++;
//     ui->horizontalLayout->insertWidget( insertIdx, m_updaterConfiguration );
//
//     m_updaterConfiguration->show();
}


void
InfoBar::onFilterEdited()
{
    emit filterTextChanged( m_searchWidget->text() );
}


void
InfoBar::createTile( int w )
{
    QImage tile = QImage( RESPATH "images/playlist-header-tiled.png" );

    if ( tile.isNull() )
        return;

    if ( tile.height() < height() )
    {
        // image must be at least as tall as we are
        QImage taller( tile.width(), height(), QImage::Format_ARGB32_Premultiplied );
        QPainter p( &taller );
        int curY = 0;
        while ( curY < taller.height() )
        {
            const int thisHeight = (curY + tile.height() > height()) ? height() - curY : tile.height();
            p.drawImage( QRect( 0, curY, tile.width(), thisHeight ), tile, QRect( 0, 0, tile.width(), thisHeight ) );
            curY += tile.height();
        }
        tile = taller;
    }

    m_bgTile = QPixmap( w, height() );
    m_bgTile.fill( Qt::transparent );

    int curWidth = 0;
    QPainter p( &m_bgTile );
    while ( curWidth < w )
    {
        const int thisWidth = (curWidth + tile.width() > w) ? w - curWidth : tile.width();

        const QRect source( 0, 0, thisWidth, m_bgTile.height() );
        const QRect dest( curWidth, 0, thisWidth, m_bgTile.height() );
        p.drawImage( dest, tile, source );
        curWidth += thisWidth;
    }
}


void
InfoBar::paintEvent( QPaintEvent* e )
{
    if ( m_bgTile.isNull() || width() > m_bgTile.width() )
        createTile( width() );

    if ( m_bgTile.isNull() )
        return;

    QPainter p( this );

    // Truncate bg pixmap and paint into bg
    p.drawPixmap( rect(), m_bgTile, rect() );
}


void
InfoBar::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}
