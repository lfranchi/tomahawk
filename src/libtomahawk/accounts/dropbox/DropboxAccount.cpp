/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "DropboxAccount.h"
#include "DropboxConfig.h"

#include "infosystem/InfoSystem.h"
#include "utils/TomahawkUtilsGui.h"
#include "resolvers/QtScriptResolver.h"
#include "AtticaManager.h"
#include "Pipeline.h"
#include "accounts/AccountManager.h"
#include "Source.h"

using namespace Tomahawk;
using namespace InfoSystem;
using namespace Accounts;

DropboxAccountFactory::DropboxAccountFactory()
{
}


Account*
DropboxAccountFactory::createAccount( const QString& accountId )
{
    return new DropboxAccount( accountId.isEmpty() ? generateId( factoryId() ) : accountId );
}


QPixmap
DropboxAccountFactory::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::DropboxIcon );
}

bool
DropboxAccountFactory::acceptsPath(const QString &path) const
{
    return path.endsWith("dropbox.js");
}

Account*
DropboxAccountFactory::createFromPath(const QString &)
{
    return new DropboxAccount(generateId(factoryId()));
}


DropboxAccount::DropboxAccount( const QString& accountId )
    : CustomAtticaAccount( accountId )
{
    setAccountFriendlyName( "Dropbox" );

    AtticaManager::instance()->registerCustomAccount( "dropbox", this );

    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( resolverInstalled( QString ) ) );

    const Attica::Content res = AtticaManager::instance()->resolverForId( "dropbox" );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    if ( state == AtticaManager::Installed )
    {
        hookupResolver();
    }
}


DropboxAccount::~DropboxAccount()
{
    delete m_resolver.data();
}


void
DropboxAccount::authenticate()
{
    if ( !AtticaManager::instance()->resolversLoaded() )
    {
        // If we're still waiting to load, wait for the attica resolvers to come down the pipe
        connect( AtticaManager::instance(), SIGNAL(resolversLoaded(Attica::Content::List)), this, SLOT( atticaLoaded( Attica::Content::List ) ), Qt::UniqueConnection );
        return;
    }

    const Attica::Content res = AtticaManager::instance()->resolverForId( "dropbox" );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    qDebug() << "Dropbox account authenticating...";
    if ( m_resolver.isNull() && state == AtticaManager::Installed )
    {
        hookupResolver();
    }
    else if ( m_resolver.isNull() )
    {
        qDebug() << "Got null resolver but asked to authenticate, so installing i we have one from attica:" << res.isValid() << res.id();
        if ( res.isValid() && !res.id().isEmpty() )
            AtticaManager::instance()->installResolver( res, false );
    }
    else
    {
        m_resolver.data()->start();
    }

    emit connectionStateChanged( connectionState() );
}


void
DropboxAccount::atticaLoaded( Attica::Content::List )
{
    disconnect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( atticaLoaded( Attica::Content::List ) ) );
    authenticate();
}


void
DropboxAccount::deauthenticate()
{
    if ( !m_resolver.isNull() && m_resolver.data()->running() )
        m_resolver.data()->stop();

    emit connectionStateChanged( connectionState() );
}


AccountConfigWidget*
DropboxAccount::configurationWidget()
{
    if ( m_configWidget.isNull() )
        m_configWidget = QPointer<DropboxConfig>( new DropboxConfig( this ) );

    return m_configWidget.data();
}


Account::ConnectionState
DropboxAccount::connectionState() const
{
    return ( !m_resolver.isNull() && m_resolver.data()->running() ) ? Account::Connected : Account::Disconnected;
}


QPixmap
DropboxAccount::icon() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::DropboxIcon );
}

InfoPluginPtr
DropboxAccount::infoPlugin()
{
    //@TODO : Find out what to return
//    if ( m_infoPlugin.isNull() )
//        m_infoPlugin = QPointer< LastFmInfoPlugin >( new LastFmInfoPlugin( this ) );

//    return InfoPluginPtr( m_infoPlugin.data() );
    return NULL;
}

bool
DropboxAccount::isAuthenticated() const
{
    return !m_resolver.isNull() && m_resolver.data()->running();
}


void
DropboxAccount::saveConfig()
{
//  No data saved from the widget, OAuth data is stored in the account.
//    if ( !m_configWidget.isNull() )
//    {
//        setUsername( m_configWidget.data()->username() );
//        setPassword( m_configWidget.data()->password() );
//        setScrobble( m_configWidget.data()->scrobble() );
//    }

    sync();
}


QString
DropboxAccount::accessToken() const
{
    return credentials().value( "accessToken" ).toString();
}


void
DropboxAccount::setAccessToken( const QString& accessToken )
{
    QVariantHash creds = credentials();
    creds[ "accessToken" ] = accessToken;
    setCredentials( creds );
}

QString
DropboxAccount::accessSecret() const
{
    return credentials().value( "accessSecret" ).toString();
}


void
DropboxAccount::setAccessSecret( const QString& accessSecret )
{
    QVariantHash creds = credentials();
    creds[ "accessSecret" ] = accessSecret;
    setCredentials( creds );
}


QString
DropboxAccount::accountName() const
{
    return credentials().value( "accountName" ).toString();
}


void
DropboxAccount::setAccountName( const QString& accountName )
{
    QVariantHash creds = credentials();
    creds[ "accountName" ] = accountName;
    setCredentials( creds );
}


void
DropboxAccount::resolverInstalled( const QString &resolverId )
{
    if ( resolverId == "dropbox" )
    {
        // We requested this install, so we want to launch it
        hookupResolver();
        AccountManager::instance()->enableAccount( this );
    }
}


void
DropboxAccount::resolverChanged()
{
    emit connectionStateChanged( connectionState() );
}


void
DropboxAccount::hookupResolver()
{
    // If there is a dropbox resolver from attica installed, create the corresponding ExternalResolver* and hook up to it
    const Attica::Content res = AtticaManager::instance()->resolverForId( "dropbox" );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );
    Q_ASSERT( state == AtticaManager::Installed );
    Q_UNUSED( state );

    const AtticaManager::Resolver data = AtticaManager::instance()->resolverData( res.id() );

    m_resolver = QPointer< ExternalResolverGui >( qobject_cast< ExternalResolverGui* >( Pipeline::instance()->addScriptResolver( data.scriptPath ) ) );
    m_resolver.data()->setIcon( icon() );
    connect( m_resolver.data(), SIGNAL( changed() ), this, SLOT( resolverChanged() ) );
}


Attica::Content
DropboxAccount::atticaContent() const
{
    return AtticaManager::instance()->resolverForId( "dropbox" );
}
