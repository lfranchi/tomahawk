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

#ifndef REMOTECOLLECTION_H
#define REMOTECOLLECTION_H

#include "typedefs.h"

#include "network/controlconnection.h"
#include "database/databasecollection.h"

class RemoteCollection : public DatabaseCollection
{
Q_OBJECT

friend class ControlConnection; // for receiveTracks()

public:
    explicit RemoteCollection( Tomahawk::source_ptr source, QObject* parent = 0 );
    ~RemoteCollection()
    {
        Davros::debug() << Q_FUNC_INFO;
    }

public slots:
    virtual void addTracks( const QList<QVariant>& newitems );
    virtual void removeTracks( const QDir& dir );
};

#endif // REMOTECOLLECTION_H
