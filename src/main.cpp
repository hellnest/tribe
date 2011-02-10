/***************************************************************************
 *   Copyright (C) 2008 - 2009 Dario Freddi <drf@chakra-project.org>       *
 *   Copyright (C) 2008        Lukas Appelhans <l.appelhans@gmx.de>        *
 *   Copyright (C) 2010 - 2011 Drake Justice <djustice@chakra-project.org> *
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


#include <QFile>

#include <KUniqueApplication>
#include <KAboutData>
#include <KLocale>
#include <KCmdLineArgs>
#include <KDebug>

#include <KMessageBox>

#include <config-tribe.h>

#include "mainwindow.h"
#include "installationhandler.h"


int main(int argc, char *argv[])
{
    KAboutData aboutData("tribe", 0, ki18n("Tribe"),
                         TRIBE_VERSION, ki18n("Graphical Installer for Chakra"), KAboutData::License_GPL,
                         ki18n("(c) 2008 - 2011 the Chakra Development Team"), ki18n("chakra@chakra-project.org"), "http://chakra-project.org");

    aboutData.addAuthor(ki18n("Dario Freddi"), ki18n("Maintainer"), "drf@chakra-project.org", "http://drfav.wordpress.com");
    aboutData.addAuthor(ki18n("Lukas Appelhans"), ki18n("Developer"), "boom1992@chakra-project.org", "http://boom1992.wordpress.com");
    aboutData.addAuthor(ki18n("Jan Mette"), ki18n("PostInstall Backend and Artwork"), "", "");
    aboutData.addAuthor(ki18n("Phil Miller"), ki18n("PostInstall Backend"), "philm@chakra-project.org", "http://chakra-project.org");
    aboutData.addAuthor(ki18n("Manuel Tortosa"), ki18n("PostInstall Backend"), "manutortosa@chakra-project.org", "http://chakra-project.org");
    aboutData.addAuthor(ki18n("Drake Justice"), ki18n("Developer"), "djustice@chakra-project.org", "");

    KCmdLineArgs::init(argc, argv, &aboutData);

    if (!KUniqueApplication::start()) {
        qWarning("Tribe is already running!\n");
        return 0;
    }

    KUniqueApplication app;

    app.setWindowIcon(KIcon("tribe"));

//     if (Solid::Control::PowerManager::acAdapterState() == Solid::Control::PowerManager::Unplugged) {
//         int r = KMessageBox::warningContinueCancel(0, i18n("It looks like your power adaptor is unplugged. "
//                 "Installation is a delicate and lenghty process, "
//                 "hence it is strongly advised to have your "
//                 "PC connected to AC to minimize possible risks."));
// 
//         if (r == KMessageBox::Cancel) {
//             return 0;
//         }
//     }

    /* Load the stylesheet */
    QFile file(STYLESHEET_INSTALL_PATH);
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());

    qApp->setStyleSheet(styleSheet);

    MainWindow mw;

    mw.show();

    return app.exec();
}
