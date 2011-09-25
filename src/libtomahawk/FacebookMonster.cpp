#include "FacebookMonster.h"

#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"
#include "DropJob.h"

#include <qjson/parser.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QWebView>
#include <QUrl>
#include <QDebug>
#include <QNetworkReply>
#include <QRegExp>

FacebookMonster* FacebookMonster::s_instance = 0;

FacebookMonster::FacebookMonster(QObject *parent) :
    QObject(parent)
{
    m_accessToken = TomahawkSettings::instance()->value( "facebook/accesstoken" ).toString();
    m_accessToken = TomahawkSettings::instance()->value( "facebook/code" ).toString();
}

void
FacebookMonster::auth( QWidget* parent )
{
    m_authDialog = QWeakPointer< QDialog >( new QDialog( parent, Qt::Sheet ) );
    m_authDialog.data()->setLayout( new QVBoxLayout );
    QWebView* v = new QWebView( m_authDialog.data() ) ;
    m_authDialog.data()->layout()->addWidget( v );

    connect( v, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));
    QString url = QString( "https://graph.facebook.com/oauth/authorize?"
                           "client_id=%1&"
                           "redirect_uri=http://www.facebook.com/connect/login_success.html&"
                           "type=user_agent&"
                           "scope=%2" ).arg( "195872347126570" )
                                       .arg( ( QStringList() << "read_stream" ).join( "," ) );

    v->load( QUrl( url ) );

    m_authDialog.data()->show();
}

void
FacebookMonster::urlChanged(const QUrl &url)
{
    qDebug() << "New URL:" << url.toString() << url.fragment();
    if ( url.path().contains( "login_success" ) )
    {
        QString frag = url.fragment();
        QStringList parts = frag.split( "&" );
        foreach ( const QString& part, parts )
        {
            QStringList pieces = part.split( "=" );
            if ( pieces.size() != 2 )
                continue;
            if ( pieces[ 0 ] == "access_token" )
                m_accessToken = pieces[ 1 ];
            if ( pieces[ 0 ] == "code" )
                m_code = pieces[ 1 ];
        }
        qDebug() << "Got access token and code:" << m_accessToken << m_code;

        TomahawkSettings::instance()->setValue( "facebook/accessToken", m_accessToken );
        TomahawkSettings::instance()->setValue( "facebook/code", m_code );

        m_authDialog.data()->hide();
        m_authDialog.data()->deleteLater();

        fetchNewsFeed();
    }
}

void
FacebookMonster::fetchNewsFeed()
{
    if ( m_accessToken.isEmpty() )
        return;

    QUrl url( "https://graph.facebook.com/me/home" );
    url.addQueryItem( "access_token", m_accessToken );
    url.addQueryItem( "limit", "1000" );
    QNetworkReply* r = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    connect( r, SIGNAL( finished() ), this, SLOT( newsFinished() ) );
}

void
FacebookMonster::newsFinished()
{
    QNetworkReply* r = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( r );

    if ( r->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap data = p.parse( r, &ok ).toMap();
        if ( !ok )
        {
            tLog() << "Failed to parse JSON from facebook! Oh Noes....";
            return;
        }

        if ( data.contains( "data" ) )
        {
            QList< FacebookStatusUpdate > entries;
            QVariantList updates = data[ "data" ].toList();
            foreach( const QVariant& entry, updates )
            {
                QVariantMap realEntry = entry.toMap();
                FacebookStatusUpdate update;

                update.message = realEntry.value( "message" ).toString();
                update.link = realEntry.value( "link" ).toString();

//                qDebug() << "Parsed entry:" << update.message << update.author;
                if ( DropJob::acceptsTrackUrl( update.link ) )
                {
                    QVariantMap from = realEntry.value( "from" ).toMap();
                    update.author = from[ "name" ].toString();
                    entries << update;
                }
            }
            emit statusUpdates( entries );
        }
    }
}
