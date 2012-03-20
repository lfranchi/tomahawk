/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#include "SpotifyAccountConfig.h"

#include "SpotifyAccount.h"
#include "ui_SpotifyAccountConfig.h"

#include <QListWidget>
#include <QListWidgetItem>

using namespace Tomahawk;
using namespace Accounts;

SpotifyAccountConfig::SpotifyAccountConfig( SpotifyAccount *account )
    : QWidget( 0 )
    , m_ui( new Ui::SpotifyConfig )
    , m_account( account )
{
    m_ui->setupUi( this );

    connect( m_ui->verifyCreds, SIGNAL( clicked( bool ) ), this, SLOT( verifyLogin() ) );

    m_resetTimer.setSingleShot( true );
    m_resetTimer.setInterval( 10000 );

    connect( &m_resetTimer, SIGNAL( timeout() ), this, SLOT( resetVerifyButton() ) );
    connect( m_ui->usernameEdit, SIGNAL( textChanged( QString ) ), this, SLOT( clearVerifyButton() ) );
    connect( m_ui->passwordEdit, SIGNAL( textChanged( QString ) ), this, SLOT( clearVerifyButton() ) );

    loadFromConfig();
}


void
SpotifyAccountConfig::loadFromConfig()
{
    m_ui->usernameEdit->setText( m_account->credentials().value( "username" ).toString() );
    m_ui->passwordEdit->setText( m_account->credentials().value( "password" ).toString() );
    m_ui->streamingCheckbox->setChecked( m_account->credentials().value( "highQuality" ).toBool() );
}

void
SpotifyAccountConfig::saveSettings()
{
    for( int i = 0; i < m_ui->playlistList->count(); i++ )
    {
        const QListWidgetItem* item = m_ui->playlistList->itemAt( i, 0 );
        SpotifyPlaylistInfo* pl = item->data( Qt::UserRole ).value< SpotifyPlaylistInfo* >();
        const bool toSync = ( item->checkState() == Qt::Checked );
        if ( pl->sync != toSync )
        {
            pl->changed = true;
            pl->sync = toSync;
        }
    }
}


QString
SpotifyAccountConfig::username() const
{
    return m_ui->usernameEdit->text().trimmed();
}

QString
SpotifyAccountConfig::password() const
{
    return m_ui->passwordEdit->text().trimmed();
}

bool
SpotifyAccountConfig::highQuality() const
{
    return m_ui->streamingCheckbox->isChecked();
}


void
SpotifyAccountConfig::setPlaylists( const QList<SpotifyPlaylistInfo *>& playlists )
{
    m_ui->playlistList->clear();
    foreach ( SpotifyPlaylistInfo* pl, playlists )
    {
        QListWidgetItem* item = new QListWidgetItem( pl->name, m_ui->playlistList );
        item->setData( Qt::UserRole, QVariant::fromValue< SpotifyPlaylistInfo* >( pl ) );
        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        item->setCheckState( pl->sync ? Qt::Checked : Qt::Unchecked );
    }
}


void
SpotifyAccountConfig::verifyLogin()
{
    QVariantMap msg;
    msg[ "_msgtype" ] = "checkLogin";
    msg[ "username" ] = username();
    msg[ "password" ] = password();

    m_account->sendMessage( msg, this, "verifyResult" );

    m_ui->verifyCreds->setText( tr( "Verifying..." ) );
    m_ui->verifyCreds->setEnabled( false );

    m_resetTimer.start();
}


void
SpotifyAccountConfig::resetVerifyButton()
{
    m_ui->verifyCreds->setText( tr( "Failed!" ) );
    m_ui->verifyCreds->setEnabled( true );
    m_ui->verifyCreds->setToolTip( tr( "No response from Spotify, bad credentials likely." ) );
}


void
SpotifyAccountConfig::clearVerifyButton()
{
    m_ui->verifyCreds->setText( tr( "Verify" ) );
    m_ui->verifyCreds->setEnabled( true );
    m_ui->verifyCreds->setToolTip( QString() );
}


void
SpotifyAccountConfig::verifyResult( const QString& msgType, const QVariantMap& msg )
{
    const bool success = msg.value( "success" ).toBool();
    const QString message = msg.value( "message" ).toString();

    m_resetTimer.stop();

    if ( success )
        m_ui->verifyCreds->setText( tr( "Success!" ) );
    else
    {
        m_ui->verifyCreds->setText( tr( "Failed!" ) );
        m_ui->verifyCreds->setEnabled( true );
        m_ui->verifyCreds->setToolTip( message );
    }
}