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

#include <QDebug>

#include <QFile>
#include <QDir>
#include <QScrollBar>

#include <config-tribe.h>

#include "../installationhandler.h"
#include "licensepage.h"


LicensePage::LicensePage(QWidget *parent)
        : AbstractPage(parent),
        m_handler(InstallationHandler::instance()),
        m_iterator(0)
{
}

LicensePage::~LicensePage()
{
}

void LicensePage::createWidget()
{
    ui.setupUi(this);

    connect(ui.acceptButton, SIGNAL(toggled(bool)), SLOT(enableNext()));
    connect(ui.declineButton, SIGNAL(toggled(bool)), SLOT(enableNext()));

    // Ok, let's find out which licenses we need to accept...
    if (QFile::exists("/tmp/catalyst")) {
        m_licenses.insert("catalyst", i18n("Proprietary ATI Driver"));
    }

    if (QFile::exists("/tmp/nvidia")) {
        m_licenses.insert("nvidia", i18n("Proprietary NVidia Driver"));
    }

    setUpLicense();
}

void LicensePage::aboutToGoToNext()
{
    if (ui.declineButton->isChecked()) {
        m_notAccepted.append(m_licenses.keys().at(m_iterator));
    }

    m_iterator++;

    setUpLicense();
}

void LicensePage::aboutToGoToPrevious()
{
    emit enableNextButton(true);
    emit goToPreviousStep();
}

void LicensePage::enableNext()
{
    if (!ui.acceptButton->isChecked() && !ui.declineButton->isChecked())
        emit enableNextButton(false);
    else
        emit enableNextButton(true);
}

void LicensePage::setUpLicense()
{
    if (m_iterator >= m_licenses.keys().count()) {
        qDebug() << "The following packages will be removed:" << m_notAccepted;
        m_handler->setRemoveLicenses(m_notAccepted);
        emit goToNextStep();
        return;
    }

    ui.acceptButton->setAutoExclusive(false);
    ui.declineButton->setAutoExclusive(false);
    ui.acceptButton->setChecked(false);
    ui.declineButton->setChecked(false);
    ui.acceptButton->setAutoExclusive(true);
    ui.declineButton->setAutoExclusive(true);
    ui.acceptButton->setEnabled(true);
    ui.acceptButton->setChecked(true);
    ui.textEdit->verticalScrollBar()->setValue(0);

    enableNext();

    if (m_licenses.keys().at(m_iterator).isEmpty()) {
        m_iterator++;

        setUpLicense();

        return;
    }

    ui.licenseLabel->setText(i18n("Accept license for: ") + "<b>" + m_licenses.values().at(m_iterator) + "</b>");

    QDir dir("/usr/share/licenses/" + m_licenses.keys().at(m_iterator));

    QStringList list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QString filename;

    if (list.count() == 1) {
        filename = list.at(0);
    } else {
        foreach(const QString &ent, list) {
            if (ent.contains("license", Qt::CaseInsensitive)) {
                filename = ent;
                break;
            }
        }
    }

    QFile file("/usr/share/licenses/" + m_licenses.keys().at(m_iterator) + '/' + filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return; // Deep shit...

    ui.textEdit->setText(QString::fromUtf8(file.readAll()));
}

#include "licensepage.moc"
