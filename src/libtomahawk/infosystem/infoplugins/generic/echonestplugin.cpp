/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "echonestplugin.h"
#include <echonest/Artist.h>
#include <echonest/ArtistTypes.h>

#include "utils/tomahawkutils.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

#include <QNetworkConfiguration>

using namespace Tomahawk::InfoSystem;
using namespace Echonest;

// for internal neatness

EchoNestPlugin::EchoNestPlugin()
    : InfoPlugin()
{
    Davros::debug() << Q_FUNC_INFO;
    m_supportedGetTypes << Tomahawk::InfoSystem::InfoArtistBiography << Tomahawk::InfoSystem::InfoArtistFamiliarity << Tomahawk::InfoSystem::InfoArtistHotttness << Tomahawk::InfoSystem::InfoArtistTerms << Tomahawk::InfoSystem::InfoMiscTopTerms;
    Echonest::Config::instance()->setNetworkAccessManager( TomahawkUtils::nam() );
}


EchoNestPlugin::~EchoNestPlugin()
{
    Davros::debug() << Q_FUNC_INFO;
}


void
EchoNestPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case Tomahawk::InfoSystem::InfoArtistBiography:
            return getArtistBiography( requestData );
        case Tomahawk::InfoSystem::InfoArtistFamiliarity:
            return getArtistFamiliarity( requestData );
        case Tomahawk::InfoSystem::InfoArtistHotttness:
            return getArtistHotttnesss( requestData );
        case Tomahawk::InfoSystem::InfoArtistTerms:
            return getArtistTerms( requestData );
        case Tomahawk::InfoSystem::InfoTrackEnergy:
            return getSongProfile( requestData, "energy" );
        case Tomahawk::InfoSystem::InfoMiscTopTerms:
            return getMiscTopTerms( requestData );
        default:
        {
            emit info( requestData, QVariant() );
            return;
        }
    }
}


void
EchoNestPlugin::getSongProfile( const Tomahawk::InfoSystem::InfoRequestData &requestData, const QString &item )
{
    //WARNING: Totally not implemented yet
    Q_UNUSED( item );

    if( !isValidTrackData( requestData ) )
        return;

//     Track track( input.toString() );
//     Artist artist( customData.input()->property("artistName").toString() );
//     reply->setProperty("artist", QVariant::fromValue<Artist>(artist));
//     reply->setProperty( "input", input );
//     m_replyMap[reply] = customData;
//     connect(reply, SIGNAL(finished()), SLOT(getArtistBiographySlot()));
}


