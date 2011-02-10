/*
 * Copyright (c) 2008, 2009  Dario Freddi <drf@chakra-project.org>
 *               2010        Drake Justice <djustice.kde@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <QDebug>

#include <QFile>
#include <QMovie>
#include <QProcess>
#include <QRegExpValidator>

#include <KIcon>
#include <KIO/Job>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <KMessageBox>

#include <config-tribe.h>

#include "../installationhandler.h"
#include "configpage.h"


const QString tI = "/tmp/tribe_initcpio_enable_";
QStringList tmpInitRd = QStringList() << QString(tI + "usb") << QString(tI + "firewire")
                                      << QString(tI + "pcmcia") << QString(tI + "nfs")
                                      << QString(tI + "softwareraid") << QString(tI + "softwareraidmdp")
                                      << QString(tI + "lvm2") << QString(tI + "encrypted");


ConfigPage::ConfigPage(QWidget *parent)
        : AbstractPage(parent),
        m_install(InstallationHandler::instance())
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, SIGNAL(finished(int)), this, SLOT(processComplete()));

    m_timer = new QTimer(this);

    m_soFarDownloadSize = 0;
    m_soFarPkgDownloadSize = 0;
    m_downloadSize = 0;
    m_currentPage = 0;
}

ConfigPage::~ConfigPage()
{
}

void ConfigPage::createWidget()
{
    ui.setupUi(this);

    ui.changeAppearanceButton->setVisible(false);

    // page connections
    connect(ui.installPkgzButton, SIGNAL(clicked()), this, SLOT(setInstallPkgzPage()));
    connect(ui.downloadBundlesButton, SIGNAL(clicked()), this, SLOT(setDownloadBundlesPage()));
    connect(ui.changeAppearanceButton, SIGNAL(clicked()), this, SLOT(setChangeAppearancePage()));
    connect(ui.initRamDiskButton, SIGNAL(clicked()), this, SLOT(setInitRamdiskPage()));
    connect(ui.bootloaderSettingsButton, SIGNAL(clicked()), this, SLOT(setBootloaderPage()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));
    // pkg installer
    connect(ui.pkgList, SIGNAL(currentRowChanged(int)), this, SLOT(currentPkgItemChanged(int)));
    connect(ui.pkgInstallButton, SIGNAL(clicked()), this, SLOT(pkgInstallButtonClicked()));
    ui.pkgScreenLabel->installEventFilter(this);
    ui.screenshotLabel->installEventFilter(this);
    // bundle download
    connect(ui.bundlesDownloadButton, SIGNAL(clicked()), this, SLOT(bundlesDownloadButtonClicked()));
    // generate initrd
    connect(ui.generateInitRamDiskButton, SIGNAL(clicked()), this, SLOT(generateInitRamDisk()));

    // page icons
    ui.installPkgzButton->setIcon(KIcon("repository"));
    ui.downloadBundlesButton->setIcon(KIcon("x-cb-bundle"));
    ui.changeAppearanceButton->setIcon(KIcon("preferences-desktop-color"));
    ui.initRamDiskButton->setIcon(KIcon("cpu"));
    ui.bootloaderSettingsButton->setIcon(KIcon("go-first"));
    ui.cancelButton->setIcon(KIcon("dialog-cancel"));
    // pkg installer
    ui.pkgInstallButton->setIcon(KIcon("run-build-install"));
    // bundle download
    ui.bundlesDownloadButton->setIcon(KIcon("download"));
    // generate initrd
    ui.generateInitRamDiskButton->setIcon(KIcon("debug-run"));

    // remove the initrd tmp files
    QProcess::execute("bash -c \"rm " + tmpInitRd.join(" ") + " > /dev/null 2&>1\"");

    // first call to check internet connection
    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(handleNetworkData(QNetworkReply*)));
    networkManager.get(QNetworkRequest(QString("http://chakra-project.org")));

    populatePkgzList();
    populateBundlesList();
}

bool ConfigPage::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        if (obj == ui.pkgScreenLabel) {
            ui.stackedWidget->setCurrentIndex(5);
            m_currentPage = 5;
        } else if (obj == ui.screenshotLabel) {
            ui.stackedWidget->setCurrentIndex(2);
            m_currentPage = 2;
        }
    }

    return 0;
}

void ConfigPage::switchPkgScreenshot()
{
    if (ui.stackedWidget->currentIndex() == 5) {
        ui.stackedWidget->setCurrentIndex(2);
        m_currentPage = 2;
    } else if (ui.stackedWidget->currentIndex() == 2) {
        ui.stackedWidget->setCurrentIndex(5);
        m_currentPage = 5;
    }
}

void ConfigPage::populatePkgzList()
{
    ui.pkgList->clear();

    QFile pkglistFile(QString(CONFIG_INSTALL_PATH) + "/configPagePkgData");

    if (pkglistFile.open(QIODevice::ReadOnly)) {
        m_incomingList = QString(pkglistFile.readAll()).trimmed().split("\n");
    } else {
        qDebug() << pkglistFile.errorString();
    }

    if (m_incomingList.isEmpty())
        return;

    foreach (QString pkg, m_incomingList) {
        QListWidgetItem *item = new QListWidgetItem(ui.pkgList);
        item->setSizeHint(QSize(0, 22));
        item->setText(pkg.split("::").at(1));
        item->setData(60, pkg.split("::").at(0));
        item->setData(61, pkg.split("::").at(2));
        item->setIcon(QIcon(QString(CONFIG_INSTALL_PATH) + "/" + item->data(60).toString() + ".png"));
        item->setCheckState(Qt::Unchecked);
        ui.pkgList->addItem(item);
        m_incomingList.append(pkg.split("::").at(0) + "_thumb");
        m_incomingList.append(pkg.split("::").at(0));
    }

    m_incomingIncr = 0;
    m_incomingExtension = ".jpeg";

    KUrl r(QUrl("http://chakra-project.org/packages/screenshots/" + 
                m_incomingList.at(m_incomingIncr) + m_incomingExtension));
    m_job = KIO::get(r, KIO::Reload, KIO::Overwrite | KIO::HideProgressInfo);
    connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(incomingData(KIO::Job*, QByteArray)));
    connect(m_job, SIGNAL(result(KJob*)), this, SLOT(result(KJob*)));
}

void ConfigPage::incomingData(KIO::Job* job, QByteArray data)
{
    if (ui.progressBar->maximum() != job->totalAmount(KJob::Bytes))
        ui.progressBar->setMaximum(job->totalAmount(KJob::Bytes));

    if (data.isNull()) {
        if (job->processedAmount(KJob::Bytes) == job->totalAmount(KJob::Bytes))
            return;
    }

    if (m_incomingIncr == m_incomingList.count())
        return;

    if (m_incomingExtension == ".jpeg") {
        QFile x("/tmp/" + m_incomingList.at(m_incomingIncr) + m_incomingExtension);
        if (x.open(QIODevice::Append)) {
            x.write(data);
            x.flush();
            x.close();
        }
    } else {
        ui.progressBar->setValue(job->processedAmount(KJob::Bytes));
        ui.progressLabel->setText(i18n("Downloading") + " " + m_incomingList.at(m_incomingIncr));
        QFile x(QString(INSTALLATION_TARGET) + "/home/" + m_install->userLoginList().first() + "/Desktop/" + m_incomingList.at(m_incomingIncr));
        if (x.open(QIODevice::Append)) {
            x.write(data);
            x.flush();
            x.close();
        }
    }
}

void ConfigPage::result(KJob* job)
{
    m_incomingIncr++;

    if (m_incomingIncr == m_incomingList.count()) {
        m_incomingIncr = 0;
        m_incomingList.clear();
        if (m_incomingExtension != ".jpeg")
            ui.stackedWidget->setCurrentIndex(3);
            m_currentPage = 3;
            ui.bundlesDownloadButton->setEnabled(true);
            enableNextButton(true);
        m_incomingExtension = "";
        return;
    }

    if (job->error()) {
        qDebug() << ">> m_job error " + job->errorString();
        ui.stackedWidget->setCurrentIndex(m_currentPage);
        ui.bundlesDownloadButton->setEnabled(true);
        ui.installPkgzButton->setEnabled(true);
        enableNextButton(true);
        return;
    }

    if (m_incomingExtension == ".jpeg") {
        KUrl r(QUrl("http://chakra-project.org/packages/screenshots/" + 
                    m_incomingList.at(m_incomingIncr) + m_incomingExtension));
        m_job = KIO::get(r, KIO::Reload, KIO::Overwrite | KIO::HideProgressInfo);
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(incomingData(KIO::Job*, QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(result(KJob*)));
    } else {
        KUrl r(QUrl("http://mirror.rit.edu/kdemod/bundles" + m_currentBranch + "/" +
                    m_currentArch + "/" + m_incomingList.at(m_incomingIncr)));
        m_job = KIO::get(r, KIO::Reload, KIO::Overwrite | KIO::HideProgressInfo);
        connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(incomingData(KIO::Job*, QByteArray)));
        connect(m_job, SIGNAL(result(KJob*)), this, SLOT(result(KJob*)));
    }
}

void ConfigPage::populateBundlesList()
{
    ui.bundlesList->clear();

    QStringList bundleDataList;
    QFile bundlelistFile(QString(CONFIG_INSTALL_PATH) + "/configPageBundleData");

    if (bundlelistFile.open(QIODevice::ReadOnly)) {
        bundleDataList = QString(bundlelistFile.readAll()).trimmed().split("\n");
    } else {
        qDebug() << ">> bundlelistFile error: " + bundlelistFile.errorString();
    }

    if (bundleDataList.isEmpty())
        return;

    foreach (QString pkg, bundleDataList) {
        QListWidgetItem *item = new QListWidgetItem(ui.bundlesList);
        item->setSizeHint(QSize(160, 36));
        item->setText(pkg.split("::").at(1));
        item->setCheckState(Qt::Unchecked);
        item->setData(60, pkg.split("::").at(0));
        item->setData(61, pkg.split("::").at(2));
        item->setIcon(QIcon(QString(CONFIG_INSTALL_PATH) + "/" + item->data(60).toString() + ".png"));
        ui.bundlesList->addItem(item);
    }
}

void ConfigPage::cancelButtonClicked()
{
    delete m_process;
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, SIGNAL(finished(int)), this, SLOT(processComplete()));

    ui.bundlesDownloadButton->setEnabled(true);
    ui.installPkgzButton->setEnabled(true);
    enableNextButton(true);
    ui.stackedWidget->setCurrentIndex(m_currentPage);
}

void ConfigPage::handleNetworkData(QNetworkReply *networkReply)
{
  // no error -> internet access
  if (!networkReply->error())
    m_currentOnlineStatus = "Online";
  else
    m_currentOnlineStatus = "Offline";

  networkReply->deleteLater();
}

void ConfigPage::bundlesDownloadButtonClicked()
{
    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(handleNetworkData(QNetworkReply*)));

    //use the url of the preferred mirror 
    networkManager.get(QNetworkRequest(QString("http://chakra-project.org")));
    if (m_currentOnlineStatus == "Offline") {
        QString completeMessage = i18n("Sorry, you have no internet connection at the moment \n"
                                       "Will stop bundle(s) installation now");

        KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
        dialog->setButtons(KDialog::Ok);
        dialog->setModal(true);
        bool retbool;

        KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, completeMessage,
                                       QStringList(), QString(), &retbool, KMessageBox::Notify);
        return;
    }

    m_incomingList.clear();
    m_incomingExtension = "";
    m_incomingIncr = 0;

    ui.bundlesDownloadButton->setEnabled(false);
    enableNextButton(false);
    ui.stackedWidget->setCurrentIndex(7);
    ui.progressBar->setValue(0);
    ui.progressLabel->setText("Waiting for server...");

    QStringList checkedList;

    for ( int i = 0; i < ui.bundlesList->count(); i++ ) {
        if (ui.bundlesList->item(i)->checkState() == Qt::Checked) {
            checkedList.append(ui.bundlesList->item(i)->data(60).toString());
        }
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // check stable/testing
    QFile pacmanConf("/etc/pacman.conf", this);
    pacmanConf.open(QIODevice::ReadOnly);
    QString sPacmanConf(pacmanConf.readAll());
    pacmanConf.close();

    foreach (QString line, sPacmanConf.split("\n")) {
        if (line.contains("-testing")) {
            if (line.simplified().trimmed().startsWith("#"))
                continue;

            m_currentBranch = "-testing";
        }
    }

    // check uname for arch
    m_process->start("uname -m");
    m_process->waitForFinished();
    if (m_process->readAll().contains("x86_64")) {
        m_currentArch = "x86_64";
    } else {
        m_currentArch = "i686";
    }

    // call rsync
    foreach (QString bundle, checkedList) {
        m_process->start("bash -c \"echo $(rsync -avh --list-only cinstall@chakra-project.org::cinstall/bundles" +
                        m_currentBranch + "/" + m_currentArch +
                        "/" + bundle + "*  | cut -d\':\' -f3 | cut -d\' \' -f2)\"");
        m_process->waitForFinished();
        QString result(m_process->readAll());
        if (result.simplified().trimmed().split("\n").count() > 1) {
            m_incomingList.append(result.simplified().trimmed().split("\n").first().simplified().trimmed().split(" ").at(1));
        } else {
            m_incomingList.append(result.simplified().trimmed().split(" ").at(1));
        }
    }

    KUrl r(QUrl("http://mirror.rit.edu/kdemod/bundles" + m_currentBranch + "/" +
                    m_currentArch + "/" + m_incomingList.at(m_incomingIncr)));
    m_job = KIO::get(r, KIO::Reload, KIO::Overwrite | KIO::HideProgressInfo);
    connect(m_job, SIGNAL(data(KIO::Job*,QByteArray)), this, SLOT(incomingData(KIO::Job*, QByteArray)));
    connect(m_job, SIGNAL(result(KJob*)), this, SLOT(result(KJob*)));
}

void ConfigPage::currentPkgItemChanged(int i)
{
    // pkg name
    ui.pkgNameLabel->setText("<font size=5><b>" + ui.pkgList->item(i)->data(60).toString());

    // pkg version
    QProcess p;
    p.start("pacman -Sp --print-format %v " + ui.pkgList->item(i)->data(60).toString());
    p.waitForFinished();
    QString pkgVer = QString(p.readAll()).simplified().split(" ").last();
    p.start("chroot " + QString(INSTALLATION_TARGET) + " pacman -Qq");
    p.waitForFinished();
    QString installedPkgz(p.readAll());
    if (installedPkgz.contains(ui.pkgList->item(i)->data(60).toString()))
        pkgVer.append("  (" + i18n("installed") + ")");
    ui.pkgVerLabel->setText(pkgVer);

    // pkg description
    ui.pkgDescLabel->setText(ui.pkgList->item(i)->data(61).toString());

    // pkg screenshot
    ui.pkgScreenLabel->setPixmap(QPixmap("/tmp/" + ui.pkgList->item(i)->data(60).toString() + "_thumb.jpeg"));
    ui.screenshotLabel->setPixmap(QPixmap("/tmp/" + ui.pkgList->item(i)->data(60).toString() + ".jpeg"));
}

void ConfigPage::setInstallPkgzPage()
{
    if (ui.stackedWidget->currentIndex() != 2) {
        ui.stackedWidget->setCurrentIndex(2);
        m_currentPage = 2;
        ui.currentPageLabel->setText(i18n("Install Packaged Software"));
    } else {
        ui.stackedWidget->setCurrentIndex(0);
        m_currentPage = 0;
        ui.currentPageLabel->setText("");
    }
}

void ConfigPage::pkgInstallButtonClicked()
{
    connect(&networkManager, SIGNAL(finished(QNetworkReply*)),
             this, SLOT(handleNetworkData(QNetworkReply*)));

    //use the url of the preferred mirror 
    networkManager.get(QNetworkRequest(QString("http://chakra-project.org")));
    if (m_currentOnlineStatus == "Offline") {
        QString completeMessage = i18n("Sorry, you have no internet connection at the moment \n"
                                       "Will stop package(s) installation now");

        KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
        dialog->setButtons(KDialog::Ok);
        dialog->setModal(true);
        bool retbool;

        KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, completeMessage,
                                       QStringList(), QString(), &retbool, KMessageBox::Notify);
        return;
    }

    // disable buttons
    ui.pkgInstallButton->setEnabled(false);
    enableNextButton(false);
    // mount special folders
    QProcess::execute("mount -v -t proc none " + QString(INSTALLATION_TARGET) + "/proc");
    QProcess::execute("mount -v -t sysfs none " + QString(INSTALLATION_TARGET) + "/sys");
    QProcess::execute("mount -v -o bind /dev " + QString(INSTALLATION_TARGET) + "/dev");
    QProcess::execute("mount -v -t devpts devpts " + QString(INSTALLATION_TARGET) + "/dev/pts");
    QProcess::execute("xhost +");
    // install pkgz

    QStringList pkgz;
    for (int i = 0; i < ui.pkgList->count(); i++) {
        if (ui.pkgList->item(i)->checkState() == Qt::Checked)
            pkgz.append(ui.pkgList->item(i)->data(60).toString());
    }

    if (pkgz.isEmpty())
        return;

    QProcess::execute("rm -f " + QString(INSTALLATION_TARGET) + "/var/lib/pacman/db.lck");

    connect(m_timer, SIGNAL(timeout()), this, SLOT(updatePacmanProgress()));
    m_timer->start(500);

    m_process->start("chroot " + QString(INSTALLATION_TARGET) + " su " + m_install->userLoginList().first() + 
                     " -c \"kdesu -t -d --noignorebutton -- pacman --noconfirm -Sf " + pkgz.join(" ") + "\"");

    ui.progressLabel->setText(i18n("Waiting for server..."));
    ui.progressBar->setValue(0);
    ui.stackedWidget->setCurrentIndex(7);
}

void ConfigPage::updatePacmanProgress()
{
    QString str = QString::fromUtf8(m_process->readAll());
    QRegExp targetRegEx("(*/*)");
    targetRegEx.setPatternSyntax(QRegExp::Wildcard);

    foreach (QString line, str.split("\n")) {
        if (line.contains(i18n("loading package data..."))) {
            ui.progressLabel->setText(i18n("Loading package data..."));
        } else if (line.contains(i18n("checking dependencies..."))) {
            ui.progressLabel->setText(i18n("Checking dependencies..."));
        } else if (line.contains(i18n("resolving dependencies..."))) {
            ui.progressLabel->setText(i18n("Resolving dependencies..."));
        } else if (line.contains(i18n("checking package integrity..."))) {
            ui.progressLabel->setText(i18n("Checking package integrity..."));
        } else if (line.contains(i18n("Total Download Size:"))) {
            int mb = line.simplified().split(" ").at(3).split(".").at(0).toInt();
            int kb = line.simplified().split(" ").at(3).split(".").at(1).toInt();
            m_downloadSize = (mb * 1000) + (kb * 10);
            ui.progressBar->setMaximum(m_downloadSize);
        } else if (line.contains("/s 0")) {
            ui.progressLabel->setText(i18n("Downloading") + " " + m_currentPkgName);

            QString cDld = line.simplified().split(" ").at(1);
            m_currentPkgName = line.simplified().split(" ").at(0);
            cDld.append(" ");
            if (cDld.contains("K ")) {
                if (cDld.contains("0.0K")) {
                    m_soFarDownloadSize += m_soFarPkgDownloadSize;
                    if (m_soFarDownloadSize >= ui.progressBar->value())
                        ui.progressBar->setValue(m_soFarDownloadSize);
                    m_soFarPkgDownloadSize = 0;
                } else {
                    m_soFarPkgDownloadSize = cDld.split(".").at(0).toInt();
                    if (m_soFarDownloadSize >= ui.progressBar->value())
                        ui.progressBar->setValue(m_soFarDownloadSize);
                }
            } else if (cDld.contains("M ")) {
                m_soFarPkgDownloadSize = cDld.split(".").at(0).toInt() * 1000;
                if (m_soFarDownloadSize >= ui.progressBar->value())
                    ui.progressBar->setValue(m_soFarDownloadSize);
            }

            int v = m_soFarDownloadSize + m_soFarPkgDownloadSize;
            if (v >= ui.progressBar->value())
                ui.progressBar->setValue(v);
        } else if (line.left(6) == "kdesu(") {

        } else if (line.contains(targetRegEx)) {
            ui.progressLabel->setText(str.simplified().split(" (").at(0));
            m_currentPkgName = str.simplified();
            ui.progressBar->setMaximum(str.split("/").at(1).split(")").at(0).toInt());
            ui.progressBar->setValue(str.split("/").at(0).split("(").at(1).toInt());
        }
    }
}

