/***************************************************************************
 *   Copyright (C) 2008 by Lukas Appelhans                                 *
 *   l.appelhans@gmx.de                                                    *
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

#ifndef LOCALEPAGE_H
#define LOCALEPAGE_H

#include <QModelIndex>
#include <QHash>

#include "../abstractpage.h"
#include "ui_locale.h"


class InstallationHandler;

class LocalePage : public AbstractPage, public Ui::Locale
{
    Q_OBJECT

public:
    LocalePage(QWidget *parent = 0);
    virtual ~LocalePage();

    bool eventFilter(QObject *object, QEvent *event);

private slots:
    void createWidget();

    void zoom(int value);
    void zoomChanged(int value);

    void continentChanged(int index);
    void regionChanged(int index);
    void updateLocales();

    bool validate();

    void aboutToGoToPrevious();
    void aboutToGoToNext();

private:
    InstallationHandler *m_install;

    QList<QStringList> locales;
    QStringList m_allLocales;
    QHash<QString, QString> m_allKDELangs;
    QHash<QString, QStringList> m_allTimezones;
};

#endif /*LOCALEPAGE_H_*/
