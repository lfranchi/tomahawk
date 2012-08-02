/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Casey Link <unnamedrambler@gmail.com>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "ChartsPlugin.h"

#include <QDir>
#include <QSettings>
#include <QNetworkConfiguration>
#include <QNetworkReply>
#include <QtPlugin>

#include "Album.h"
#include "CountryUtils.h"
#include "Typedefs.h"
#include "audio/AudioEngine.h"
#include "TomahawkSettings.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"
#include "utils/TomahawkCache.h"
#include "Source.h"

#define CHART_URL "http://charts.tomahawk-player.org/"
//#define CHART_URL "http://localhost:8080/"
#include <qjson/parser.h>
#include <qjson/serializer.h>

namespace Tomahawk
{

namespace InfoSystem
{

ChartsPlugin::ChartsPlugin()
    : InfoPlugin()
    , m_chartsFetchJobs( 0 )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QThread::currentThread();
    /// If you add resource, update version aswell
    m_chartVersion = "2.3";
    m_supportedGetTypes <<  InfoChart << InfoChartCapabilities;
}


ChartsPlugin::~ChartsPlugin()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QThread::currentThread();
}


void
ChartsPlugin::init()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QThread::currentThread();
    QVariantList source_qvarlist = TomahawkUtils::Cache::instance()->getData( "ChartsPlugin", "chart_sources" ).toList();
    foreach( const QVariant & source, source_qvarlist ) {
        m_chartResources.append( source.toString() );
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "fetched source from cache" << source.toString();

    }
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "total sources" << m_chartResources.size() << source_qvarlist.size();
    if( m_chartResources.size() == 0 )
        fetchChartSourcesList( true );
}


void
ChartsPlugin::dataError( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    emit info( requestData, QVariant() );
    return;
}


void
ChartsPlugin::getInfo( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    //qDebug() << Q_FUNC_INFO << requestData.caller;
    //qDebug() << Q_FUNC_INFO << requestData.customData;

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    bool foundSource = false;

    switch ( requestData.type )
    {
        case InfoChart:
            /// We need something to check if the request is actually ment to go to this plugin
            if ( !hash.contains( "chart_source" ) )
            {
                tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required param!";
                dataError( requestData );
                break;
            }
            else
            {
                foreach( QString resource, m_chartResources )
                {
                    if( resource == hash["chart_source"] )
                    {
                        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "get source" << resource;
                        foundSource = true;
                    }
                }

                if( !foundSource )
                {
                    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "no such source" << hash["chart_source"] << "(" << m_chartResources.size() << " total sources)";
                    dataError( requestData );
                    break;
                }

            }
            fetchChartFromCache( requestData );
            break;

        case InfoChartCapabilities:
            fetchChartCapabilitiesFromCache( requestData );
            break;
        default:
            dataError( requestData );
    }
}


void
ChartsPlugin::fetchChartFromCache( Tomahawk::InfoSystem::InfoRequestData requestData )
{

    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not convert requestData to InfoStringHash!";
        dataError( requestData );
        return;
    }

    InfoStringHash hash = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
    Tomahawk::InfoSystem::InfoStringHash criteria;

    /// Each request needs to contain both a id and source
    if ( !hash.contains( "chart_id" ) && !hash.contains( "chart_source" ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Hash did not contain required params!";
        dataError( requestData );
        return;

    }
    /// Set the criterias for current chart
    criteria["chart_id"] = hash["chart_id"];
    criteria["chart_source"] = hash["chart_source"];
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking cache for " << hash["chart_id"] << " from " << hash["chart_source"];


    emit getCachedInfo( criteria, 86400000, requestData );
}

void
ChartsPlugin::fetchChartCapabilitiesFromCache( Tomahawk::InfoSystem::InfoRequestData requestData )
{
    if ( !requestData.input.canConvert< Tomahawk::InfoSystem::InfoStringHash >() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Could not convert requestData to InfoStringHash!";
        dataError( requestData );
        return;
    }

    Tomahawk::InfoSystem::InfoStringHash criteria;
    criteria[ "InfoChartCapabilities" ] = "chartsplugin";
    criteria[ "InfoChartVersion" ] = m_chartVersion;
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking cache for " << "InfoChartCapabilities" << m_chartVersion;
    emit getCachedInfo( criteria, 864000000, requestData );
}

void
ChartsPlugin::notInCacheSlot( QHash<QString, QString> criteria, Tomahawk::InfoSystem::InfoRequestData requestData )
{
    switch ( requestData.type )
    {
        case InfoChart:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChart not in cache! Fetching...";
            fetchChart( requestData, criteria["chart_source"], criteria["chart_id"] );
            return;

        }

        case InfoChartCapabilities:
        {
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChartCapabilities not in cache! Fetching...";
            fetchChartSourcesList( false );
            m_cachedRequests.append( requestData );

            return;
        }

        default:
        {
            tLog() << Q_FUNC_INFO << "Couldn't figure out what to do with this type of request after cache miss";
            emit info( requestData, QVariant() );
            return;
        }
    }
}

