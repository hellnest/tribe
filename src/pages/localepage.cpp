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

#include <QDebug>

#include <QFile>
#include <QItemSelectionModel>
#include <QMouseEvent>

#include <KIcon>
#include <KLocale>
#include <KMessageBox>

#include <marble/MarbleWidget.h>
#include <marble/MarbleModel.h>

#include <config-tribe.h>

#include "../installationhandler.h"
#include "localepage.h"


using namespace Marble;

LocalePage::LocalePage(QWidget *parent)
        : AbstractPage(parent),
        m_install(InstallationHandler::instance())
{
}

LocalePage::~LocalePage()
{
}

void LocalePage::createWidget()
{
    setupUi(this);

    zoomInButton->setIcon(KIcon("zoom-in"));
    zoomOutButton->setIcon(KIcon("zoom-out"));

    // setup marble widget
    marble->installEventFilter(this);
    marble->setCenterLatitude(35.0);
    marble->setCenterLongitude(-28.0);
    marble->setMapThemeId("earth/citylights/citylights.dgml");
    marble->setShowCities(false);
    marble->setShowOverviewMap(false);
    marble->setShowScaleBar(false);
    marble->setShowCompass(false);
    marble->setShowCrosshairs(false);
    marble->setShowGrid(false);
    marble->addGeoDataFile(QString(DATA_INSTALL_DIR) + "/marble/data/placemarks/cities.kml");

    // parse timezone data
    QFile f(QString(CONFIG_INSTALL_PATH) + "/timezones");
    if (!f.open(QIODevice::ReadOnly))
        qDebug() << f.errorString();
    QTextStream stream(&f);
    locales.clear();
    while (!stream.atEnd()) {
        QString line(stream.readLine());
        QStringList split = line.split(':');
        locales.append(split);
    }
    f.close();

    // parse language pkg data
    f.setFileName(QString(CONFIG_INSTALL_PATH) + "/all_kde_langpacks");
    if (!f.open(QIODevice::ReadOnly))
        qDebug() << f.errorString();
    while (!stream.atEnd()) {
        QString line(stream.readLine());
        QStringList split = line.split(":");
        m_allKDELangs.insert(split.first(), split.last());
    }
    f.close();

    // sort locales
    QStringList keys;
    foreach (const QStringList &l, locales) {
        keys << l.first();
    }
    keys.sort();

    // populate combo boxes
    foreach(const QString &string, keys) {
        QStringList split = string.split("/");

        if (m_allTimezones.contains(split.first())) {
            m_allTimezones[split.first()].append(split.last());
        } else {
            m_allTimezones.insert(split.first(), QStringList(split.last()));
            continentCombo->addItem(split.first());
        }

        regionCombo->addItem(string);
    }

    // this looks interesting.. finish it. ;)
    locationsSearch->hide();
    locationsView->hide();

    // trigger changed for new combo box data
    continentChanged(continentCombo->currentIndex());
    regionChanged(regionCombo->currentIndex());

    connect(zoomInButton, SIGNAL(clicked()), marble, SLOT(zoomIn()));
    connect(zoomOutButton, SIGNAL(clicked()), marble, SLOT(zoomOut()));
    connect(zoomSlider, SIGNAL(valueChanged(int)), SLOT(zoom(int)));

    connect(marble, SIGNAL(zoomChanged(int)), this, SLOT(zoomChanged(int)));

    connect(continentCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(continentChanged(int)));
    connect(regionCombo, SIGNAL(currentIndexChanged(int)), SLOT(regionChanged(int)));

    connect(showLocalesCheck, SIGNAL(stateChanged(int)), SLOT(updateLocales()));
    connect(showKDELangsCheck, SIGNAL(stateChanged(int)), SLOT(updateLocales()));

    if (!m_install->continent().isEmpty()) {
        continentCombo->addItem(m_install->continent());
        continentCombo->setCurrentIndex(continentCombo->count() - 1);
    }

    if (!m_install->region().isEmpty()) {
        regionCombo->addItem(m_install->region());
        regionCombo->setCurrentIndex(regionCombo->count() - 1);
    }

    if (!m_install->KDELangPack().isEmpty()) {
        kdeLanguageCombo->addItem(m_allKDELangs.value(m_install->KDELangPack()));
        kdeLanguageCombo->setCurrentIndex(kdeLanguageCombo->count() - 1);
    }

    if (!m_install->locale().isEmpty()) {
        localeCombo->addItem(m_install->locale());
        localeCombo->setCurrentIndex(localeCombo->count() - 1);
    }

    zoom(55);
}

void LocalePage::zoom(int value)
{
    marble->zoomView(value * 20);
}

