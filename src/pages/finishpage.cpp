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

#include <KIcon>

#include "finishpage.h"


FinishPage::FinishPage(QWidget *parent)
        : AbstractPage(parent)
{
}

FinishPage::~FinishPage()
{
}

void FinishPage::createWidget()
{
    ui.setupUi(this);
    ui.rebootButton->setIcon(KIcon("system-reboot"));
    ui.chakraButton->setIcon(KIcon("dialog-ok-apply"));

    connect(ui.rebootButton, SIGNAL(clicked()), SIGNAL(reboot()));
    connect(ui.chakraButton, SIGNAL(clicked()), SIGNAL(keepChakra()));
}

#include "finishpage.moc"