void ConfigPage::processComplete()
{
    // clean-up
    QProcess::execute("umount -v " + QString(INSTALLATION_TARGET) + "/proc " + 
                                     QString(INSTALLATION_TARGET) + "/sys "  + 
                                     QString(INSTALLATION_TARGET) + "/dev/pts " + 
                                     QString(INSTALLATION_TARGET) + "/dev");
    // re-enable buttons
    ui.stackedWidget->setCurrentIndex(2);
    m_currentPage = 2;
    ui.pkgInstallButton->setEnabled(true);
    enableNextButton(true);

    m_timer->stop();
}

void ConfigPage::setDownloadBundlesPage()
{
    if (ui.stackedWidget->currentIndex() != 3) {
        ui.stackedWidget->setCurrentIndex(3);
        m_currentPage = 3;
        ui.currentPageLabel->setText(i18n("Download Popular Bundles"));
    } else {
        ui.stackedWidget->setCurrentIndex(0);
        m_currentPage = 0;
        ui.currentPageLabel->setText("");
    }
}

void ConfigPage::setChangeAppearancePage()
{

}

void ConfigPage::setBootloaderPage()
{
    if (ui.stackedWidget->currentIndex() != 6) {
        ui.stackedWidget->setCurrentIndex(6);
        m_currentPage = 6;
        ui.currentPageLabel->setText(i18n("Bootloader Settings"));
    } else {
        ui.stackedWidget->setCurrentIndex(0);
        m_currentPage = 0;
        ui.currentPageLabel->setText("");
    }
}

