/*
 * Copyright (c) 2008, 2009  Dario Freddi <drf@chakra-project.org>
 *               2009        Lukas Appelhans <l.appelhans@gmx.de>
 *               2010        Drake Justice <djustice.kde@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <QDir>
#include <QTimer>

#include <KIcon>
#include <KDebug>
#include <KMessageBox>
#include <KIO/Job>
#include <KIO/NetAccess>

#include <config-tribe.h>

#include "../installationhandler.h"
#include "usercreationpage.h"


UserCreationPage::UserCreationPage(QWidget *parent)
        : AbstractPage(parent),
        m_handler(InstallationHandler::instance())
{
}

UserCreationPage::~UserCreationPage()
{
}

void UserCreationPage::createWidget()
{
    ui.setupUi(this);
    ui.addUser->setIcon(KIcon("list-add"));

    ui.hostname->setText("chakra-pc");

    connect(ui.addUser, SIGNAL(clicked(bool)), this, SLOT(addUserClicked()));

    ui.scrollArea->setWidgetResizable(true);

    UserWidget *f = new UserWidget(0, this);
    m_userList.append(f);
    ui.verticalLayout->insertWidget(0, f);
    connect(f, SIGNAL(removeUserClicked(int)), this, SLOT(removeUserClicked(int)));
    connect(f, SIGNAL(autoLoginToggled(int)), this, SLOT(updateAutoLogin(int)));

    if (!m_handler->userLoginList().isEmpty()) {
        foreach (QString login, m_handler->userLoginList()) {
            int i = m_userList.count() - 1;
            QString password = m_handler->userPasswordList().at(i);
            QString rootpassword = m_handler->userPasswordList().last();
            QString autologin = m_handler->userAutoLoginList().at(i);
            QString name = m_handler->userNameList().at(i);
            QString avatar = m_handler->userAvatarList().at(i);

            QString userootpw = "0";
            if (password == rootpassword)
                userootpw = "1";

            m_userList.at(i)->setLogin(login);
            m_userList.at(i)->setPassword(password);
            m_userList.at(i)->setUseRootPassword(userootpw);
            m_userList.at(i)->setRootPassword(rootpassword);
            m_userList.at(i)->setAutoLogin(autologin);
            m_userList.at(i)->setName(name);
            m_userList.at(i)->setAvatar(avatar);

            if (m_handler->userLoginList().count() > m_userList.count())
                addUserClicked();
        }

        ui.hostname->setText(m_handler->hostname());
    }
}

void UserCreationPage::addUserClicked()
{
    UserWidget *f = new UserWidget(m_userList.count(), this);
    m_userList.append(f);
    ui.verticalLayout->insertWidget(ui.verticalLayout->count() - 1, f);
    connect(f, SIGNAL(removeUserClicked(int)), this, SLOT(removeUserClicked(int)));
    connect(f, SIGNAL(autoLoginToggled(int)), this, SLOT(updateAutoLogin(int)));

    QTimer::singleShot(300, this, SLOT(updateScrollView()));
}

void UserCreationPage::removeUserClicked(int number)
{
    m_userList.at(number)->deleteLater();
    m_userList.removeAt(number);
    updateUserNumbers();
}

void UserCreationPage::updateAutoLogin(int i)
{
    if (m_userList.at(i)->autoLogin) {
        foreach (UserWidget *user, m_userList) {
            if (user->number != i) {
                user->setAutoLogin("0");
            }
        }
    }
}

void UserCreationPage::updateUserNumbers()
{
    int n = 0;
    foreach (UserWidget *user, m_userList) {
        user->setNumber(n);
        n++;
    }
}

void UserCreationPage::updateScrollView()
{
    ui.scrollArea->ensureWidgetVisible(ui.addUser);
}

bool UserCreationPage::validate()
{
    QStringList loginList;
    QStringList passwordList;
    QStringList nameList;
    QStringList avatarList;
    QStringList autoLoginList;

    QString rootPw;

    int n = 0;
    foreach(UserWidget* user, m_userList) {
        n++;

        KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
        dialog->setButtons(KDialog::Ok);
        dialog->setModal(true);
        bool retbool;

        if (n == 1) {
            if (user->login.isEmpty()) {
                KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, i18n("You must give at least one login name."),
                                               QStringList(), QString(), &retbool, KMessageBox::Notify);
                return false;
            } else if (!user->rootPasswordsMatch) {
                KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, i18n("Root Passwords do not match..."),
                                               QStringList(), QString(), &retbool, KMessageBox::Notify);
                return false;
            }

            if (!user->rootPassword.isEmpty() && user->rootPasswordsMatch &&
                user->useRootPw) {
                rootPw = user->rootPassword;
            } else if (!user->useRootPw) {
                rootPw = user->password;
            }
        }

        if ((user->password.isEmpty()) && (user->passwordsMatch == true)) {
            KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, i18n("Passwords cannot be empty."),
                                           QStringList(), QString(), &retbool, KMessageBox::Notify);
            return false;
        } else if (!user->passwordsMatch) {
            KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, i18n("Passwords do not match..."),
                                           QStringList(), QString(), &retbool, KMessageBox::Notify);
            return false;
        } else if (user->login.isEmpty()) {
            KMessageBox::createKMessageBox(dialog, QMessageBox::Warning, i18n("Login names cannot be empty."),
                                           QStringList(), QString(), &retbool, KMessageBox::Notify);
            return false;
        }

        loginList.append(user->login);
        passwordList.append(user->password);
        nameList.append(user->name);
        avatarList.append(user->avatar);
        autoLoginList.append(QString::number(user->autoLogin));
    }

    passwordList.append(rootPw);

    m_userList.clear();

    m_handler->setUserLoginList(loginList);
    m_handler->setUserPasswordList(passwordList);
    m_handler->setUserNameList(nameList);
    m_handler->setUserAvatarList(avatarList);
    m_handler->setUserAutoLoginList(autoLoginList);

    if (ui.hostname->text().isEmpty())
        ui.hostname->setText("chakra-pc");

    m_handler->setHostname(ui.hostname->text());

    return true;
}

void UserCreationPage::aboutToGoToNext()
{
    if (validate())
        emit goToNextStep();
}

void UserCreationPage::aboutToGoToPrevious()
{
    if (validate())
        emit goToPreviousStep();
}

void UserCreationPage::validateNext()
{
    bool enable = true;

    emit enableNextButton(enable);
}

#include "usercreationpage.moc"
