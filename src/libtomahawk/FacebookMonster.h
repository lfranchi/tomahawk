#ifndef FACEBOOKMONSTER_H
#define FACEBOOKMONSTER_H

#include <QObject>
#include <QUrl>
#include <QWeakPointer>
#include <QMetaType>

class QDialog;
class QWebView;

typedef struct
{
    QString author;
    QString message;
    QString link;
} FacebookStatusUpdate;

class FacebookMonster : public QObject
{
    Q_OBJECT
public:
    static FacebookMonster* instance() {
        if ( !s_instance )
        {
            s_instance = new FacebookMonster();
        }
        return s_instance;
    }

    explicit FacebookMonster(QObject *parent = 0);

    void auth( QWidget* parent );

    // emits statusUpdates with a list of music-related status updates for the current user that is authenticated
    void fetchNewsFeed();

signals:
    void statusUpdates( const QList< FacebookStatusUpdate >& updates );

private slots:
    void urlChanged( const QUrl& url );

    void newsFinished();

private:
    static FacebookMonster* s_instance;

    QString m_accessToken, m_code;

    QWeakPointer<QDialog> m_authDialog;
};

Q_DECLARE_METATYPE(FacebookStatusUpdate)
Q_DECLARE_METATYPE(QList<FacebookStatusUpdate>)

#endif // FACEBOOKMONSTER_H
