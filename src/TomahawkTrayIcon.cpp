/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TomahawkTrayIcon.h"

#include <QtGui/QWheelEvent>

#include "Artist.h"

#include "audio/AudioEngine.h"
#include "TomahawkApp.h"
#include "TomahawkWindow.h"
#include "Query.h"

#include "utils/Logger.h"
#include <ActionCollection.h>


TomahawkTrayIcon::TomahawkTrayIcon( QObject* parent )
    : QSystemTrayIcon( parent )
    , m_currentAnimationFrame( 0 )
    , m_showWindowAction( 0 )
    , m_stopContinueAfterTrackAction( 0 )
{
    QIcon icon( RESPATH "icons/tomahawk-icon-128x128-grayscale.png" );
    setIcon( icon );

    refreshToolTip();

    m_contextMenu = new QMenu();
    setContextMenu( m_contextMenu );
    
    m_stopContinueAfterTrackAction = new QAction( tr( "&Stop Playback after current Track" ), this );

    ActionCollection *ac = ActionCollection::instance();
    m_contextMenu->addAction( ac->getAction( "playPause" ) );
    m_contextMenu->addAction( ac->getAction( "stop" ) );
    m_contextMenu->addAction( m_stopContinueAfterTrackAction );
    m_contextMenu->addSeparator();
    m_contextMenu->addAction( ac->getAction( "previousTrack" ) );
    m_contextMenu->addAction( ac->getAction( "nextTrack" ) );
    m_contextMenu->addSeparator();
    m_contextMenu->addAction( ActionCollection::instance()->getAction( "togglePrivacy" ) );
    
    connect( m_stopContinueAfterTrackAction, SIGNAL( triggered(bool) ), this, SLOT( stopContinueAfterTrackActionTriggered() ) );

#ifdef Q_WS_MAC
    // On mac you can close the windows while leaving the app open. We then need a way to show the main window again
    m_contextMenu->addSeparator();
    m_showWindowAction = m_contextMenu->addAction( tr( "Hide Tomahawk Window" ) );
    m_showWindowAction->setData( true );
    connect( m_showWindowAction, SIGNAL( triggered() ), this, SLOT( showWindow() ) );

    connect( m_contextMenu, SIGNAL( aboutToShow() ), this, SLOT( menuAboutToShow() ) );
#endif

    m_contextMenu->addSeparator();
    m_contextMenu->addAction( ac->getAction( "quit" ) );

    connect( AudioEngine::instance(), SIGNAL( loading( Tomahawk::result_ptr ) ), SLOT( setResult( Tomahawk::result_ptr ) ) );
    connect( AudioEngine::instance(), SIGNAL( started( Tomahawk::result_ptr ) ), SLOT( onPlay() ) );
    connect( AudioEngine::instance(), SIGNAL( resumed() ), this, SLOT( onResume() ) );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), this, SLOT( onStop() ) );
    connect( AudioEngine::instance(), SIGNAL( paused() ),  this, SLOT( onPause() ) );
    connect( AudioEngine::instance(), SIGNAL( stopAfterTrack_changed() ) , this, SLOT( stopContinueAfterTrack_StatusChanged() ) );

    connect( &m_animationTimer, SIGNAL( timeout() ), SLOT( onAnimationTimer() ) );
    connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason ) ), SLOT( onActivated( QSystemTrayIcon::ActivationReason ) ) );

    show();
}


TomahawkTrayIcon::~TomahawkTrayIcon()
{
    delete m_contextMenu;
}


void
TomahawkTrayIcon::setShowHideWindow( bool show )
{
    if ( show )
    {
        m_showWindowAction->setText( tr( "Hide Tomahawk Window" ) );
        m_showWindowAction->setData( show );
    }
    else
    {
        m_showWindowAction->setText( tr( "Show Tomahawk Window" ) );
    }

    m_showWindowAction->setData( show );
}


void
TomahawkTrayIcon::showWindow()
{
    if( !m_showWindowAction->data().toBool() )
    {
        APP->mainWindow()->show();
        APP->mainWindow()->raise();

        setShowHideWindow( true );
    }
    else
    {
        APP->mainWindow()->hide();

        setShowHideWindow( false );
    }
}


