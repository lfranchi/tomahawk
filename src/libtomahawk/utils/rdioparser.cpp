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

#include "rdioparser.h"

#include <QUrl>
#include <QStringList>
#include "shortenedlinkparser.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"
 #include <math.h>
#include <stdlib.h>
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <QtCore/QCryptographicHash>

#define RDIO_URL "http://rdioconsole.appspot.com/call";
using namespace Tomahawk;

RdioParser::RdioParser( QObject* parent )
    : QObject( parent )
    , m_count( 0 )
{
}

RdioParser::~RdioParser()
{
}

void
RdioParser::parse( const QString& url )
{
    m_multi = false;
    m_total = 1;
    parseUrl( url );
}

void
RdioParser::parse( const QStringList& urls )
{
    m_multi = true;
    m_total = urls.count();

    foreach( const QString& url, urls )
        parseUrl( url );
}

void
RdioParser::handleRdioLink( const QString url, RdioParser::linkType type)
{

    // this is a "full" url, no redirection needed
    QString realUrl = QUrl::fromUserInput( url ).toString().replace( "_", " " );
    QString artist, trk, album, playlist;
    QString matchStr = "/%1/([^/]*)/";
    QString matchPlStr = "/%1/(?:[^/]*)/([^/]*)/";
    query_ptr query;
    m_count++;

    QRegExp r( QString( matchStr ).arg( "artist" ) );

    int loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        artist = r.cap( 1 );

    r = QRegExp( QString( matchStr ).arg( "album" ) );
    loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        album = r.cap( 1 );

    r = QRegExp( QString( matchStr ).arg( "track" ) );
    loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        trk = r.cap( 1 );

    r = QRegExp( QString( matchPlStr ).arg( "playlists" ) );
    loc = r.indexIn( realUrl );
    if ( loc >= 0 )
        playlist = r.cap( 1 );

    switch( type )
    {
        case Artist:

            if ( !artist.isEmpty() )
            {
                qDebug() << "Parsing artist" << artist;
            }
            break;

        case Album:

            if ( !album.isEmpty() && !artist.isEmpty() )
            {
                qDebug() << "Parsing album" << album << " by " << artist;
            }
            break;

        case Track:

            if ( !trk.isEmpty() && !artist.isEmpty() )
            {
                query = Query::get( artist, trk, album, uuid(), true );
            }

            if ( m_multi )
            {
                if ( !query.isNull() )
                    m_queries << query;

                if ( m_count == m_total )
                    emit tracks( m_queries );
            }
            if ( !m_multi && !query.isNull() )
                emit track( query );

            break;

        case Playlist:

                if ( !playlist.isEmpty() )
                {


                    QUrl rawurl = QUrl( QString( "POST&http://api.rdio.com/1/?extras=tracks&method=getObjectFromUrl&oauth_consumer_key=%1&oauth_nonce=%2&oauth_timestamp=%3&url=%4" )
                                        .arg("gk8zmyzj5xztt8aj48csaart")
                                        .arg( QString::number( qrand() % ((int(100000000) + 1) - int(0)) + int(0) ) )
                                        .arg( QString::number( QDateTime::currentMSecsSinceEpoch() / 1000 ) )
                                        .arg( QString( url ).remove( "http://www.rdio.com" ) )
                                        //.arg ( "yt35kakDyW" )
                                       );

                    qDebug() << "Rdio url" << rawurl.toString() << "hmac" << hmacSha1("yt35kakDyW", rawurl.toString().toLatin1() );

                    rawurl.addEncodedQueryItem( "oauth_signature",  hmacSha1("yt35kakDyW", rawurl.toString().toLatin1() ).toLatin1() );
                    rawurl.addEncodedQueryItem( "oauth_signature_method", "HMAC-SHA1" );
                    rawurl.addEncodedQueryItem( "oauth_version", "1.0" );
                    QNetworkReply* reply = TomahawkUtils::nam()->post( QNetworkRequest( rawurl.toString().remove( "POST&" ) ), rawurl.encodedQuery() );

                    connect( reply, SIGNAL( finished() ), this, SLOT( rdioReturned() ) );
                    qDebug() << "Parsing playlist"<< rawurl.toString() << playlist;
                }
            break;

        default:
            qDebug() << "Bad type";
            break;

    }


}

QString
RdioParser::hmacSha1(QByteArray key, QByteArray baseString)
{
    int blockSize = 64; // HMAC-SHA-1 block size, defined in SHA-1 standard
    if (key.length() > blockSize) { // if key is longer than block size (64), reduce key length with SHA-1 compression
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    }

    QByteArray innerPadding(blockSize, char(0x36)); // initialize inner padding with char "6"
    QByteArray outerPadding(blockSize, char(0x5c)); // initialize outer padding with char "\"
    // ascii characters 0x36 ("6") and 0x5c ("\") are selected because they have large
    // Hamming distance (http://en.wikipedia.org/wiki/Hamming_distance)

    for (int i = 0; i < key.length(); i++) {
        innerPadding[i] = innerPadding[i] ^ key.at(i); // XOR operation between every byte in key and innerpadding, of key length
        outerPadding[i] = outerPadding[i] ^ key.at(i); // XOR operation between every byte in key and outerpadding, of key length
    }

    // result = hash ( outerPadding CONCAT hash ( innerPadding CONCAT baseString ) ).toBase64
    QByteArray total = outerPadding;
    QByteArray part = innerPadding;
    part.append(baseString);
    total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
    QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);
    return hashed.toBase64();
}

void
RdioParser::rdioReturned()
{

    /// Chart request returned something! Woho
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );


    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( reply, &ok ).toMap();
        qDebug() << "RdioReply" << res;
    }else qDebug() << "RdioReply" << reply->errorString();

}


void
RdioParser::parseUrl( const QString& url )
{
    if ( url.contains( "rd.io" ) ) // shortened
    {
        ShortenedLinkParser* p = new ShortenedLinkParser( QStringList() << url, this );
        connect( p, SIGNAL( urls( QStringList ) ), this, SLOT( expandedLinks( QStringList ) ) );
        return;
    }

    if ( url.contains( "artist" ) && url.contains( "album" ) && url.contains( "track" ) )
        setLinkType( Track );
    else if ( url.contains( "artist" ) && url.contains( "album" ) )
        setLinkType( Album );
    else if ( url.contains( "artist" ) )
       setLinkType( Artist );
    else if ( url.contains( "people" ) && url.contains( "playlists" ) )
        setLinkType( Playlist );
    else
        setLinkType( None );

    if( LinkType() != None )
        handleRdioLink( url, LinkType() );

}

void
RdioParser::expandedLinks( const QStringList& urls )
{
    foreach( const QString& url, urls )
    {
        if ( url.contains( "rdio.com" ) || url.contains( "rd.io" ) )
            parseUrl( url );
    }
}

