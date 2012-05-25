/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "DiscogsPlugin.h"

#include <QNetworkReply>
#include <QDomDocument>
#include <QtPlugin>

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/Closure.h"

using namespace Tomahawk::InfoSystem;


DiscogsPlugin::DiscogsPlugin()
    : InfoPlugin()
{
    qDebug() << Q_FUNC_INFO;
    m_supportedGetTypes << Tomahawk::InfoSystem::InfoAlbumSongs;
}


DiscogsPlugin::~DiscogsPlugin() {}


void
DiscogsPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        emit info( requestData, QVariant() );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    if ( !hash.contains( "artist" ) || !hash.contains( "album" ) )
    {
        emit info( requestData, QVariant() );
        return;
    }

    switch ( requestData.type )
    {
        case InfoAlbumSongs:
        {

            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria["artist"] = hash["artist"];
            criteria["album"] = hash["album"];

            emit getCachedInfo( criteria, 2419200000, requestData );

            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }
}


void
DiscogsPlugin::notInCacheSlot( InfoStringHash criteria, InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoAlbumSongs:
        {
            QString requestString( "http://api.discogs.com/database/search" );
            QUrl url( requestString );
            url.addQueryItem( "type", "releases" );
            url.addQueryItem( "release_title", criteria[ "album" ] );
            url.addQueryItem( "artist", criteria[ "artist" ] );
            QNetworkRequest req( url );
            req.setRawHeader( "User-Agent", "TomahawkPlayer/1.0 +http://tomahawk-player.org" );
            QNetworkReply* reply = TomahawkUtils::nam()->get(  );

            NewClosure( reply, SIGNAL( finished() ),  this, SLOT( albumSearchSlot( Tomahawk::InfoSystem::InfoRequestData, QNetworkReply* ) ), requestData, reply )
            break;
        }

        default:
        {
            Q_ASSERT( false );
            break;
        }
    }
}


bool
DiscogsPlugin::isValidTrackData( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( requestData.input.isNull() || !requestData.input.isValid() || !requestData.input.canConvert< QVariantMap >() )
    {
        emit info( requestData, QVariant() );
        qDebug() << Q_FUNC_INFO << "Data null, invalid, or can't convert";
        return false;
    }
    QVariantMap hash = requestData.input.value< QVariantMap >();
    if ( hash[ "trackName" ].toString().isEmpty() )
    {
        emit info( requestData, QVariant() );
        qDebug() << Q_FUNC_INFO << "Track name is empty";
        return false;
    }
    if ( hash[ "artistName" ].toString().isEmpty() )
    {
        emit info( requestData, QVariant() );
        qDebug() << Q_FUNC_INFO << "No artist name found";
        return false;
    }
    return true;
}


void
DiscogsPlugin::albumSearchSlot( const InfoRequestData &requestData, QNetworkReply *reply )
{

}



Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::DiscogsPlugin )
