/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include <string.h>

#include <QTimer>

#include "infosystem/infosystemworker.h"
#include "artist.h"
#include "result.h"
#include "tomahawksettings.h"
#include "globalactionmanager.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

#include "adiumplugin.h"
#include "adium.h"

QString adium_beforeStatus;
QString adium_afterStatus;

static void setStatus(const QString &status)
{
    // The command that updates the status
    QString scriptqstr;
    scriptqstr.append(adium_beforeStatus);
    scriptqstr.append(status);
    scriptqstr.append(adium_afterStatus);

    const char* scriptstr = scriptqstr.toUtf8();
    script( scriptstr );
}

using namespace Tomahawk::InfoSystem;

AdiumPlugin::AdiumPlugin()
    : InfoPlugin()
{
    Davros::debug() << Q_FUNC_INFO;

    adium_beforeStatus = "if appIsRunning(\"Adium\") then\n";
    adium_beforeStatus.append("tell application \"Adium\"\n");
    adium_beforeStatus.append("set the status message of every account to \"");

    adium_afterStatus.append("\"\nend tell\n");
    adium_afterStatus.append("end if\n");
    adium_afterStatus.append("on appIsRunning(appName)\n");
    adium_afterStatus.append("tell application \"System Events\" to (name of processes) contains appName\n");
    adium_afterStatus.append("end appIsRunning\n");

    m_supportedPushTypes << InfoNowPlaying << InfoNowPaused << InfoNowResumed << InfoNowStopped;

    m_active = TomahawkSettings::instance()->nowPlayingEnabled();

    connect( TomahawkSettings::instance(), SIGNAL( changed() ),
                                             SLOT( settingsChanged() ), Qt::QueuedConnection );

    m_pauseTimer = new QTimer( this );
    m_pauseTimer->setSingleShot( true );
    connect( m_pauseTimer, SIGNAL( timeout() ),
             this, SLOT( clearStatus() ) );

    connect( GlobalActionManager::instance(), SIGNAL( shortLinkReady( QUrl, QUrl ) ),
             SLOT( shortLinkReady( QUrl, QUrl ) ) );
}


AdiumPlugin::~AdiumPlugin()
{
    Davros::debug() << Q_FUNC_INFO;
    if( m_active )
      setStatus( "" );
}

void
AdiumPlugin::shortLinkReady( QUrl longUrl, QUrl shortUrl )
{
    // The URL we received is either from a previous track, or not requested by us
    if( longUrl != m_currentLongUrl )
        return;

    // Build the core of the now-playing string
    QString nowPlaying = "";
    nowPlaying.append( m_currentArtist );
    nowPlaying.append(" - ");
    nowPlaying.append( m_currentTitle );
    nowPlaying.replace( "\"", "\\\"" );  // Escape quotes, or Applescript gets confused

    // We failed to get the short URL, just update the status with the metadata
    if( ( longUrl.toString() == "" ) )
    {
        setStatus( nowPlaying );
        return;
    }

    // Add the short URL
    nowPlaying.append( " " );
    nowPlaying.append( shortUrl.toEncoded() );
    setStatus( nowPlaying );

}

void
AdiumPlugin::clearStatus()
{
    Davros::debug() << Q_FUNC_INFO;
    setStatus( "" );
}


void
AdiumPlugin::settingsChanged()
{
    m_active = TomahawkSettings::instance()->nowPlayingEnabled();
}


void
AdiumPlugin::pushInfo( QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input )
{
    Davros::debug() << Q_FUNC_INFO;

    if( !m_active )
        return;

    switch ( type )
    {
        case InfoNowPlaying:
          audioStarted( input );
          break;
        case InfoNowPaused:
          audioPaused();
          return;
        case InfoNowResumed:
          audioResumed( input );
          break;
        case InfoNowStopped:
          audioStopped();
          break;

        default:
          return;
    }

    // Stop the pause timer always, unless pausing of course
    m_pauseTimer->stop();
}


/** Audio state slots */
void
AdiumPlugin::audioStarted( const QVariant &input )
{
    Davros::debug() << Q_FUNC_INFO;

    if ( !input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
        return;

    InfoStringHash hash = input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "title" ) || !hash.contains( "artist" ) )
        return;

    m_currentTitle = hash["title"];
    m_currentArtist = hash["artist"];

    // Request a short URL
    m_currentLongUrl = openLinkFromHash( hash );
    GlobalActionManager::instance()->shortenLink( m_currentLongUrl );
}

QUrl
AdiumPlugin::openLinkFromHash( const Tomahawk::InfoSystem::InfoStringHash& hash ) const
{
    QString title, artist, album;

    if( !hash.isEmpty() && hash.contains( "title" ) && hash.contains( "artist" ) )
    {
        title = hash["title"];
        artist = hash["artist"];
        if( hash.contains( "album" ) )
            album = hash["album"];
    }

    return GlobalActionManager::instance()->openLink( title, artist, album );
}

void
AdiumPlugin::audioFinished( const QVariant &input )
{
    //Davros::debug() << Q_FUNC_INFO;
}

void
AdiumPlugin::audioStopped()
{
    Davros::debug() << Q_FUNC_INFO;
    setStatus( "" );
}

void
AdiumPlugin::audioPaused()
{
    Davros::debug() << Q_FUNC_INFO;
    m_pauseTimer->start( 60 * 1000 );
}

void
AdiumPlugin::audioResumed( const QVariant &input )
{
    Davros::debug() << Q_FUNC_INFO;
    audioStarted( input );
}

