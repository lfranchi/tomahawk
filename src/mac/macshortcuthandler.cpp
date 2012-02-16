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

#include "macshortcuthandler.h"

#include <IOKit/hidsystem/ev_keymap.h>


#include "libdavros/davros.h"
#include "utils/logger.h"

using namespace Tomahawk;

MacShortcutHandler::MacShortcutHandler(QObject *parent) :
    Tomahawk::ShortcutHandler(parent)
{

}

void
MacShortcutHandler::macMediaKeyPressed( int key )
{
    switch (key) {
      case NX_KEYTYPE_PLAY:
        Davros::debug() << "emitting PlayPause pressed";
        emit playPause();
        break;
      case NX_KEYTYPE_FAST:
        Davros::debug() << "emitting next pressed";
        emit next();
        break;
      case NX_KEYTYPE_REWIND:
        Davros::debug() << "emitting prev pressed";
        emit previous();
        break;
    }
}
