/*
 * Copyright (c) 2010  Drake Justice <djustice@chakra-project.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef AVATARDIALOG_H
#define AVATARDIALOG_H

#include <QWidget>

#include "ui_avatardialog.h"


class AvatarDialog : public QDialog
{
    Q_OBJECT
    
    public:
        AvatarDialog(QWidget*);
        ~AvatarDialog() {};

    public slots:
        void okClicked();
        void cancelClicked();
        
        void selectionChanged();
        
    signals:
        void setAvatar(QString);

    private:
        Ui::AvatarDialog ui;
        
        void populateList();
};

#endif // AVATARDIALOG_H
