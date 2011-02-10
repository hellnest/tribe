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

#ifndef INSTALLATIONHANDLER_H
#define INSTALLATIONHANDLER_H

#include <QObject>
#include <QTime>
#include <QProcess>
#include <QMap>
#include <QPointer>
#include <QThread>

#include <KLocalizedString>
#include <KIO/CopyJob>


class Partition;

class InstallationHandler : public QObject
{
    Q_OBJECT

    public:

        enum HomeAction {
            KeepHome,
            OverwriteAll,
            OverwriteKDEConfigs
        };

        enum CurrentAction {
            DiskPreparation,
            SystemInstallation,
            PostInstall
        };

        enum FileHandling {
            OverwriteFile,
            KeepFile
        };

        enum Status {
            InProgress,
            Success,
            Error
        };

        virtual ~InstallationHandler();

        static InstallationHandler *instance();

        void init();
        void abortInstallation();

        qint64 minSizeForTarget() const;
        void addPartitionToMountList(const Partition*, const QString&);

        QString getMountPointFor(const QString&);

        void clearMounts();

        void mountNextPartition();
        void unmountPartition(const QString&);
        QStringList getMountedPartitions();

        void setHomeBehaviour(HomeAction);
        void setFileHandlingBehaviour(FileHandling);

        QStringList checkExistingHomeDirs();

        void installSystem();
        void copyFiles();

        void installBootloader(int);

        void populateCommandParameters();

        QMap<QString, const Partition*> getMountPartitions() { return m_mount; }

        void resetIterators() { m_mapIterator = m_mount.constBegin();
                                m_stringlistIterator = m_removeLicenses.constBegin();
                              }

        void setUpUsers(QStringList);

        void setRemoveLicenses(const QStringList &packages) { m_removeLicenses = packages; }
        QStringList removeLicenses() { return m_removeLicenses; }

        void setHostname(const QString &name) { m_hostname = name; }
        QString hostname() { return m_hostname; }

        void setContinent(const QString &continent) { m_continent = continent; }
        QString continent() { return m_continent; }

        void setRegion(const QString &region) { m_region = region; }
        QString region() { return m_region; }

        void setTimezone(const QString &time) { m_timezone = time; }
        QString timezone() { return m_timezone; }

        void setLocale(const QString &loc) { m_locale = loc; }
        QString locale() { return m_locale; }

        void setKDELangPack(const QString &pack) { m_KDELangPack = pack; }
        QString KDELangPack() { return m_KDELangPack; }

        void setInstallDocumentation(bool doc) { m_doc = doc; }
        bool installDocumentation() { return m_doc; }

        void setUserLoginList(QStringList s) { m_userLoginList = s; }
        QStringList userLoginList() { return m_userLoginList; }

        void setUserPasswordList(QStringList s) { m_userPasswordList = s; }
        QStringList userPasswordList() { return m_userPasswordList; }

        void setUserAvatarList(QStringList s) { m_userAvatarList = s; }
        QStringList userAvatarList() { return m_userAvatarList; }

        void setUserNameList(QStringList s) { m_userNameList = s; }
        QStringList userNameList() { return m_userNameList; }

        void setUserAutoLoginList(QStringList s) { m_userAutoLoginList = s; }
        QStringList userAutoLoginList() { return m_userAutoLoginList; }

        void setConfigurePacman(bool pac) { m_configurePacman = pac; }
        bool configurePacman() { return m_configurePacman; }

        void setRootDevice(const QString& s) { m_rootDevice = s; }
        QString rootDevice() { return m_rootDevice; }

        QString m_postcommand;

    public slots:
        void cleanup();

        void postInstall();
        void postInstallDone(int, QProcess::ExitStatus);

        void parseUnsquashfsOutput();

        void jobDone(int);
        void reconnectJobSlot();

        void postRemove();
        void readyPost();

        void partitionMounted(KJob*);
        void unmountAll();
        void partitionsFormatted();

        void killProcesses();

        void streamPassword();
        void streamRootPassword();

    signals:
        void streamProgress(int);
        void streamLabel(const QString&);

        void installationDone();

        void errorInstalling(const QString &message = i18n("No Further details given"));

        void mounting(const QString&, InstallationHandler::Status);
        void mountingDone();

        void bootloaderInstalled(int, QProcess::ExitStatus);

    private:
        InstallationHandler(QObject *parent = 0);

        void installHome();
        void installKDEConfiguration();
        void handleProgress(CurrentAction, int);

        int antiFlicker();

        QString trimDevice(const Partition*);

        bool isMounted(const QString&);

        QTime eTime;

        QMap<QString, const Partition*> m_mount;
        QMap<QString, const Partition*>::const_iterator m_mapIterator;

        QStringList::const_iterator m_stringlistIterator;

        QString m_postjob;
        QString m_postlabel;

        QPointer<QProcess> m_process;
        QProcess *m_userProcess;
        QProcess *m_rootUserProcess;

        HomeAction homeAction;
        CurrentAction currAction;
        FileHandling fileAction;

        QStringList m_mtab;

        QStringList m_removeLicenses;

        QStringList m_userLoginList;
        QStringList m_userPasswordList;
        QStringList m_userAvatarList;
        QStringList m_userNameList;
        QStringList m_userAdminList;
        QStringList m_userAutoLoginList;

        QString m_hostname;
        QString m_continent;
        QString m_region;
        QString m_timezone;
        QString m_locale;
        QString m_KDELangPack;
        QString m_rootDevice;

        bool m_doc;
        bool m_configurePacman;

        int m_iterator;
        int m_unsquashFsCachedBottom;
        int m_passwdCount;

        qint64 m_minSize;

        friend class PMHandler;
};

#endif /*INSTALLATIONHANDLER_H*/
