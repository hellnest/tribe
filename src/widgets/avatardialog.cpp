/*
 * Copyright (c) 2010  Drake Justice <djustice@chakra-project.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <QDebug>

#include <QDir>

#include <KFileDialog>
#include <KLocalizedString>

#include "avatardialog.h"


AvatarDialog::AvatarDialog(QWidget* parent)
{
    setParent(parent);

    ui.setupUi(this);
    
    setWindowFlags(Qt::FramelessWindowHint);
    
    connect(ui.okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(ui.avatarList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(selectionChanged()));

    ui.okButton->setIcon(KIcon("dialog-ok"));
    ui.cancelButton->setIcon(KIcon("dialog-cancel"));
    
    populateList();
}

void AvatarDialog::populateList()
{
    QDir d("/usr/share/tribe/avatars");
    foreach (QString file, d.entryList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        QListWidgetItem *item = new QListWidgetItem(ui.avatarList);
        item->setIcon(KIcon(d.path() + "/" + file));
        item->setData(58, d.path() + "/" + file);
        item->setText("");
        ui.avatarList->addItem(item);
    }

    QListWidgetItem *itemC = new QListWidgetItem(ui.avatarList);
    itemC->setIcon(KIcon("view-user-offline-kopete"));
    itemC->setText(i18n("Custom"));
    ui.avatarList->addItem(itemC);
}

void AvatarDialog::okClicked()
{
    emit setAvatar(ui.avatarList->currentItem()->data(58).toString());
    hide();
}

void AvatarDialog::cancelClicked()
{
    emit setAvatar("z");
    hide();
}

void AvatarDialog::selectionChanged()
{
    ui.okButton->setEnabled(true);
    if (ui.avatarList->currentItem()->text() == i18n("Custom")) {
        QString x = KFileDialog::getOpenFileName(KUrl(QDir::homePath()), "*.png *.jpg *.bmp|Image Files", this);

        if (x.isEmpty()) {
            emit setAvatar(ui.avatarList->currentItem()->data(58).toString());
            hide();
            return;
        }
        
        ui.avatarList->currentItem()->setIcon(KIcon(x));
        ui.avatarList->currentItem()->setData(58, x);
        emit setAvatar(x);
        hide();
    }
}

#include "avatardialog.moc"