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

#include "treeproxymodelplaylistinterface.h"

#include "treeproxymodel.h"

#include "source.h"
#include "query.h"
#include "database/database.h"
#include "database/databaseimpl.h"
#include "database/databasecommand_allalbums.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

using namespace Tomahawk;

TreeProxyModelPlaylistInterface::TreeProxyModelPlaylistInterface( TreeProxyModel *proxyModel )
    : PlaylistInterface()
    , m_proxyModel( proxyModel )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
}


TreeProxyModelPlaylistInterface::~TreeProxyModelPlaylistInterface()
{
    m_proxyModel.clear();
}


QString
TreeProxyModelPlaylistInterface::filter() const
{
    if ( m_proxyModel.isNull() )
        return 0;
    TreeProxyModel* proxyModel = m_proxyModel.data();
    return proxyModel->filterRegExp().pattern();
}


void
TreeProxyModelPlaylistInterface::setFilter( const QString& pattern )
{
    if ( m_proxyModel.isNull() )
        return;
    m_proxyModel.data()->newFilterFromPlaylistInterface( pattern );
}


int
TreeProxyModelPlaylistInterface::unfilteredTrackCount() const
{
    if ( m_proxyModel.isNull() )
        return 0;
    TreeProxyModel* proxyModel = m_proxyModel.data();
    return proxyModel->sourceModel()->rowCount( QModelIndex() );
}


int
TreeProxyModelPlaylistInterface::trackCount() const
{
    if ( m_proxyModel.isNull() )
        return 0;
    TreeProxyModel* proxyModel = m_proxyModel.data();
    return proxyModel->rowCount( QModelIndex() );
}


bool
TreeProxyModelPlaylistInterface::hasNextItem()
{
    return !( siblingItem( 1, true ).isNull() );
}


Tomahawk::result_ptr
TreeProxyModelPlaylistInterface::siblingItem( int itemsAway )
{
    return siblingItem( itemsAway, false );
}


Tomahawk::result_ptr
TreeProxyModelPlaylistInterface::siblingItem( int itemsAway, bool readOnly )
{
    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();
    TreeProxyModel* proxyModel = m_proxyModel.data();

    QModelIndex idx = proxyModel->currentIndex();
    if ( !idx.isValid() )
        return Tomahawk::result_ptr();

    if ( m_shuffled )
    {
        idx = proxyModel->index( qrand() % proxyModel->rowCount( idx.parent() ), 0, idx.parent() );
    }
    else
    {
        if ( m_repeatMode != PlaylistInterface::RepeatOne )
            idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
    }

    if ( !idx.isValid() && m_repeatMode == PlaylistInterface::RepeatAll )
    {
        if ( itemsAway > 0 )
        {
            // reset to first item
            idx = proxyModel->index( 0, 0, proxyModel->currentIndex().parent() );
        }
        else
        {
            // reset to last item
            idx = proxyModel->index( proxyModel->rowCount( proxyModel->currentIndex().parent() ) - 1, 0, proxyModel->currentIndex().parent() );
        }
    }

    // Try to find the next available PlaylistItem (with results)
    while ( idx.isValid() )
    {
        TreeModelItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( idx ) );
        if ( item && !item->result().isNull() && item->result()->isOnline() )
        {
            Davros::debug() << "Next PlaylistItem found:" << item->result()->url();
            if ( !readOnly )
                proxyModel->setCurrentIndex( idx );
            return item->result();
        }

        idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
    }

    if ( !readOnly )
        proxyModel->setCurrentIndex( QModelIndex() );
    return Tomahawk::result_ptr();
}


Tomahawk::result_ptr
TreeProxyModelPlaylistInterface::currentItem() const
{
    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();
    TreeProxyModel* proxyModel = m_proxyModel.data();

    TreeModelItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->currentIndex() ) );
    if ( item && !item->result().isNull() && item->result()->isOnline() )
        return item->result();
    return Tomahawk::result_ptr();
}
