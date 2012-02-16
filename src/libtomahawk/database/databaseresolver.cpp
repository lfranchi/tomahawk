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

#include "databaseresolver.h"

#include "pipeline.h"
#include "network/servent.h"
#include "database/database.h"
#include "database/databasecommand_resolve.h"


#include "libdavros/davros.h"
#include "utils/logger.h"


DatabaseResolver::DatabaseResolver( int weight )
    : Resolver()
    , m_weight( weight )
{
}


void
DatabaseResolver::resolve( const Tomahawk::query_ptr& query )
{
    DatabaseCommand_Resolve* cmd = new DatabaseCommand_Resolve( query );

    connect( cmd, SIGNAL( results( Tomahawk::QID, QList< Tomahawk::result_ptr > ) ),
                    SLOT( gotResults( Tomahawk::QID, QList< Tomahawk::result_ptr > ) ), Qt::QueuedConnection );
    connect( cmd, SIGNAL( albums( Tomahawk::QID, QList< Tomahawk::album_ptr > ) ),
                    SLOT( gotAlbums( Tomahawk::QID, QList< Tomahawk::album_ptr > ) ), Qt::QueuedConnection );
    connect( cmd, SIGNAL( artists( Tomahawk::QID, QList< Tomahawk::artist_ptr > ) ),
                    SLOT( gotArtists( Tomahawk::QID, QList< Tomahawk::artist_ptr > ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );

}


void
DatabaseResolver::gotResults( const Tomahawk::QID qid, QList< Tomahawk::result_ptr> results )
{
    Davros::debug() << Q_FUNC_INFO << qid << results.length();

    Tomahawk::Pipeline::instance()->reportResults( qid, results );
}


void
DatabaseResolver::gotAlbums( const Tomahawk::QID qid, QList< Tomahawk::album_ptr> albums )
{
    Tomahawk::Pipeline::instance()->reportAlbums( qid, albums );
}


void
DatabaseResolver::gotArtists( const Tomahawk::QID qid, QList< Tomahawk::artist_ptr> artists )
{
    Tomahawk::Pipeline::instance()->reportArtists( qid, artists );
}


QString
DatabaseResolver::name() const
{
    return QString( "DatabaseResolver" );
}
