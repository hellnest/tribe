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


#include <QDebug>

#include <QTreeWidget>
#include <QPainter>
#include <QtAlgorithms>
#include <QTimeLine>

#include <KLineEdit>
#include <KMessageBox>
#include <kio/global.h>

#include <tribepartitionmanager/core/device.h>
#include <tribepartitionmanager/core/partition.h>
#include <tribepartitionmanager/core/partitiontable.h>
#include <tribepartitionmanager/util/capacity.h>
#include <tribepartitionmanager/util/report.h>
#include <tribepartitionmanager/ops/deleteoperation.h>
#include <tribepartitionmanager/ops/createpartitiontableoperation.h>
#include <tribepartitionmanager/ops/newoperation.h>
#include <tribepartitionmanager/ops/resizeoperation.h>
#include <tribepartitionmanager/fs/filesystem.h>
#include <tribepartitionmanager/fs/filesystemfactory.h>

#include "../pmhandler.h"
#include "../installationhandler.h"
#include "partitionpage.h"
#include "ui_partition.h"


Q_DECLARE_METATYPE(const Partition*)
Q_DECLARE_METATYPE(Device*)

const int SPACING = 3;
const int MOUNTPOINT_ROLE = Qt::UserRole + 123;
const int PARTITION_ROLE = Qt::UserRole + 51;
const int DEVICE_ROLE = Qt::UserRole + 50;

QStringList s_mountPoints = QStringList() << "None" <<
                                             "/" <<
                                             "/usr" <<
                                             "/home" <<
                                             "/var" <<
                                             "/tmp" <<
                                             "/opt" <<
                                             "/etc" <<
                                             "/boot" <<
                                             "swap" <<
                                             "Other...";

QHash<const Partition*, QString> s_partitionToMountPoint;


bool caseInsensitiveLessThan(const QString& s1, const QString& s2)
{
    return s1.toLower() < s2.toLower();
}

///////////// internals for the items in the partition tree

PartitionDelegate::PartitionDelegate(QObject * parent) : QStyledItemDelegate(parent),
                                                         m_lockIcon("object-locked"),
                                                         m_partIcon("partitionmanager")
{ }

PartitionDelegate::~PartitionDelegate()
{ }

void PartitionDelegate::slotIndexChanged(const QString &text)
{
    // what to do what a partition mountpoint was chosen
    if (text == "Other...") {
        // custom mountpoint dialog

        KDialog *dialog = new KDialog();
        QWidget *dialogWidget = new QWidget;
        KLineEdit *dialogLineEdit = new KLineEdit();
        QLabel *dialogLabel = new QLabel(i18n("Please enter a custom mountpoint"));
        QVBoxLayout *dialogLayout = new QVBoxLayout;

        dialogLayout->addWidget(dialogLabel);
        dialogLayout->addWidget(dialogLineEdit);
        dialogWidget->setLayout(dialogLayout);
        dialog->setMainWidget(dialogWidget);
        dialog->setButtons(KDialog::Ok | KDialog::Cancel);

        if (dialog->exec() == KDialog::Accepted) {
            if (!dialogLineEdit->text().startsWith('/')) {
                KMessageBox::information(0,
                                         i18n("The mount point must start with /"),
                                         i18n("Error"));

                QComboBox *box = qobject_cast< QComboBox* >(sender());
                box->setCurrentIndex(box->findText("None"));

                return;
            }

            s_mountPoints.insert(s_mountPoints.length() - 1, dialogLineEdit->text());

            QComboBox *box = qobject_cast< QComboBox* >(sender());
            box->setCurrentIndex(-1);
            box->insertItem(box->count() - 1, dialogLineEdit->text());
            box->setCurrentIndex(box->findText(dialogLineEdit->text()));

            commitData();
        }

        dialog->deleteLater();
    } else {
        // set duplicously chosen mountpoints to 'none'

        const Partition * part = sender()->property("_partition_").value<const Partition*>();
        if (s_partitionToMountPoint.values().contains(text) && text != "None") {

            QComboBox *box = qobject_cast< QComboBox* >(sender());
            box->setCurrentIndex(box->findText("None"));

            return;
        }

        s_partitionToMountPoint[part] = text;
    }
}

void PartitionDelegate::commitData()
{
    QStyledItemDelegate::commitData(qobject_cast<QWidget*>(QObject::sender()));
}

void PartitionDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox *partitionCombo = qobject_cast<QComboBox*>(editor);
    QString mountPoint = index.data(MOUNTPOINT_ROLE).toString();

    if (mountPoint.isEmpty()) {
        if (s_partitionToMountPoint.contains(index.data(PARTITION_ROLE).value<const Partition*>())) {
            mountPoint = s_partitionToMountPoint[index.data(PARTITION_ROLE).value<const Partition*>()];
            partitionCombo->setCurrentIndex(partitionCombo->findText(mountPoint));
        } else {
            partitionCombo->setCurrentIndex(0);
        }
    } else {
        partitionCombo->setCurrentIndex(partitionCombo->findText(mountPoint));
    }
}

void PartitionDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    model->setData(index, qobject_cast<QComboBox*>(editor)->currentText(), MOUNTPOINT_ROLE);
}

QWidget* PartitionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option)

    QComboBox *combo = new QComboBox(parent);
    combo->addItems(s_mountPoints);
    combo->setProperty("_partition_", index.data(PARTITION_ROLE));

    connect(combo, SIGNAL(currentIndexChanged(int)), SLOT(commitData()));
    connect(combo, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotIndexChanged(QString)));

    return combo;
}

void PartitionDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &idx) const
{
    QStyleOptionViewItemV4 optV4 = option;
    initStyleOption(&optV4, idx);

    painter->save();
    painter->setClipRect(optV4.rect);

    QStyle *style = QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter);

    QRect iconRect = optV4.rect;
    iconRect.setSize(QSize(optV4.rect.height() - SPACING * 2 - 10, optV4.rect.height() - SPACING * 2 - 10));
    iconRect.moveTo(QPoint(iconRect.x() + SPACING + 3, iconRect.y() + SPACING + 4));

    painter->setPen(Qt::NoPen);

    if (idx.data(58) != "p") {
        if (optV4.text.left(3) == i18n("New") || optV4.text.left(11) == i18n("Unsupported")) {
            QRect overlayRect = QRect(QPoint(optV4.rect.left() + 2, optV4.rect.top() + 2),
                                    QPoint(optV4.rect.right() - 2, optV4.rect.bottom() - 2));
            painter->setBrush(QBrush(QColor(Qt::black)));
            painter->drawRect(overlayRect);
            painter->drawPixmap(iconRect, optV4.icon.pixmap(iconRect.size()));
        } else {
            QRect overlayRect = QRect(QPoint(optV4.rect.left() + 2, optV4.rect.top() + 2),
                                    QPoint(optV4.rect.right() - 2, optV4.rect.bottom() - 2));
            painter->setBrush(QBrush(QColor(Qt::darkBlue).darker().darker().darker()));
            painter->drawRect(overlayRect);
            painter->drawPixmap(iconRect, optV4.icon.pixmap(iconRect.size()));
        }
    } else if (idx.data(58) == "p") {
        QRect overlayRect = QRect(QPoint(optV4.rect.left() + 2, optV4.rect.top() + 2),
                                  QPoint(optV4.rect.right() - 2, optV4.rect.bottom() - 2));
        painter->setBrush(QBrush(QColor(Qt::black)));
        painter->drawRect(overlayRect);
        painter->drawPixmap(iconRect, optV4.icon.pixmap(iconRect.size()));

        painter->setBrush(QColor(idx.data(60).toString()).darker().darker());
        painter->drawRect(iconRect);
        painter->setBrush(QColor(idx.data(60).toString()).darker());
        painter->drawRect(QRect(QPoint(iconRect.left() + 3, iconRect.top() + 3),
                                QPoint(iconRect.right() - 3, iconRect.bottom() - 3)));
        painter->setBrush(QColor(idx.data(60).toString()));
        painter->drawRect(QRect(QPoint(iconRect.left() + 6, iconRect.top() + 6),
                                QPoint(iconRect.right() - 6, iconRect.bottom() - 6)));
        painter->drawPixmap(QRect(QPoint(iconRect.left() + 1, iconRect.top() + 1), 
                                  QPoint(iconRect.width() - 11, iconRect.height() - 11)), 
                            m_partIcon.pixmap(iconRect.width() - 9, iconRect.height() - 9));
        QRect sepLineRect = QRect(QPoint(optV4.rect.left() + 15, optV4.rect.bottom() - 2),
                                  QPoint(optV4.rect.right() - 15, optV4.rect.bottom() - 2));
        painter->setBrush(QBrush(QColor(idx.data(60).toString()).darker().darker()));
        painter->drawRect(sepLineRect);
        QRect sepLineRect2 = QRect(QPoint(optV4.rect.left() + 35, optV4.rect.bottom() - 2),
                                   QPoint(optV4.rect.right() - 35, optV4.rect.bottom() - 2));
        painter->setBrush(QBrush(QColor(idx.data(60).toString()).darker()));
        painter->drawRect(sepLineRect2);
    }

    const Partition *partition = idx.data(PARTITION_ROLE).value<const Partition*>();
    if (partition && partition->isMounted()) {
        QRect overlayRect = optV4.rect;
        overlayRect.setSize(iconRect.size() / 1.9);
        overlayRect.moveTo(QPoint(iconRect.right() - overlayRect.width() + 2,
                                  iconRect.bottom() - overlayRect.height() + 2));

        painter->drawPixmap(overlayRect, m_lockIcon.pixmap(overlayRect.size()));
    }

    QPen p(Qt::white);
    painter->setPen(p);

    painter->save();

    QFont nameFont = painter->font();
    nameFont.setWeight(QFont::Bold);
    painter->setFont(nameFont);

    QRect textRect = optV4.rect;
    textRect.setX(iconRect.x() + iconRect.width() + 8);
    textRect.setY(iconRect.y() + 1);

    if (idx.data(58) != "p") {
        painter->drawText(textRect, optV4.text + "  (" + idx.data(50).toString() + ")");
    } else {
        painter->drawText(textRect, optV4.text);
    }

    painter->restore();

    QRect typeRect;
    typeRect = painter->boundingRect(optV4.rect, Qt::AlignLeft | Qt::AlignBottom, idx.data(51).toString());
    typeRect.moveTo(QPoint(typeRect.x() + iconRect.x() + iconRect.width() + SPACING,
                           typeRect.y() - SPACING));
    painter->drawText(typeRect, idx.data(51).toString());

    QRect sizeRect;

    if (idx.data(58) != "p") {

    } else {
        sizeRect = painter->boundingRect(optV4.rect, Qt::AlignRight | Qt::AlignTop, idx.data(50).toString());
        sizeRect.moveTo(QPoint(sizeRect.x() - SPACING, sizeRect.y() + SPACING));
        painter->drawText(sizeRect, idx.data(50).toString());
    }

    painter->restore();
}

