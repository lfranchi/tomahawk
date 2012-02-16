/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011  Leo Franchi <lfranchi@kde.org>
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

#include "tomahawksettings.h"

#include <QDir>

#include "sip/SipHandler.h"
#include "playlistinterface.h"


#include "libdavros/davros.h"
#include "utils/logger.h"
#include "utils/tomahawkutils.h"

#include "database/databasecommand_updatesearchindex.h"
#include "database/database.h"

using namespace Tomahawk;

TomahawkSettings* TomahawkSettings::s_instance = 0;

TomahawkSettings*
TomahawkSettings::instance()
{
    return s_instance;
}


TomahawkSettings::TomahawkSettings( QObject* parent )
    : QSettings( parent )
{
    s_instance = this;

    if( !contains( "configversion") )
    {
        setValue( "configversion", TOMAHAWK_SETTINGS_VERSION );
        doInitialSetup();
    }
    else if( value( "configversion" ).toUInt() != TOMAHAWK_SETTINGS_VERSION )
    {
        Davros::debug() << "Config version outdated, old:" << value( "configversion" ).toUInt()
                 << "new:" << TOMAHAWK_SETTINGS_VERSION
                 << "Doing upgrade, if any...";

        int current = value( "configversion" ).toUInt();
        while( current < TOMAHAWK_SETTINGS_VERSION )
        {
            doUpgrade( current, current + 1 );

            current++;
        }
        // insert upgrade code here as required
        setValue( "configversion", TOMAHAWK_SETTINGS_VERSION );
    }

}


TomahawkSettings::~TomahawkSettings()
{
    s_instance = 0;
}


void
TomahawkSettings::doInitialSetup()
{
    // by default we add a local network resolver
    addAccount( "sipzeroconf_autocreated" );
}


