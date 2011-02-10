/*
 * Copyright (c) 2010  Drake Justice <djustice.kde@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <QDebug>

#include <QPainter>
#include <QProcess>
#include <QMouseEvent>

#include "partitionwidget.h"


PartitionWidget::PartitionWidget(QWidget* parent) : QAbstractSlider(parent)
{
    m_colorList.append(QColor(Qt::darkGray).darker());
    m_colorList.append(Qt::darkGray);
    m_colorList.append(Qt::darkBlue);
    m_colorList.append(Qt::blue);
    m_colorList.append(QColor(Qt::darkGreen).darker());
    m_colorList.append(Qt::darkGreen);
    m_colorList.append(QColor(Qt::darkRed).darker());
    m_colorList.append(Qt::darkRed);
    m_colorList.append(QColor(Qt::darkYellow).darker());
    m_colorList.append(Qt::darkYellow);
    m_colorList.append(QColor(Qt::darkCyan).darker());
    m_colorList.append(Qt::darkCyan);

    m_isStatic = true;
}

double PartitionWidget::getSizeInKB(const QString& s)
{
    QString in = s;

    double dec;

    in = in.replace("i", "");
    in = in.simplified();

    if (in.contains(".")) {
        dec = in.split(".").at(1).left(1).toDouble();
    }

    if (in.contains("PB")) {
        return in.left(in.length() - 2).toDouble() * 1024 * 1024 * 1024 * 1024 +
                       (dec * 1024 * 1024 * 1024);
    } else if (in.contains("TB")) {
        return in.left(in.length() - 2).toDouble() * 1024 * 1024 * 1024 +
                       (dec * 1024 * 1024);
    } else if (in.contains("GB")) {
        return in.left(in.length() - 2).toDouble() * 1024 * 1024 +
                       (dec * 1024);
    } else if (in.contains("MB")) {
        return in.left(in.length() - 2).toDouble() * 1024 +
                       dec;
    } else if (in.contains("kB")) {
        return in.left(in.length() - 2).toDouble();
    }

    return 0.0;
}

void PartitionWidget::mouseMoveEvent(QMouseEvent* event)
{
    qDebug() << event->pos();
}

void PartitionWidget::mousePressEvent(QMouseEvent* event)
{
    grabMouse(Qt::ClosedHandCursor);
}

void PartitionWidget::mouseReleaseEvent(QMouseEvent* event)
{
    releaseMouse();
}

void PartitionWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);

    // set size vars
    m_current = 0;

    double totalSizeInKB = m_totalSize;

    QString pSize = "0";

    // background rect (base outline and 1px 'shadow' ;)
    p.setBrush(QBrush(Qt::black));
    p.drawRoundedRect(0, 1, width() - 3, 21, 4, 4);

    foreach(QString s, m_sizeList) {

        /// prepare color gradient, thin black outline

        QLinearGradient gradient(0, 0, 0, 20);
        gradient.setColorAt(0.0, m_colorList.at(m_current));
        gradient.setColorAt(0.35, m_colorList.at(m_current).lighter());
        gradient.setColorAt(0.55, m_colorList.at(m_current).darker());

        QBrush b(gradient);
        p.setBrush(b);
        p.setPen(Qt::black);

        /// calc and draw current part

        double start = pSize.toDouble();
        double size = getSizeInKB(s);

        pSize = QString::number(getSizeInKB(s));

        int space = 0;

        if (m_current > 0) { space = 9; }

        QRect volRect((start / totalSizeInKB) * width() - 7, 1, (size / totalSizeInKB) * width() + space, 20);

        // ensure the first few align to the start
        if ((start / totalSizeInKB) * width() < 5) {
            volRect.setTopLeft(QPoint(0, 1));
        }

        // last, align it to the end
        if (m_current == m_sizeList.count() - 1) {
                volRect.setBottomRight(QPoint(width() - 3, 20));
        }

        // dont draw tiny ones (less than 5px)
        if ((size / totalSizeInKB) * width() > 5) {
            p.drawRoundedRect(volRect, 7, 7);
        }

        // check, reset the counter
        if (m_current >= m_typeList.count()) {
            m_current++;

            if (m_current > 9)
                m_current = 0;

            continue;
        }

        /// legend colors

        QPoint topLeftLegendDot(m_current * 150 + 0, 26);
        QPoint bottomRightLegendDot(m_current * 150 + 7, 33);

        QRect volLegendDot(topLeftLegendDot, bottomRightLegendDot);

        p.drawRoundedRect(volLegendDot, 2, 2);

        /// legend text

        p.setPen(Qt::white);
        p.drawText(m_current * 150 + 12, 35, " " + m_labelList.at(m_current) +
                                             " (" + m_typeList.at(m_current).toLower() +
                                             ")");
        p.drawText(m_current * 150 + 12, 48, " " + QString(m_usageList.at(m_current).split("::").at(0)).replace("16.0EiB", "-").replace("i", "") +
                                             "/" + QString(m_sizeList.at(m_current)).replace("i", ""));

        /// usage bars

        foreach (QString s, m_usageList) {
            if (s.split("::").at(1).toInt() == m_current + 1) {
                if ((size / totalSizeInKB) * width() > 14) {
                    p.setPen(QColor(m_colorList.at(m_current).darker()));
                    p.setBrush(QBrush(QColor(m_colorList.at(m_current).darker().darker())));

                    QRect volUsageRect((start / totalSizeInKB) * width() + 25, 10, (getSizeInKB(s.split("::").at(0)) / totalSizeInKB) * width(), 10);

                    p.drawRoundedRect(volUsageRect, 3, 3);
                }
            }
        }

        /// os pixmaps

        QPixmap pLin(":Images/images/lin.png");
        QPixmap pWin(":Images/images/win.png");

        if (volRect.width() > 50) {
            if (m_typeList.at(m_current).toLower().contains("ext") || m_typeList.at(m_current).toLower().contains("reiser")) {
                p.setOpacity(0.75);
                p.drawPixmap(volRect.left() + 5, volRect.top(), pLin);
                p.setOpacity(1.0);
            } else if (m_typeList.at(m_current).toLower().contains("fat") || m_typeList.at(m_current).toLower().contains("ntfs")) {
                p.setOpacity(0.75);
                p.drawPixmap(volRect.left() + 5, volRect.top(), pWin);
                p.setOpacity(1.0);
            }
        }

        // increment / reset the current var
        m_current++;

        if (m_current > 9) m_current = 0;
    }

    // slider handle and new volume
    if (!m_isStatic) {
        // new volume
        QRect newVolRect(width() - (width() * 0.2), 1, width() * 0.2, 20);

        QLinearGradient gradient(0, 0, 0, 20);
        gradient.setColorAt(0.0, Qt::black);
        gradient.setColorAt(0.35, QColor(Qt::darkGray).darker());
        gradient.setColorAt(0.55, QColor(Qt::black));

        QBrush b(gradient);
        p.setBrush(b);
        p.setPen(Qt::black);

        p.drawRoundedRect(newVolRect, 7, 7);

        // slide handle
        p.drawPixmap(width() - (width() * 0.2), 1, QPixmap(":Images/images/slidehandle.png"));
    }
}