void PartitionDelegate::updateEditorGeometry(QWidget* editor,
                                             const QStyleOptionViewItem& opt,
                                             const QModelIndex& index) const
{
    Q_UNUSED(index);

    QStyleOptionViewItemV4 option(opt);
    QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
    comboBox->resize(QSize(option.rect.width() / 5 - SPACING * 2,
                           option.rect.height() / 2 - SPACING * 2));
    comboBox->move(QPoint(option.rect.right() - comboBox->width() - SPACING,
                          option.rect.bottom() - comboBox->height() - SPACING));
}

QSize PartitionDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    Q_UNUSED(index);

    return QSize(0, QFontMetrics(option.font).height() * 2 + 4 * SPACING);
}

/////////////// tree widget item

class PartitionTreeWidgetItem : public QTreeWidgetItem
{
    Q_DISABLE_COPY(PartitionTreeWidgetItem)

    public:
        PartitionTreeWidgetItem(const Partition* p, Device *d) : QTreeWidgetItem(),
                                                                 m_Partition(p),
                                                                 m_Device(d)
        {
            setData(0, DEVICE_ROLE, QVariant::fromValue(d));
            setData(0, PARTITION_ROLE, QVariant::fromValue(p));
        }

        PartitionTreeWidgetItem(Device* d) : QTreeWidgetItem(),
                                             m_Partition(0),
                                             m_Device(d)
        { }

        const Partition* partition() const { return m_Partition; }
        Device* device() { return m_Device; }

    private:
        const Partition* m_Partition;
        Device* m_Device;
};

/////////////// partition view widget

PartitionViewWidget::PartitionViewWidget(QWidget* parent)
        : QTreeWidget(parent)
        , m_retainPaintEvent(false)
        , m_fadeTimeLine(new QTimeLine(1200, this))
        , m_spinnerTimeLine(new QTimeLine(2000, this))
        , m_sequence("process-working", KIconLoader::SizeSmallMedium)
        , m_loadingPixmap(KIconLoader::global()->loadIcon("applications-system",
                                                             KIconLoader::Desktop,
                                                             KIconLoader::SizeEnormous))
{
    m_spinnerTimeLine->setFrameRange(0, m_sequence.frameCount());
    m_spinnerTimeLine->setLoopCount(0);
    m_spinnerTimeLine->setCurveShape(QTimeLine::LinearCurve);
    connect(m_spinnerTimeLine, SIGNAL(frameChanged(int)), viewport(), SLOT(repaint()));

    m_fadeTimeLine->setFrameRange(0, 100);
    m_fadeTimeLine->setCurveShape(QTimeLine::EaseOutCurve);
    connect(m_fadeTimeLine, SIGNAL(frameChanged(int)), viewport(), SLOT(repaint()));
    connect(m_fadeTimeLine, SIGNAL(finished()), this, SLOT(stopRetainingPaintEvent()));
}

PartitionViewWidget::~PartitionViewWidget()
{ }

void PartitionViewWidget::stopRetainingPaintEvent()
{
    m_retainPaintEvent = false;
    viewport()->update();
}

void PartitionViewWidget::setEnabled(bool enabled)
{
    if (enabled == isEnabled()) {
        return;
    }

    QTreeView::setEnabled(enabled);

    if (enabled) {
        m_fadeTimeLine->setDirection(QTimeLine::Backward);
        m_fadeTimeLine->start();
        m_spinnerTimeLine->stop();
        m_retainPaintEvent = true;
        viewport()->update();
    } else {
        m_fadeTimeLine->setDirection(QTimeLine::Forward);
        m_fadeTimeLine->start();
        m_spinnerTimeLine->start();
        viewport()->update();
    }
}

