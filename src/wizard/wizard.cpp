#include "wizard.h"

#include <QDebug>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QWidget>
#include <QPropertyAnimation>
#include <tomahawkwindow.h>
#include <QTreeView>
#include <sourcetreeview.h>
#include <qvarlengtharray.h>

WizardMask::WizardMask(TomahawkWindow* window, QGraphicsItem* parent, QGraphicsScene* scene)
    : QGraphicsItem(parent, scene)
    , m_window( window )
{

}

void WizardMask::setWidgetToMask(QWidget* w)
{
    m_toMask = w;
}

void WizardMask::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->save();
    painter->setRenderHints( QPainter::Antialiasing );
    QPainterPath p;
    p.addRect( m_window->rect() );
    
    if( !m_toMask )
        return;

    QPainterPath mRect;
    QPoint parentPos = m_toMask->mapTo( m_window, m_toMask->pos() );
    mRect.addRoundedRect( QRect( parentPos, m_toMask->contentsRect().size() ), 5, 5 );
    p = p.subtracted( mRect );
    
    // paint the translucent background
    QColor top( Qt::gray );
    top.setAlpha( 180 );
    QColor bottom( Qt::gray);
    bottom.setAlpha( 220 );
    // put gradient from middle of top to middle of bottom
    QLinearGradient g( QPointF( m_window->size().width() / 2, 0 ), QPointF( m_window->width() / 2, m_window->height() ) );
    g.setColorAt( 0, top );
    g.setColorAt( 1, bottom );
    QBrush b( g );
    painter->fillPath( p, b );
    
    // paint the radial gradient that acts as a shadow
//     QRadialGradient g2( QRect( parentPos, m_toMask->contentsRect().size() ).center(), 250 );
//     QColor c( Qt::white );
//     c.setAlpha( 255 );
//     g2.setColorAt( 0, c );
//     c.setAlpha( 0 );
//     g2.setColorAt( 1, c );
//     QBrush b2( g2 );
//     QPainterPath circle;
//     circle.addEllipse( QRect( parentPos, m_toMask->contentsRect().size() ).center(), m_toMask->contentsRect().size().width(), m_toMask->contentsRect().size().height() / 2 );
//     circle = circle.subtracted( mRect );
//     painter->fillPath( circle, b2 );
    
    
    painter->restore();
}


QRectF WizardMask::boundingRect() const
{
    return m_window->rect();
}


Wizard::Wizard( TomahawkWindow* window)
    : QObject( window )
    , m_parent( window )
    , m_scene( new QGraphicsScene( this ) )
    , m_view( new QGraphicsView( m_scene, window ) )
    , m_mask( 0 )
    , m_title( new QGraphicsTextItem )
    , m_pageFadeOut( new QPropertyAnimation )
    , m_pageFadeIn( new QPropertyAnimation )
{
    Q_ASSERT( window );
    
    init();
    layout();
}

void Wizard::init()
{
    m_scene->setSceneRect( m_parent->rect() );
    m_scene->setItemIndexMethod( QGraphicsScene::NoIndex ); // Faster, don't have many items
    m_view->setFixedSize( m_parent->size() );
    m_view->setFrameStyle( QFrame::NoFrame );
    m_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setBackgroundRole( QPalette::Window );
    m_view->setAutoFillBackground( true );
    m_view->show();
    
    m_title->setHtml( QString( "<b>%1</b>" ).arg( tr( "Welcome to Tomahawk" ) ) );
    m_title->setCursor( Qt::ArrowCursor );
    m_scene->addItem( m_title );
    
    m_mask = new WizardMask( m_parent );
    m_mask->setZValue( -100 );
    m_scene->addItem( m_mask );
    m_mask->setPos( 0, 0 );
    
    m_pageFadeOut->setDuration( 100 );
    m_pageFadeOut->setPropertyName( "opacity" );
    m_pageFadeOut->setStartValue( 0 );
    m_pageFadeOut->setStartValue( 1 );
    m_pageFadeIn->setDuration( 100 );
    m_pageFadeOut->setPropertyName( "opacity" );
    m_pageFadeOut->setStartValue( 1 );
    m_pageFadeOut->setStartValue( 0 );
    
    QPalette p = m_view->palette();
    p.setColor( QPalette::Window, Qt::transparent );
    QColor c;
    c.setAlpha( 0 );
//     b.setColor( c );
    m_view->setPalette( p );

    
    m_parent->installEventFilter( this ); // React to resize events
   
    sourceListWizard();
}


Wizard::~Wizard()
{
}

void Wizard::start()
{
    layout();
}

void Wizard::layout()
{
    // Set up title and pages
    
    QSizeF totalSize =QSizeF( m_parent->size() );
    
    // Center title above the page box.
    m_title->setPos( ( totalSize.width() / 2 ) - ( m_title->boundingRect().width() / 2 ), 100 ); // center at 100px from top
    /*
    if( m_currentPage >= 0 ) { // We've got at least one page
        QSizeF pageSize = totalSize; // I wish QSize had an adjusted()...
        pageSize.setHeight( pageSize.height() - ( 2 * WIZARD_PAGE_TOP_BORDER ) );
        pageSize.setWidth( pageSize.width() - ( 2 * WIZARD_PAGE_SIDE_BORDER ) );
        QPointF pagePos( WIZARD_PAGE_SIDE_BORDER, WIZARD_PAGE_TOP_BORDER );
        
        Q_ASSERT( m_currentPage < m_pages.size() );
        m_pages[ m_currentPage ]->setPos( pagePos );
        m_pages[ m_currentPage ]->resize( pageSize );
        m_pages[ m_currentPage ]->setOpacity( 1 );
    }*/
}

void Wizard::sourceListWizard()
{
    // setup the mask to exclude the sourcelist, and highlight around
    SourceTreeView* sourceList = m_parent->findChild<SourceTreeView*>();
    if( sourceList ) {
        qDebug() << "Found sourcelist!";
        m_mask->setWidgetToMask( sourceList );
    } else {
        qDebug() << "DIDNT find sourcelist";
    }
}

bool Wizard::eventFilter( QObject* obj, QEvent* event )
{
    if( obj->isWidgetType() && obj == m_parent && event->type() == QEvent::Resize ) {
        m_scene->setSceneRect( m_parent->rect() );
        m_view->setFixedSize( m_parent->size() );
        
        layout();
        return false;
    }
    
    return QObject::eventFilter( obj, event );
}