void
TomahawkSettings::doUpgrade( int oldVersion, int newVersion )
{
    Q_UNUSED( newVersion );

    if( oldVersion == 1 )
    {
        Davros::debug() << "Migrating config from verson 1 to 2: script resolver config name";
        if( contains( "script/resolvers" ) ) {
            setValue( "script/loadedresolvers", value( "script/resolvers" ) );
            remove( "script/resolvers" );
        }
    } else if( oldVersion == 2 )
    {
        Davros::debug() << "Migrating config from version 2 to 3: Converting jabber and twitter accounts to new SIP Factory approach";
        // migrate old accounts to new system. only jabber and twitter, and max one each. create a new plugin for each if needed
        // not pretty as we hardcode a plugin id and assume that we know how the config layout is, but hey, this is migration after all
        if( contains( "jabber/username" ) && contains( "jabber/password" ) )
        {
            QString sipName = "sipjabber";
            if( value( "jabber/username" ).toString().contains( "@gmail" ) )
                sipName = "sipgoogle";

            setValue( QString( "%1_legacy/username" ).arg( sipName ), value( "jabber/username" ) );
            setValue( QString( "%1_legacy/password" ).arg( sipName ), value( "jabber/password" ) );
            setValue( QString( "%1_legacy/autoconnect" ).arg( sipName ), value( "jabber/autoconnect" ) );
            setValue( QString( "%1_legacy/port" ).arg( sipName ), value( "jabber/port" ) );
            setValue( QString( "%1_legacy/server" ).arg( sipName ), value( "jabber/server" ) );

            addSipPlugin( QString( "%1_legacy" ).arg( sipName ) );

            remove( "jabber/username" );
            remove( "jabber/password" );
            remove( "jabber/autoconnect" );
            remove( "jabber/server" );
            remove( "jabber/port" );
        }
        if( contains( "twitter/ScreenName" ) && contains( "twitter/OAuthToken" ) )
        {
            setValue( "siptwitter_legacy/ScreenName", value( "twitter/ScreenName" ) );
            setValue( "siptwitter_legacy/OAuthToken", value( "twitter/OAuthToken" ) );
            setValue( "siptwitter_legacy/OAuthTokenSecret", value( "twitter/OAuthTokenSecret" ) );
            setValue( "siptwitter_legacy/CachedFriendsSinceID", value( "twitter/CachedFriendsSinceID" ) );
            setValue( "siptwitter_legacy/CachedMentionsSinceID", value( "twitter/CachedMentionsSinceID" ) );
            setValue( "siptwitter_legacy/CachedDirectMessagesSinceID", value( "twitter/CachedDirectMessagesSinceID" ) );
            setValue( "siptwitter_legacy/CachedPeers", value( "twitter/CachedPeers" ) );
            setValue( "siptwitter_legacy/AutoConnect", value( "jabber/autoconnect" ) );

            addSipPlugin( "siptwitter_legacy" );
            remove( "twitter/ScreenName" );
            remove( "twitter/OAuthToken" );
            remove( "twitter/OAuthTokenSecret" );
            remove( "twitter/CachedFriendsSinceID" );
            remove( "twitter/CachedMentionsSinceID" );
            remove( "twitter/CachedDirectMessagesSinceID" );
        }
        // create a zeroconf plugin too
        addSipPlugin( "sipzeroconf_legacy" );
    } else if ( oldVersion == 3 )
    {
        if ( contains( "script/atticaresolverstates" ) )
        {
            // Do messy binary upgrade. remove attica resolvers :(
            setValue( "script/atticaresolverstates", QVariant() );

            QDir resolverDir = TomahawkUtils::appDataDir();
            if ( !resolverDir.cd( QString( "atticaresolvers" ) ) )
                return;

            QStringList toremove;
            QStringList resolvers = resolverDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
            QStringList listedResolvers = allScriptResolvers();
            QStringList enabledResolvers = enabledScriptResolvers();
            foreach ( const QString& resolver, resolvers )
            {
                foreach ( const QString& r, listedResolvers )
                {
                    if ( r.contains( resolver ) )
                    {
                        Davros::debug() << "Deleting listed resolver:" << r;
                        listedResolvers.removeAll( r );
                    }
                }
                foreach ( const QString& r, enabledResolvers )
                {
                    if ( r.contains( resolver ) )
                    {
                        Davros::debug() << "Deleting enabled resolver:" << r;
                        enabledResolvers.removeAll( r );
                    }
                }
            }
            setAllScriptResolvers( listedResolvers );
            setEnabledScriptResolvers( enabledResolvers );
            Davros::debug() << "UPGRADING AND DELETING:" << resolverDir.absolutePath();
            TomahawkUtils::removeDirectory( resolverDir.absolutePath() );
        }
    } else if ( oldVersion == 4 )
    {
        // 0.3.0 contained a bug which prevent indexing local files. Force a reindex.
        QTimer::singleShot( 0, this, SLOT( updateIndex() ) );
    } else if ( oldVersion == 5 )
    {
        // Migrate to accounts from sipplugins.
        // collect old connected and enabled sip plugins
        const QStringList allSip = sipPlugins();
        const QStringList enabledSip = enabledSipPlugins();

        QStringList accounts;
        foreach ( const QString& sipPlugin, allSip )
        {
            const QStringList parts = sipPlugin.split( "_" );
            Q_ASSERT( parts.size() == 2 );

            const QString pluginName = parts[ 0 ];
            const QString pluginId = parts[ 1 ];

            // new key, <plugin>account_<id>
            QString rawpluginname = pluginName;
            rawpluginname.replace( "sip", "" );
            if ( rawpluginname.contains( "jabber" ) )
                rawpluginname.replace( "jabber", "xmpp" );

            QString accountKey = QString( "%1account_%2" ).arg( rawpluginname ).arg( pluginId );

            if ( pluginName == "sipjabber" || pluginName == "sipgoogle" )
            {
                QVariantHash credentials;
                credentials[ "username" ] = value( sipPlugin + "/username" );
                credentials[ "password" ] = value( sipPlugin + "/password" );
                credentials[ "port" ] = value( sipPlugin + "/port" );
                credentials[ "server" ] = value( sipPlugin + "/server" );

                setValue( QString( "accounts/%1/credentials" ).arg( accountKey ), credentials );
                setValue( QString( "accounts/%1/accountfriendlyname" ).arg( accountKey ), value( sipPlugin + "/username" ) );

            }
            else if ( pluginName == "siptwitter" )
            {
                // Only port twitter plugin if there's a valid twitter config
                if ( value( sipPlugin + "/oauthtokensecret" ).toString().isEmpty() &&
                     value( sipPlugin + "/oauthtoken" ).toString().isEmpty() &&
                     value( sipPlugin + "/screenname" ).toString().isEmpty() )
                    continue;

                QVariantHash credentials;
                credentials[ "oauthtoken" ] = value( sipPlugin + "/oauthtoken" );
                credentials[ "oauthtokensecret" ] = value( sipPlugin + "/oauthtokensecret" );
                credentials[ "username" ] = value( sipPlugin + "/screenname" );

                QVariantHash configuration;
                configuration[ "cachedfriendssinceid" ] = value( sipPlugin + "/cachedfriendssinceid" );
                configuration[ "cacheddirectmessagessinceid" ] = value( sipPlugin + "/cacheddirectmessagessinceid" );
                configuration[ "cachedpeers" ] = value( sipPlugin + "/cachedpeers" );
                Davros::debug() << "FOUND CACHED PEERS:" << value( sipPlugin + "/cachedpeers" ).toHash();
                configuration[ "cachedmentionssinceid" ] = value( sipPlugin + "/cachedmentionssinceid" );
                configuration[ "saveddbid" ] = value( sipPlugin + "/saveddbid" );

                setValue( QString( "accounts/%1/credentials" ).arg( accountKey ), credentials );
                setValue( QString( "accounts/%1/configuration" ).arg( accountKey ), configuration );
                setValue( QString( "accounts/%1/accountfriendlyname" ).arg( accountKey ), "@" + value( sipPlugin + "/screenname" ).toString() );
                sync();
            }
            else if ( pluginName == "sipzeroconf" )
            {
                setValue( QString( "accounts/%1/accountfriendlyname" ).arg( accountKey ), "Local Network" );
            }

            beginGroup( "accounts/" + accountKey );
            setValue( "enabled", enabledSip.contains( sipPlugin ) == true );
            setValue( "autoconnect", true );
            setValue( "acl", QVariantMap() );
            setValue( "types", QStringList() << "SipType" );
            endGroup();
            accounts << accountKey;

            remove( sipPlugin );
        }
        remove( "sip" );

        // Migrate all resolvers from old resolvers settings to new accounts system
        const QStringList allResolvers = value( "script/resolvers" ).toStringList();
        const QStringList enabledResolvers = value( "script/loadedresolvers" ).toStringList();

        foreach ( const QString& resolver, allResolvers )
        {
            const QString accountKey = QString( "resolveraccount_%1" ).arg( QUuid::createUuid().toString().mid( 1, 8 ) );
            accounts << accountKey;

            beginGroup( "accounts/" + accountKey );
            setValue( "enabled", enabledResolvers.contains( resolver ) == true );
            setValue( "autoconnect", true );
            setValue( "types", QStringList() << "ResolverType" );

            QVariantHash configuration;
            configuration[ "path" ] = resolver;

            // reasonably ugly check for attica resolvers
            if ( resolver.contains( "atticaresolvers" ) && resolver.contains( "code" ) )
            {
                setValue( "atticaresolver", true );

                QFileInfo info( resolver );
                configuration[ "atticaId" ] = info.baseName();
            }

            setValue( "configuration", configuration );
            endGroup();

        }

        remove( "script/resolvers" );
        remove( "script/loadedresolvers" );

        setValue( "accounts/allaccounts", accounts );
    }
}


