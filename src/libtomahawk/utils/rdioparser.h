/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef RDIOPARSER_H
#define RDIOPARSER_H

#include <QtCore/QObject>
#include <QStringList>

#include "query.h"

class QNetworkReply;
namespace Tomahawk
{

/**
 * Small class to parse spotify links into query_ptrs
 *
 * Connect to the signals to get the results
 */

class RdioParser : public QObject
{
    Q_OBJECT
public:

    explicit RdioParser( QObject* parent = 0 );
    virtual ~RdioParser();


    enum linkType {
        None =      0x00,
        Track =     0x01,
        Album =     0x02,
        Artist =    0x04,
        Playlist =  0x06
    };
    void setLinkType( linkType type ) { m_linkType = type; }
    linkType LinkType() const { return m_linkType; }

    QString hmacSha1(QByteArray key, QByteArray baseString);
    void parse( const QString& url );
    void parse( const QStringList& urls );

signals:
    void track( const Tomahawk::query_ptr& track );
    void tracks( const QList< Tomahawk::query_ptr > tracks );
public slots:
    void rdioReturned();
private slots:
    void expandedLinks( const QStringList& );

private:
    void parseUrl( const QString& url );
    void handleRdioLink( const QString url, linkType type);
    bool m_multi;
    int m_count, m_total;
    linkType m_linkType;
    QList< query_ptr > m_queries;
};

}

#endif // RDIOPARSER_H
