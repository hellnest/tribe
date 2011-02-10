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

#ifndef ABSTRACTPAGE
#define ABSTRACTPAGE

#include <QWidget>

#include "mainwindow.h"


class AbstractPage : public QWidget
{
    Q_OBJECT

public:
    AbstractPage(QWidget *parent = 0);
    virtual ~AbstractPage();

private slots:
    virtual void createWidget() = 0;
    virtual void aboutToGoToNext() = 0;
    virtual void aboutToGoToPrevious() = 0;

signals:
    /* These signals are used to communicate with MainWindow and the whole
     * installation process.
     */

    void setInstallationStep(MainWindow::InstallationStep step, bool done);
    void goToNextStep();
    void goToPreviousStep();
    void abortInstallation();
    void showProgressWidget();
    void updateProgressWidget(int percentage);
    void setProgressWidgetText(const QString &);
    void deleteProgressWidget();
    void enableNextButton(bool enable);
    void enablePreviousButton(bool enable);
};

#endif /*ABSTRACTPAGE_H_*/
