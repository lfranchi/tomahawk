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

#include "DropboxConfig.h"
#include "ui_DropboxConfig.h"

#include <boost/bind.hpp>

#include "DropboxAccount.h"
#include "database/Database.h"
#include "database/DatabaseCommand_LogPlayback.h"
#include <database/DatabaseCommand_LoadSocialActions.h>
#include <database/DatabaseCommand_SocialAction.h>
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/Closure.h"

using namespace Tomahawk::Accounts;


DropboxConfig::DropboxConfig( DropboxAccount* account )
    : AccountConfigWidget( 0 )
    , m_account( account )
{
    m_ui = new Ui_DropboxConfig;
    m_ui->setupUi( this );

    m_ui->accountNameLabel->setText(m_account->accountName());

    connect( m_ui->associateButton, SIGNAL( clicked( bool ) ), SLOT( associateClicked() ) );

//    connect( m_ui->importHistory, SIGNAL( clicked( bool ) ), SLOT( loadHistory() ) );
//    connect( m_ui->syncLovedTracks, SIGNAL( clicked( bool ) ), SLOT( syncLovedTracks() ) );

//    connect( m_ui->username, SIGNAL( textChanged( QString ) ), SLOT( enableButton() ) );
//    connect( m_ui->password, SIGNAL( textChanged( QString ) ), SLOT( enableButton() ) );
}


QString
DropboxConfig::accountName() const
{
    return m_ui->accountNameLabel->text();
}

void
DropboxConfig::setAccountName(const QString &accountName)
{
    m_ui->accountNameLabel->setText(accountName);
}


void
DropboxConfig::associateClicked()
{

}

