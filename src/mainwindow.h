
/*
 * Copyright (c) 2008, 2009  Dario Freddi <drf@chakra-project.org>
 *               2008        Lukas Appelhans <l.appelhans@gmx.de>
 *               2010        Drake Justice <djustice@chakra-project.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef MAINWINDOW
#define MAINWINDOW

#include <QWidget>

#include <KMainWindow>

#include "ui_base.h"

class InstallationHandler;
class QMovie;


class MainWindow : public KMainWindow
{
    Q_OBJECT

public:
    enum InstallationStep {
        Welcome,
        ReleaseNotes,
        LicenseApproval,
        Language,
        CreateUser,
        Partition,
        ReadyToInstall,
        InstallSystem,
        Configuration,
        Bootloader,
        FinishStep
    };

    enum StepStatus {
        InProgress,
        Done,
        ToDo
    };

public:
    MainWindow(QWidget *parent = 0);
    virtual ~MainWindow();

protected:
    void closeEvent(QCloseEvent *evt);

private slots:
    void loadPage(InstallationStep page);
    void setInstallationStep(InstallationStep step, StepStatus status);
    void abortInstallation();
    void goToNextStep();
    void goToPreviousStep();
    void showProgressWidget();
    void deleteProgressWidget();

    void enableNextButton(bool enable);
    void enablePreviousButton(bool enable);

    void errorOccurred(const QString &error);

    void quitToChakra();
    void quitAndReboot();

    void setUpCleanupPage();

signals:
    void updateProgressWidget(int percentage);
    void setProgressWidgetText(const QString &);
    void readyToCreate();

private:
    Ui::Base m_ui;

    QWidget *m_centralWidget;

    InstallationHandler *m_install;
    
    InstallationStep m_currentAction;

    QMovie *m_movie;
};

#endif /*MAINWINDOW*/
