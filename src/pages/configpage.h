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

#ifndef CONFIGPAGE_H
#define CONFIGPAGE_H

#include <QProcess>
#include <QMovie>

#include <KIO/NetAccess>
#include <KIO/TransferJob>
#include <KIO/Job>

#include "../abstractpage.h"
#include "ui_config.h"
#include <QTimer>

#include <QNetworkAccessManager>

class InstallationHandler;

class ConfigPage : public AbstractPage
{
    Q_OBJECT

public:
    ConfigPage(QWidget *parent = 0);
    virtual ~ConfigPage();
    
public slots:
    // handle network data
    void handleNetworkData(QNetworkReply *networkReply);

private slots:
    virtual void createWidget();
    virtual void aboutToGoToNext();
    virtual void aboutToGoToPrevious();

    void setDownloadBundlesPage();
    void setChangeAppearancePage();
    void setInitRamdiskPage();
    void setBootloaderPage();

    void incomingData(KIO::Job*, QByteArray);
    void result(KJob*);
    void updatePacmanProgress();
    void processComplete();
    
    // download bundles page
    void populateBundlesList();
    void bundlesDownloadButtonClicked();
    
    // customize initrd page
    void initRdGenerationComplete();
    void generateInitRamDisk();
    
    // bootloader page
    void bootloaderInstalled(int, QProcess::ExitStatus);
    void menulstInstalled(int, QProcess::ExitStatus);
    
    void cancelButtonClicked();

private:
    Ui::Config ui;

    InstallationHandler *m_install;

    QProcess *m_process;

    KIO::TransferJob *m_job;
    
    QMovie *m_busyAnim;

    QStringList m_incomingList;
    QString m_incomingExtension;
    int m_incomingIncr;

    QString m_currentBranch;
    QString m_currentArch;
    QString m_currentPkgName;
    QString m_currentOnlineStatus;
    
    QNetworkAccessManager networkManager;
    
    int m_downloadSize;
    int m_soFarDownloadSize;
    int m_soFarPkgDownloadSize;

    QTimer *m_timer;
    
    int m_currentPage;
};

#endif /* CONFIGPAGE_H */