void ConfigPage::setInitRamdiskPage()
{
    if (ui.stackedWidget->currentIndex() != 1) {
        ui.stackedWidget->setCurrentIndex(1);
        m_currentPage = 1;
        ui.currentPageLabel->setText(i18n("Customize Initial Ramdisk"));
    } else {
        ui.stackedWidget->setCurrentIndex(0);
        m_currentPage = 0;
        ui.currentPageLabel->setText("");
    }
}

void ConfigPage::generateInitRamDisk()
{
    ui.generateInitRamDiskButton->setEnabled(false);
    enableNextButton(false);
    m_busyAnim = new QMovie(":Images/images/busywidget.gif");
    m_busyAnim->start();
    ui.initRdBusyLabel->setMovie(m_busyAnim);
    ui.initRdBusyLabel->setVisible(true);

    if (ui.usb->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(0));
    if (ui.firewire->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(1));
    if (ui.pcmcia->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(2));
    if (ui.nfs->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(3));
    if (ui.raid->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(4));
    if (ui.mdp->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(5));
    if (ui.lvm2->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(6));
    if (ui.encrypted->isChecked())
        QProcess::execute("touch " + tmpInitRd.at(7));

    QString command  = QString("sh " + QString(SCRIPTS_INSTALL_PATH) +
                               "/postinstall.sh --job create-initrd %1")
                               .arg(m_install->m_postcommand);
    connect(m_process, SIGNAL(finished(int)), this, SLOT(initRdGenerationComplete()));
    m_process->start(command);
}

