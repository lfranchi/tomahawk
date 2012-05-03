/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *             2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *             2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef SIPPLUGIN_H
#define SIPPLUGIN_H

#include "SipInfo.h"

#include <QObject>
#include <QString>
#include <QNetworkProxy>

#include "accounts/Account.h"
#ifndef ENABLE_HEADLESS
    #include <QMenu>
#endif

#include "DllMacro.h"

class SipPlugin;

class DLLEXPORT SipPlugin : public QObject
{
    Q_OBJECT

public:
    SipPlugin();
    explicit SipPlugin( Tomahawk::Accounts::Account *account, QObject* parent = 0 );
    virtual ~SipPlugin();

    // plugin id is "pluginfactoryname_someuniqueid".  get it from SipPluginFactory::generateId
    QString pluginId() const;

    virtual bool isValid() const = 0;
    virtual const QString friendlyName() const;
    virtual const QString serviceName() const;
#ifndef ENABLE_HEADLESS
    virtual QMenu* menu();
#endif
    virtual Tomahawk::Accounts::Account* account() const;

    // peer infos
    virtual const QStringList peersOnline() const;

public slots:
    virtual void connectPlugin() = 0;
    virtual void disconnectPlugin() = 0;
    virtual void checkSettings() = 0;
    virtual void configurationChanged() = 0;

    virtual void addContact( const QString &jid, const QString& msg = QString() ) = 0;
    virtual void sendMsg( const QString& to, const QString& msg ) = 0;

signals:
    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void msgReceived( const QString& from, const QString& msg );
    void sipInfoReceived( const QString& peerId, const SipInfo& info );
    void softwareVersionReceived( const QString& peerId, const QString& versionString );

#ifndef ENABLE_HEADLESS
    // new data for own source
    void avatarReceived ( const QPixmap& avatar );

    // new data for other sources;
    void avatarReceived ( const QString& from,  const QPixmap& avatar);

    void addMenu( QMenu* menu );
    void removeMenu( QMenu* menu );
#endif

    void dataError( bool );

private slots:
    void onPeerOnline( const QString &peerId );
    void onPeerOffline( const QString &peerId );

protected:
    Tomahawk::Accounts::Account *m_account;

private:
    QStringList m_peersOnline;
};

#endif