void
ChartsPlugin::fetchChartSourcesList( bool fetchOnlySourceList )
{
    QUrl url = QUrl( QString( CHART_URL "charts" ) );
    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    reply->setProperty( "only_source_list", fetchOnlySourceList );


    tDebug() << Q_FUNC_INFO << "fetching:" << url;
    connect( reply, SIGNAL( finished() ), SLOT( chartSourcesList() ) );

}

void
ChartsPlugin::chartSourcesList()
{
    tDebug( LOGVERBOSE )  << Q_FUNC_INFO << "Got chart sources list";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse( reply, &ok ).toMap();
        const QVariantList sources = res.value( "sources" ).toList();

        if ( !ok )
        {
            tLog() << Q_FUNC_INFO << "Failed to parse sources" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        m_chartResources.clear();
        foreach(const QVariant &source, sources) {
            m_chartResources << source.toString();

        }
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "storing sources in cache" << m_chartResources;
        TomahawkUtils::Cache::instance()->putData( "ChartsPlugin", 172800000 /* 2 days */, "chart_sources", m_chartResources );
        if( !reply->property("only_source_list" ).toBool() )
            fetchAllChartSources();
    }
    else
        tDebug() << Q_FUNC_INFO << "Encountered error fetching chart sources list";
}

void ChartsPlugin::fetchAllChartSources()
{
    if ( !m_chartResources.isEmpty() && m_allChartsMap.isEmpty() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "InfoChart fetching source data";
        foreach ( QString source, m_chartResources )
        {
            QUrl url = QUrl( QString( CHART_URL "charts/%1" ).arg( source ) );
            QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
            reply->setProperty( "chart_source", source);

            tDebug() << Q_FUNC_INFO << "fetching:" << url;
            connect( reply, SIGNAL( finished() ), SLOT( chartsList() ) );

            m_chartsFetchJobs++;
        }
    }
}

void ChartsPlugin::fetchChart( Tomahawk::InfoSystem::InfoRequestData requestData, const QString& source, const QString& chart_id)
{
    /// Fetch the chart, we need source and id
    QUrl url = QUrl( QString( CHART_URL "charts/%1/%2" ).arg( source ).arg( chart_id ) );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "fetching: " << url;

    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    reply->setProperty( "requestData", QVariant::fromValue< Tomahawk::InfoSystem::InfoRequestData >( requestData ) );

    connect( reply, SIGNAL( finished() ), SLOT( chartReturned() ) );
}



