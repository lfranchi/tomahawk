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

#include "streamconnection.h"

#include <QFile>

#include "result.h"

#include "bufferiodevice.h"
#include "network/controlconnection.h"
#include "network/servent.h"
#include "database/databasecommand_loadfiles.h"
#include "database/database.h"
#include "sourcelist.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

using namespace Tomahawk;


StreamConnection::StreamConnection( Servent* s, ControlConnection* cc, QString fid, const Tomahawk::result_ptr& result )
    : Connection( s )
    , m_cc( cc )
    , m_fid( fid )
    , m_type( RECEIVING )
    , m_curBlock( 0 )
    , m_badded( 0 )
    , m_bsent( 0 )
    , m_allok( false )
    , m_result( result )
    , m_transferRate( 0 )
{
    Davros::debug() << Q_FUNC_INFO;

    BufferIODevice* bio = new BufferIODevice( result->size() );
    m_iodev = QSharedPointer<QIODevice>( bio, &QObject::deleteLater ); // device audio data gets written to
    m_iodev->open( QIODevice::ReadWrite );

    Servent::instance()->registerStreamConnection( this );

    // if the audioengine closes the iodev (skip/stop/etc) then kill the connection
    // immediately to avoid unnecessary network transfer
    connect( m_iodev.data(), SIGNAL( aboutToClose() ), SLOT( shutdown() ), Qt::QueuedConnection );
    connect( m_iodev.data(), SIGNAL( blockRequest( int ) ), SLOT( onBlockRequest( int ) ) );

    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ), Qt::QueuedConnection );

    // don't fuck with our messages at all. No compression, no parsing, nothing:
    this->setMsgProcessorModeIn ( MsgProcessor::NOTHING );
    this->setMsgProcessorModeOut( MsgProcessor::NOTHING );
}


StreamConnection::StreamConnection( Servent* s, ControlConnection* cc, QString fid )
    : Connection( s )
    , m_cc( cc )
    , m_fid( fid )
    , m_type( SENDING )
    , m_badded( 0 )
    , m_bsent( 0 )
    , m_allok( false )
    , m_transferRate( 0 )
{
    Servent::instance()->registerStreamConnection( this );
    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ), Qt::QueuedConnection );
}


StreamConnection::~StreamConnection()
{
    Davros::debug() << Q_FUNC_INFO << "TX/RX:" << bytesSent() << bytesReceived();
    if( m_type == RECEIVING && !m_allok )
    {
        Davros::debug() << "FTConnection closing before last data msg received, shame.";
        //TODO log the fact that our peer was bad-mannered enough to not finish the upload

        // protected, we could expose it:
        //m_iodev->setErrorString("FTConnection providing data went away mid-transfer");

        ((BufferIODevice*)m_iodev.data())->inputComplete();
    }

    Servent::instance()->onStreamFinished( this );
}


QString
StreamConnection::id() const
{
    return QString( "FTC[%1 %2]" )
              .arg( m_type == SENDING ? "TX" : "RX" )
              .arg( m_fid );
}


Tomahawk::source_ptr
StreamConnection::source() const
{
	return m_source;
}

void
StreamConnection::showStats( qint64 tx, qint64 rx )
{
    if( tx > 0 || rx > 0 )
    {
        Davros::debug() << id()
                 << QString( "Down: %L1 bytes/sec," ).arg( rx )
                 << QString( "Up: %L1 bytes/sec" ).arg( tx );
    }

    m_transferRate = tx + rx;
    emit updated();
}


