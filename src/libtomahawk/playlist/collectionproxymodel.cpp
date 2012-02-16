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

#include "collectionproxymodel.h"

#include "collectionproxymodelplaylistinterface.h"

#include <QTreeView>

#include "album.h"
#include "query.h"

#include "libdavros/davros.h"
#include "utils/logger.h"


CollectionProxyModel::CollectionProxyModel( QObject* parent )
    : TrackProxyModel( parent )
{
}

Tomahawk::playlistinterface_ptr
CollectionProxyModel::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::CollectionProxyModelPlaylistInterface( this ) );
    }

    return m_playlistInterface;
}
