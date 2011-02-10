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

#ifndef BOOTLOADERPAGE_H
#define BOOTLOADERPAGE_H

#include <QProcess>

#include "../abstractpage.h"
#include "ui_bootloader.h"


class InstallationHandler;

class BootloaderPage : public AbstractPage
{
    Q_OBJECT

public:
    BootloaderPage(QWidget *parent = 0);
    virtual ~BootloaderPage();

private slots:
    void createWidget();
    void aboutToGoToNext();
    void aboutToGoToPrevious();
    void bootloaderInstalled(int exitCode, QProcess::ExitStatus exitStatus);
    void menulstInstalled(int exitCode, QProcess::ExitStatus exitStatus);

private:
    Ui::Bootloader ui;
    InstallationHandler *m_handler;
};

#endif /*BOOTLOADERPAGE_H_*/