void ConfigPage::initRdGenerationComplete()
{
    ui.generateInitRamDiskButton->setEnabled(true);
    enableNextButton(true);
    ui.initRdBusyLabel->setVisible(false);

    // remove tmp files
    QProcess::execute("bash -c \"rm " + tmpInitRd.join(" ") + " > /dev/null 2&>1\"");
}

void ConfigPage::bootloaderInstalled(int exitCode, QProcess::ExitStatus exitStatus)
{
    disconnect(m_install, SIGNAL(bootloaderInstalled(int, QProcess::ExitStatus)), 0, 0);

    Q_UNUSED(exitStatus)

    if (exitCode != 0) {

    } else {
        connect(m_install, SIGNAL(bootloaderInstalled(int, QProcess::ExitStatus)), SLOT(menulstInstalled(int, QProcess::ExitStatus)));

        emit setProgressWidgetText(i18n("Creating OS list..."));
        emit updateProgressWidget(50);

        m_install->installBootloader(1);  /// Start second step of grub install
    }
}

void ConfigPage::menulstInstalled(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    if (exitCode != 0) {

    } else {
        emit deleteProgressWidget();
        emit goToNextStep();
    }
}

void ConfigPage::aboutToGoToNext()
{
    ui.stackedWidget->setCurrentIndex(0);
    m_currentPage = 0;

    if (!ui.bootloaderCheckBox->isChecked()) {
        emit goToNextStep();
        return;
    }

    emit showProgressWidget();
    emit setProgressWidgetText(i18n("Installing bootloader..."));
    emit updateProgressWidget(0);

    connect(m_install, SIGNAL(bootloaderInstalled(int, QProcess::ExitStatus)), SLOT(bootloaderInstalled(int, QProcess::ExitStatus)));

    m_install->installBootloader(0); /// Start first step of grub install
}

void ConfigPage::aboutToGoToPrevious()
{
    emit goToPreviousStep();
}

#include "configpage.moc"
