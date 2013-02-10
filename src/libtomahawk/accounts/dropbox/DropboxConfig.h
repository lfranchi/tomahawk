/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013, Rémi Benoit <r3m1.benoit@gmail.com>
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

#ifndef DROPBOXCONFIG_H
#define DROPBOXCONFIG_H

#include "Query.h"
#include "accounts/AccountConfigWidget.h"
#include "database/DatabaseCommand_LoadSocialActions.h"

#include <QWidget>
#include <QSet>
#include <QNetworkReply>

class Ui_DropboxConfig;

namespace Tomahawk {

namespace Accounts {

class DropboxAccount;

class DropboxConfig : public AccountConfigWidget
{
    Q_OBJECT
public:
    explicit DropboxConfig( DropboxAccount* account );

    QString accountName() const;
    void setAccountName( const QString& );

private slots:
    void associateClicked();
private:

    DropboxAccount* m_account;
    Ui_DropboxConfig* m_ui;

};

}

}

#endif // LASTFMCONFIG_H