void
EchoNestPlugin::getArtistBiography( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if( !isValidArtistData( requestData ) )
        return;

    Echonest::Artist artist( requestData.input.toString() );
    QNetworkReply *reply = artist.fetchBiographies();
    reply->setProperty( "artist", QVariant::fromValue< Echonest::Artist >( artist ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
    connect( reply, SIGNAL( finished() ), SLOT( getArtistBiographySlot() ) );
}


void
EchoNestPlugin::getArtistFamiliarity( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if( !isValidArtistData( requestData ) )
        return;

    Davros::debug() << "Fetching artist familiarity!" << requestData.input;
    Echonest::Artist artist( requestData.input.toString() );
    QNetworkReply* reply = artist.fetchFamiliarity();
    reply->setProperty( "artist", QVariant::fromValue< Echonest::Artist >( artist ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
    connect( reply, SIGNAL( finished() ), SLOT( getArtistFamiliaritySlot() ) );
}


void
EchoNestPlugin::getArtistHotttnesss( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if( !isValidArtistData( requestData ) )
        return;

    Echonest::Artist artist( requestData.input.toString() );
    QNetworkReply* reply = artist.fetchHotttnesss();
    reply->setProperty( "artist", QVariant::fromValue< Echonest::Artist >( artist ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
    connect( reply, SIGNAL( finished() ), SLOT( getArtistHotttnesssSlot() ) );
}


void
EchoNestPlugin::getArtistTerms( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if( !isValidArtistData( requestData ) )
        return;

    Echonest::Artist artist( requestData.input.toString() );
    QNetworkReply* reply = artist.fetchTerms( Echonest::Artist::Weight );
    reply->setProperty( "artist", QVariant::fromValue< Echonest::Artist >( artist ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
    connect( reply, SIGNAL( finished() ), SLOT( getArtistTermsSlot() ) );
}


void
EchoNestPlugin::getMiscTopTerms( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    QNetworkReply* reply = Echonest::Artist::topTerms( 20 );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );
    connect( reply, SIGNAL( finished() ), SLOT( getMiscTopSlot() ) );
}


void
EchoNestPlugin::getArtistBiographySlot()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Echonest::Artist artist = artistFromReply( reply );
    BiographyList biographies = artist.biographies();
    QVariantMap biographyMap;
    Q_FOREACH( const Biography& biography, biographies )
    {
        QVariantHash siteData;
        siteData[ "site" ] = biography.site();
        siteData[ "url" ] = biography.url().toString();
        siteData[ "text" ] = biography.text();
        siteData[ "attribution" ] = biography.license().attribution;
        siteData[ "licensetype" ] = biography.license().type;
        siteData[ "attribution" ] = biography.license().url.toString();
        biographyMap[ biography.site() ] = siteData;
    }
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    emit info( requestData, biographyMap );
    reply->deleteLater();
}


void
EchoNestPlugin::getArtistFamiliaritySlot()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Echonest::Artist artist = artistFromReply( reply );
    qreal familiarity = artist.familiarity();
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    emit info( requestData, familiarity );
    reply->deleteLater();
}


void
EchoNestPlugin::getArtistHotttnesssSlot()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Echonest::Artist artist = artistFromReply( reply );
    qreal hotttnesss = artist.hotttnesss();
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    emit info( requestData, hotttnesss );
    reply->deleteLater();
}


void
EchoNestPlugin::getArtistTermsSlot()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    Echonest::Artist artist = artistFromReply( reply );
    TermList terms = artist.terms();
    QVariantMap termsMap;
    Q_FOREACH( const Echonest::Term& term, terms ) {
        QVariantHash termHash;
        termHash[ "weight" ] = QString::number( term.weight() );
        termHash[ "frequency" ] = QString::number( term.frequency() );
        termsMap[ term.name() ] = termHash;
    }
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    emit info( requestData, termsMap );
    reply->deleteLater();
}


void
EchoNestPlugin::getMiscTopSlot()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    TermList terms = Echonest::Artist::parseTopTerms( reply );
    QVariantMap termsMap;
    Q_FOREACH( const Echonest::Term& term, terms ) {
        QVariantHash termHash;
        termHash[ "weight" ] = QString::number( term.weight() );
        termHash[ "frequency" ] = QString::number( term.frequency() );
        termsMap[ term.name() ] = termHash;
    }
    Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();
    emit info( requestData, termsMap );
    reply->deleteLater();
}


bool
EchoNestPlugin::isValidArtistData( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if ( requestData.input.isNull() || !requestData.input.isValid() || !requestData.input.canConvert< QString >() )
    {
        emit info( requestData, QVariant() );
        return false;
    }
    QString artistName = requestData.input.toString();
    if ( artistName.isEmpty() )
    {
        emit info( requestData, QVariant() );
        return false;
    }
    return true;
}


bool
EchoNestPlugin::isValidTrackData( const Tomahawk::InfoSystem::InfoRequestData &requestData )
{
    if ( requestData.input.isNull() || !requestData.input.isValid() || !requestData.input.canConvert< QString >() )
    {
        emit info( requestData, QVariant() );
        return false;
    }
    QString trackName = requestData.input.toString();
    if ( trackName.isEmpty() )
    {
        emit info( requestData, QVariant() );
        return false;
    }
    if ( !requestData.customData.contains( "artistName" ) || requestData.customData[ "artistName" ].toString().isEmpty() )
    {
        emit info( requestData, QVariant() );
        return false;
    }
    return true;
}


Artist
EchoNestPlugin::artistFromReply( QNetworkReply* reply )
{
    Echonest::Artist artist = reply->property("artist").value<Echonest::Artist>();
    try {
        artist.parseProfile( reply );
    } catch( const Echonest::ParseError& e ) {
        qWarning() << "Caught parser error from echonest!" << e.what();
    }
    return artist;
}
//