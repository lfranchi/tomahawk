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

#include "fuzzyindex.h"

#include <QDir>
#include <QTime>

#include <CLucene.h>

#include "databaseimpl.h"
#include "utils/tomahawkutils.h"

#include "libdavros/davros.h"
#include "utils/logger.h"

using namespace lucene::analysis;
using namespace lucene::document;
using namespace lucene::store;
using namespace lucene::index;
using namespace lucene::queryParser;
using namespace lucene::search;


FuzzyIndex::FuzzyIndex( DatabaseImpl& db, bool wipeIndex )
    : QObject()
    , m_db( db )
    , m_luceneReader( 0 )
    , m_luceneSearcher( 0 )
{
    QString m_lucenePath = TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" );
    m_luceneDir = FSDirectory::getDirectory( m_lucenePath.toStdString().c_str() );
    m_analyzer = _CLNEW SimpleAnalyzer();

    if ( wipeIndex )
    {
        tLog( LOGVERBOSE ) << "Wiping fuzzy index...";
        beginIndexing();
        endIndexing();
    }
}


FuzzyIndex::~FuzzyIndex()
{
    delete m_luceneSearcher;
    delete m_luceneReader;
    delete m_analyzer;
    delete m_luceneDir;
}


void
FuzzyIndex::beginIndexing()
{
    m_mutex.lock();

    try
    {
        Davros::debug() << Q_FUNC_INFO << "Starting indexing.";
        if ( m_luceneReader != 0 )
        {
            Davros::debug() << "Deleting old lucene stuff.";
            m_luceneSearcher->close();
            m_luceneReader->close();
            delete m_luceneSearcher;
            delete m_luceneReader;
            m_luceneSearcher = 0;
            m_luceneReader = 0;
        }

        Davros::debug() << "Creating new index writer.";
        IndexWriter luceneWriter = IndexWriter( m_luceneDir, m_analyzer, true );
    }
    catch( CLuceneError& error )
    {
        Davros::debug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }
}


void
FuzzyIndex::endIndexing()
{
    m_mutex.unlock();
    emit indexReady();
}


void
FuzzyIndex::appendFields( const QString& table, const QMap< unsigned int, QString >& fields )
{
    try
    {
        Davros::debug() << "Appending to index:" << fields.count();
        bool create = !IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() );
        IndexWriter luceneWriter = IndexWriter( m_luceneDir, m_analyzer, create );
        Document doc;

        QMapIterator< unsigned int, QString > it( fields );
        while ( it.hasNext() )
        {
            it.next();
            unsigned int id = it.key();
            QString name = it.value();

            {
                Field* field = _CLNEW Field( table.toStdWString().c_str(), DatabaseImpl::sortname( name ).toStdWString().c_str(),
                                            Field::STORE_YES | Field::INDEX_UNTOKENIZED );
                doc.add( *field );
            }

            {
                Field* field = _CLNEW Field( _T( "id" ), QString::number( id ).toStdWString().c_str(),
                Field::STORE_YES | Field::INDEX_NO );
                doc.add( *field );
            }

            luceneWriter.addDocument( &doc );
            doc.clear();
        }

        luceneWriter.close();
    }
    catch( CLuceneError& error )
    {
        Davros::debug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }
}


void
FuzzyIndex::loadLuceneIndex()
{
    emit indexReady();
}


QMap< int, float >
FuzzyIndex::search( const QString& table, const QString& name )
{
    QMutexLocker lock( &m_mutex );

    QMap< int, float > resultsmap;
    try
    {
        if ( !m_luceneReader )
        {
            if ( !IndexReader::indexExists( TomahawkUtils::appDataDir().absoluteFilePath( "tomahawk.lucene" ).toStdString().c_str() ) )
            {
                Davros::debug() << Q_FUNC_INFO << "index didn't exist.";
                return resultsmap;
            }

            m_luceneReader = IndexReader::open( m_luceneDir );
            m_luceneSearcher = _CLNEW IndexSearcher( m_luceneReader );
        }

        if ( name.isEmpty() )
            return resultsmap;

        SimpleAnalyzer analyzer;
        QueryParser parser( table.toStdWString().c_str(), m_analyzer );
        Hits* hits = 0;

        FuzzyQuery* qry = _CLNEW FuzzyQuery( _CLNEW Term( table.toStdWString().c_str(), DatabaseImpl::sortname( name ).toStdWString().c_str() ) );
        hits = m_luceneSearcher->search( qry );

        for ( uint i = 0; i < hits->length(); i++ )
        {
            Document* d = &hits->doc( i );

            float score = hits->score( i );
            int id = QString::fromWCharArray( d->get( _T( "id" ) ) ).toInt();
            QString result = QString::fromWCharArray( d->get( table.toStdWString().c_str() ) );

            if ( DatabaseImpl::sortname( result ) == DatabaseImpl::sortname( name ) )
                score = 1.0;
            else
                score = qMin( score, (float)0.99 );

            if ( score > 0.05 )
            {
                resultsmap.insert( id, score );
//                Davros::debug() << "Hitres:" << result << id << score << table << name;
            }
        }

        delete hits;
        delete qry;
    }
    catch( CLuceneError& error )
    {
        Davros::debug() << "Caught CLucene error:" << error.what();
        Q_ASSERT( false );
    }

    return resultsmap;
}
