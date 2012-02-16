/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "SipHandler.h"
#include "sip/SipPlugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QPluginLoader>

#ifndef ENABLE_HEADLESS
    #include <QMessageBox>
#endif

#include "functimeout.h"

#include "database/database.h"
#include "network/controlconnection.h"
#include "network/servent.h"
#include "sourcelist.h"
#include "tomahawksettings.h"

#include "libdavros/davros.h"
#include "utils/logger.h"
#include "accounts/AccountManager.h"

#include "config.h"

SipHandler* SipHandler::s_instance = 0;


SipHandler*
SipHandler::instance()
{
    if ( !s_instance )
        new SipHandler( 0 );

    return s_instance;
}


SipHandler::SipHandler( QObject* parent )
    : QObject( parent )
{
    s_instance = this;
}


SipHandler::~SipHandler()
{
    Davros::debug() << Q_FUNC_INFO;
    s_instance = 0;
}


#ifndef ENABLE_HEADLESS
const QPixmap
SipHandler::avatar( const QString& name ) const
{
//    Davros::debug() << Q_FUNC_INFO << "Getting avatar" << name; // << m_usernameAvatars.keys();
    if( m_usernameAvatars.contains( name ) )
    {
//        Davros::debug() << Q_FUNC_INFO << "Getting avatar and avatar != null ";
        Q_ASSERT(!m_usernameAvatars.value( name ).isNull());
        return m_usernameAvatars.value( name );
    }
    else
    {
//        Davros::debug() << Q_FUNC_INFO << "Getting avatar and avatar == null :-(";
        return QPixmap();
    }
}
#endif

const SipInfo
SipHandler::sipInfo( const QString& peerId ) const
{
    return m_peersSipInfos.value( peerId );
}

const QString
SipHandler::versionString( const QString& peerId ) const
{
    return m_peersSoftwareVersions.value( peerId );
}


void
SipHandler::hookUpPlugin( SipPlugin* sip )
{
    QObject::connect( sip, SIGNAL( peerOnline( QString ) ), SLOT( onPeerOnline( QString ) ) );
    QObject::connect( sip, SIGNAL( peerOffline( QString ) ), SLOT( onPeerOffline( QString ) ) );
    QObject::connect( sip, SIGNAL( msgReceived( QString, QString ) ), SLOT( onMessage( QString, QString ) ) );
    QObject::connect( sip, SIGNAL( sipInfoReceived( QString, SipInfo ) ), SLOT( onSipInfo( QString, SipInfo ) ) );
    QObject::connect( sip, SIGNAL( softwareVersionReceived( QString, QString ) ), SLOT( onSoftwareVersion( QString, QString ) ) );

    QObject::connect( sip, SIGNAL( avatarReceived( QString, QPixmap ) ), SLOT( onAvatarReceived( QString, QPixmap ) ) );
    QObject::connect( sip, SIGNAL( avatarReceived( QPixmap ) ), SLOT( onAvatarReceived( QPixmap ) ) );

    QObject::connect( sip->account(), SIGNAL( configurationChanged() ), sip, SLOT( configurationChanged() ) );
}


void
SipHandler::onPeerOnline( const QString& jid )
{
//    Davros::debug() << Q_FUNC_INFO;
    Davros::debug() << "SIP online:" << jid;

    SipPlugin* sip = qobject_cast<SipPlugin*>(sender());

    QVariantMap m;
    if( Servent::instance()->visibleExternally() )
    {
        QString key = uuid();
        ControlConnection* conn = new ControlConnection( Servent::instance(), QString() );

        const QString& nodeid = Database::instance()->dbid();
        conn->setName( jid.left( jid.indexOf( "/" ) ) );
        conn->setId( nodeid );

        Servent::instance()->registerOffer( key, conn );
        m["visible"] = true;
        m["ip"] = Servent::instance()->externalAddress();
        m["port"] = Servent::instance()->externalPort();
        m["key"] = key;
        m["uniqname"] = nodeid;

        Davros::debug() << "Asking them to connect to us:" << m;
    }
    else
    {
        m["visible"] = false;
        Davros::debug() << "We are not visible externally:" << m;
    }

    QJson::Serializer ser;
    QByteArray ba = ser.serialize( m );

    sip->sendMsg( jid, QString::fromAscii( ba ) );
}


void
SipHandler::onPeerOffline( const QString& jid )
{
//    Davros::debug() << Q_FUNC_INFO;
    Davros::debug() << "SIP offline:" << jid;
}


void
SipHandler::onSipInfo( const QString& peerId, const SipInfo& info )
{
    Davros::debug() << Q_FUNC_INFO << "SIP Message:" << peerId << info;

    /*
      If only one party is externally visible, connection is obvious
      If both are, peer with lowest IP address initiates the connection.
      This avoids dupe connections.
     */
    if ( info.isVisible() )
    {
        if( !Servent::instance()->visibleExternally() ||
            Servent::instance()->externalAddress() <= info.host().hostName() )
        {
            Davros::debug() << "Initiate connection to" << peerId;
            Servent::instance()->connectToPeer( info.host().hostName(),
                                          info.port(),
                                          info.key(),
                                          peerId,
                                          info.uniqname() );
        }
        else
        {
            Davros::debug() << Q_FUNC_INFO << "They should be conecting to us...";
        }
    }
    else
    {
        Davros::debug() << Q_FUNC_INFO << "They are not visible, doing nothing atm";
    }

    m_peersSipInfos.insert( peerId, info );
}

void SipHandler::onSoftwareVersion(const QString& peerId, const QString& versionString)
{
    m_peersSoftwareVersions.insert( peerId, versionString );
}

void
SipHandler::onMessage( const QString& from, const QString& msg )
{
    Davros::debug() << Q_FUNC_INFO << from << msg;
}

#ifndef ENABLE_HEADLESS
void
SipHandler::onAvatarReceived( const QString& from, const QPixmap& avatar )
{
//    Davros::debug() << Q_FUNC_INFO << "setting avatar on source for" << from;
    if ( avatar.isNull() )
    {
//        Davros::debug() << Q_FUNC_INFO << "got null pixmap, not adding anything";
        return;
    }

    m_usernameAvatars.insert( from, avatar );

    //

    //Tomahawk::source_ptr source = ->source();
    ControlConnection *conn = Servent::instance()->lookupControlConnection( from );
    if( conn )
    {
//        Davros::debug() << Q_FUNC_INFO << from << "got control connection";
        Tomahawk::source_ptr source = conn->source();
        if( source )
        {

//            Davros::debug() << Q_FUNC_INFO << from << "got source, setting avatar";
            source->setAvatar( avatar );
        }
        else
        {
//            Davros::debug() << Q_FUNC_INFO << from << "no source found, not setting avatar";
        }
    }
    else
    {
//        Davros::debug() << Q_FUNC_INFO << from << "no control connection setup yet";
    }
}


void
SipHandler::onAvatarReceived( const QPixmap& avatar )
{
//    Davros::debug() << Q_FUNC_INFO << "Set own avatar on MyCollection";
    SourceList::instance()->getLocal()->setAvatar( avatar );
}
#endif
