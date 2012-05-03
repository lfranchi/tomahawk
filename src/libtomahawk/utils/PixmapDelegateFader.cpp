/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeffe@tomahawk-player.org>
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

#include "PixmapDelegateFader.h"
#include "TomahawkUtilsGui.h"

#include <QPainter>
#include <QBuffer>
#include <QPaintEngine>
#include <QTimer>

using namespace Tomahawk;

#define COVER_FADEIN 1000

QWeakPointer< TomahawkUtils::SharedTimeLine > PixmapDelegateFader::s_stlInstance = QWeakPointer< TomahawkUtils::SharedTimeLine >();

QWeakPointer< TomahawkUtils::SharedTimeLine >
PixmapDelegateFader::stlInstance()
{
    if ( s_stlInstance.isNull() )
        s_stlInstance = QWeakPointer< TomahawkUtils::SharedTimeLine> ( new TomahawkUtils::SharedTimeLine() );

    return s_stlInstance;
}


PixmapDelegateFader::PixmapDelegateFader( const artist_ptr& artist, const QSize& size, TomahawkUtils::ImageMode mode, bool forceLoad )
    : m_artist( artist )
    , m_size( size )
    , m_mode( mode )
    , m_startFrame( 0 )
    , m_connectedToStl( false )
    , m_fadePct( 100 )
{
    if ( !m_artist.isNull() )
    {
        connect( m_artist.data(), SIGNAL( coverChanged() ), this, SLOT( artistChanged() ) );
        m_currentReference = m_artist->cover( size, forceLoad );
    }

    init();
}

PixmapDelegateFader::PixmapDelegateFader( const album_ptr& album, const QSize& size, TomahawkUtils::ImageMode mode, bool forceLoad )
    : m_album( album )
    , m_size( size )
    , m_mode( mode )
    , m_startFrame( 0 )
    , m_connectedToStl( false )
    , m_fadePct( 100 )
{
    if ( !m_album.isNull() )
    {
        connect( m_album.data(), SIGNAL( coverChanged() ), this, SLOT( albumChanged() ) );
        m_currentReference = m_album->cover( size, forceLoad );
    }

    init();
}


PixmapDelegateFader::PixmapDelegateFader( const query_ptr& track, const QSize& size, TomahawkUtils::ImageMode mode, bool forceLoad )
    : m_track( track )
    , m_size( size )
    , m_mode( mode )
    , m_startFrame( 0 )
    , m_connectedToStl( false )
    , m_fadePct( 100 )
{
    if ( !m_track.isNull() )
    {
        connect( m_track.data(), SIGNAL( coverChanged() ), this, SLOT( trackChanged() ) );
        m_currentReference = m_track->cover( size, forceLoad );
    }

    init();
}


PixmapDelegateFader::~PixmapDelegateFader()
{

}


void
PixmapDelegateFader::init()
{
    m_current = QPixmap( m_size );
    m_current.fill( Qt::transparent );
    
    if ( m_currentReference.isNull() )
    {
        // No cover loaded yet, use default and don't fade in
        if ( !m_album.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, m_mode, m_size );
        else if ( !m_artist.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, m_mode, m_size );
        else if ( !m_track.isNull() )
            m_current = m_currentReference = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, m_mode, m_size );

        return;
    }

    stlInstance().data()->setUpdateInterval( 20 );
    m_startFrame = stlInstance().data()->currentFrame();
    m_connectedToStl = true;
    m_fadePct = 0;
    connect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
}


void
PixmapDelegateFader::albumChanged()
{
    if ( m_album.isNull() )
        return;

    QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_album->cover( m_size ) ) );
}

void
PixmapDelegateFader::artistChanged()
{
    if ( m_artist.isNull() )
        return;

    QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_artist->cover( m_size ) ) );
}


void
PixmapDelegateFader::trackChanged()
{
    if ( m_track.isNull() )
        return;

    QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_track->cover( m_size ) ) );
}


