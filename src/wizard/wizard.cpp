#include "wizard.h"

#include "wizardpage.h"

#include <QDebug>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QWidget>
#include <QPropertyAnimation>

// The border on each side of the page being displayed
#define WIZARD_PAGE_TOP_BORDER 100
#define WIZARD_PAGE_SIDE_BORDER 200

Wizard::Wizard( QWidget* widget, QObject* parent )
    : QObject( parent )
    , m_currentPage( -1 )
    , m_parent( widget )
    , m_scene( new QGraphicsScene( this ) )
    , m_view( new QGraphicsView( m_scene, widget ) )
    , m_title( new QGraphicsTextItem )
    , m_pageFadeOut( new QPropertyAnimation )
    , m_pageFadeIn( new QPropertyAnimation )
{
    Q_ASSERT( widget );
    
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
    
    m_pageFadeOut->setDuration( 100 );
    m_pageFadeOut->setPropertyName( "opacity" );
    m_pageFadeOut->setStartValue( 0 );
    m_pageFadeOut->setStartValue( 1 );
    m_pageFadeIn->setDuration( 100 );
    m_pageFadeOut->setPropertyName( "opacity" );
    m_pageFadeOut->setStartValue( 1 );
    m_pageFadeOut->setStartValue( 0 );
    
    m_parent->installEventFilter( this ); // React to resize events
    
    // paint the translucent background
    QColor top( Qt::gray );
    top.setAlpha( 200 );
    QColor bottom( Qt::darkGray );
    bottom.setAlpha( 250 );
    // put gradient from middle of top to middle of bottom
    QLinearGradient g( QPointF( m_parent->size().width() / 2, 0 ), QPointF( m_parent->width() / 2, m_parent->height() ) );
    g.setColorAt( 0, top );
    g.setColorAt( 1, bottom );
    QBrush b( g );
    QPalette p = m_view->palette();
    p.setBrush( QPalette::Window, b );
    m_view->setPalette( p );
}


Wizard::~Wizard()
{
    qDeleteAll( m_pages );
}

void Wizard::addPage( WizardPage* page )
{
    page->setOpacity( 0 );
    m_scene->addItem( page );
    m_pages << page;
}

void Wizard::start()
{
    if( m_pages.isEmpty() )
        return;
    
    m_currentPage = 0;
    layout();
}

void Wizard::nextPage()
{
    if( m_currentPage + 1 >= m_pages.size() )
        return;
    qDebug() << Q_FUNC_INFO;
    m_pageFadeOut->setTargetObject( m_pages[ m_currentPage ] );
    m_pageFadeIn->setTargetObject( m_pages[ m_currentPage + 1 ] );
    
    m_pageFadeOut->start();
    m_pageFadeIn->start();
}


void Wizard::layout()
{
    // Set up title and pages
    
    QSizeF totalSize =QSizeF( m_parent->size() );
    
    // Center title above the page box.
    m_title->setPos( ( totalSize.width() / 2 ) - ( m_title->boundingRect().width() / 2 ), ( WIZARD_PAGE_TOP_BORDER / 2 ) - ( m_title->boundingRect().height() / 2 ) );
    
    if( m_currentPage >= 0 ) { // We've got at least one page
        QSizeF pageSize = totalSize; // I wish QSize had an adjusted()...
        pageSize.setHeight( pageSize.height() - ( 2 * WIZARD_PAGE_TOP_BORDER ) );
        pageSize.setWidth( pageSize.width() - ( 2 * WIZARD_PAGE_SIDE_BORDER ) );
        QPointF pagePos( WIZARD_PAGE_SIDE_BORDER, WIZARD_PAGE_TOP_BORDER );
        
        Q_ASSERT( m_currentPage < m_pages.size() );
        m_pages[ m_currentPage ]->setPos( pagePos );
        m_pages[ m_currentPage ]->resize( pageSize );
        m_pages[ m_currentPage ]->setOpacity( 1 );
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
