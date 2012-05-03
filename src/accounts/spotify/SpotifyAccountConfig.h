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

#ifndef SPOTIFYACCOUNTCONFIG_H
#define SPOTIFYACCOUNTCONFIG_H

#include <QWidget>
#include <QVariantMap>
#include <QTimer>

class AnimatedSpinner;
class QShowEvent;

namespace Ui
{
    class SpotifyConfig;
}

namespace Tomahawk
{
namespace Accounts
{

class SpotifyAccount;
struct SpotifyPlaylistInfo;

class SpotifyAccountConfig : public QWidget
{
    Q_OBJECT
public:
    explicit SpotifyAccountConfig( SpotifyAccount* account );

    QString username() const;
    QString password() const;
    bool highQuality() const;
    bool deleteOnUnsync() const;

    void setPlaylists( const QList< SpotifyPlaylistInfo* >& playlists );

    void loadFromConfig();
    void saveSettings();

    void loginResponse( bool success, const QString& msg );

    bool loggedInManually() const { return m_loggedInManually; }
signals:
    void login( const QString& username, const QString& pw );

public slots:
//     void verifyResult( const QString& msgType, const QVariantMap& msg );

protected:
    void showEvent( QShowEvent* event );

private slots:
    void doLogin();
    void resetLoginButton();

private:
    Ui::SpotifyConfig* m_ui;
    SpotifyAccount* m_account;
    AnimatedSpinner* m_playlistsLoading;
    bool m_loggedInManually;
};

}
}

#endif // SPOTIFYACCOUNTCONFIG_H
