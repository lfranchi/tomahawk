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

#include "PipelineStatusItem.h"

#include "utils/tomahawkutils.h"
#include "pipeline.h"
#include "tomahawkapp.h"
#include "JobStatusModel.h"

PipelineStatusItem::PipelineStatusItem()
    : JobStatusItem()
{
    m_icon.load( RESPATH"images/search-icon.png" );

    connect( Tomahawk::Pipeline::instance(), SIGNAL( resolving( Tomahawk::query_ptr ) ), this, SLOT( resolving( Tomahawk::query_ptr ) ) );
    connect( Tomahawk::Pipeline::instance(), SIGNAL( idle() ), this, SLOT( idle() ) );
}

PipelineStatusItem::~PipelineStatusItem()
{
}


QString
PipelineStatusItem::rightColumnText() const
{
    return QString( "%1" ).arg( Tomahawk::Pipeline::instance()->activeQueryCount() + Tomahawk::Pipeline::instance()->pendingQueryCount() );
}

QString
PipelineStatusItem::mainText() const
{
    return m_latestQuery;
}

void
PipelineStatusItem::idle()
{
    if ( !Tomahawk::Pipeline::instance()->activeQueryCount() )
        emit finished();
}

void
PipelineStatusItem::resolving( const Tomahawk::query_ptr& query )
{
    m_latestQuery = QString( "%1 - %2" ).arg( query->artist() ).arg( query->track() );
    emit statusChanged();
}

PipelineStatusManager::PipelineStatusManager( QObject* parent )
    : QObject(parent)
{
    connect( Tomahawk::Pipeline::instance(), SIGNAL( resolving( Tomahawk::query_ptr ) ), this, SLOT( resolving( Tomahawk::query_ptr ) ) );
}

void
PipelineStatusManager::resolving( const Tomahawk::query_ptr& p )
{
    if ( m_curItem.isNull() )
    {
        // No current query item and we're resolving something, so show it
        m_curItem = QWeakPointer< PipelineStatusItem >( new PipelineStatusItem );
        APP->mainWindow()->jobsModel()->addJob( m_curItem.data() );
    }
}