void
PixmapDelegateFader::setPixmap( const QPixmap& pixmap )
{
    if ( pixmap.isNull() )
        return;

    QByteArray ba;
    QBuffer buffer( &ba );
    buffer.open( QIODevice::WriteOnly );
    pixmap.save( &buffer, "PNG" );
    QString newImageMd5 = TomahawkUtils::md5( buffer.data() );
    if ( m_oldImageMd5 == newImageMd5 )
        return;

    m_oldImageMd5 = newImageMd5;
    
    if ( m_connectedToStl )
    {
        m_pixmapQueue.enqueue( pixmap );
        return;
    }
    
    m_oldReference = m_currentReference;
    m_currentReference = pixmap;

    stlInstance().data()->setUpdateInterval( 20 );
    m_startFrame = stlInstance().data()->currentFrame();
    m_connectedToStl = true;
    m_fadePct = 0;
    connect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );
}


void
PixmapDelegateFader::onAnimationStep( int step )
{
    m_fadePct = (float)( step - m_startFrame ) / 10.0;
    if ( m_fadePct > 100.0 )
        m_fadePct = 100.0;

    if ( m_fadePct == 100.0 )
        QTimer::singleShot( 0, this, SLOT( onAnimationFinished() ) );
    
    const qreal opacity = m_fadePct / 100.0;
    const qreal oldOpacity =  ( 100.0 - m_fadePct ) / 100.0;
    m_current.fill( Qt::transparent );

    // Update our pixmap with the new opacity
    QPainter p( &m_current );

    if ( !m_oldReference.isNull() )
    {
        p.setOpacity( oldOpacity );
        p.drawPixmap( 0, 0, m_oldReference );
    }

    Q_ASSERT( !m_currentReference.isNull() );
    if ( !m_currentReference.isNull() ) // Should never be null..
    {
        p.setOpacity( opacity );
        p.drawPixmap( 0, 0, m_currentReference );
    }

    p.end();

    emit repaintRequest();
    /**
     * Avoids using setOpacity that is slow on X11 (turns off graphics-backed painting, forces fallback to
     * software rasterizer.
     *
     * but a bit buggy.
     */
    /*
     const int opacity = ((float)step* / 1000.) * 255;
     const int oldOpacity = 255 - opacity;
    if ( !m_oldReference.isNull() )
    {
        p.setCompositionMode( QPainter::CompositionMode_Source );
        p.drawPixmap( 0, 0, m_oldReference );

        // Reduce the source opacity by the value of the alpha channel
        p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
        qDebug() << Q_FUNC_INFO << "Drawing old pixmap w/ opacity;" << oldOpacity;
        p.fillRect( m_current.rect(), QColor( 0, 0, 0, oldOpacity ) );
    }

    Q_ASSERT( !m_currentReference.isNull() );
    if ( !m_currentReference.isNull() ) // Should never be null..
    {
        QPixmap temp( m_size );
        temp.fill( Qt::transparent );

        QPainter p2( &temp );
        p2.drawPixmap( 0, 0, m_currentReference );

        p2.setCompositionMode( QPainter::CompositionMode_DestinationIn );
        qDebug() << Q_FUNC_INFO << "Drawing NEW pixmap w/ opacity;" << opacity;

        p2.fillRect( temp.rect(), QColor( 0, 0, 0, opacity ) );
        p2.end();

        p.setCompositionMode( QPainter::CompositionMode_Source );
        p.drawPixmap( 0, 0, temp );
    }
    */

}


void
PixmapDelegateFader::onAnimationFinished()
{
    m_oldReference = QPixmap();

    m_connectedToStl = false;
    disconnect( stlInstance().data(), SIGNAL( frameChanged( int ) ), this, SLOT( onAnimationStep( int ) ) );

    if ( !m_pixmapQueue.isEmpty() )
        QMetaObject::invokeMethod( this, "setPixmap", Qt::QueuedConnection, Q_ARG( QPixmap, m_pixmapQueue.dequeue() ) );
}



QPixmap
PixmapDelegateFader::currentPixmap() const
{
    return m_current;
}
