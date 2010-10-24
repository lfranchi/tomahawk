#ifndef TOMAHAWK_WIZARD_H
#define TOMAHAWK_WIZARD_H

#include <QObject>

class WizardPage;

class QGraphicsScene;
class QGraphicsView;
class QGraphicsTextItem;
class QPropertyAnimation;
class QWidget;

/**
 * This class is a blingy wizard used to initially set up a few options from user input.
 *  Create it with a parent widget, and this widget will take over the whole space of the parent
 *  and position the wizard pages inside of it.
 */
class Wizard : public QObject
{
    Q_OBJECT
public:
    explicit Wizard( QWidget* widget, QObject* parent = 0 );
    virtual ~Wizard();
    
    /**
     * Add a page to the wizard. This object will take ownership of the wizard page.
     */
    void addPage( WizardPage* page );

    /**
     * Start the wizard, once all pages have been added.
     */
    void start();
    
    virtual bool eventFilter(QObject* , QEvent* );
    
private slots:
    void nextPage();
    
private:
    void init();
    void layout();
    
    int m_currentPage;
    QList< WizardPage* > m_pages;
    
    QWidget* m_parent;
    
    QGraphicsScene* m_scene;
    QGraphicsView* m_view;
    QGraphicsTextItem* m_title;
    
    QPropertyAnimation* m_pageFadeOut;
    QPropertyAnimation* m_pageFadeIn;
};

#endif
