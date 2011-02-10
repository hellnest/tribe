/*
 * Copyright (c) 2010  Drake Justice <djustice.kde@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef PARTITIONWIDGET_H
#define PARTITIONWIDGET_H

#include <QAbstractSlider>

class QMouseEvent;
class QPaintEvent;


class PartitionWidget : public QAbstractSlider
{
    Q_OBJECT
public:
    PartitionWidget(QWidget* = 0);
    ~PartitionWidget() { }

    double getSizeInKB(const QString&);

    void setStatic(bool b) { m_isStatic = b; }

    void setTotalSize(double d) { m_totalSize = d; }

    void setSizeList(QStringList l) { m_sizeList = l; }
    void setTypeList(QStringList l) { m_typeList = l; }
    void setLabelList(QStringList l) { m_labelList = l; }
    void setUsageList(QStringList l) { m_usageList = l; }

    void setDevice(const QString& s) { m_device = s; }

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent*);

private:
    double m_totalSize;

    QStringList m_labelList;
    QStringList m_typeList;
    QStringList m_sizeList;
    QStringList m_usageList;

    QList<QColor> m_colorList;

    QString m_device;

    bool m_isStatic;

    int m_current;
};

#endif /*PARTITIONWIDGET_H*/
