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

#ifndef SCREENSHOTS_H
#define SCREENSHOTS_H

#include <QObject>
#include <QPixmap>


class Screenshots : public QObject
{
/// Q_OBJECT
public:
    explicit Screenshots(QObject *parent = 0) : QObject(parent) {
        m_screenshots.append(QPixmap(":/Images/images/screenshot01.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot02.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot03.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot04.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot05.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot06.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot07.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot08.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot09.png"));
        m_screenshots.append(QPixmap(":/Images/images/screenshot10.png"));
    }
    ~Screenshots() {}

    QList<QPixmap> getScreenshots() {
        return m_screenshots;
    }

private:
    QList<QPixmap> m_screenshots;
};

#endif /*SCREENSHOTS_H*/
