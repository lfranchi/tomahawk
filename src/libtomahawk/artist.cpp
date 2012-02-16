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

#include "artist.h"

#include "artistplaylistinterface.h"
#include "collection.h"
#include "database/database.h"
#include "database/databaseimpl.h"
#include "query.h"


#include "libdavros/davros.h"
#include "utils/logger.h"

using namespace Tomahawk;


Artist::~Artist()
{
}


artist_ptr
Artist::get( const QString& name, bool autoCreate )
{
    int artid = Database::instance()->impl()->artistId( name, autoCreate );
    if ( artid < 1 && autoCreate )
        return artist_ptr();

    return Artist::get( artid, name );
}


artist_ptr
Artist::get( unsigned int id, const QString& name )
{
    static QHash< unsigned int, artist_ptr > s_artists;
    static QMutex s_mutex;

    QMutexLocker lock( &s_mutex );
    if ( s_artists.contains( id ) )
    {
        return s_artists.value( id );
    }

    artist_ptr a = artist_ptr( new Artist( id, name ) );
    if ( id > 0 )
        s_artists.insert( id, a );

    return a;
}


Artist::Artist( unsigned int id, const QString& name )
    : QObject()
    , m_id( id )
    , m_name( name )
    , m_infoLoaded( false )
{
    m_sortname = DatabaseImpl::sortname( name, true );

    connect( Tomahawk::InfoSystem::InfoSystem::instance(),
             SIGNAL( info( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ),
             SLOT( infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData, QVariant ) ) );
}


void
Artist::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks )
{
    Tomahawk::ArtistPlaylistInterface* api = dynamic_cast< Tomahawk::ArtistPlaylistInterface* >( playlistInterface().data() );
    if ( api )
        api->addQueries( tracks );
    emit tracksAdded( tracks );
}


QByteArray
Artist::cover() const
{
    if ( !m_infoLoaded )
    {
        m_uuid = uuid();

        Tomahawk::InfoSystem::InfoStringHash trackInfo;
        trackInfo["artist"] = name();

        Tomahawk::InfoSystem::InfoRequestData requestData;
        requestData.caller = m_uuid;
        requestData.type = Tomahawk::InfoSystem::InfoArtistImages;
        requestData.input = QVariant::fromValue< Tomahawk::InfoSystem::InfoStringHash >( trackInfo );
        requestData.customData = QVariantMap();

        Tomahawk::InfoSystem::InfoSystem::instance()->getInfo( requestData );
    }

    return m_cover;
}


void
Artist::infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output )
{
    if ( requestData.caller != m_uuid ||
         requestData.type != Tomahawk::InfoSystem::InfoArtistImages )
    {
        return;
    }

    m_infoLoaded = true;
    if ( !output.isNull() && output.isValid() )
    {
        QVariantMap returnedData = output.value< QVariantMap >();
        const QByteArray ba = returnedData["imgbytes"].toByteArray();
        if ( ba.length() )
        {
            m_cover = ba;
        }
    }

    emit updated();
}


Tomahawk::playlistinterface_ptr
Artist::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::ArtistPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
