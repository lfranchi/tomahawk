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

#ifndef DROPBOXACCOUNT_H
#define DROPBOXACCOUNT_H

#include "accounts/Account.h"
#include "AtticaManager.h"
#include "DllMacro.h"

#include <attica/content.h>

#include <QObject>
#include <QSet>

namespace Tomahawk
{
    class ExternalResolverGui;

namespace Accounts
{

class DropboxConfig;

class DLLEXPORT DropboxAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    DropboxAccountFactory();

    virtual Account* createAccount(const QString& accountId = QString());
    virtual QString description() const { return tr( "Search and read the music stored on your Dropbox account." ); }
    virtual QString factoryId() const { return "dropboxaccount"; }
    virtual QString prettyName() const { return "Dropbox"; }
    virtual AccountTypes types() const { return AccountTypes( ResolverType ); }
    virtual bool allowUserCreation() const { return false; }
    virtual QPixmap icon() const;
    virtual bool isUnique() const { return true; }
    bool acceptsPath( const QString& ) const;
    Account* createFromPath(const QString &);
};

class DLLEXPORT DropboxAccount : public CustomAtticaAccount
{
    Q_OBJECT
public:
    explicit DropboxAccount( const QString& accountId );
    ~DropboxAccount();

    virtual void deauthenticate();
    virtual void authenticate();

    virtual Tomahawk::InfoSystem::InfoPluginPtr infoPlugin();
    virtual SipPlugin* sipPlugin() { return 0; }

    virtual bool isAuthenticated() const;

    virtual ConnectionState connectionState() const;
    virtual QPixmap icon() const;
    virtual QWidget* aclWidget() { return 0; }
    virtual AccountConfigWidget* configurationWidget();
    virtual void saveConfig();


    QString accountName() const;
    void setAccountName( const QString& );
    QString accessToken() const;
    void setAccessToken( const QString& );
    QString accessSecret() const;
    void setAccessSecret( const QString& );

    Attica::Content atticaContent() const;

private slots:
    void atticaLoaded( Attica::Content::List );

    void resolverInstalled( const QString& resolverId );

    void resolverChanged();
private:
    void hookupResolver();

    QPointer<Tomahawk::ExternalResolverGui> m_resolver;
    QPointer<DropboxConfig> m_configWidget;

};

}

}

#endif // LASTFMACCOUNT_H
