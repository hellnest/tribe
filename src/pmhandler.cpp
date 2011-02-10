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

#include <QDebug>

#include <QEventLoop>

#include <KGlobal>

#include <tribepartitionmanager/fs/filesystemfactory.h>
#include <tribepartitionmanager/core/partition.h>
#include <tribepartitionmanager/ops/newoperation.h>
#include <tribepartitionmanager/core/device.h>
#include <tribepartitionmanager/util/capacity.h>
#include <tribepartitionmanager/util/helpers.h>
#include <tribepartitionmanager/util/report.h>
#include <tribepartitionmanager/ops/createfilesystemoperation.h>

#include "installationhandler.h"
#include "pmhandler.h"

class PMHandlerHelper
{
public:
    PMHandlerHelper() : q(0) {}
    ~PMHandlerHelper() {
        delete q;
    }
    PMHandler *q;
};

K_GLOBAL_STATIC(PMHandlerHelper, s_globalPMHandler)

PMHandler *PMHandler::instance()
{
    if (!s_globalPMHandler->q) {
        new PMHandler;
    }

    return s_globalPMHandler->q;
}

PMHandler::PMHandler(QObject *parent)
        : QObject(parent)
        , m_OperationStack()
        , m_OperationRunner(this, operationStack())
        , m_DeviceScanner(this, operationStack())
{
    Q_ASSERT(!s_globalPMHandler->q);
    s_globalPMHandler->q = this;

    // Load stuff from PM
    registerMetaTypes();
    loadBackend();

    FileSystemFactory::init();
    report = new Report(0);
    connect(&deviceScanner(), SIGNAL(finished()), SIGNAL(devicesReady()));
}

PMHandler::~PMHandler()
{

}

void PMHandler::reload()
{
    deviceScanner().start();
}

void PMHandler::clearMountList()
{
    m_mounts.clear();
}

