/***************************************************************************
 *   Copyright (C) 2008 - 2009 by Dario Freddi                             *
 *   drf@chakra-project.org                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef PARTITIONPAGE_H
#define PARTITIONPAGE_H

#include <QStyledItemDelegate>
#include <QTreeWidget>

#include <KPixmapSequence>

#include "../installationhandler.h"
#include "../abstractpage.h"


class QTimeLine;
class Device;
class QTreeWidgetItem;
class PMActions;
class Partition;
class TribePartition;
class PartitionManagerInterface;

namespace Ui {
    class Partition;
}


class PartitionDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    PartitionDelegate(QObject * parent = 0);
    ~PartitionDelegate();

    virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
    virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;

private slots:
    void commitData();
    void slotIndexChanged(const QString&);

private:
    KIcon m_lockIcon;
    KIcon m_partIcon;
};

class PartitionViewWidget : public QTreeWidget
{
    Q_OBJECT

public:
    PartitionViewWidget(QWidget* parent = 0);
    virtual ~PartitionViewWidget();

    virtual void paintEvent(QPaintEvent* event);

public slots:
    void setEnabled(bool enabled);

private slots:
    void stopRetainingPaintEvent();

private:
    bool m_retainPaintEvent;
    QTimeLine *m_fadeTimeLine;
    QTimeLine *m_spinnerTimeLine;
    KPixmapSequence m_sequence;
    QPixmap m_loadingPixmap;
};

class PartitionPage : public AbstractPage
{
    Q_OBJECT

public:
    PartitionPage(QWidget *parent = 0);
    virtual ~PartitionPage();

private slots:
    virtual void createWidget();
    virtual void aboutToGoToNext();
    virtual void aboutToGoToPrevious();

    void newClicked();
    void deleteClicked();
    void formatToggled(bool);
    void newPartTableClicked();
    void undoClicked();
    void unmountClicked();
    void advancedClicked();

    void populateTreeWidget();
    void setVisibleParts(bool);
    void currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*);
    void dataChanged(QModelIndex,QModelIndex);
    void slotTypeChanged(QString);

    void cancelFormat();
    void applyFormat();
    void cancelNew();
    void applyNew();

private:
    Ui::Partition *m_ui;
    QWidget *m_pmWidget;
    InstallationHandler *m_install;

    PartitionManagerInterface *m_iface;
    Partition *m_newPartition;

    QTreeWidgetItem *createItem(const Partition *p, Device *dev);

    QHash<const Partition*, QString> m_toFormat;

    QList<QColor> m_colorList;
    int m_currentPart;
};

#endif /*PARTITIONPAGE_H*/