void
StreamConnection::setup()
{
    QList<source_ptr> sources = SourceList::instance()->sources();
    foreach( const source_ptr& src, sources )
    {
        // local src doesnt have a control connection, skip it:
        if( src.isNull() || src->isLocal() )
            continue;

        if ( src->controlConnection() == m_cc )
        {
            m_source = src;
            break;
        }
    }

    connect( this, SIGNAL( statsTick( qint64, qint64 ) ), SLOT( showStats( qint64, qint64 ) ) );
    if( m_type == RECEIVING )
    {
        Davros::debug() << "in RX mode";
        emit updated();
        return;
    }

    Davros::debug() << "in TX mode, fid:" << m_fid;

    DatabaseCommand_LoadFiles* cmd = new DatabaseCommand_LoadFiles( m_fid.toUInt() );
    connect( cmd, SIGNAL( result( Tomahawk::result_ptr ) ), SLOT( startSending( Tomahawk::result_ptr ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
StreamConnection::startSending( const Tomahawk::result_ptr& result )
{
    if ( result.isNull() )
    {
        Davros::debug() << "Can't handle invalid result!";
        shutdown();
        return;
    }

    m_result = result;
    Davros::debug() << "Starting to transmit" << m_result->url();

    QSharedPointer<QIODevice> io = Servent::instance()->getIODeviceForUrl( m_result );
    if( !io )
    {
        Davros::debug() << "Couldn't read from source:" << m_result->url();
        shutdown();
        return;
    }

    m_readdev = QSharedPointer<QIODevice>( io );
    sendSome();

    emit updated();
}


void
StreamConnection::handleMsg( msg_ptr msg )
{
    Q_ASSERT( msg->is( Msg::RAW ) );

    if ( msg->payload().startsWith( "block" ) )
    {
        int block = QString( msg->payload() ).mid( 5 ).toInt();
        m_readdev->seek( block * BufferIODevice::blockSize() );

        Davros::debug() << "Seeked to block:" << block;

        QByteArray sm;
        sm.append( QString( "doneblock%1" ).arg( block ) );

        sendMsg( Msg::factory( sm, Msg::RAW | Msg::FRAGMENT ) );
        QTimer::singleShot( 0, this, SLOT( sendSome() ) );
    }
    else if ( msg->payload().startsWith( "doneblock" ) )
    {
        int block = QString( msg->payload() ).mid( 9 ).toInt();
        ((BufferIODevice*)m_iodev.data())->seeked( block );

        m_curBlock = block;
        Davros::debug() << "Next block is now:" << block;
    }
    else if ( msg->payload().startsWith( "data" ) )
    {
        m_badded += msg->payload().length() - 4;
        ((BufferIODevice*)m_iodev.data())->addData( m_curBlock++, msg->payload().mid( 4 ) );
    }

    //Davros::debug() << Q_FUNC_INFO << "flags" << (int) msg->flags()
    //         << "payload len" << msg->payload().length()
    //         << "written to device so far: " << m_badded;

    if ( ((BufferIODevice*)m_iodev.data())->nextEmptyBlock() < 0 )
    {
        m_allok = true;
        // tell our iodev there is no more data to read, no args meaning a success:
        ((BufferIODevice*)m_iodev.data())->inputComplete();
        shutdown();
    }
}


Connection*
StreamConnection::clone()
{
    Q_ASSERT( false );
    return 0;
}


void
StreamConnection::sendSome()
{
    Q_ASSERT( m_type == StreamConnection::SENDING );

    QByteArray ba = "data";
    ba.append( m_readdev->read( BufferIODevice::blockSize() ) );
    m_bsent += ba.length() - 4;

    if( m_readdev->atEnd() )
    {
        sendMsg( Msg::factory( ba, Msg::RAW ) );
        return;
    }
    else
    {
        // more to come -> FRAGMENT
        sendMsg( Msg::factory( ba, Msg::RAW | Msg::FRAGMENT ) );
    }

    // HINT: change the 0 to 50 to transmit at 640Kbps, for example
    //       (this is where upload throttling could be implemented)
    QTimer::singleShot( 0, this, SLOT( sendSome() ) );
}


void
StreamConnection::onBlockRequest( int block )
{
    Davros::debug() << Q_FUNC_INFO << block;

    if ( m_curBlock == block )
        return;

    QByteArray sm;
    sm.append( QString( "block%1" ).arg( block ) );

    sendMsg( Msg::factory( sm, Msg::RAW | Msg::FRAGMENT ) );
}