void
TomahawkSettings::setAcceptedLegalWarning( bool accept )
{
    setValue( "acceptedLegalWarning", accept );
}


bool
TomahawkSettings::acceptedLegalWarning() const
{
    return value( "acceptedLegalWarning", false ).toBool();
}


void
TomahawkSettings::setInfoSystemCacheVersion( uint version )
{
    setValue( "infosystemcacheversion", version );
}


uint
TomahawkSettings::infoSystemCacheVersion() const
{
    return value( "infosystemcacheversion", 0 ).toUInt();
}


QString
TomahawkSettings::storageCacheLocation() const
{
    return QDir::tempPath() + "/tomahawk/";
}


QStringList
TomahawkSettings::scannerPaths() const
{
    QString musicLocation;

#if defined(Q_WS_X11)
    musicLocation = QDir::homePath() + QLatin1String("/Music");
#endif

    return value( "scanner/paths", musicLocation ).toStringList();
}


void
TomahawkSettings::setScannerPaths( const QStringList& paths )
{
    setValue( "scanner/paths", paths );
}


bool
TomahawkSettings::hasScannerPaths() const
{
    //FIXME: After enough time, remove this hack
    return contains( "scanner/paths" ) || contains( "scannerpath" ) || contains( "scannerpaths" );
}


