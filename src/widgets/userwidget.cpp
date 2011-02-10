/*
 * Copyright (c) 2008, 2009  Dario Freddi <drf@chakra-project.org>
 *               2010, 2011  Drake Justice <djustice@chakra-project.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <QDebug>

#include <QDesktopWidget>

#include "avatardialog.h"
#include "userwidget.h"


UserWidget::UserWidget(int a_userNumber, QWidget* parent): QWidget(parent)
{
    number = a_userNumber;

    ui.setupUi(this);

    ui.extWidget->hide();
    ui.rootPwWidget->hide();

    ui.passLine->setEchoMode(QLineEdit::Password);
    ui.confirmPassLine->setEchoMode(QLineEdit::Password);
    ui.rootPassLine->setEchoMode(QLineEdit::Password);
    ui.confirmRootPassLine->setEchoMode(QLineEdit::Password);

    ui.removeUser->setIcon(KIcon("list-remove"));
    ui.userDetails->setIcon(KIcon("view-list-details"));

    ui.avatar->setIconSize(QSize(48, 48));
    ui.avatar->setIcon(KIcon("view-user-offline-kopete"));

    m_avatarDialog = new AvatarDialog(0);

    if (number == 0) {
        autoLogin = true;
        useRootPw = false;
        ui.autoLoginCheckBox->setChecked(true);
        ui.rootUsesUserPwCheckBox->setChecked(true);
        ui.removeUser->setVisible(false);
    } else {
        autoLogin = false;
        ui.rootUsesUserPwCheckBox->setVisible(false);
    }

    passwordsMatch = true;

    connect(ui.loginLine, SIGNAL(textChanged(QString)), this, SLOT(testFields()));
    connect(ui.passLine, SIGNAL(textChanged(QString)), this, SLOT(testFields()));
    connect(ui.confirmPassLine, SIGNAL(textChanged(QString)), this, SLOT(testFields()));

    connect(ui.userDetails, SIGNAL(clicked(bool)), this, SLOT(showDetails()));
    connect(ui.removeUser, SIGNAL(clicked(bool)), this, SLOT(emitRemove()));

    connect(ui.nameLine, SIGNAL(textChanged(QString)), this, SLOT(testFields()));

    connect(ui.avatar, SIGNAL(clicked(bool)), this, SLOT(avatarClicked()));
    connect(ui.autoLoginCheckBox, SIGNAL(toggled(bool)), this, SLOT(autoLoginToggled()));

    connect(ui.rootUsesUserPwCheckBox, SIGNAL(toggled(bool)), this, SLOT(showRootPw()));
    connect(ui.rootUsesUserPwCheckBox, SIGNAL(toggled(bool)), this, SLOT(useUserPwToggled()));

    connect(ui.rootPassLine, SIGNAL(textChanged(QString)), this, SLOT(testFields()));
    connect(ui.confirmRootPassLine, SIGNAL(textChanged(QString)), this, SLOT(testFields()));

    connect(m_avatarDialog, SIGNAL(setAvatar(QString)), this, SLOT(setAvatar(QString)));
}

UserWidget::~UserWidget()
{

}

void UserWidget::setAvatar(const QString& avatar_)
{
    if (avatar_ == "z") {

    } else {
        ui.avatar->setIcon(KIcon(avatar_));
        avatar = avatar_;
    }
}

void UserWidget::showRootPw()
{
    ui.rootPwWidget->setVisible(!ui.rootPwWidget->isVisible());
}

void UserWidget::showDetails()
{
    ui.extWidget->setVisible(!ui.extWidget->isVisible());
}

void UserWidget::emitRemove()
{
    emit removeUserClicked(number);
}

void UserWidget::testFields()
{
    if ((ui.passLine->text() == ui.confirmPassLine->text()) &&
        (!ui.passLine->text().isEmpty())) {
        ui.confirmPwCheck->setPixmap(QPixmap(":Images/images/green-check.png"));
        password = ui.passLine->text();
        passwordsMatch = true;
    } else {
        ui.confirmPwCheck->setPixmap(QPixmap());
        passwordsMatch = false;
    }

    if ((ui.rootPassLine->text() == ui.confirmRootPassLine->text()) &&
        (!ui.rootPassLine->text().isEmpty())) {
        ui.confirmRootPwCheck->setPixmap(QPixmap(":Images/images/green-check.png"));
        rootPassword = ui.rootPassLine->text();
        rootPasswordsMatch = true;
    } else {
        ui.confirmRootPwCheck->setPixmap(QPixmap());
        rootPasswordsMatch = false;
    }

    QRegExp r("\\D\\w{0,45}");
    QRegExp s("\\D\\w{0,45}[ ]");

    if (r.exactMatch(ui.loginLine->text())) {
        ui.loginLine->setText(ui.loginLine->text().toLower());
    } else if (s.exactMatch(ui.loginLine->text())) {
        ui.loginLine->setText(ui.loginLine->text().left(ui.loginLine->text().length() - 1));
    } else {
        ui.loginLine->setText("");
    }

    login = ui.loginLine->text();
    name = ui.nameLine->text();
    autoLogin = ui.autoLoginCheckBox->isChecked();
    if (!useRootPw) {
        rootPasswordsMatch = true;
    }
}

void UserWidget::avatarClicked()
{
    m_avatarDialog->show();
    m_avatarDialog->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, m_avatarDialog->size(), qApp->desktop()->availableGeometry()));
}

void UserWidget::autoLoginToggled()
{
    autoLogin = ui.autoLoginCheckBox->isChecked();
    if (autoLogin)
        emit autoLoginToggled(number);
}

void UserWidget::useUserPwToggled()
{
    useUserPw = ui.rootUsesUserPwCheckBox->isChecked();
    rootPasswordsMatch = ui.rootUsesUserPwCheckBox->isChecked();
    if (!useUserPw) {
        useRootPw = true;
    } else {
        useRootPw = false;
    }
}

void UserWidget::setLogin(const QString& login_)
{
    ui.loginLine->setText(login_);
    login = login_;
}

void UserWidget::setName(const QString& name_)
{
    ui.nameLine->setText(name_);
    name  = name_;
}

void UserWidget::setPassword(const QString& pass_)
{
    ui.passLine->setText(pass_);
    ui.confirmPassLine->setText(pass_);
    password = pass_;
}

void UserWidget::setRootPassword(const QString& pass_)
{
    ui.rootPassLine->setText(pass_);
    ui.confirmRootPassLine->setText(pass_);
    rootPassword = pass_;
}

void UserWidget::setUseRootPassword(const QString& useRootPw_)
{
    if (useRootPw_.toInt() > 0) {
        ui.rootUsesUserPwCheckBox->setCheckState(Qt::Checked);
    } else {
        ui.rootUsesUserPwCheckBox->setCheckState(Qt::Unchecked);
    }

    useUserPwToggled();
}

void UserWidget::setAutoLogin(const QString& autologin_) {
    if (autologin_.toInt() > 0) {
        ui.autoLoginCheckBox->setCheckState(Qt::Checked);
        autoLogin = true;
    } else {
        ui.autoLoginCheckBox->setCheckState(Qt::Unchecked);
        autoLogin = false;
    }
}

#include "userwidget.moc"