void PartitionViewWidget::paintEvent(QPaintEvent* event)
{
    if (isEnabled()) {
        QTreeView::paintEvent(event);
    }

    if (!isEnabled() || m_retainPaintEvent) {
        QPainter painter(viewport());

        QRect spinnerRect(QPoint(0, 0), QSize((int)KIconLoader::SizeSmallMedium, (int)KIconLoader::SizeSmallMedium));
        spinnerRect.moveCenter(QPoint(size().width()/2, size().height()/2  + 32));

        QRect textRect(QPoint(0, 0), QSize(size().width(),64));
        textRect.moveCenter(QPoint(size().width()/2, size().height()/2));

        QRect iconRect(QPoint(0, 0), QSize((int)KIconLoader::SizeEnormous, (int)KIconLoader::SizeEnormous));
        iconRect.moveCenter(QPoint(size().width()/2, size().height()/2));

        painter.setOpacity(m_fadeTimeLine->currentFrame() * 0.0035);
        painter.drawPixmap(iconRect, m_loadingPixmap);
        painter.setOpacity(m_fadeTimeLine->currentFrame() * 0.01);
        painter.drawPixmap(spinnerRect, m_sequence.frameAt(m_spinnerTimeLine->currentFrame()));

        QPen p(Qt::white);
        painter.setPen(p);
        Qt::Alignment align(Qt::AlignVCenter | Qt::AlignHCenter);
        painter.drawText(textRect, i18n("Loading partitions..."), QTextOption(align));
    }
}


///////////////  actual page

PartitionPage::PartitionPage(QWidget *parent)
        : AbstractPage(parent)
        , m_ui(new Ui::Partition)
        , m_install(InstallationHandler::instance())
{
    m_colorList.append(QColor(Qt::blue).darker().darker());
    m_colorList.append(QColor(Qt::red).darker().darker());
    m_colorList.append(QColor(Qt::green).darker().darker());
    m_colorList.append(QColor(Qt::yellow).darker().darker());
    m_colorList.append(QColor(Qt::gray).darker().darker());
    m_colorList.append(QColor(Qt::blue).darker().darker());
    m_colorList.append(QColor(Qt::red).darker().darker());
    m_colorList.append(QColor(Qt::green).darker().darker());
    m_colorList.append(QColor(Qt::yellow).darker().darker());
    m_colorList.append(QColor(Qt::gray).darker().darker());
    m_colorList.append(QColor(Qt::blue).darker().darker().darker());
    m_colorList.append(QColor(Qt::red).darker().darker().darker());
    m_colorList.append(QColor(Qt::green).darker().darker().darker());
    m_colorList.append(QColor(Qt::yellow).darker().darker().darker());
    m_colorList.append(QColor(Qt::gray).darker().darker().darker());

    m_currentPart = 0;
}

PartitionPage::~PartitionPage()
{
}

void PartitionPage::createWidget()
{
    m_ui->setupUi(this);

    m_ui->advancedButton->setIcon(KIcon("office-chart-ring"));
    m_ui->advancedButton->setEnabled(false);
    connect(m_ui->advancedButton, SIGNAL(clicked(bool)), this, SLOT(advancedClicked()));

    m_ui->newPartTableButton->setIcon(KIcon("insert-table"));
    m_ui->newPartTableButton->setEnabled(false);
    connect(m_ui->newPartTableButton, SIGNAL(clicked(bool)), this, SLOT(newPartTableClicked()));
    
    m_ui->newButton->setIcon(KIcon("list-add"));
    m_ui->newButton->setEnabled(false);
    connect(m_ui->newButton, SIGNAL(clicked(bool)), this, SLOT(newClicked()));

    m_ui->deleteButton->setIcon(KIcon("list-remove"));
    m_ui->deleteButton->setEnabled(false);
    connect(m_ui->deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteClicked()));

    m_ui->formatButton->setIcon(KIcon("draw-eraser"));
    m_ui->formatButton->setEnabled(false);
    m_ui->formatButton->setCheckable(true);
    connect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));

    m_ui->undoButton->setIcon(KIcon("edit-undo"));
    m_ui->undoButton->setEnabled(PMHandler::instance()->operationStack().operations().size() > 0);
    connect(m_ui->undoButton, SIGNAL(clicked(bool)), this, SLOT(undoClicked()));

    m_ui->unmountButton->setIcon(KIcon("object-unlocked"));
    m_ui->unmountButton->setVisible(false);
    connect(m_ui->unmountButton, SIGNAL(clicked(bool)), this, SLOT(unmountClicked()));

    connect(m_ui->sizeSlider, SIGNAL(valueChanged(int)), m_ui->sizeSpinBox, SLOT(setValue(int)));
    connect(m_ui->sizeSpinBox, SIGNAL(valueChanged(int)), m_ui->sizeSlider, SLOT(setValue(int)));

    connect(m_ui->typeBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(slotTypeChanged(QString)));

    m_ui->treeWidget->setItemDelegate(new PartitionDelegate);
    m_ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->treeWidget->setEnabled(false);

    setVisibleParts(false);

    QStringList fsNames;
    foreach (const FileSystem* fs, FileSystemFactory::map()) {
        if (fs->supportCreate() != FileSystem::cmdSupportNone && 
            fs->type() != FileSystem::Extended &&
            fs->type() != FileSystem::Unformatted) {
            fsNames.append(fs->name());
        }
    }

    qSort(fsNames.begin(), fsNames.end(), caseInsensitiveLessThan);

    m_ui->filesystemBox->addItems(fsNames);
    m_ui->filesystemBox->setVisible(false);
    m_ui->filesystemLabel->setVisible(false);

    m_ui->editPartitionCancelButton->setIcon(KIcon("dialog-cancel"));
    m_ui->editPartitionOkButton->setIcon(KIcon("dialog-ok"));

    enableNextButton(false);

    m_currentPart = 0;

    PMHandler::instance();
    connect(PMHandler::instance(), SIGNAL(devicesReady()), this, SLOT(populateTreeWidget()));
    PMHandler::instance()->reload();
}

