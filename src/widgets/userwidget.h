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

#ifndef USERWIDGET_H
#define USERWIDGET_H

#include "avatardialog.h"
#include "ui_userwidget.h"


class UserWidget : public QWidget
{
    Q_OBJECT

public:
    UserWidget(int, QWidget *parent = 0);
    virtual ~UserWidget();

    QString login;
    QString password;
    QString rootPassword;
    QString avatar;
    QString name;
    bool autoLogin;
    bool useRootPw;
    bool useUserPw;
    bool passwordsMatch;
    bool rootPasswordsMatch;

    int number;
    void setNumber(int i) { number = i; }

public slots:
    void setLogin(const QString&);
    void setPassword(const QString&);
    void setRootPassword(const QString&);
    void setAvatar(const QString&);
    void setName(const QString&);
    void setAutoLogin(const QString&);
    void setUseRootPassword(const QString&);

signals:
    void addUserClicked();
    void removeUserClicked(int);
    void autoLoginToggled(int);

private slots:
    void showDetails();
    void showRootPw();

    void emitRemove();

    void avatarClicked();
    void autoLoginToggled();
    void useUserPwToggled();

    void testFields();

private:
    Ui::UserWidget ui;
    AvatarDialog *m_avatarDialog;
};

#endif /* USERWIDGET_H */