uint
TomahawkSettings::scannerTime() const
{
    return value( "scanner/intervaltime", 60 ).toUInt();
}


void
TomahawkSettings::setScannerTime( uint time )
{
    setValue( "scanner/intervaltime", time );
}


bool
TomahawkSettings::watchForChanges() const
{
    return value( "scanner/watchforchanges", false ).toBool();
}


void
TomahawkSettings::setWatchForChanges( bool watch )
{
    setValue( "scanner/watchforchanges", watch );
}


bool
TomahawkSettings::httpEnabled() const
{
    return value( "network/http", true ).toBool();
}


void
TomahawkSettings::setHttpEnabled( bool enable )
{
    setValue( "network/http", enable );
}


bool
TomahawkSettings::crashReporterEnabled() const
{
    return value( "ui/crashReporter", true ).toBool();
}


void
TomahawkSettings::setCrashReporterEnabled( bool enable )
{
    setValue( "ui/crashReporter", enable );
}


QString
TomahawkSettings::proxyHost() const
{
    return value( "network/proxy/host", QString() ).toString();
}


void
TomahawkSettings::setProxyHost( const QString& host )
{
    setValue( "network/proxy/host", host );
}


QString
TomahawkSettings::proxyNoProxyHosts() const
{
    return value( "network/proxy/noproxyhosts", QString() ).toString();
}


void
TomahawkSettings::setProxyNoProxyHosts( const QString& hosts )
{
    setValue( "network/proxy/noproxyhosts", hosts );
}


qulonglong
TomahawkSettings::proxyPort() const
{
    return value( "network/proxy/port", 1080 ).toULongLong();
}


void
TomahawkSettings::setProxyPort( const qulonglong port )
{
    setValue( "network/proxy/port", port );
}


QString
TomahawkSettings::proxyUsername() const
{
    return value( "network/proxy/username", QString() ).toString();
}


void
TomahawkSettings::setProxyUsername( const QString& username )
{
    setValue( "network/proxy/username", username );
}


QString
TomahawkSettings::proxyPassword() const
{
    return value( "network/proxy/password", QString() ).toString();
}


void
TomahawkSettings::setProxyPassword( const QString& password )
{
    setValue( "network/proxy/password", password );
}


QNetworkProxy::ProxyType
TomahawkSettings::proxyType() const
{
    return static_cast< QNetworkProxy::ProxyType>( value( "network/proxy/type", QNetworkProxy::NoProxy ).toInt() );
}


void
TomahawkSettings::setProxyType( const QNetworkProxy::ProxyType type )
{
    setValue( "network/proxy/type", static_cast< uint >( type ) );
}


bool
TomahawkSettings::proxyDns() const
{
    return value( "network/proxy/dns", false ).toBool();
}


void
TomahawkSettings::setProxyDns( bool lookupViaProxy )
{
    setValue( "network/proxy/dns", lookupViaProxy );
}


QStringList
TomahawkSettings::aclEntries() const
{
    return value( "acl/entries", QStringList() ).toStringList();
}


void
TomahawkSettings::setAclEntries( const QStringList &entries )
{
    setValue( "acl/entries", entries );
}