void PartitionPage::newPartTableClicked()
{
    int ret;

    if (m_ui->treeWidget->currentItem()->childCount() > 0) {
        ret = KMessageBox::questionYesNo(0, i18n("The selected volume already has a partition table.. Overwrite it? (this will destroy all data on the device)"), i18n("Warning"));
        if (ret == KMessageBox::Yes) {
            QProcess p;
            p.start("parted -s " + m_ui->treeWidget->currentItem()->data(0, 52).toString() + " mktable msdos");
            p.waitForFinished();
        }
    } else {
        ret = KMessageBox::questionYesNo(0, i18n("This operation will destroy all data on the selected volume.. Continue anyway?"), i18n("Warning"));
        if (ret == KMessageBox::Yes) {
            QProcess p;
            p.start("parted -s " + m_ui->treeWidget->currentItem()->data(0, 52).toString() + " mktable msdos");
            p.waitForFinished();
        }
    }

    PMHandler::instance()->reload();
}

void PartitionPage::advancedClicked()
{
    QProcess p;
    p.start("partitionmanager");
    p.waitForFinished();
    populateTreeWidget();
}

void PartitionPage::setVisibleParts(bool b)
{
    m_ui->editPartitionCancelButton->setVisible(b);
    m_ui->editPartitionOkButton->setVisible(b);
    m_ui->lowLine->setVisible(b);
    m_ui->filesystemLabel->setVisible(b);
    m_ui->filesystemBox->setVisible(b);
    m_ui->typeLabel->setVisible(b);
    m_ui->typeBox->setVisible(b);
    m_ui->sizeSlider->setVisible(b);
    m_ui->sizeSpinBox->setVisible(b);

    if (m_ui->typeBox->currentText() == i18n("Extended")) {
        m_ui->filesystemLabel->setVisible(false);
        m_ui->filesystemBox->setVisible(false);
    }
}

void PartitionPage::populateTreeWidget()
{
    m_ui->advancedButton->setEnabled(true);

    m_ui->undoButton->setEnabled(PMHandler::instance()->operationStack().size() > 0);

    QReadLocker lockDevices(&PMHandler::instance()->operationStack().lock());

    disconnect(m_ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
               this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    m_ui->treeWidget->clear();

    foreach(Device* dev, PMHandler::instance()->operationStack().previewDevices()) {
        QTreeWidgetItem* deviceItem = new PartitionTreeWidgetItem(dev);
        deviceItem->setText(0, dev->name());
        deviceItem->setData(0, 50, KIO::convertSize(dev->capacity()).replace("i", ""));
        deviceItem->setData(0, 52, dev->deviceNode());
        deviceItem->setIcon(0, KIcon(dev->iconName()));
        m_ui->treeWidget->addTopLevelItem(deviceItem);
        deviceItem->setExpanded(true);

        if (dev->partitionTable() != NULL) {
            foreach(const Partition* p, dev->partitionTable()->children()) {
                QTreeWidgetItem *item = createItem(p, dev);

                foreach(const Partition* child, p->children()) {
                    QTreeWidgetItem* childItem = createItem(child, dev);
                    item->addChild(childItem);
                }

                deviceItem->addChild(item);
                item->setExpanded(true);
            }
        }
    }

    connect(m_ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(m_ui->treeWidget->model(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(dataChanged(QModelIndex,QModelIndex)));

    currentItemChanged(0, 0);

    m_ui->treeWidget->setEnabled(true);
    m_ui->deleteButton->setVisible(true);
    m_ui->newButton->setVisible(true);
    m_ui->newPartTableButton->setVisible(true);
    m_ui->formatButton->setVisible(true);

    QTreeWidgetItemIterator it(m_ui->treeWidget);
    while(*it) {
        const Partition *partition = (*it)->data(0, PARTITION_ROLE).value<const Partition*>();
        if (partition != 0) {
            if (!partition->roles().has(PartitionRole::Extended) &&
                partition->fileSystem().type() != FileSystem::Unknown &&
                partition->fileSystem().type() != FileSystem::LinuxSwap &&
                partition->fileSystem().type() != FileSystem::Unformatted) {

                m_ui->treeWidget->openPersistentEditor(*it);
            }
        }
        ++it;
    }

    enableNextButton(true);
    currentItemChanged(m_ui->treeWidget->currentItem(), 0);
}

void PartitionPage::currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* )
{
    if (!current) {
        m_ui->deleteButton->setEnabled(false);
        m_ui->newButton->setEnabled(false);
        m_ui->newPartTableButton->setEnabled(false);
        m_ui->formatButton->setEnabled(false);
        m_ui->unmountButton->setVisible(false);
        enableNextButton(false);
        return;
    }

    const Partition *partition = current->data(0, PARTITION_ROLE).value<const Partition*>();
    if (partition != 0) {
        m_ui->unmountButton->setVisible(partition->isMounted());
        m_ui->unmountButton->setEnabled(partition->canUnmount());
        m_ui->deleteButton->setEnabled(DeleteOperation::canDelete(partition));
        m_ui->newButton->setEnabled(NewOperation::canCreateNew(partition));
        m_ui->formatButton->setEnabled(DeleteOperation::canDelete(partition));
        disconnect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));
        m_ui->formatButton->setChecked(m_toFormat.contains(partition));
        connect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));
        m_ui->newPartTableButton->setEnabled(false);
    } else {
        m_ui->unmountButton->setVisible(false);
        m_ui->deleteButton->setEnabled(false);
        m_ui->newButton->setEnabled(false);
        m_ui->formatButton->setEnabled(false);
        m_ui->newPartTableButton->setEnabled(true);
    }

    QTreeWidgetItemIterator it(m_ui->treeWidget);
    while (*it) {
        QString text = (*it)->data(0, MOUNTPOINT_ROLE).toString();
        if (text == "/") {
            enableNextButton(true);
            return;
        }
        ++it;
    }

    enableNextButton(false);
}

