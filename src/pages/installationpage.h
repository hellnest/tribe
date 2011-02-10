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

#ifndef INSTALLATIONPAGE_H
#define INSTALLATIONPAGE_H

#include <QPointer>
#include <QThread>
#include <QTimer>

#include "../abstractpage.h"
#include "../installationhandler.h"

#include "ui_installation.h"


class Screenshots;

class InstallationPage : public AbstractPage
{
    Q_OBJECT

public:
    InstallationPage(QWidget *parent = 0);
    virtual ~InstallationPage();

private slots:
    virtual void createWidget();
    virtual void aboutToGoToNext();
    virtual void aboutToGoToPrevious();
    void setPercentage(int percentage);
    void setLabel(const QString &label);
    void changeScreenShot();
    void error();

private:
    InstallationHandler *m_install;
    Ui::Installation ui;
    int m_index;
    QPointer<QTimer> m_timer;
    Screenshots *m_screenshots;
};

#endif /*INSTALLATIONPAGE_H*/
