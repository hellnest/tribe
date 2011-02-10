/***************************************************************************
 *   Copyright (C) 2010 by Dario Freddi                                    *
 *   drf@chakra-project.org                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/


#ifndef PMHANDLER_H
#define PMHANDLER_H

#include <QHash>
#include <QObject>

#include <tribepartitionmanager/core/operationrunner.h>
#include <tribepartitionmanager/core/operationstack.h>
#include <tribepartitionmanager/core/devicescanner.h>


class PMHandler : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PMHandler)

    public:
        virtual ~PMHandler();
        static PMHandler *instance();

        void reload();

        OperationRunner& operationRunner() { return m_OperationRunner; }
        const OperationRunner& operationRunner() const { return m_OperationRunner; }

        OperationStack& operationStack() { return m_OperationStack; }
        const OperationStack& operationStack() const { return m_OperationStack; }

        DeviceScanner& deviceScanner() { return m_DeviceScanner; }
        const DeviceScanner& deviceScanner() const { return m_DeviceScanner; }

        void preparePartitions(const Partition *p, Device *dev);
        void apply();

        void addSectorToMountList(Device* dev, qint64 firstSector, const QString& point, const QString& formatAs = QString());
        void clearMountList();
        QHash< QString, const Partition* > mountList(const QString&);

    public slots:
        void progressSub(int);
        void finished();
        void opStarted(int,Operation*);
        void error();
        void opFinished(int,Operation*);

    Q_SIGNALS:
        void devicesReady();
        void applyDone();

    private:
        PMHandler(QObject* parent = 0);
        const Partition *findNextPartition(Device *dev);

    private:
        OperationStack m_OperationStack;
        OperationRunner m_OperationRunner;
        DeviceScanner m_DeviceScanner;
        Report *report;
        QString m_currentStatus;
        int m_numJobs;
        int m_jobsDone;
        QHash< QString, QHash< qint64, QString > > m_mounts;
};

#endif // PMHANDLER_H