QByteArray
TomahawkSettings::mainWindowGeometry() const
{
    return value( "ui/mainwindow/geometry" ).toByteArray();
}


void
TomahawkSettings::setMainWindowGeometry( const QByteArray& geom )
{
    setValue( "ui/mainwindow/geometry", geom );
}


QByteArray
TomahawkSettings::mainWindowState() const
{
    return value( "ui/mainwindow/state" ).toByteArray();
}


void
TomahawkSettings::setMainWindowState( const QByteArray& state )
{
    setValue( "ui/mainwindow/state", state );
}


QByteArray
TomahawkSettings::mainWindowSplitterState() const
{
    return value( "ui/mainwindow/splitterState" ).toByteArray();
}


void
TomahawkSettings::setMainWindowSplitterState( const QByteArray& state )
{
    setValue( "ui/mainwindow/splitterState", state );
}


bool
TomahawkSettings::verboseNotifications() const
{
    return value( "ui/notifications/verbose", false ).toBool();
}


void
TomahawkSettings::setVerboseNotifications( bool notifications )
{
    setValue( "ui/notifications/verbose", notifications );
}


bool
TomahawkSettings::showOfflineSources() const
{
    return value( "collection/sources/showoffline", false ).toBool();
}


void
TomahawkSettings::setShowOfflineSources( bool show )
{
    setValue( "collection/sources/showoffline", show );
}


bool
TomahawkSettings::enableEchonestCatalogs() const
{
    return value( "collection/enable_catalogs", false ).toBool();
}


void
TomahawkSettings::setEnableEchonestCatalogs( bool enable )
{
    setValue( "collection/enable_catalogs", enable );
}


QByteArray
TomahawkSettings::playlistColumnSizes( const QString& playlistid ) const
{
    return value( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ) ).toByteArray();
}


void
TomahawkSettings::setPlaylistColumnSizes( const QString& playlistid, const QByteArray& state )
{
    setValue( QString( "ui/playlist/%1/columnSizes" ).arg( playlistid ), state );
}


bool
TomahawkSettings::shuffleState( const QString& playlistid ) const
{
    return value( QString( "ui/playlist/%1/shuffleState" ).arg( playlistid )).toBool();
}


void
TomahawkSettings::setShuffleState( const QString& playlistid, bool state)
{
    setValue( QString( "ui/playlist/%1/shuffleState" ).arg( playlistid ), state );
}


void
TomahawkSettings::removePlaylistSettings( const QString& playlistid )
{
    remove( QString( "ui/playlist/%1/shuffleState" ).arg( playlistid ) );
    remove( QString( "ui/playlist/%1/repeatMode" ).arg( playlistid ) );
}


void
TomahawkSettings::setRepeatMode( const QString& playlistid, Tomahawk::PlaylistInterface::RepeatMode mode )
{
    setValue( QString( "ui/playlist/%1/repeatMode" ).arg( playlistid ), (int)mode );
}


Tomahawk::PlaylistInterface::RepeatMode
TomahawkSettings::repeatMode( const QString& playlistid )
{
    return (PlaylistInterface::RepeatMode)value( QString( "ui/playlist/%1/repeatMode" ).arg( playlistid )).toInt();
}


QList<Tomahawk::playlist_ptr>
TomahawkSettings::recentlyPlayedPlaylists() const
{
    QStringList playlist_guids = value( "playlists/recentlyPlayed" ).toStringList();

    QList<playlist_ptr> playlists;
    foreach( const QString& guid, playlist_guids )
    {
        playlist_ptr pl = Playlist::load( guid );
        if ( !pl.isNull() )
            playlists << pl;
    }

    return playlists;
}


QStringList
TomahawkSettings::recentlyPlayedPlaylistGuids( unsigned int amount ) const
{
    QStringList p = value( "playlists/recentlyPlayed" ).toStringList();

    while ( amount && p.count() > (int)amount )
        p.removeAt( 0 );

    return p;
}