QHash< QString, const Partition* > PMHandler::mountList(const QString& rootDevNode)
{
    QHash< QString, const Partition* > rethash;
    // Iterate over devices and stuff and try associating each mountpoint to a partition.
    QReadLocker lockDevices(&operationStack().lock());
    foreach(Device* dev, PMHandler::instance()->operationStack().previewDevices()) {
        if (dev->partitionTable() != NULL) {
            foreach(const Partition* p, dev->partitionTable()->children()) {
                if (p->children().isEmpty()) {
                    if ((p->roles().roles() & PartitionRole::Primary || p->roles().roles() & PartitionRole::Logical) &&
                        p->fileSystem().type() != FileSystem::Unformatted && p->fileSystem().type() != FileSystem::Unknown) {
                        // Ok, check the sector. We have a range of error of 10 sectors
                        if (p->fileSystem().type() == FileSystem::LinuxSwap) {
                            if (dev->deviceNode() != rootDevNode)
                                continue;

                            rethash["swap"] = p;
                            continue;
                        }
                        QHash<qint64, QString>::const_iterator i;
                        for (i = m_mounts[dev->deviceNode()].constBegin(); i != m_mounts[dev->deviceNode()].constEnd(); ++i) {
                            if (p->firstSector() >= (i.key() - 10) && p->firstSector() <= (i.key() + 10)) {
                                rethash[i.value()] = p;
                            }
                        }
                    }
                } else {
                    foreach(const Partition* child, p->children()) {
                        if ((child->roles().roles() & PartitionRole::Primary ||
                             child->roles().roles() & PartitionRole::Logical) &&
                             child->fileSystem().type() != FileSystem::Unformatted &&
                             child->fileSystem().type() != FileSystem::Unknown) {
                            // Ok, check the sector. We have a range of error of 10 sectors
                            if (child->fileSystem().type() == FileSystem::LinuxSwap) {
                                // Hoh, use as swap then.
                                rethash["swap"] = child;
                                continue;
                            }
                            QHash<qint64, QString>::const_iterator i;
                            for (i = m_mounts[dev->deviceNode()].constBegin(); i != m_mounts[dev->deviceNode()].constEnd(); ++i) {
                                if (child->firstSector() >= (i.key() - 10) && child->firstSector() <= (i.key() + 10)) {
                                    // Match!
                                    rethash[i.value()] = child;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return rethash;
}

void PMHandler::preparePartitions(const Partition* p, Device *dev)
{
    // We already know p is an unallocated partition.
    // Can we actually create an extended partition?
    if (dev->partitionTable()->childRoles(*p) & PartitionRole::Extended) {
        // It's ok
    } else {
        // It's not ok. We'll fall back to creating a primary or logical partition only for /
        Partition* newPartition = NewOperation::createNew(*p);
        newPartition->deleteFileSystem();
        // Attempt logical first
        if (dev->partitionTable()->childRoles(*p) & PartitionRole::Logical) {
            newPartition->setRoles(PartitionRole(PartitionRole::Logical));
        } else {
            // Go for primary
            newPartition->setRoles(PartitionRole(PartitionRole::Primary));
        }
        newPartition->setFileSystem(FileSystemFactory::create(FileSystem::Ext4,
                                    newPartition->firstSector(), newPartition->lastSector()));
        operationStack().push(new NewOperation(*dev, newPartition));
        m_mounts[dev->deviceNode()].insert(newPartition->firstSector(), "/");

        // return now
        return;
    }

    // Let's go for an extended approach

    Partition* newPartition = NewOperation::createNew(*p);
    newPartition->deleteFileSystem();
    newPartition->setRoles(PartitionRole(PartitionRole::Extended));
    newPartition->setFileSystem(FileSystemFactory::create(FileSystem::Extended,
                                newPartition->firstSector(), newPartition->lastSector()));
    operationStack().push(new NewOperation(*dev, newPartition));
    qDebug() << "Operation pushed";

    // Done. Now let's go for the single partitions
    // Before that, do we actually need to create a swap partition?
    bool needsSwap = true;
    QReadLocker lockDevices(&PMHandler::instance()->operationStack().lock());
    foreach(Device* dev, PMHandler::instance()->operationStack().previewDevices()) {
        if (dev->partitionTable() != NULL) {
            foreach(const Partition* p, dev->partitionTable()->children()) {
                foreach(const Partition* child, p->children()) {
                    if (child->fileSystem().type() == FileSystem::LinuxSwap) {
                        needsSwap = false;
                    }
                }

                if (p->fileSystem().type() == FileSystem::LinuxSwap) {
                    needsSwap = false;
                }
            }
        }
    }

    const Partition *extendedBody = findNextPartition(dev);

    // Find out the total size first
    qint64 gbcapacity = Capacity(*extendedBody).toInt(Capacity::GiB);
    qint64 sectorLastSwap = -1;
    qint64 sectorLastRoot = -1;
    qint64 sectorLastVar = -1;
    qint64 sectorLastHome = -1;
    if (gbcapacity < 9) {
        // Create just /, and 512 MB of swap if needed.
        // Root Partition
        if (needsSwap) {
            sectorLastSwap = extendedBody->lastSector();
            sectorLastRoot = ((extendedBody->lastSector() - extendedBody->firstSector()) / (gbcapacity*2)) * ((gbcapacity*2) -1);
        } else {
            sectorLastRoot = extendedBody->lastSector();
        }
    } else if (gbcapacity < 26) {
        // Create / and /var
        if (needsSwap) {
            // Give swap 1GB. TODO: Find out something better
            sectorLastRoot = ((extendedBody->lastSector() - extendedBody->firstSector()) / gbcapacity) * (gbcapacity-3);
            sectorLastSwap = ((extendedBody->lastSector() - extendedBody->firstSector()) / gbcapacity) + sectorLastRoot;
        } else {
            sectorLastRoot = ((extendedBody->lastSector() - extendedBody->firstSector()) / gbcapacity) * (gbcapacity-2);
        }
        sectorLastVar = extendedBody->lastSector();
    } else {
        // Create a separate /, /home, and /var
        qint64 validSectors = (extendedBody->lastSector() - extendedBody->firstSector());
        sectorLastRoot = (validSectors / gbcapacity) * 12; // 12 GB -> /
        if (needsSwap) {
            // Give swap 2GB. TODO: Give swap based on available RAM
            sectorLastSwap = ((validSectors / gbcapacity) * 2) + sectorLastRoot;
            sectorLastVar = ((validSectors / gbcapacity) * 4) + sectorLastSwap; // 4 GB-> /var;
        } else {
            sectorLastVar = ((validSectors / gbcapacity) * 4) + sectorLastRoot; // 4 GB-> /var;
        }
        sectorLastHome = extendedBody->lastSector();
    }

    // Create partitions
    // Root Partition
    if (sectorLastRoot > 0) {
        Partition* rootPartition = NewOperation::createNew(*extendedBody);
        rootPartition->setRoles(PartitionRole(PartitionRole::Logical));
        rootPartition->deleteFileSystem();
        if (sectorLastRoot != extendedBody->lastSector()) {
            rootPartition->setLastSector(sectorLastRoot);
        }
        rootPartition->setFileSystem(FileSystemFactory::create(FileSystem::Ext4,
                                                               rootPartition->firstSector(),
                                                               rootPartition->lastSector(),
                                                               -1, "Chakra"));
        operationStack().push(new NewOperation(*dev, rootPartition));
        m_mounts[dev->deviceNode()].insert(rootPartition->firstSector(), "/");
        qDebug() << "Will create root partition at " << rootPartition->firstSector();
    }

    // Swap partition
    if (sectorLastSwap > 0) {
        const Partition *previewedParent = findNextPartition(dev);
        Partition* swapPartition = NewOperation::createNew(*previewedParent);
        swapPartition->setRoles(PartitionRole(PartitionRole::Logical));
        swapPartition->deleteFileSystem();
        if (sectorLastSwap != extendedBody->lastSector()) {
            swapPartition->setLastSector(sectorLastSwap);
        }
        swapPartition->setFileSystem(FileSystemFactory::create(FileSystem::LinuxSwap, swapPartition->firstSector(),
                                                                swapPartition->lastSector()));
        operationStack().push(new NewOperation(*dev, swapPartition));

        qDebug() << "Will create swap partition at " << swapPartition->firstSector();
    }

    // Var partition
    if (sectorLastVar > 0) {
        const Partition *previewedParent = findNextPartition(dev);
        Partition* varPartition = NewOperation::createNew(*previewedParent);
        varPartition->setRoles(PartitionRole(PartitionRole::Logical));
        varPartition->deleteFileSystem();
        if (sectorLastVar != extendedBody->lastSector()) {
            varPartition->setLastSector(sectorLastVar);
        }
        varPartition->setFileSystem(FileSystemFactory::create(FileSystem::ReiserFS,
                                                              varPartition->firstSector(),
                                                              varPartition->lastSector()));
        operationStack().push(new NewOperation(*dev, varPartition));
        m_mounts[dev->deviceNode()].insert(varPartition->firstSector(), "/var");
        qDebug() << "Will create var partition at " << varPartition->firstSector();
    }

    // Home partition
    if (sectorLastHome > 0) {
        const Partition *previewedParent = findNextPartition(dev);
        Partition* homePartition = NewOperation::createNew(*previewedParent);
        homePartition->setRoles(PartitionRole(PartitionRole::Logical));
        homePartition->deleteFileSystem();
        if (sectorLastHome != extendedBody->lastSector()) {
            homePartition->setLastSector(sectorLastHome);
        }
        homePartition->setFileSystem(FileSystemFactory::create(FileSystem::Ext4,
                                                               homePartition->firstSector(),
                                                               homePartition->lastSector()));
        operationStack().push(new NewOperation(*dev, homePartition));
        m_mounts[dev->deviceNode()].insert(homePartition->firstSector(), "/home");
        qDebug() << "Will create home partition at " << homePartition->firstSector();
    }
}

const Partition *PMHandler::findNextPartition(Device* dev)
{
    QReadLocker lockDevices(&operationStack().lock());
    foreach(Device* device, PMHandler::instance()->operationStack().previewDevices()) {
        if (dev->deviceNode() == device->deviceNode()) {
            foreach(const Partition* p, dev->partitionTable()->children()) {
                foreach(const Partition* child, p->children()) {
                    qDebug() << "Analyzing" << child->fileSystem().name() << child->firstSector() << child->lastSector();
                    if (child->fileSystem().type() == FileSystem::Unknown) {
                        qDebug() << "Partition found" << child->firstSector() << child->lastSector();
                        return child;
                    }
                }
            }
        }
    }

    qDebug() << "Bad times ahead, no partition found";
    return 0;
}

void PMHandler::apply()
{
    // If there is nothing to do, straight to finished
    if (operationStack().size() <= 0) {
        finished();
        return;
    }

    // Set the report to avoid bad crashes
    operationRunner().setReport(report);

    // Undo all operations so the runner has a defined starting point
    for (int i = operationStack().operations().size() - 1; i >= 0; i--) {
        operationStack().operations()[i]->undo();
        operationStack().operations()[i]->setStatus(Operation::StatusNone);
    }

    connect(&operationRunner(), SIGNAL(progressSub(int)), this, SLOT(progressSub(int)));
    connect(&operationRunner(), SIGNAL(finished()), this, SLOT(finished()));
    connect(&operationRunner(), SIGNAL(error()), this, SLOT(error()));
    connect(&operationRunner(), SIGNAL(opStarted(int,Operation*)), this, SLOT(opStarted(int,Operation*)));
    connect(&operationRunner(), SIGNAL(opFinished(int,Operation*)), this, SLOT(opFinished(int,Operation*)));

    m_numJobs = operationRunner().numOperations();
    m_jobsDone = 0;

    InstallationHandler::instance()->streamLabel(i18n("Preparing hard drive for installation..."));

    operationRunner().start();
}

void PMHandler::addSectorToMountList(Device *dev, qint64 firstSector, const QString& point, const QString& formatAs)
{
    if (!formatAs.isEmpty()) {
        FileSystem::Type ftype = FileSystem::typeForName(formatAs);
        Partition *p = dev->partitionTable()->findPartitionBySector(firstSector, PartitionRole(PartitionRole::Any));
        p->deleteFileSystem();
        p->setFileSystem(FileSystemFactory::create(FileSystem::typeForName(formatAs), p->firstSector(), p->lastSector()));
        operationStack().push(new CreateFileSystemOperation(*dev, *p, ftype));
    }
    m_mounts[dev->deviceNode()].insert(firstSector, point);
}

void PMHandler::finished()
{
    // Mount partitions
    // Ok, here we gotta associate the right device node to the right partition.
    InstallationHandler::instance()->streamLabel(i18n("Analyzing disk..."));
    QEventLoop e;
    connect(this, SIGNAL(devicesReady()), &e, SLOT(quit()));
    reload();
    e.exec();
    QReadLocker lockDevices(&operationStack().lock());
    foreach(Device* dev, PMHandler::instance()->operationStack().previewDevices()) {
        if (dev->partitionTable() != NULL) {
            foreach(const Partition* p, dev->partitionTable()->children()) {
                if (p->children().isEmpty()) {
                    if ((p->roles().roles() & PartitionRole::Primary || p->roles().roles() & PartitionRole::Logical) &&
                        p->fileSystem().type() != FileSystem::Unformatted && p->fileSystem().type() != FileSystem::Unknown) {
                        // Ok, check the sector. We have a range of error of 10 sectors
                        qDebug() << "Found eligible primary partition at " << p->firstSector();
                        if (p->fileSystem().type() == FileSystem::LinuxSwap) {
                            // Hoh, use as swap then.
                            InstallationHandler::instance()->addPartitionToMountList(p, "swap");
                            qDebug() << p->devicePath() << p->number() << "will be used as swap";
                            continue;
                        }
                        QHash<qint64, QString>::const_iterator i;
                        for (i = m_mounts[dev->deviceNode()].constBegin(); i != m_mounts[dev->deviceNode()].constEnd(); ++i) {
                            if (p->firstSector() >= (i.key() - 10) && p->firstSector() <= (i.key() + 10)) {
                                // Match!
                                qDebug() << p->devicePath() << p->number() << "was set to be mounted";
                                InstallationHandler::instance()->addPartitionToMountList(p, i.value());
                            }
                        }
                    }
                } else {
                    foreach(const Partition* child, p->children()) {
                        if ((child->roles().roles() & PartitionRole::Primary ||
                             child->roles().roles() & PartitionRole::Logical) &&
                            child->fileSystem().type() != FileSystem::Unformatted &&
                            child->fileSystem().type() != FileSystem::Unknown) {
                            // Ok, check the sector. We have a range of error of 10 sectors
                            qDebug() << "Found eligible logical partition at " << child->firstSector();
                            if (child->fileSystem().type() == FileSystem::LinuxSwap) {
                                // Hoh, use as swap then.
                                InstallationHandler::instance()->addPartitionToMountList(child, "swap");
                                qDebug() << child->devicePath() << child->number() << "will be used as swap";
                                continue;
                            }
                            QHash<qint64, QString>::const_iterator i;
                            for (i = m_mounts[dev->deviceNode()].constBegin(); i != m_mounts[dev->deviceNode()].constEnd(); ++i) {
                                if (child->firstSector() >= (i.key() - 10) && child->firstSector() <= (i.key() + 10)) {
                                    // Match!
                                    qDebug() << child->devicePath() << child->number() << "was set to be mounted";
                                    InstallationHandler::instance()->addPartitionToMountList(child, i.value());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Ok, now that we have populated the mount list, let's notify we're done
    InstallationHandler::instance()->partitionsFormatted();
}

void PMHandler::error()
{
    emit InstallationHandler::instance()->errorInstalling(i18n("There was an error while formatting partitions"));
}

void PMHandler::opStarted(int , Operation* op)
{
    m_currentStatus = op->description();
    InstallationHandler::instance()->streamLabel(m_currentStatus);
}

void PMHandler::opFinished(int , Operation*)
{
    ++m_jobsDone;
    int realprog = m_jobsDone * 100;
    realprog /= m_numJobs;
    InstallationHandler::instance()->handleProgress(InstallationHandler::DiskPreparation, realprog);
}

void PMHandler::progressSub(int progress)
{
    int realprog = m_jobsDone * 100;
    realprog += progress;
    realprog /= m_numJobs;
    InstallationHandler::instance()->handleProgress(InstallationHandler::DiskPreparation, realprog);
}

