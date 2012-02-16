/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "databasecommand_createdynamicplaylist.h"

#include <QSqlQuery>
#include <QSqlDriver>

#include "databaseimpl.h"
#include "tomahawksqlquery.h"
#include "dynamic/DynamicPlaylist.h"
#include "dynamic/DynamicControl.h"
#include "dynamic/GeneratorInterface.h"

#include "source.h"
#include "network/servent.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

#ifndef ENABLE_HEADLESS
    #include "viewmanager.h"
#endif

using namespace Tomahawk;


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( QObject* parent )
    : DatabaseCommand_CreatePlaylist( parent )
    , m_autoLoad( true )
{
    Davros::debug() << Q_FUNC_INFO << "creating dynamiccreatecommand 1";
}


DatabaseCommand_CreateDynamicPlaylist::DatabaseCommand_CreateDynamicPlaylist( const source_ptr& author,
                                                                const dynplaylist_ptr& playlist, bool autoLoad )
    : DatabaseCommand_CreatePlaylist( author, playlist.staticCast<Playlist>() )
    , m_playlist( playlist )
    , m_autoLoad( autoLoad )
{
    Davros::debug() << Q_FUNC_INFO << "creating dynamiccreatecommand 2";
}

DatabaseCommand_CreateDynamicPlaylist::~DatabaseCommand_CreateDynamicPlaylist()
{}

QVariant
DatabaseCommand_CreateDynamicPlaylist::playlistV() const
{
        if( m_v.isNull() )
            return QJson::QObjectHelper::qobject2qvariant( (QObject*)m_playlist.data() );
        else
            return m_v;
}

void
DatabaseCommand_CreateDynamicPlaylist::exec( DatabaseImpl* lib )
{
    Davros::debug() << Q_FUNC_INFO;
    Q_ASSERT( !( m_playlist.isNull() && m_v.isNull() ) );
    Q_ASSERT( !source().isNull() );

    DatabaseCommand_CreatePlaylist::createPlaylist( lib, true );
    Davros::debug() << "Created normal playlist, now creating additional dynamic info!";

    Davros::debug() << "Create dynamic execing!" << m_playlist << m_v;
    TomahawkSqlQuery cre = lib->newquery();

    cre.prepare( "INSERT INTO dynamic_playlist( guid, pltype, plmode, autoload ) "
                 "VALUES( ?, ?, ?, ? )" );

    if( m_playlist.isNull() ) {
        QVariantMap m = m_v.toMap();
        cre.addBindValue( m.value( "guid" ) );
        cre.addBindValue( m.value( "type" ) );
        cre.addBindValue( m.value( "mode" ) );
    } else {
        cre.addBindValue( m_playlist->guid() );
        cre.addBindValue( m_playlist->type() );
        cre.addBindValue( m_playlist->mode() );
    }
    cre.addBindValue( m_autoLoad );
    cre.exec();
}


void
DatabaseCommand_CreateDynamicPlaylist::postCommitHook()
{
    Davros::debug() << Q_FUNC_INFO;
    if ( source().isNull() || source()->collection().isNull() )
    {
        Davros::debug() << "Source has gone offline, not emitting to GUI.";
        return;
    }

    if(  !DatabaseCommand_CreatePlaylist::report() || report() == false )
        return;

    Davros::debug() << Q_FUNC_INFO << "..reporting..";
    if( m_playlist.isNull() ) {
        source_ptr src = source();
#ifndef ENABLE_HEADLESS
        QMetaObject::invokeMethod( ViewManager::instance(),
                                   "createDynamicPlaylist",
                                   Qt::BlockingQueuedConnection,
                                   QGenericArgument( "Tomahawk::source_ptr", (const void*)&src ),
                                   Q_ARG( QVariant, m_v ) );
#endif
    } else {
        m_playlist->reportCreated( m_playlist );
    }
    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}