void
TomahawkSettings::appendRecentlyPlayedPlaylist( const Tomahawk::playlist_ptr& playlist )
{
    QStringList playlist_guids = value( "playlists/recentlyPlayed" ).toStringList();

    playlist_guids.removeAll( playlist->guid() );
    playlist_guids.append( playlist->guid() );

    setValue( "playlists/recentlyPlayed", playlist_guids );

    emit recentlyPlayedPlaylistAdded( playlist );
}


QString
TomahawkSettings::bookmarkPlaylist() const
{
    return value( "playlists/bookmark", QString() ).toString();
}


void
TomahawkSettings::setBookmarkPlaylist( const QString& guid )
{
    setValue( "playlists/bookmark", guid );
}


QStringList
TomahawkSettings::sipPlugins() const
{
    return value( "sip/allplugins", QStringList() ).toStringList();
}


void
TomahawkSettings::setSipPlugins( const QStringList& plugins )
{
    setValue( "sip/allplugins", plugins );
}


QStringList
TomahawkSettings::enabledSipPlugins() const
{
    return value( "sip/enabledplugins", QStringList() ).toStringList();
}


void
TomahawkSettings::setEnabledSipPlugins( const QStringList& list )
{
    setValue( "sip/enabledplugins", list );
}


void
TomahawkSettings::enableSipPlugin( const QString& pluginId )
{
    QStringList list = enabledSipPlugins();
    list << pluginId;
    setEnabledSipPlugins( list );
}


void
TomahawkSettings::disableSipPlugin( const QString& pluginId )
{
    QStringList list = enabledSipPlugins();
    list.removeAll( pluginId );
    setEnabledSipPlugins( list );
}


void
TomahawkSettings::addSipPlugin( const QString& pluginId, bool enable )
{
    QStringList list = sipPlugins();
    list << pluginId;
    setSipPlugins( list );

    if ( enable )
        enableSipPlugin( pluginId );
}


void
TomahawkSettings::removeSipPlugin( const QString& pluginId )
{
    QStringList list = sipPlugins();
    list.removeAll( pluginId );
    setSipPlugins( list );

    if( enabledSipPlugins().contains( pluginId ) )
        disableSipPlugin( pluginId );
}


QStringList
TomahawkSettings::accounts() const
{
    return value( "accounts/allaccounts", QStringList() ).toStringList();
}


void
TomahawkSettings::setAccounts( const QStringList& accountIds )
{
    setValue( "accounts/allaccounts", accountIds );
}


void
TomahawkSettings::addAccount( const QString& accountId )
{
    QStringList list = accounts();
    list << accountId;
    setAccounts( list );
}


void
TomahawkSettings::removeAccount( const QString& accountId )
{
    QStringList list = accounts();
    list.removeAll( accountId );
    setAccounts( list );
}


TomahawkSettings::ExternalAddressMode
TomahawkSettings::externalAddressMode() const
{
    return (TomahawkSettings::ExternalAddressMode) value( "network/external-address-mode", TomahawkSettings::Upnp ).toInt();
}


void
TomahawkSettings::setExternalAddressMode( ExternalAddressMode externalAddressMode )
{
    setValue( "network/external-address-mode", externalAddressMode );
}


bool TomahawkSettings::preferStaticHostPort() const
{
    return value( "network/prefer-static-host-and-port" ).toBool();
}


void TomahawkSettings::setPreferStaticHostPort( bool prefer )
{
    setValue( "network/prefer-static-host-and-port", prefer );
}


QString
TomahawkSettings::externalHostname() const
{
    return value( "network/external-hostname" ).toString();
}


void
TomahawkSettings::setExternalHostname(const QString& externalHostname)
{
    setValue( "network/external-hostname", externalHostname );
}


int
TomahawkSettings::defaultPort() const
{
    return 50210;
}


int
TomahawkSettings::externalPort() const
{
    return value( "network/external-port", 50210 ).toInt();
}


void
TomahawkSettings::setExternalPort(int externalPort)
{
    if ( externalPort == 0 )
        setValue( "network/external-port", 50210);
    else
        setValue( "network/external-port", externalPort);
}