void PartitionPage::dataChanged(QModelIndex i, QModelIndex p)
{
    Q_UNUSED(i);
    Q_UNUSED(p);

    QTreeWidgetItemIterator it(m_ui->treeWidget);
    while (*it) {
        QString text = (*it)->data(0, MOUNTPOINT_ROLE).toString();
        if (text == "/") {
            enableNextButton(true);
            return;
        }
                ++it;
    }

    enableNextButton(false);
}

void PartitionPage::unmountClicked()
{
    const Partition *partition = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();
    Device *device = m_ui->treeWidget->selectedItems().first()->data(0, DEVICE_ROLE).value<Device*>();
    // Delete the partition
    Partition *p = device->partitionTable()->findPartitionBySector(partition->firstSector(), PartitionRole(PartitionRole::Any));

    Report *rep = new Report(0);
    p->unmount(*rep);
    rep->deleteLater();

    populateTreeWidget();
}

void PartitionPage::slotTypeChanged(QString s)
{
    if (s == i18n("Extended")) {
        m_ui->filesystemLabel->setVisible(false);
        m_ui->filesystemBox->setVisible(false);
    } else {
        m_ui->filesystemLabel->setVisible(true);
        m_ui->filesystemBox->setVisible(true);
    }
}

void PartitionPage::formatToggled(bool status)
{
    m_ui->advancedButton->setEnabled(false);
    if (status) {
        setVisibleParts(true);
        m_ui->sizeSlider->setVisible(false);
        m_ui->sizeSpinBox->setVisible(false);
        m_ui->typeBox->setVisible(false);
        m_ui->typeLabel->setVisible(false);
        m_ui->filesystemLabel->setVisible(true);
        m_ui->filesystemBox->setVisible(true);
    } else {
        cancelFormat();
        return;
    }

    const Partition *p = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();
    m_ui->filesystemBox->setCurrentIndex(m_ui->filesystemBox->findText(FileSystem::nameForType(p->fileSystem().type())));

    connect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(cancelFormat()));
    connect(m_ui->editPartitionCancelButton, SIGNAL(clicked(bool)), this, SLOT(cancelFormat()));
    connect(m_ui->editPartitionOkButton, SIGNAL(clicked(bool)), this, SLOT(applyFormat()));
}

void PartitionPage::applyFormat()
{
    disconnect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(cancelFormat()));
    disconnect(m_ui->editPartitionCancelButton, SIGNAL(clicked(bool)), this, SLOT(cancelFormat()));
    disconnect(m_ui->editPartitionOkButton, SIGNAL(clicked(bool)), this, SLOT(applyFormat()));
    disconnect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));

    const Partition *p = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();
    m_toFormat.insert(p, m_ui->filesystemBox->currentText());
    m_ui->formatButton->setChecked(true);
    setVisibleParts(false);

    connect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));
    m_ui->advancedButton->setEnabled(true);
}

void PartitionPage::cancelFormat()
{
    disconnect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));
    disconnect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(cancelFormat()));
    disconnect(m_ui->editPartitionCancelButton, SIGNAL(clicked(bool)), this, SLOT(cancelFormat()));
    disconnect(m_ui->editPartitionOkButton, SIGNAL(clicked(bool)), this, SLOT(applyFormat()));

    m_ui->formatButton->setChecked(false);
    const Partition *p = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();
    m_toFormat.remove(p);
    setVisibleParts(false);

    connect(m_ui->formatButton, SIGNAL(toggled(bool)), this, SLOT(formatToggled(bool)));
    m_ui->advancedButton->setEnabled(true);
}