bool LocalePage::eventFilter(QObject * object, QEvent * event)
{
    /// there is some way to stop the long/lat popup menu from here..

    // if mouse was pressed on the marble widget
    if (object == marble && event->type() == QEvent::MouseButtonPress) {
        // if an actual place was clicked
        QVector<QModelIndex> indexes = marble->whichFeatureAt(marble->mapFromGlobal(QCursor::pos()));
        if (!indexes.isEmpty()) {
            // check the place against the data, and set the combo box accordingly
            QHash<QString, QStringList>::const_iterator it;
            for (it = m_allTimezones.constBegin(); it != m_allTimezones.constEnd(); it++) {
                if ((*it).contains(indexes.first().data(Qt::DisplayRole).toString().replace(' ', '_'))) {
                    continentCombo->setCurrentIndex(continentCombo->findText(it.key()));
                    regionCombo->setCurrentIndex(regionCombo->findText(indexes.first().data(Qt::DisplayRole).toString().replace(' ', '_')));
                }
            }

            return true;
        }
    }

    return false;
}

void LocalePage::continentChanged(int index)
{
    regionCombo->clear();

    QStringList timezones = m_allTimezones.value(continentCombo->itemText(index));

    QStringList::const_iterator it;
    for (it = timezones.constBegin(); it != timezones.constEnd(); ++it) {
        regionCombo->addItem((*it));
    }

    regionChanged(regionCombo->currentIndex());
}

void LocalePage::regionChanged(int index)
{
    if (!showLocalesCheck->isChecked() || !showKDELangsCheck->isChecked()) {
        QString time = continentCombo->itemText(continentCombo->currentIndex()) + "/" + regionCombo->itemText(index);

        QList<QStringList>::const_iterator it;
        for (it = locales.constBegin(); it != locales.constEnd(); ++it) {
            if ((*it).first() == time) {
                if (!showLocalesCheck->isChecked()) {
                    localeCombo->clear();

                    foreach(const QString &str, (*it).at(1).split(',')) {
                        localeCombo->addItem(str);
                    }
                }

                if (!showKDELangsCheck->isChecked()) {
                    kdeLanguageCombo->clear();

                    foreach(const QString &str, (*it).at(2).split(',')) {
                        kdeLanguageCombo->addItem(m_allKDELangs.value(str));
                    }
                }
            }
        }
    }
}

void LocalePage::updateLocales()
{
    if (showLocalesCheck->isChecked()) {
        if (m_allLocales.isEmpty()) {
            QFile fp(QString(CONFIG_INSTALL_PATH) + "/all_locales");

            if (!fp.open(QIODevice::ReadOnly | QIODevice::Text))
                qDebug() << "LocalePage: Failed to open file";

            QTextStream in(&fp);

            while (!in.atEnd()) {
                QString line(in.readLine());
                m_allLocales.append(line);
            }

            fp.close();
        }

        QString current = localeCombo->currentText();
        localeCombo->clear();

        foreach (const QString &loc, m_allLocales) {
            localeCombo->addItem(loc);
        }

        if (localeCombo->findText(current) != -1) {
            localeCombo->setCurrentIndex(localeCombo->findText(current));
        }
    }

    if (showKDELangsCheck->isChecked()) {
        QString current = kdeLanguageCombo->currentText();
        QStringList values = m_allKDELangs.values();
        values.sort();

        kdeLanguageCombo->clear();

        foreach (const QString &loc, values) {
            kdeLanguageCombo->addItem(loc);
        }

        if (kdeLanguageCombo->findText(current) != -1) {
            kdeLanguageCombo->setCurrentIndex(kdeLanguageCombo->findText(current));
        }
    }

    if (!showKDELangsCheck->isChecked() || !showLocalesCheck->isChecked()) {
        regionChanged(regionCombo->currentIndex());
    }
}

void LocalePage::zoomChanged(int)
{
    disconnect(zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(zoom(int)));
    zoomSlider->setValue(marble->zoom() / 20);
    connect(zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(zoom(int)));
}

bool LocalePage::validate()
{
    if (regionCombo->currentText().isEmpty()) {
        bool retbool;
        KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
        dialog->setButtons(KDialog::Ok);
        dialog->setModal(true);

        KMessageBox::createKMessageBox(dialog, QMessageBox::Warning,
                                        i18n("You need to select a timezone"),
                                        QStringList(), QString(), &retbool, KMessageBox::Notify);
        return false;
    }

    m_install->setContinent(continentCombo->currentText());
    m_install->setRegion(regionCombo->currentText());
    m_install->setTimezone(continentCombo->currentText()+"/"+regionCombo->currentText());
    m_install->setKDELangPack(m_allKDELangs.key(kdeLanguageCombo->currentText()));

    if (localeCombo->currentText().contains("utf-8", Qt::CaseInsensitive)) {
        QStringList localeSplit = localeCombo->currentText().split('.');
        localeSplit.last().replace('-', QString());
        localeSplit.last() = localeSplit.last().toLower();
        m_install->setLocale(localeSplit.join("."));
    } else {
        m_install->setLocale(localeCombo->currentText());
    }

    return true;
}

void LocalePage::aboutToGoToNext()
{
    if (validate())
        emit goToNextStep();
}

void LocalePage::aboutToGoToPrevious()
{
    if (validate())
        emit goToPreviousStep();
}

#include "localepage.moc"