QString
TomahawkSettings::lastFmPassword() const
{
    return value( "lastfm/password" ).toString();
}


void
TomahawkSettings::setLastFmPassword( const QString& password )
{
    setValue( "lastfm/password", password );
}


QByteArray
TomahawkSettings::lastFmSessionKey() const
{
    return value( "lastfm/session" ).toByteArray();
}


void
TomahawkSettings::setLastFmSessionKey( const QByteArray& key )
{
    setValue( "lastfm/session", key );
}


QString
TomahawkSettings::lastFmUsername() const
{
    return value( "lastfm/username" ).toString();
}


void
TomahawkSettings::setLastFmUsername( const QString& username )
{
    setValue( "lastfm/username", username );
}


bool
TomahawkSettings::scrobblingEnabled() const
{
    return value( "lastfm/enablescrobbling", false ).toBool();
}


void
TomahawkSettings::setScrobblingEnabled( bool enable )
{
    setValue( "lastfm/enablescrobbling", enable );
}


QString
TomahawkSettings::xmppBotServer() const
{
    return value( "xmppBot/server", QString() ).toString();
}


void
TomahawkSettings::setXmppBotServer( const QString& server )
{
    setValue( "xmppBot/server", server );
}


QString
TomahawkSettings::xmppBotJid() const
{
    return value( "xmppBot/jid", QString() ).toString();
}


void
TomahawkSettings::setXmppBotJid( const QString& component )
{
    setValue( "xmppBot/jid", component );
}


QString
TomahawkSettings::xmppBotPassword() const
{
    return value( "xmppBot/password", QString() ).toString();
}


void
TomahawkSettings::setXmppBotPassword( const QString& password )
{
    setValue( "xmppBot/password", password );
}


int
TomahawkSettings::xmppBotPort() const
{
    return value( "xmppBot/port", -1 ).toInt();
}


void
TomahawkSettings::setXmppBotPort( const int port )
{
    setValue( "xmppBot/port", port );
}


void
TomahawkSettings::addScriptResolver(const QString& resolver)
{
    setValue( "script/resolvers", allScriptResolvers() << resolver );
}


QStringList
TomahawkSettings::allScriptResolvers() const
{
    return value( "script/resolvers" ).toStringList();
}


void
TomahawkSettings::setAllScriptResolvers( const QStringList& resolver )
{
    setValue( "script/resolvers", resolver );
}


QStringList
TomahawkSettings::enabledScriptResolvers() const
{
    return value( "script/loadedresolvers" ).toStringList();
}


void
TomahawkSettings::setEnabledScriptResolvers( const QStringList& resolvers )
{
    setValue( "script/loadedresolvers", resolvers );
}


QString
TomahawkSettings::scriptDefaultPath() const
{
    return value( "script/defaultpath", QDir::homePath() ).toString();
}


void
TomahawkSettings::setScriptDefaultPath( const QString& path )
{
    setValue( "script/defaultpath", path );
}


QString
TomahawkSettings::playlistDefaultPath() const
{
    return value( "playlists/defaultpath", QDir::homePath() ).toString();
}


void
TomahawkSettings::setPlaylistDefaultPath( const QString& path )
{
    setValue( "playlists/defaultpath", path );
}


bool
TomahawkSettings::nowPlayingEnabled() const
{
    return value( "adium/enablenowplaying", false ).toBool();
}


void
TomahawkSettings::setNowPlayingEnabled( bool enable )
{
    setValue( "adium/enablenowplaying", enable );
}

TomahawkSettings::PrivateListeningMode
TomahawkSettings::privateListeningMode() const
{
    return ( TomahawkSettings::PrivateListeningMode ) value( "privatelisteningmode", TomahawkSettings::PublicListening ).toInt();
}


void
TomahawkSettings::setPrivateListeningMode( TomahawkSettings::PrivateListeningMode mode )
{
    setValue( "privatelisteningmode", mode );
}


void
TomahawkSettings::updateIndex()
{
    DatabaseCommand* cmd = new DatabaseCommand_UpdateSearchIndex();
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}
