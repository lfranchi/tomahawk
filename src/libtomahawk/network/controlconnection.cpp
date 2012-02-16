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

#include "controlconnection.h"

#include "streamconnection.h"
#include "database/database.h"
#include "database/databasecommand_collectionstats.h"
#include "dbsyncconnection.h"
#include "sourcelist.h"
#include "network/dbsyncconnection.h"
#include "network/servent.h"
#include "sip/SipHandler.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

#define TCP_TIMEOUT 600

using namespace Tomahawk;


ControlConnection::ControlConnection( Servent* parent, const QHostAddress &ha )
    : Connection( parent )
    , m_dbsyncconn( 0 )
    , m_registered( false )
    , m_pingtimer( 0 )
{
    Davros::debug() << "CTOR controlconnection";
    setId("ControlConnection()");

    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ) );

    this->setMsgProcessorModeIn( MsgProcessor::UNCOMPRESS_ALL | MsgProcessor::PARSE_JSON );
    this->setMsgProcessorModeOut( MsgProcessor::COMPRESS_IF_LARGE );

    m_peerIpAddress = ha;
}


ControlConnection::ControlConnection( Servent* parent, const QString &ha )
    : Connection( parent )
    , m_dbsyncconn( 0 )
    , m_registered( false )
    , m_pingtimer( 0 )
{
    Davros::debug() << "CTOR controlconnection";
    setId("ControlConnection()");

    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ) );

    this->setMsgProcessorModeIn( MsgProcessor::UNCOMPRESS_ALL | MsgProcessor::PARSE_JSON );
    this->setMsgProcessorModeOut( MsgProcessor::COMPRESS_IF_LARGE );

    if ( !ha.isEmpty() )
    {
        QHostAddress qha( ha );
        if ( !qha.isNull() )
            m_peerIpAddress = qha;
        else
        {
            QHostInfo qhi = QHostInfo::fromName( ha );
            if ( !qhi.addresses().isEmpty() )
                m_peerIpAddress = qhi.addresses().first();
        }
    }
}


ControlConnection::~ControlConnection()
{
    Davros::debug() << "DTOR controlconnection";

    if ( !m_source.isNull() )
        m_source->setOffline();

    delete m_pingtimer;
    m_servent->unregisterControlConnection( this );
    if ( m_dbsyncconn )
        m_dbsyncconn->deleteLater();
}

source_ptr
ControlConnection::source() const
{
    return m_source;
}

Connection*
ControlConnection::clone()
{
    ControlConnection* clone = new ControlConnection( servent(), m_peerIpAddress.toString() );
    clone->setOnceOnly( onceOnly() );
    clone->setName( name() );
    return clone;
}


void
ControlConnection::setup()
{
    Davros::debug() << Q_FUNC_INFO << id() << name();

    if ( !m_source.isNull() )
    {
        Davros::debug() << "This source seems to be online already.";
        Q_ASSERT( false );
        return;
    }

    QString friendlyName = name();

    Davros::debug() << "Detected name:" << name() << friendlyName << m_sock->peerAddress();

    // setup source and remote collection for this peer
    m_source = SourceList::instance()->get( id(), friendlyName );
    m_source->setControlConnection( this );

    // delay setting up collection/etc until source is synced.
    // we need it DB synced so it has an ID + exists in DB.
    connect( m_source.data(), SIGNAL( syncedWithDatabase() ),
                                SLOT( registerSource() ), Qt::QueuedConnection );

    m_source->setOnline();

    m_pingtimer = new QTimer;
    m_pingtimer->setInterval( 5000 );
    connect( m_pingtimer, SIGNAL( timeout() ), SLOT( onPingTimer() ) );
    m_pingtimer->start();
    m_pingtimer_mark.start();
}


// source was synced to DB, set it up properly:
void
ControlConnection::registerSource()
{
    Davros::debug() << Q_FUNC_INFO << m_source->id();
    Source* source = (Source*) sender();
    Q_UNUSED( source )
    Q_ASSERT( source == m_source.data() );

#ifndef ENABLE_HEADLESS
//    Davros::debug() << Q_FUNC_INFO << "Setting avatar ... " << name() << !SipHandler::instance()->avatar( name() ).isNull();
    if( !SipHandler::instance()->avatar( name() ).isNull() )
    {
        source->setAvatar( SipHandler::instance()->avatar( name() ) );
    }
#endif

    m_registered = true;
    m_servent->registerControlConnection( this );
    setupDbSyncConnection();
}


