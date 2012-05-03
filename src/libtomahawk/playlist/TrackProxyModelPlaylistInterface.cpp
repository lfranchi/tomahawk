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

#include "TrackProxyModelPlaylistInterface.h"

#include "TrackProxyModel.h"
#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "utils/Logger.h"

using namespace Tomahawk;

TrackProxyModelPlaylistInterface::TrackProxyModelPlaylistInterface( TrackProxyModel* proxyModel )
    : PlaylistInterface()
    , m_proxyModel( proxyModel )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
}


TrackProxyModelPlaylistInterface::~TrackProxyModelPlaylistInterface()
{
    m_proxyModel.clear();
}


int
TrackProxyModelPlaylistInterface::unfilteredTrackCount() const
{
    return ( m_proxyModel.isNull() ? 0 : m_proxyModel.data()->sourceModel()->trackCount() );
}


int
TrackProxyModelPlaylistInterface::trackCount() const
{
    return ( m_proxyModel.isNull() ? 0 : m_proxyModel.data()->rowCount( QModelIndex() ) );
}


QString
TrackProxyModelPlaylistInterface::filter() const
{
    return ( m_proxyModel.isNull() ? QString() : m_proxyModel.data()->filterRegExp().pattern() );
}


void
TrackProxyModelPlaylistInterface::setFilter( const QString& pattern )
{
    if ( m_proxyModel.isNull() )
        return;

    m_proxyModel.data()->setFilterRegExp( pattern );
    m_proxyModel.data()->emitFilterChanged( pattern );

    emit trackCountChanged( trackCount() );
}


QList< Tomahawk::query_ptr >
TrackProxyModelPlaylistInterface::tracks()
{
    if ( m_proxyModel.isNull() )
        return QList< Tomahawk::query_ptr >();

    TrackProxyModel* proxyModel = m_proxyModel.data();
    QList<Tomahawk::query_ptr> queries;

    for ( int i = 0; i < proxyModel->rowCount( QModelIndex() ); i++ )
    {
        TrackModelItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->index( i, 0 ) ) );
        if ( item )
            queries << item->query();
    }

    return queries;
}


Tomahawk::result_ptr
TrackProxyModelPlaylistInterface::siblingItem( int itemsAway )
{
    return siblingItem( itemsAway, false );
}


bool
TrackProxyModelPlaylistInterface::hasNextItem()
{
    return !( siblingItem( 1, true ).isNull() );
}


Tomahawk::result_ptr
TrackProxyModelPlaylistInterface::siblingItem( int itemsAway, bool readOnly )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();

    TrackProxyModel* proxyModel = m_proxyModel.data();

    QModelIndex idx = proxyModel->index( 0, 0 );
    if ( proxyModel->rowCount() )
    {
        if ( m_shuffled )
        {
            // random mode is enabled
            // TODO come up with a clever random logic, that keeps track of previously played items
            idx = proxyModel->index( qrand() % proxyModel->rowCount(), 0 );
        }
        else if ( proxyModel->currentIndex().isValid() )
        {
            idx = proxyModel->currentIndex();

            // random mode is disabled
            if ( m_repeatMode != PlaylistInterface::RepeatOne )
            {
                // keep progressing through the playlist normally
                idx = proxyModel->index( idx.row() + itemsAway, 0 );
            }
        }
    }

    if ( !idx.isValid() && m_repeatMode == PlaylistInterface::RepeatAll )
    {
        // repeat all tracks
        if ( itemsAway > 0 )
        {
            // reset to first item
            idx = proxyModel->index( 0, 0 );
        }
        else
        {
            // reset to last item
            idx = proxyModel->index( proxyModel->rowCount() - 1, 0 );
        }
    }

    // Try to find the next available PlaylistItem (with results)
    while ( idx.isValid() )
    {
        TrackModelItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( idx ) );
        if ( item && item->query()->playable() )
        {
            qDebug() << "Next PlaylistItem found:" << item->query()->toString() << item->query()->results().at( 0 )->url();
            if ( !readOnly )
                proxyModel->setCurrentIndex( idx );
            return item->query()->results().at( 0 );
        }

        idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0 );
    }

    if ( !readOnly )
        proxyModel->setCurrentIndex( QModelIndex() );
    return Tomahawk::result_ptr();
}


Tomahawk::result_ptr
TrackProxyModelPlaylistInterface::currentItem() const
{
    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();

    TrackProxyModel* proxyModel = m_proxyModel.data();

    TrackModelItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->currentIndex() ) );
    if ( item && !item->query().isNull() && item->query()->playable() )
        return item->query()->results().at( 0 );
    return Tomahawk::result_ptr();
}

