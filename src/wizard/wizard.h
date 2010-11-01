#ifndef TOMAHAWK_WIZARD_H
#define TOMAHAWK_WIZARD_H

#include <QObject>
#include <tomahawk/album.h>
#include <qgraphicsitem.h>

class QGraphicsItem;
class TomahawkWindow;
class WizardPage;

class QGraphicsScene;
class QGraphicsView;
class QGraphicsTextItem;
class QPropertyAnimation;

class WizardMask : public QGraphicsItem
{
public:
    WizardMask( TomahawkWindow* window, QGraphicsItem* parent = 0, QGraphicsScene* scene = 0 );
    
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
    
    void setWidgetToMask( QWidget* w );
    
private:
    TomahawkWindow* m_window;
    QWidget* m_toMask;
};

/**
 * This class is a blingy wizard used to initially set up a few options from user input.
 *  Create it with the TomahawkWindow, and this class will ask the user for a few options
 *  as well as show some info.
 */
class Wizard : public QObject
{
    Q_OBJECT
public:
    explicit Wizard( TomahawkWindow* window );
    virtual ~Wizard();
    
    /**
     * Start the wizard, once all pages have been added.
     */
    void start();
    
    virtual bool eventFilter(QObject* , QEvent* );
private slots:
    void sourceListWizard();
    
private:
    void init();
    void layout();
    
    TomahawkWindow* m_parent;
    
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    
    WizardMask* m_mask;
    QGraphicsTextItem* m_title;
    
    QPropertyAnimation* m_pageFadeOut;
    QPropertyAnimation* m_pageFadeIn;
};

#endif
