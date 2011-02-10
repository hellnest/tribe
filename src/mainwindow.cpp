
/*
 * Copyright (c) 2008, 2009  Dario Freddi <drf@chakra-project.org>
 *               2008        Lukas Appelhans <l.appelhans@gmx.de>
 *               2010        Drake Justice <djustice.kde@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <QResizeEvent>
#include <QCloseEvent>
#include <QMovie>

#include <KIcon>
#include <KMessageBox>
#include <KApplication>

#include <config-tribe.h>

#include "pages.h"
#include "installationhandler.h"
#include "abstractpage.h"
#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
          : KMainWindow(parent,
                       (Qt::WindowFlags) KDE_DEFAULT_WINDOWFLAGS | Qt::FramelessWindowHint)
{
    m_centralWidget = new QWidget(this);
    m_ui.setupUi(m_centralWidget);

    m_ui.buildSignatureLabel->setText(QString("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN"
                                              "http://www.w3.org/TR/REC-html40/strict.dtd\">"
                                              "<html>"
                                                 "<head>"
                                                    "<meta name=\"qrichtext\" content=\"1\" />"
                                                    "<style type=\"text/css\">"
                                                       "p, li { white-space: pre-wrap; }"
                                                    "</style>"
                                                 "</head>"
                                                 "<body style=\"font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">"
                                                    "<p style=\"margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
                                                       "<span style=\"font-family:'Bitstream Vera Sans'; font-size:7pt; color:#aa0000;\">" +
                                                          QString(TRIBE_BUILD_REVISION) +
                                                       "</span>"
                                                    "</p>"
                                                 "</body>"
                                              "</html>"));

    m_ui.abortButton->setIcon(KIcon("dialog-cancel"));
    connect(m_ui.abortButton, SIGNAL(clicked()), SLOT(abortInstallation()));

    m_ui.nextButton->setIcon(KIcon("go-next"));
    m_ui.previousButton->setIcon(KIcon("go-previous"));

    m_ui.welcomeIcon->setPixmap(QPixmap(":/Images/images/installation-stage-icon.png"));
    m_ui.preparationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-icon.png"));
    m_ui.installationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-icon.png"));
    m_ui.configurationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-icon.png"));

    m_movie = new QMovie(":/Images/images/active-page-anim-18.mng", QByteArray(), this);
    m_movie->start();

    m_install = InstallationHandler::instance();
    connect(m_install, SIGNAL(errorInstalling(const QString&)), SLOT(errorOccurred(const QString&)));

    m_install->init();

    m_currentAction = MainWindow::Welcome;

    setCentralWidget(m_centralWidget);
    setWindowState(Qt::WindowFullScreen);

    loadPage(m_currentAction);
}

MainWindow::~MainWindow()
{
    m_movie->stop();
    m_movie->deleteLater();
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
    Q_UNUSED(evt);

    abortInstallation();
}

void MainWindow::abortInstallation()
{
    QString msg;

    msg.append(i18n("Do you really want to abort the installation?"));

    KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
    bool retbool = true;
    dialog->setButtons(KDialog::Yes | KDialog::No);

    if (KMessageBox::createKMessageBox(dialog, KIcon("dialog-warning"), msg, QStringList(),
                                       QString(), &retbool, KMessageBox::Notify) == KDialog::Yes) {
        setUpCleanupPage();

        InstallationHandler::instance()->abortInstallation();

        qApp->exit(0);
    }
}

void MainWindow::loadPage(InstallationStep page)
{
    m_ui.nextButton->setVisible(true);
    m_ui.previousButton->setVisible(true);

    disconnect(this, SLOT(showProgressWidget()));
    disconnect(this, SIGNAL(readyToCreate()), 0, 0);

    while (m_ui.stackedWidget->count() != 0) {
        QWidget *w = m_ui.stackedWidget->widget(0);
        m_ui.stackedWidget->removeWidget(w);
        w->deleteLater();
    }

    switch (page) {
    case MainWindow::Welcome:
        m_ui.stackedWidget->addWidget(new IntroPage(this));
        m_ui.abortButton->setVisible(true);
        m_ui.previousButton->setVisible(false);
        break;

    case MainWindow::ReleaseNotes:
        m_ui.stackedWidget->addWidget(new ReleaseNotesPage(this));
        break;

    case MainWindow::LicenseApproval:
        m_ui.stackedWidget->addWidget(new LicensePage(this));
        break;

    case MainWindow::Language:
        m_ui.stackedWidget->addWidget(new LocalePage(this));
        break;

    case MainWindow::CreateUser:
        m_ui.stackedWidget->addWidget(new UserCreationPage(this));
        break;

    case MainWindow::Partition:
        m_ui.stackedWidget->addWidget(new PartitionPage(this));
        break;

    case MainWindow::ReadyToInstall:
        m_ui.stackedWidget->addWidget(new ReadyInstallPage(this));
        break;

    case MainWindow::InstallSystem:
        m_ui.stackedWidget->addWidget(new InstallationPage(this));
        m_ui.previousButton->setVisible(false);
        m_ui.nextButton->setVisible(false);
        break;

    case MainWindow::Configuration:
        m_ui.stackedWidget->addWidget(new ConfigPage(this));
        m_ui.previousButton->setVisible(false);
        break;

    case MainWindow::FinishStep:
        m_ui.stackedWidget->addWidget(new FinishPage(this));
        m_ui.previousButton->setVisible(false);
        m_ui.nextButton->setVisible(false);
        m_ui.abortButton->setVisible(false);
        break;

    default:
        break;
    }

    setInstallationStep(page, MainWindow::InProgress);

    m_ui.stackedWidget->setCurrentIndex(0);

    AbstractPage *abstractPage = qobject_cast<AbstractPage *>(m_ui.stackedWidget->widget(0));

    if (abstractPage) {
        connect(abstractPage, SIGNAL(abortInstallation()), this, SLOT(abortInstallation()));
        connect(abstractPage, SIGNAL(goToNextStep()), this, SLOT(goToNextStep()));
        connect(abstractPage, SIGNAL(goToPreviousStep()), this, SLOT(goToPreviousStep()));
        connect(abstractPage, SIGNAL(showProgressWidget()), this, SLOT(showProgressWidget()));
        connect(abstractPage, SIGNAL(deleteProgressWidget()), this, SLOT(deleteProgressWidget()));
        connect(abstractPage, SIGNAL(updateProgressWidget(int)), this, SIGNAL(updateProgressWidget(int)));
        connect(abstractPage, SIGNAL(setProgressWidgetText(const QString&)), this, SIGNAL(setProgressWidgetText(const QString&)));
        connect(abstractPage, SIGNAL(enableNextButton(bool)), this, SLOT(enableNextButton(bool)));
        connect(abstractPage, SIGNAL(enablePreviousButton(bool)), this, SLOT(enablePreviousButton(bool)));

        connect(m_ui.nextButton, SIGNAL(clicked()), abstractPage, SLOT(aboutToGoToNext()));
        connect(m_ui.previousButton, SIGNAL(clicked()), abstractPage, SLOT(aboutToGoToPrevious()));

        connect(this, SIGNAL(readyToCreate()), abstractPage, SLOT(createWidget()));

        if (page == MainWindow::FinishStep) {
            FinishPage *finishPage = qobject_cast<FinishPage *>(abstractPage);

            connect(finishPage, SIGNAL(reboot()), this, SLOT(quitAndReboot()));
            connect(finishPage, SIGNAL(keepChakra()), this, SLOT(quitToChakra()));
        }
    }

    emit readyToCreate();
}

void MainWindow::setInstallationStep(InstallationStep step, StepStatus status)
{
    switch (step) {
    case MainWindow::Welcome:
        if (status == MainWindow::Done)
            m_ui.welcomeIcon->setPixmap(QPixmap(":/Images/images/installation-stage-done.png"));
        else if (status == MainWindow::ToDo)
            m_ui.welcomeIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
        else if (status == MainWindow::InProgress)
            m_ui.welcomeIcon->setMovie(m_movie);

        break;

    case MainWindow::ReleaseNotes:
        if (status == MainWindow::Done)
            m_ui.releaseIcon->setPixmap(KIcon("games-endturn").pixmap(18));
        else if (status == MainWindow::ToDo)
            m_ui.releaseIcon->setPixmap(QPixmap());
        else if (status == MainWindow::InProgress)
            m_ui.releaseIcon->setMovie(m_movie);

        break;

    case MainWindow::Language:
        if (status == MainWindow::Done)
            m_ui.localeIcon->setPixmap(KIcon("games-endturn").pixmap(18));
        else if (status == MainWindow::ToDo)
            m_ui.localeIcon->setPixmap(QPixmap());
        else if (status == MainWindow::InProgress)
            m_ui.localeIcon->setMovie(m_movie);
        break;

    case MainWindow::LicenseApproval:
        if (status == MainWindow::Done) {
            m_ui.licenseIcon->setPixmap(KIcon("games-endturn").pixmap(18));
            m_ui.preparationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-done.png"));
        } else if (status == MainWindow::ToDo) {
            m_ui.welcomeIcon->setPixmap(QPixmap(":/Images/images/installation-stage-done.png"));
            m_ui.licenseIcon->setPixmap(QPixmap());
            m_ui.preparationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
        } else if (status == MainWindow::InProgress) {
            m_ui.licenseIcon->setMovie(m_movie);
            m_ui.preparationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
        }

        break;

    case MainWindow::CreateUser:
        if (status == MainWindow::Done)
            m_ui.createuserIcon->setPixmap(KIcon("games-endturn").pixmap(18));
        else if (status == MainWindow::ToDo)
            m_ui.createuserIcon->setPixmap(QPixmap());
        else if (status == MainWindow::InProgress)
            m_ui.createuserIcon->setMovie(m_movie);

        break;

    case MainWindow::Partition:
        if (status == MainWindow::Done)
            m_ui.partitioningIcon->setPixmap(KIcon("games-endturn").pixmap(18));
        else if (status == MainWindow::ToDo)
            m_ui.partitioningIcon->setPixmap(QPixmap());
        else if (status == MainWindow::InProgress)
            m_ui.partitioningIcon->setMovie(m_movie);

        break;

    case MainWindow::ReadyToInstall:
        if (status == MainWindow::Done) {
            m_ui.installationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-done.png"));
            m_ui.readyInstallIcon->setPixmap(KIcon("games-endturn").pixmap(18));
        } else if (status == MainWindow::ToDo) {
            m_ui.installationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
            m_ui.readyInstallIcon->setPixmap(QPixmap());
        } else if (status == MainWindow::InProgress)
            m_ui.readyInstallIcon->setMovie(m_movie);

        break;

    case MainWindow::InstallSystem:
        if (status == MainWindow::Done) {
            m_ui.configurationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-done.png"));
            m_ui.installSystemIcon->setPixmap(KIcon("games-endturn").pixmap(18));
        } else if (status == MainWindow::ToDo) {
            m_ui.configurationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
            m_ui.installSystemIcon->setPixmap(QPixmap());
        } else if (status == MainWindow::InProgress)
            m_ui.installSystemIcon->setMovie(m_movie);

        break;

    case MainWindow::Configuration:
        if (status == MainWindow::Done) {
            m_ui.configurationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
        } else if (status == MainWindow::ToDo) {
            m_ui.configurationIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
        } else if (status == MainWindow::InProgress)
            m_ui.configurationIcon->setMovie(m_movie);

        break;

    case MainWindow::FinishStep:
        if (status == MainWindow::Done) {
            m_ui.doneIcon->setPixmap(QPixmap(":/Images/images/installation-stage-done.png"));
        } else if (status == MainWindow::ToDo) {
            m_ui.doneIcon->setPixmap(QPixmap(":/Images/images/installation-stage-notdone.png"));
        } else if (status == MainWindow::InProgress) {
            m_ui.doneIcon->setMovie(m_movie);
        }

    default:
        break;
    }
}

void MainWindow::goToNextStep()
{
    switch (m_currentAction) {
    case MainWindow::Welcome:
        m_currentAction = MainWindow::ReleaseNotes;
        setInstallationStep(MainWindow::Welcome, MainWindow::Done);
        break;

    case MainWindow::ReleaseNotes:
        m_currentAction = MainWindow::LicenseApproval;
        setInstallationStep(MainWindow::ReleaseNotes, MainWindow::Done);
        break;

    case MainWindow::LicenseApproval:
        m_currentAction = MainWindow::Language;
        setInstallationStep(MainWindow::LicenseApproval, MainWindow::Done);
        break;

    case MainWindow::Language:
        m_currentAction = MainWindow::CreateUser;
        setInstallationStep(MainWindow::Language, MainWindow::Done);
        break;
        
    case MainWindow::CreateUser:
        m_currentAction = MainWindow::Partition;
        setInstallationStep(MainWindow::CreateUser, MainWindow::Done);
        break;

    case MainWindow::Partition:
        m_currentAction = MainWindow::ReadyToInstall;
        setInstallationStep(MainWindow::Partition, MainWindow::Done);
        break;

    case MainWindow::ReadyToInstall:
        m_currentAction = MainWindow::InstallSystem;
        setInstallationStep(MainWindow::ReadyToInstall, MainWindow::Done);
        break;

    case MainWindow::InstallSystem:
        m_currentAction = MainWindow::Configuration;
        setInstallationStep(MainWindow::InstallSystem, MainWindow::Done);
        break;

    case MainWindow::Configuration:
        m_currentAction = MainWindow::FinishStep;
        setInstallationStep(MainWindow::Configuration, MainWindow::Done);
        break;

    case MainWindow::FinishStep:
        m_currentAction = MainWindow::FinishStep;
        setInstallationStep(MainWindow::FinishStep, MainWindow::Done);
        break;

    default:
        break;
    }

    loadPage(m_currentAction);
}

void MainWindow::goToPreviousStep()
{
    switch (m_currentAction) {
    case MainWindow::Welcome:
        return;
        break;

    case MainWindow::ReleaseNotes:
        m_currentAction = MainWindow::Welcome;
        setInstallationStep(MainWindow::ReleaseNotes, MainWindow::ToDo);
        break;

    case MainWindow::LicenseApproval:
        m_currentAction = MainWindow::ReleaseNotes;
        setInstallationStep(MainWindow::LicenseApproval, MainWindow::ToDo);
        break;

    case MainWindow::Language:
        m_currentAction = MainWindow::ReleaseNotes;
        setInstallationStep(MainWindow::LicenseApproval, MainWindow::ToDo);
        setInstallationStep(MainWindow::Language, MainWindow::ToDo);
        break;

    case MainWindow::CreateUser:
        m_currentAction = MainWindow::Language;
        setInstallationStep(MainWindow::CreateUser, MainWindow::ToDo);
        break;

    case MainWindow::Partition:
        m_currentAction = MainWindow::CreateUser;
        setInstallationStep(MainWindow::Partition, MainWindow::ToDo);
        break;

    case MainWindow::ReadyToInstall:
        m_currentAction = MainWindow::Partition;
        setInstallationStep(MainWindow::ReadyToInstall, MainWindow::Done);
        break;

    case MainWindow::InstallSystem:
        return;
        break;

    case MainWindow::Configuration:
        break;

    case MainWindow::FinishStep:
        break;

    default:
        break;
    }

    loadPage(m_currentAction);
}

void MainWindow::showProgressWidget()
{
    ProgressWidget *page = new ProgressWidget(this);

    connect(this, SIGNAL(updateProgressWidget(int)), page, SLOT(updateProgressWidget(int)));
    connect(this, SIGNAL(setProgressWidgetText(const QString&)), page, SLOT(setProgressWidgetText(const QString&)));
    
    m_ui.stackedWidget->addWidget(page);
    m_ui.stackedWidget->setCurrentIndex(1);

    m_ui.abortButton->setVisible(false);
    m_ui.nextButton->setVisible(false);
    m_ui.previousButton->setVisible(false);
}

void MainWindow::deleteProgressWidget()
{
    if (m_ui.stackedWidget->count() != 2)
        return;

    m_ui.abortButton->setVisible(true);
    m_ui.nextButton->setVisible(true);
    m_ui.previousButton->setVisible(true);

    while (m_ui.stackedWidget->count() != 1) {
        QWidget *w = m_ui.stackedWidget->widget(1);
        m_ui.stackedWidget->removeWidget(w);
        w->deleteLater();
    }
}

void MainWindow::enableNextButton(bool enable)
{
    m_ui.nextButton->setEnabled(enable);
}

void MainWindow::enablePreviousButton(bool enable)
{
    m_ui.previousButton->setEnabled(enable);
}

void MainWindow::quitAndReboot()
{
    setUpCleanupPage();

    m_install->cleanup();

    QProcess p;
    p.startDetached("shutdown -r now");

    qApp->exit(0);
}

void MainWindow::quitToChakra()
{
    setUpCleanupPage();
    m_install->cleanup();
    qApp->exit(0);
}

void MainWindow::errorOccurred(const QString &message)
{
    QString completeMessage = i18n("Installation failed! The following error has been reported: %1.\n"
                                   "Tribe will now quit, please check the installation logs in /tmp\n"
                                   "or ask for help in our forums.", message);

    KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
    dialog->setButtons(KDialog::Ok);
    dialog->setModal(true);
    bool retbool;

    KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, completeMessage,
                                   QStringList(), QString(), &retbool, KMessageBox::Notify);

    quitToChakra();
}

void MainWindow::setUpCleanupPage()
{
    for (int i = 0; i < m_ui.stackedWidget->count(); ++i) {
        QWidget *w = m_ui.stackedWidget->widget(i);
        m_ui.stackedWidget->removeWidget(w);
        w->deleteLater();
    }

    QVBoxLayout *lay = new QVBoxLayout;
    lay->addStretch();

    QLabel *logo = new QLabel();
    logo->setPixmap(KIcon("chakra-shiny").pixmap(128));
    lay->addWidget(logo);

    QLabel *l = new QLabel("Cleaning up...");
    lay->addWidget(l);

    lay->addStretch();

    QWidget *w = new QWidget;
    w->setLayout(lay);

    m_ui.stackedWidget->addWidget(w);

    m_ui.previousButton->setVisible(false);
    m_ui.nextButton->setVisible(false);
    m_ui.abortButton->setVisible(false);
}

#include "mainwindow.moc"