void PartitionPage::deleteClicked()
{
    const Partition *partition = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();
    Device *device = m_ui->treeWidget->selectedItems().first()->data(0, DEVICE_ROLE).value<Device*>();
    // Delete the partition
    Partition *p = device->partitionTable()->findPartitionBySector(partition->firstSector(), PartitionRole(PartitionRole::Any));

    PMHandler::instance()->operationStack().push(new DeleteOperation(*device, p));

    populateTreeWidget();
}

void PartitionPage::newClicked()
{
    m_ui->typeBox->clear();

    const Partition *partition = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();

    m_ui->typeBox->addItem(i18n("Extended"), (int)PartitionRole::Extended);
    m_ui->typeBox->addItem(i18n("Primary"), (int)PartitionRole::Primary);

    QString selected = FileSystem::nameForType(FileSystem::Ext4);
    m_ui->filesystemBox->setCurrentIndex(m_ui->filesystemBox->findText(selected));

    setVisibleParts(true);

    m_newPartition = NewOperation::createNew(*partition);

    qint64 minSize = qMax(m_newPartition->sectorsUsed(), m_newPartition->minimumSectors()) * m_newPartition->sectorSize();
    qint64 maxSize = qMin(m_newPartition->length(), m_newPartition->maximumSectors()) * m_newPartition->sectorSize();

    m_ui->sizeSpinBox->setRange(Capacity(minSize).toInt(Capacity::MiB), Capacity(maxSize).toInt(Capacity::MiB));

    m_ui->sizeSlider->setRange(Capacity(minSize).toInt(Capacity::MiB), Capacity(maxSize).toInt(Capacity::MiB));
    m_ui->sizeSlider->setValue(Capacity(maxSize).toInt(Capacity::MiB));

    connect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(cancelNew()));

    connect(m_ui->editPartitionCancelButton, SIGNAL(clicked(bool)), this, SLOT(cancelNew()));
    connect(m_ui->editPartitionOkButton, SIGNAL(clicked(bool)), this, SLOT(applyNew()));
}

void PartitionPage::applyNew()
{
    disconnect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(cancelNew()));

    disconnect(m_ui->editPartitionCancelButton, SIGNAL(clicked(bool)), this, SLOT(cancelNew()));
    disconnect(m_ui->editPartitionOkButton, SIGNAL(clicked(bool)), this, SLOT(applyNew()));

    const Partition *partition = m_ui->treeWidget->selectedItems().first()->data(0, PARTITION_ROLE).value<const Partition*>();
    Device *device = m_ui->treeWidget->selectedItems().first()->data(0, DEVICE_ROLE).value<Device*>();

    qint64 lastSector = (m_newPartition->lastSector() - m_newPartition->firstSector()) / m_ui->sizeSpinBox->maximum();
    lastSector *= m_ui->sizeSpinBox->value();
    lastSector += m_newPartition->firstSector();
    m_newPartition->deleteFileSystem();
    m_newPartition->setLastSector(lastSector);

    FileSystem::Type filesystem;
    if (m_ui->typeBox->count() > 0) {
        QString type = m_ui->typeBox->currentText();
        if (type == "Extended") {
            m_newPartition->setRoles(PartitionRole(PartitionRole::Extended));
            filesystem = FileSystem::Extended;
        } else {
            m_newPartition->setRoles(PartitionRole(PartitionRole::Primary));
            filesystem = FileSystem::typeForName(m_ui->filesystemBox->currentText());
        }
    } else {
        PartitionRole::Roles roles = device->partitionTable()->childRoles(*partition);
        if (roles & PartitionRole::Logical) {
            m_newPartition->setRoles(PartitionRole(PartitionRole::Logical));
        } else {
            m_newPartition->setRoles(PartitionRole(PartitionRole::Primary));
        }
        filesystem = FileSystem::typeForName(m_ui->filesystemBox->currentText());
    }

    m_newPartition->setFileSystem(FileSystemFactory::create(filesystem,
                                                            m_newPartition->firstSector(),
                                                            m_newPartition->lastSector()));

    PMHandler::instance()->operationStack().push(new NewOperation(*device, m_newPartition));

    setVisibleParts(false);

    populateTreeWidget();
}

void PartitionPage::cancelNew()
{
    disconnect(m_ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(cancelNew()));
    disconnect(m_ui->editPartitionCancelButton, SIGNAL(clicked(bool)), this, SLOT(cancelNew()));
    disconnect(m_ui->editPartitionOkButton, SIGNAL(clicked(bool)), this, SLOT(applyNew()));

    delete m_newPartition;

    setVisibleParts(false);
}

void PartitionPage::undoClicked()
{
    PMHandler::instance()->operationStack().pop();
    populateTreeWidget();
}