void
TomahawkTrayIcon::menuAboutToShow()
{
    // When using Cmd-H on mac to hide a window, it is an OS-level hide that is different from QWidget::hide().
    // Qt returns isVisible() == true for windows that are hidden with Cmd-H, which is weird. isActiveWindow() returns
    // the proper information though.
    setShowHideWindow( APP->mainWindow()->isActiveWindow() );
}


void
TomahawkTrayIcon::setResult( const Tomahawk::result_ptr& result )
{
    m_currentTrack = result;
    refreshToolTip();
}


void
TomahawkTrayIcon::refreshToolTip()
{
    #ifdef Q_WS_MAC
    // causes issues with OS X menubar, also none
    // of the other OS X menubar icons have a tooltip
    return;
    #endif

    QString tip;
    if ( !m_currentTrack.isNull() )
    {
        tip = m_currentTrack->artist()->name() + " " + QChar( 8211 ) /*en dash*/ + " " + m_currentTrack->track();
    }
    else
    {
        tip = tr( "Currently not playing." );
    }

    #ifdef Q_WS_WIN
        // Good old crappy Win32
        tip.replace( "&", "&&&" );
    #endif

    setToolTip( tip );
}


void
TomahawkTrayIcon::onAnimationTimer()
{
/*    m_currentAnimationFrame++;
    if( m_currentAnimationFrame >= m_animationPixmaps.count() )
        m_currentAnimationFrame = 0;

    setIcon( m_animationPixmaps.at( m_currentAnimationFrame ) );*/
}


void
TomahawkTrayIcon::onActivated( QSystemTrayIcon::ActivationReason reason )
{
#ifdef Q_WS_MAC
    return;
#endif

    switch( reason )
    {
        case QSystemTrayIcon::Trigger:
        {
            TomahawkWindow* mainwindow = APP->mainWindow();
            if ( mainwindow->isVisible() )
            {
                mainwindow->hide();
            }
            else
            {
                mainwindow->show();
            }
        }
        break;

        case QSystemTrayIcon::MiddleClick:
        {
            AudioEngine::instance()->playPause();
        }
        break;

        default:
            break;
    }
}


void
TomahawkTrayIcon::onPause()
{
    ActionCollection::instance()->getAction( "playPause" )->setText( tr( "Play" ) );
}


void
TomahawkTrayIcon::onPlay()
{
    m_stopContinueAfterTrackAction->setEnabled( true );
    onResume();
    stopContinueAfterTrack_StatusChanged();
}


void
TomahawkTrayIcon::onStop()
{
    m_stopContinueAfterTrackAction->setEnabled( false );
    onPause();
}


void
TomahawkTrayIcon::onResume()
{
    ActionCollection::instance()->getAction( "playPause" )->setText( tr( "Pause" ) );
}


void
TomahawkTrayIcon::stopContinueAfterTrack_StatusChanged()
{
    if ( !AudioEngine::instance()->currentTrack().isNull() )
    {
        if ( AudioEngine::instance()->currentTrack()->toQuery()->equals( AudioEngine::instance()->stopAfterTrack() ) )
            m_stopContinueAfterTrackAction->setText( tr( "&Continue Playback after current Track" ) );
        else
            m_stopContinueAfterTrackAction->setText( tr( "&Stop Playback after current Track" ) );
    }
}


void TomahawkTrayIcon::stopContinueAfterTrackActionTriggered()
{
    if ( !AudioEngine::instance()->currentTrack().isNull() )
    {
        if ( !AudioEngine::instance()->currentTrack()->toQuery()->equals( AudioEngine::instance()->stopAfterTrack() ) )
            AudioEngine::instance()->setStopAfterTrack( AudioEngine::instance()->currentTrack()->toQuery() );
        else
            AudioEngine::instance()->setStopAfterTrack( Tomahawk::query_ptr() );
    }
}


bool
TomahawkTrayIcon::event( QEvent* e )
{
    // Beginning with Qt 4.3, QSystemTrayIcon supports wheel events, but only
    // on X11. Let's make it adjust the volume.
    if ( e->type() == QEvent::Wheel )
    {
        if ( ((QWheelEvent*)e)->delta() > 0 )
        {
            AudioEngine::instance()->raiseVolume();
        }
        else
        {
            AudioEngine::instance()->lowerVolume();
        }

        return true;
    }

    return QSystemTrayIcon::event( e );
}

