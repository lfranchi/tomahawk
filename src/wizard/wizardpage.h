#ifndef TOMAHAWK_WIZARD_PAGE_H
#define TOMAHAWK_WIZARD_PAGE_H

#include <QGraphicsWidget>

class WizardPage : public QGraphicsWidget
{
    Q_OBJECT
public:
    explicit WizardPage( QGraphicsItem* parent = 0, Qt::WindowFlags wFlags = 0 );
    
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
};

#endif