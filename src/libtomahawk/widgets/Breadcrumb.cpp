/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "Breadcrumb.h"

#include "BreadcrumbButton.h"
#include "utils/stylehelper.h"
#include "kbreadcrumbselectionmodel.h"
#include "utils/logger.h"

#include <QStylePainter>
#include <QPushButton>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QDebug>

using namespace Tomahawk;

Breadcrumb::Breadcrumb( QWidget* parent, Qt::WindowFlags f )
    : QWidget( parent, f )
    , m_model( 0 )
    , m_selModel( 0 )
    , m_buttonlayout( new QHBoxLayout( this ) )
{
    m_buttonlayout->setSpacing( 0 );
    m_buttonlayout->setMargin( 0 );
    m_buttonlayout->setAlignment( Qt::AlignLeft );

    setAutoFillBackground( true );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    setLayoutDirection( Qt::LeftToRight );
    setLayout( m_buttonlayout );
    show();
}

Breadcrumb::~Breadcrumb()
{

}

void
Breadcrumb::setModel( QAbstractItemModel* model )
{
    m_model = model;

    if ( m_selModel )
        delete m_selModel;

    m_selModel = new KBreadcrumbSelectionModel( new QItemSelectionModel( m_model ) );
//     connect( m_selMode, SIGNAL( currentChanged( QModelIndex, QModelIndex) ), this, SLOT( updateButtons( QModelIndex, QModelIndex ) ) );
    updateButtons( QModelIndex() );
}

void
Breadcrumb::setRootIcon( const QPixmap& pm )
{
    m_rootIcon = pm;

    QPushButton* button = new QPushButton( QIcon( m_rootIcon ), "", this );
    button->setFlat( true );
    button->setStyleSheet( "QPushButton{ background-color: transparent; border: none; width:16px; height:16px;}" );
    m_buttonlayout->insertWidget( 0, button );
    m_buttonlayout->insertSpacing( 0,5 );
    m_buttonlayout->insertSpacing( 2,5 );
}


void
Breadcrumb::paintEvent( QPaintEvent* )
{
    QStylePainter p( this );
    StyleHelper::horizontalHeader( &p, rect() );
}

// updateFrom is the item that has changed---all children must be recomputed
// if invalid, redo the whole breadcrumb
void
Breadcrumb::updateButtons( const QModelIndex& updateFrom )
{
    qDebug() << "Updating buttons:" << updateFrom.data();
    QModelIndexList sel = m_selModel->selectedIndexes();
    int cur = 0;
    QModelIndex idx;

    int i = 0;
    if ( sel.count() > 0 )
    {
        for ( i = sel.count() - 1; i >= 0; i-- )
        {
            qDebug() << "Checking if this breadcrumb item is it:" << sel[ i ].data() << updateFrom.data() << ( sel[ i ] == updateFrom);
            if ( sel[ i ] == updateFrom )
            {
                idx = sel[ i ];
                break;
            }
        }
    }
    cur = sel.count() - i;

    // Ok, changed all indices that are at cur or past it. lets update them
    // When we get to the "end" of the tree, the leaf node is the chart itself
    qDebug() << "DONE and beginning iteration:" << idx.data() << sel;
    while ( m_model->rowCount( idx ) > 0 )
    {
        qDebug() << "CHANGED AND iterating:" << idx.data();
        BreadcrumbButton* btn = 0;
        if ( m_buttons.size() <= cur )
        {
            // We have to create a new button, doesn't exist yet
            btn = new BreadcrumbButton( this, m_model );
            connect( btn, SIGNAL( currentIndexChanged( QModelIndex ) ), this, SLOT( breadcrumbComboChanged( QModelIndex ) ) );

            m_buttonlayout->addWidget( btn );

            // Animate all buttons except the first
            if ( m_buttons.count() > 0 )
            {
                QWidget* neighbor = m_buttonlayout->itemAt( m_buttonlayout->count() - 2 )->widget();
                QPropertyAnimation* animation = new QPropertyAnimation( btn, "pos" );
                animation->setDuration( 300 );
                animation->setStartValue( neighbor->pos() );
                animation->setEndValue( btn->pos() );
                animation->start( QAbstractAnimation::DeleteWhenStopped );
            }

            m_buttons.append( btn );
        }
        else
        {
            // Got a button already, we just want to change the contents
            btn = m_buttons[ cur ];
        }

        // The children of idx are what populates this combobox.
        // It takes care of setting the default/user-populated value.
        btn->setParentIndex( idx );

        // Repeat with children
        idx = btn->currentIndex();

        cur++;
    }

    // Now we're at the leaf, lets activate the chart
    emit activateIndex( idx );
}

void
Breadcrumb::breadcrumbComboChanged( const QModelIndex& childSelected )
{
    // Some breadcrumb buttons' combobox changed. lets update the child breadcrumbs
    tDebug() << "Combo changed:" << childSelected.data();
    m_selModel->select( childSelected, QItemSelectionModel::SelectCurrent );
    updateButtons( childSelected );
}


// void
// Breadcrumb::currentIndexChanged( const QModelIndex& current, const QModelIndex& previous )
// {
//
// }
//