QTreeWidgetItem* PartitionPage::createItem(const Partition* p, Device *dev)
{
    if (m_currentPart >= m_colorList.count())
        m_currentPart = 0;

    QTreeWidgetItem* item = new PartitionTreeWidgetItem(p, dev);

    QString pCapacity = KIO::convertSize(Capacity(*p).toInt(Capacity::Byte)).replace("i", "");
    QString pUsed = KIO::convertSize(Capacity(*p, Capacity::Used).toInt(Capacity::Byte)).replace("i", "");

    pUsed.replace("16.0 EB", "-");

    if (p->number() > 0) {
        item->setText(0, QString("%1%2").arg(p->devicePath()).arg(p->number()));
    } else {
        item->setText(0, "New partition");
    }

    if (p->fileSystem().type() == FileSystem::Unknown) {
        item->setData(0, 50, pCapacity);
        item->setData(0, 51, "Unallocated space");
        item->setData(0, 60, m_colorList.at(m_currentPart).name());
    } else if (!p->children().isEmpty()) {
        item->setData(0, 50, pCapacity);
        item->setIcon(0, KIcon("drive-harddisk"));
        item->setData(0, 51, "Linux / Extended " + p->fileSystem().name());
    } else if (p->fileSystem().type() == FileSystem::Ntfs || p->fileSystem().type() == FileSystem::Fat16 ||
               p->fileSystem().type() == FileSystem::Fat32) {
        item->setData(0, 50, pUsed + " / " + pCapacity);
        item->setData(0, 58, "p");
        item->setData(0, 59, "win");
        item->setData(0, 60, m_colorList.at(m_currentPart).name());
        item->setData(0, 51, "Windows / " + p->fileSystem().name());
    } else if (p->fileSystem().type() == FileSystem::Hfs || p->fileSystem().type() == FileSystem::HfsPlus) {
        item->setData(0, 50, pUsed + " / " + pCapacity);
        item->setData(0, 58, "p");
        item->setData(0, 59, "mac");
        item->setData(0, 60, m_colorList.at(m_currentPart).name());
        item->setData(0, 51, "Apple / " + p->fileSystem().name());
    } else {
        item->setData(0, 50, pUsed + " / " + pCapacity);
        item->setData(0, 58, "p");
        item->setData(0, 59, "lin");
        item->setData(0, 60, m_colorList.at(m_currentPart).name());
        item->setData(0, 51, "Linux / " + p->fileSystem().name());
    }

    m_currentPart++;

    return item;
}

void PartitionPage::aboutToGoToNext()
{
    bool foundRoot = false;
    QTreeWidgetItemIterator it(m_ui->treeWidget);
    while (*it) {
        QString text = (*it)->data(0, MOUNTPOINT_ROLE).toString();

        if (text.startsWith('/')) {
            const Partition *partition = (*it)->data(0, PARTITION_ROLE).value<const Partition*>();
            Device *device = m_ui->treeWidget->selectedItems().first()->data(0, DEVICE_ROLE).value<Device*>();

            // If '/' is being considered, check target capacity.
            if (text == "/") {
                foundRoot = true;
                if (partition->capacity() < InstallationHandler::instance()->minSizeForTarget()) {
                KMessageBox::error(this, i18n("The partition you have chosen to mount as '/' is too small. It should "
                                              "have a capacity of at least %1 for a successful installation.",
                                              KIO::convertSize(InstallationHandler::instance()->minSizeForTarget())),
                                   i18n("Target's capacity not sufficient"));
                PMHandler::instance()->clearMountList();
                return;
                }
            }

            if (m_toFormat.contains(partition)) {
                if (text == "/") {
                    m_install->setRootDevice(QString(partition->devicePath()).left(-1));
                    foundRoot = true;
                }

                PMHandler::instance()->addSectorToMountList(device, partition->firstSector(), text, m_toFormat[partition]);
            } else {
                if (text == "/") {
                    QString msg;

                    msg.append(i18n("The partition you have chosen to mount as '/' is not marked for formatation. "
                                    "This might create problems. Do you still want to continue without formating it?"));

                    KDialog *dialog = new KDialog(this, Qt::FramelessWindowHint);
                    bool retbool = true;
                    dialog->setButtons(KDialog::Yes | KDialog::No);

                    if (KMessageBox::createKMessageBox(dialog, KIcon("dialog-warning"), msg, QStringList(),
                                       QString(), &retbool, KMessageBox::Notify) == KDialog::No) {
                    PMHandler::instance()->clearMountList();
                    return;
                    }
                }
                PMHandler::instance()->addSectorToMountList(device, partition->firstSector(), text);
            }
        } else if (!text.isEmpty() && text != "None") {
            KMessageBox::error(this, i18n("You have selected '%1' as a mountpoint, which is not valid. A valid mountpoint "
                               "always starts with '/' and represents a directory on disk", text),
                               i18n("Target's capacity not sufficient"));
            PMHandler::instance()->clearMountList();
            return;
        }

        ++it;
    }

    if (!foundRoot)
        return;   /// todo: give a msgbox

    s_partitionToMountPoint.clear();

    emit goToNextStep();
}

void PartitionPage::aboutToGoToPrevious()
{
    enableNextButton(true);

    s_partitionToMountPoint.clear();

    emit goToPreviousStep();
}

#include "partitionpage.moc"