void
ControlConnection::setupDbSyncConnection( bool ondemand )
{
    Davros::debug() << Q_FUNC_INFO << ondemand << m_source->id() << m_dbconnkey << m_dbsyncconn << m_registered;

    if ( m_dbsyncconn || !m_registered )
        return;

    Q_ASSERT( m_source->id() > 0 );

    if( !m_dbconnkey.isEmpty() )
    {
        Davros::debug() << "Connecting to DBSync offer from peer...";
        m_dbsyncconn = new DBSyncConnection( m_servent, m_source );

        m_servent->createParallelConnection( this, m_dbsyncconn, m_dbconnkey );
        m_dbconnkey.clear();
    }
    else if( !outbound() || ondemand ) // only one end makes the offer
    {
        Davros::debug() << "Offering a DBSync key to peer...";
        m_dbsyncconn = new DBSyncConnection( m_servent, m_source );

        QString key = uuid();
        m_servent->registerOffer( key, m_dbsyncconn );
        QVariantMap m;
        m.insert( "method", "dbsync-offer" );
        m.insert( "key", key );
        sendMsg( m );
    }

    if ( m_dbsyncconn )
    {
        connect( m_dbsyncconn, SIGNAL( finished() ),
                 m_dbsyncconn,   SLOT( deleteLater() ) );

        connect( m_dbsyncconn, SIGNAL( destroyed( QObject* ) ),
                                 SLOT( dbSyncConnFinished( QObject* ) ), Qt::DirectConnection );
    }
}


void
ControlConnection::dbSyncConnFinished( QObject* c )
{
    Davros::debug() << Q_FUNC_INFO << "DBSync connection closed (for now)";
    if( (DBSyncConnection*)c == m_dbsyncconn )
    {
        //Davros::debug() << "Setting m_dbsyncconn to NULL";
        m_dbsyncconn = NULL;
    }
    else
        Davros::debug() << "Old DbSyncConn destroyed?!";
}


DBSyncConnection*
ControlConnection::dbSyncConnection()
{
    Davros::debug() << Q_FUNC_INFO << m_source->id();
    if ( !m_dbsyncconn )
    {
        setupDbSyncConnection( true );
//        Q_ASSERT( m_dbsyncconn );
    }

    return m_dbsyncconn;
}


void
ControlConnection::handleMsg( msg_ptr msg )
{
    if ( msg->is( Msg::PING ) )
    {
        // Davros::debug() << "Received Connection PING, nice." << m_pingtimer_mark.elapsed();
        m_pingtimer_mark.restart();
        return;
    }

    // if small and not compresed, print it out for debug
    if( msg->length() < 1024 && !msg->is( Msg::COMPRESSED ) )
    {
        Davros::debug() << id() << "got msg:" << QString::fromAscii( msg->payload() );
    }

    // All control connection msgs are JSON
    if( !msg->is( Msg::JSON ) )
    {
        Q_ASSERT( msg->is( Msg::JSON ) );
        markAsFailed();
        return;
    }

    QVariantMap m = msg->json().toMap();
    if( !m.isEmpty() )
    {
        if( m.value("conntype").toString() == "request-offer" )
        {
            QString theirkey = m["key"].toString();
            QString ourkey   = m["offer"].toString();
            QString theirdbid = m["controlid"].toString();
            servent()->reverseOfferRequest( this, theirdbid, ourkey, theirkey );
        }
        else if( m.value( "method" ).toString() == "dbsync-offer" )
        {
            m_dbconnkey = m.value( "key" ).toString() ;
            setupDbSyncConnection();
        }
        else if( m.value( "method" ) == "protovercheckfail" )
        {
            Davros::debug() << "*** Remote peer protocol version mismatch, connection closed";
            shutdown( true );
            return;
        }
        else
        {
            Davros::debug() << id() << "Unhandled msg:" << QString::fromAscii( msg->payload() );
        }

        return;
    }

    Davros::debug() << id() << "Invalid msg:" << QString::fromAscii(msg->payload());
}



void
ControlConnection::onPingTimer()
{
    if ( m_pingtimer_mark.elapsed() >= TCP_TIMEOUT * 1000 )
    {
        Davros::debug() << "Timeout reached! Shutting down connection to" << m_source->friendlyName();
        shutdown( true );
    }

    sendMsg( Msg::factory( QByteArray(), Msg::PING ) );
}
