/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef ALBUMPROXYMODELPLAYLISTINTERFACE_H
#define ALBUMPROXYMODELPLAYLISTINTERFACE_H

#include "PlaylistInterface.h"
#include "playlist/AlbumModel.h"

#include "DllMacro.h"

class AlbumProxyModel;

namespace Tomahawk
{

class DLLEXPORT AlbumProxyModelPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    explicit AlbumProxyModelPlaylistInterface( AlbumProxyModel *proxyModel );
    virtual ~AlbumProxyModelPlaylistInterface();

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int unfilteredTrackCount() const;
    virtual int trackCount() const;

    virtual bool hasNextItem() { return true; }
    virtual Tomahawk::result_ptr currentItem() const;
    virtual Tomahawk::result_ptr siblingItem( int direction );

    virtual QString filter() const;
    virtual void setFilter( const QString& pattern );

    virtual Tomahawk::PlaylistInterface::RepeatMode repeatMode() const { return m_repeatMode; }
    virtual bool shuffled() const { return m_shuffled; }
    virtual Tomahawk::PlaylistInterface::ViewMode viewMode() const { return Tomahawk::PlaylistInterface::Album; }

signals:
    void repeatModeChanged( Tomahawk::PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

    void nextTrackReady();

public slots:
    virtual void setRepeatMode( Tomahawk::PlaylistInterface::RepeatMode mode ) { m_repeatMode = mode; emit repeatModeChanged( mode ); }
    virtual void setShuffled( bool enabled ) { m_shuffled = enabled; emit shuffleModeChanged( enabled ); }

private:
    QWeakPointer< AlbumProxyModel > m_proxyModel;
    RepeatMode m_repeatMode;
    bool m_shuffled;
};

} //ns

#endif // ALBUMPROXYMODELPLAYLISTINTERFACE_H
