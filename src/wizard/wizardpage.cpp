#include "wizardpage.h"
#include <QPainter>

WizardPage::WizardPage( QGraphicsItem* parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
{

}

void WizardPage::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    // Just paint a border
    painter->setRenderHint( QPainter::Antialiasing );
    
    QPainterPath border;
    border.addRoundedRect( boundingRect(), 5, 5 );
    
    painter->save();
    QPen p = painter->pen();
    p.setWidth( 3 );
    painter->setPen( p );
    painter->drawPath( border );
    painter->restore();
    
    QColor c( Qt::cyan );
    painter->fillPath( border, c );
}
