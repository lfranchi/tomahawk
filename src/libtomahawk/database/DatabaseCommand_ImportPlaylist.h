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

#ifndef DATABASECOMMAND_IMPORTPLAYLIST_H
#define DATABASECOMMAND_IMPORTPLAYLIST_H

#include <QObject>
#include <QVariantMap>

#include "DatabaseCommand.h"

#include "DllMacro.h"

class Playlist;

class DLLEXPORT DatabaseCommand_ImportPlaylist : public DatabaseCommand
{
    Q_OBJECT
public:
    explicit DatabaseCommand_ImportPlaylist(Playlist * p, QObject *parent = 0)
        : DatabaseCommand(parent), m_playlist(p)
    {}

    virtual void exec(DatabaseImpl *);
    virtual bool doesMutates() const { return true; }
    virtual QString commandname() const { return "importplaylist"; }

signals:
    void done(int id);

private:
    Playlist * m_playlist;
};

#endif // DATABASECOMMAND_ADDFILES_H
