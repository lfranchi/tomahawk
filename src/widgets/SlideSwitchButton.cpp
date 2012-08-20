/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "SlideSwitchButton.h"

#include "utils/TomahawkUtils.h"

#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionButton>
#include <QPixmap>

SlideSwitchButton::SlideSwitchButton( QWidget* parent )
    : QPushButton( parent )
    , m_checkedText( tr( "On" ) )
    , m_uncheckedText( tr( "Off" ) )
{
    init();
}

SlideSwitchButton::SlideSwitchButton( const QString& checkedText,
                                      const QString& uncheckedText,
                                      QWidget* parent )
    : QPushButton( parent )
    , m_checkedText( checkedText )
    , m_uncheckedText( uncheckedText )
{
    init();
}

void
SlideSwitchButton::init()
{
    setCheckable( true );
#ifndef Q_OS_MAC
    setMouseTracking( true );
#endif

    m_knob.load( RESPATH "images/sliderbutton_knob.png" );

    m_backCheckedColorTop = QColor( 8, 54, 134 );
    m_backCheckedColorBottom = QColor( 118, 172, 240 );
    m_backUncheckedColorTop = QColor( 128, 128, 128 );
    m_backUncheckedColorBottom = QColor( 179, 179, 179 );

    m_baseColorTop = m_backUncheckedColorTop;
    m_baseColorBottom = m_backUncheckedColorBottom;

    m_textColor = QColor( "#606060" );
    setFocusPolicy( Qt::NoFocus );

    m_knobX = 0.;

    m_textFont = font();
    m_textFont.setBold( true );
    m_textFont.setCapitalization( QFont::AllUppercase );


    connect( this, SIGNAL( toggled( bool ) ),
             this, SLOT( onCheckedStateChanged() ) );
}

QSize
SlideSwitchButton::sizeHint() const
{
//    QSize size = QPushButton::sizeHint();
//    QFontMetrics fm( m_textFont );
//    int maxTextLength = qMax( fm.boundingRect( m_checkedText ).width(),
//                              fm.boundingRect( m_uncheckedText ).width() );
//    size.rwidth() = contentsMargins().left() + contentsMargins().right()
//                  + 2 /*a bit of margin*/ + maxTextLength + ( height() - 4 ) * 1.25;
//    return size;
    return QSize( 70, 20 );
}


void
SlideSwitchButton::paintEvent( QPaintEvent* event )
{
    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QPalette palette;

    QStyleOptionButton option;
    initStyleOption( &option );

    QPen border;
    border.setWidth( 1 );
#ifndef Q_OS_MAC
    if ( option.state &  QStyle::State_MouseOver )
       border.setColor( palette.color( QPalette::Highlight ) );
    else
#endif
       border.setColor( QColor( "#606060" ) );
    painter.setPen( border );
    //TODO: should the whole thing be highlighted or just the knob?

    QLinearGradient gradient( 0, 0, 0, 1 );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0, m_baseColorTop );
    gradient.setColorAt( 1, m_baseColorBottom );
    painter.setBrush( gradient );

    painter.drawRoundedRect( QRect( 0, 0, width() - 0, height() - 0 ).adjusted( 3, 0, -3, 0 ), 2, 2 );

    painter.drawPixmap( m_knobX * ( width() - m_knob.width() ), 0, m_knob );

    qDebug() << "Drawn with knob at:" << m_knobX;

    if ( m_knobX != 1.0 && m_knobX != 0.0 )
        return;

    //let's draw some text...
    if ( m_baseColorTop == m_backUncheckedColorTop )
        painter.setPen( m_textColor );
    else
        painter.setPen( Qt::white );

    painter.setFont( m_textFont );
    const QRect textRect( m_knobX == 0. ? m_knob.width() : 0, 0, width() - m_knob.width(), height() );
    painter.drawText( textRect, Qt::AlignCenter, m_knobX == 0 ? m_uncheckedText : m_checkedText );
}

void
SlideSwitchButton::onCheckedStateChanged()
{
    if ( !m_knobAnimation.isNull() )
        m_knobAnimation.data()->stop();

    m_knobAnimation = QWeakPointer<QPropertyAnimation>( new QPropertyAnimation( this, "knobX" ) );
    m_knobAnimation.data()->setDuration( 50 );

    m_knobAnimation.data()->setStartValue( isChecked() ? 0 : 1 );
    m_knobAnimation.data()->setEndValue( isChecked() ? 1 : 0 );

    m_knobAnimation.data()->start( QAbstractAnimation::DeleteWhenStopped );
}


void
SlideSwitchButton::setBackChecked( bool state )
{
    if ( state != m_backChecked )
    {
        if ( !m_backTopAnimation.isNull() )
            m_backTopAnimation.data()->stop();
        if ( !m_backBottomAnimation.isNull() )
            m_backBottomAnimation.data()->stop();

        m_backChecked = state;
        m_backTopAnimation = QWeakPointer<QPropertyAnimation>( new QPropertyAnimation( this, "baseColorTop" ) );
        m_backBottomAnimation = QWeakPointer<QPropertyAnimation>( new QPropertyAnimation( this, "baseColorBottom" ) );
        m_backTopAnimation.data()->setDuration( 300 );
        m_backBottomAnimation.data()->setDuration( 300 );

        m_backTopAnimation.data()->setStartValue( state ? m_backUncheckedColorTop : m_backCheckedColorTop );
        m_backTopAnimation.data()->setEndValue( state ? m_backCheckedColorTop : m_backUncheckedColorTop );
        m_backBottomAnimation.data()->setStartValue( state ? m_backUncheckedColorBottom : m_backCheckedColorBottom );
        m_backBottomAnimation.data()->setEndValue( state ? m_backCheckedColorBottom : m_backUncheckedColorBottom );

        m_backTopAnimation.data()->start( QAbstractAnimation::DeleteWhenStopped );
        m_backBottomAnimation.data()->start( QAbstractAnimation::DeleteWhenStopped );
    }
}

bool
SlideSwitchButton::backChecked() const
{
    return m_backChecked;
}
