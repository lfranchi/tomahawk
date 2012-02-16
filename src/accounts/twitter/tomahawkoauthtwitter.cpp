#include "tomahawkoauthtwitter.h"

#include <QtGui/QInputDialog>


#include "libdavros/davros.h"
#include "utils/logger.h"


TomahawkOAuthTwitter::TomahawkOAuthTwitter( QNetworkAccessManager *nam, QObject* parent )
    : OAuthTwitter( QByteArray::fromBase64( "QzR2NFdmYTIxcmZJRGNrNEhNUjNB" ), QByteArray::fromBase64( "elhTalU2Ympydmc2VVZNSlg0SnVmcUh5amozaWV4dFkxNFNSOXVCRUFv" ), parent )
{
    setNetworkAccessManager( nam );
}


int
TomahawkOAuthTwitter::authorizationWidget()
{
    bool ok;
    int i = QInputDialog::getInt(0, tr( "Twitter PIN" ), tr( "After authenticating on Twitter's web site,\nenter the displayed PIN number here:" ), 0, 0, 2147483647, 1, &ok);
    if (ok)
        return i;

    return 0;
}

void
TomahawkOAuthTwitter::error()
{
    Davros::debug() << Q_FUNC_INFO;
    setOAuthToken( QString().toLatin1() );
    setOAuthTokenSecret( QString().toLatin1() );
}