void
ChartsPlugin::chartsList()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Got chart list result";
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        const QVariantMap res = p.parse( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse resources" << p.errorString() << "On line" << p.errorLine();

            return;
        }

        /// Got types, append!
        const QString source = reply->property("chart_source").toString();

        // We'll populate charts with the data from the server
        QVariantMap charts;
        QString chartName;
        QStringList defaultChain;
        if ( source == "wearehunted" || source == "itunes" )
        {
            // Some charts can have an extra param, itunes has geo, WAH has emerging/mainstream
            // Itunes has geographic-area based charts. So we build a breadcrumb of
            // ITunes - Country - Albums - Top Chart Type
            //                  - Tracks - Top Chart Type
            // WeAreHunted has Mainstream/Emerging
            // WeAreHunted - Type - Artists - Chart Type
            //                    - Tracks  - Chart Type
            QHash< QString, QVariantMap > extraType;
            foreach( const QVariant& chartObj, res.values() )
            {
                if( !chartObj.toMap().isEmpty() )
                {
                    const QVariantMap chart = chartObj.toMap();
                    const QString id = chart.value( "id" ).toString();
                    const QString geo = chart.value( "geo" ).toString();
                    QString name = chart.value( "genre" ).toString();
                    const QString type = chart.value( "type" ).toString();
                    const bool isDefault = ( chart.contains( "default" ) && chart[ "default" ].toInt() == 1 );

                    QString extra;
                    if( !geo.isEmpty() )
                    {

                        if ( !m_cachedCountries.contains( geo ) )
                        {
                            QLocale l( QString( "en_%1" ).arg( geo ) );
                            extra = Tomahawk::CountryUtils::fullCountryFromCode( geo );

                            for ( int i = 1; i < extra.size(); i++ )
                            {
                                if ( extra.at( i ).isUpper() )
                                {
                                    extra.insert( i, " " );
                                    i++;
                                }
                            }
                            m_cachedCountries[ geo ] = extra;
                        }
                        else
                            extra = m_cachedCountries[ geo ];
                    }
                    else
                        extra = chart.value( "extra" ).toString();

                    if ( name.isEmpty() ) // not a specific chart, an all chart
                        name = tr( "Top Overall" );

                    InfoStringHash c;
                    c[ "id" ] = id;
                    c[ "label" ] = name;
                    c[ "type" ] = "album";
                    if ( isDefault )
                        c[ "default" ] = "true";

                    QList< Tomahawk::InfoSystem::InfoStringHash > extraTypeData = extraType[ extra ][ type ].value< QList< Tomahawk::InfoSystem::InfoStringHash > >();
                    extraTypeData.append( c );
                    extraType[ extra ][ type ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( extraTypeData );

                    if ( isDefault )
                    {
                        defaultChain.clear();
                        defaultChain.append( extra );
                        defaultChain.append( type );
                        defaultChain.append( name );
                    }
                }
                foreach( const QString& c, extraType.keys() )
                {
                    charts[ c ] = extraType[ c ];
//                    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "extraType has types:" << c;
                }
                if( source == "itunes" ){
                    chartName = "iTunes";
                }
                if( source == "soundcloudwall" ){
                    chartName = "SoundCloudWall";
                }
                if( source == "wearehunted" ){
                    chartName = "WeAreHunted";
                }

            }

        }
        else
        {
            // We'll just build:
            // [Source] - Album - Chart Type
            // [Source] - Track - Chart Type
            QList< InfoStringHash > albumCharts;
            QList< InfoStringHash > trackCharts;
            QList< InfoStringHash > artistCharts;

            foreach( const QVariant& chartObj, res.values() )
            {
                if( !chartObj.toMap().isEmpty() ){
                    const QVariantMap chart = chartObj.toMap();
                    const QString type = chart.value( "type" ).toString();
                    const bool isDefault = ( chart.contains( "default" ) && chart[ "default" ].toInt() == 1 );

                    InfoStringHash c;
                    c[ "id" ] = chart.value( "id" ).toString();
                    if( chart.value( "genre").isValid() )
                        c[ "label" ] = chart.value( "genre" ).toString();
                    else
                        c[ "label" ] = chart.value( "name" ).toString();
                    if ( isDefault )
                        c[ "default" ] = "true";

                    if ( type == "Album" )
                    {
                        c[ "type" ] = "album";
                        albumCharts.append( c );
                    }
                    else if ( type == "Track" )
                    {
                        c[ "type" ] = "tracks";
                        trackCharts.append( c );

                    }else if ( type == "Artist" )
                    {
                        c[ "type" ] = "artists";
                        artistCharts.append( c );

                    }

                    if ( isDefault )
                    {
                        defaultChain.clear();
                        defaultChain.append( type + "s" ); //UGLY but it's plural to the user, see below
                        defaultChain.append( c[ "label" ] );
                    }
                }
                if( !artistCharts.isEmpty() )
                    charts.insert( tr( "Artists" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( artistCharts ) );
                if( !albumCharts.isEmpty() )
                    charts.insert( tr( "Albums" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( albumCharts ) );
                if( !trackCharts.isEmpty() )
                    charts.insert( tr( "Tracks" ), QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( trackCharts ) );

                /// @note For displaying purposes, upper the first letter
                /// @note Remeber to lower it when fetching this!
                chartName = source;
                chartName[0] = chartName[0].toUpper();
            }
        }

        /// Add the possible charts and its types to breadcrumb
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "ADDING CHART TO CHARTS:" << chartName;
        QVariantMap defaultMap = m_allChartsMap.value( "defaults" ).value< QVariantMap >();
        defaultMap[ source ] = defaultChain;
        m_allChartsMap[ "defaults" ] = defaultMap;
        m_allChartsMap[ "defaultSource" ] = "itunes";
        m_allChartsMap.insert( chartName , QVariant::fromValue< QVariantMap >( charts ) );

    }
    else
    {
        tLog() << "Error fetching charts:" << reply->errorString();
    }

    m_chartsFetchJobs--;
    if ( !m_cachedRequests.isEmpty() && m_chartsFetchJobs == 0 )
    {
        foreach ( InfoRequestData request, m_cachedRequests )
        {
            emit info( request, m_allChartsMap );
            // update cache
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Updating cache with " << m_allChartsMap.size() << "charts";
            Tomahawk::InfoSystem::InfoStringHash criteria;
            criteria[ "InfoChartCapabilities" ] = "chartsplugin";
            criteria[ "InfoChartVersion" ] = m_chartVersion;
            emit updateCache( criteria, 864000000, request.type, m_allChartsMap );
        }
        m_cachedRequests.clear();
    }

}

void
ChartsPlugin::chartReturned()
{

    /// Chart request returned something! Woho
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );
    QVariantMap returnedData;

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJson::Parser p;
        bool ok;
        QVariantMap res = p.parse( reply, &ok ).toMap();

        if ( !ok )
        {
            tLog() << "Failed to parse json from chart lookup:" << p.errorString() << "On line" << p.errorLine();
            return;
        }

        /// SO we have a result, parse it!
        QVariantList chartResponse = res.value( "list" ).toList();
        QList< Tomahawk::InfoSystem::InfoStringHash > top_tracks;
        QList< Tomahawk::InfoSystem::InfoStringHash > top_albums;
        QStringList top_artists;

        /// Deside what type, we need to handle it differently
        /// @todo: We allready know the type, append it to breadcrumb hash

        if( res.value( "type" ).toString() == "Album" )
            setChartType( Album );
        else if( res.value( "type" ).toString() == "Track" )
            setChartType( Track );
        else if( res.value( "type" ).toString() == "Artist" )
            setChartType( Artist );
        else
            setChartType( None );


//         qDebug() << "Got chart returned!" << res;
        foreach ( QVariant chartR, chartResponse )
        {
            QString title, artist, album;
            QVariantMap chartMap = chartR.toMap();

            if ( !chartMap.isEmpty() )
            {

                title = chartMap.value( "track" ).toString();
                album = chartMap.value( "album" ).toString();
                artist = chartMap.value( "artist" ).toString();
                /// Maybe we can use rank later on, to display something nice
                /// rank = chartMap.value( "rank" ).toString();

                if ( chartType() == Album )
                {

                    if ( album.isEmpty() && artist.isEmpty() ) // don't have enough...
                    {
                        tDebug( LOGVERBOSE )  << "Didn't get an artist and album name from chart, not enough to build a query on. Aborting" << title << album << artist;

                    }
                    else
                    {

                        Tomahawk::InfoSystem::InfoStringHash pair;
                        pair["artist"] = artist;
                        pair["album"] = album;
                        top_albums.append( pair );

                    }
                }

                else if ( chartType() == Track )
                {

                    if ( title.isEmpty() && artist.isEmpty() ) // don't have enough...
                    {
                        tDebug( LOGVERBOSE )  << "Didn't get an artist and track name from charts, not enough to build a query on. Aborting" << title << artist << album;

                    }
                    else
                    {

                        Tomahawk::InfoSystem::InfoStringHash pair;
                        pair["artist"] = artist;
                        pair["track"] = title;
                        top_tracks.append( pair );

                    }
                }else if( chartType() == Artist )
                {
                    if ( artist.isEmpty() ) // don't have enough...
                    {
                        tDebug( LOGVERBOSE )  << "Didn't get an artist from charts, not enough to build a query on. Aborting" << artist;

                    }
                    else
                    {
                        top_artists.append( artist );
                    }

                }
            }
        }

        if( chartType() == Artist )
        {
            tDebug() << "ChartsPlugin:" << "\tgot " << top_artists.size() << " artists";
            returnedData[ "artists" ] = QVariant::fromValue< QStringList >( top_artists );
            returnedData[ "type" ] = "artists";
        }

        if( chartType() == Track )
        {
            tDebug() << "ChartsPlugin:" << "\tgot " << top_tracks.size() << " tracks";
            returnedData[ "tracks" ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( top_tracks );
            returnedData[ "type" ] = "tracks";
        }

        if( chartType() == Album )
        {
            tDebug() << "ChartsPlugin:" << "\tgot " << top_albums.size() << " albums";
            returnedData[ "albums" ] = QVariant::fromValue< QList< Tomahawk::InfoSystem::InfoStringHash > >( top_albums );
            returnedData[ "type" ] = "albums";
        }

        Tomahawk::InfoSystem::InfoRequestData requestData = reply->property( "requestData" ).value< Tomahawk::InfoSystem::InfoRequestData >();

        emit info( requestData, returnedData );
        // update cache
        Tomahawk::InfoSystem::InfoStringHash criteria;
        Tomahawk::InfoSystem::InfoStringHash origData = requestData.input.value< Tomahawk::InfoSystem::InfoStringHash >();
        criteria[ "chart_id" ] = origData[ "chart_id" ];
        criteria[ "chart_source" ] = origData[ "chart_source" ];
        emit updateCache( criteria, 86400000, requestData.type, returnedData );
    }
    else
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Network error in fetching chart:" << reply->url().toString();

}

}

}

Q_EXPORT_PLUGIN2( Tomahawk::InfoSystem::InfoPlugin, Tomahawk::InfoSystem::ChartsPlugin )