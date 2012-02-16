/*
    Copyright (C) 2011  Leo Franchi <lfranchi@kde.org>


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "localcollection.h"

#include "sourcelist.h"
#include <tomahawksettings.h>

#include "libdavros/davros.h"
#include "utils/logger.h"

#ifndef ENABLE_HEADLESS
    #include "viewmanager.h"
#endif


LocalCollection::LocalCollection( const Tomahawk::source_ptr& source, QObject* parent )
    : DatabaseCollection( source, parent )
{

}


Tomahawk::playlist_ptr
LocalCollection::bookmarksPlaylist()
{
    if( TomahawkSettings::instance()->bookmarkPlaylist().isEmpty() )
        return Tomahawk::playlist_ptr();

    return playlist( TomahawkSettings::instance()->bookmarkPlaylist() );
}


void
LocalCollection::createBookmarksPlaylist()
{
    if( bookmarksPlaylist().isNull() ) {
        QString guid = uuid();
        Tomahawk::playlist_ptr p = Tomahawk::Playlist::create( SourceList::instance()->getLocal(), guid, tr( "Bookmarks" ), tr( "Saved tracks" ), QString(), false );

#ifndef ENABLE_HEADLESS
        ViewManager::instance()->createPageForPlaylist( p );
//         connect( p.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( loaded( Tomahawk::PlaylistRevision ) ), Qt::QueuedConnection );
        connect( p.data(), SIGNAL( created() ), this, SLOT( created() ) );
#endif
        TomahawkSettings::instance()->setBookmarkPlaylist( guid );
//         p->createNewRevision( uuid(), p->currentrevision(), QList< Tomahawk::plentry_ptr >() );
    }
}


void
LocalCollection::created()
{
    emit bookmarkPlaylistCreated( bookmarksPlaylist() );
}
