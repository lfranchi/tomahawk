/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef JOBSTATUSVIEW_H
#define JOBSTATUSVIEW_H

#include <QTreeWidget>

#include "Typedefs.h"
#include "widgets/AnimatedSplitter.h"
#include "Query.h"

class StreamConnection;

class JobStatusView : public AnimatedWidget
{
Q_OBJECT

public:
    explicit JobStatusView( AnimatedSplitter* parent );
    virtual ~JobStatusView()
    {
    }

    QSize sizeHint() const;

private slots:
    void onPipelineUpdate( const Tomahawk::query_ptr& query = Tomahawk::query_ptr() );

private:
    QTreeView* m_tree;
    AnimatedSplitter* m_parent;
};

#endif // JOBSTATUSVIEW_H
