/*
    Copyright (c) 2009-10 Qtrac Ltd. All rights reserved.

    This program or module is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version. It is provided
    for educational purposes and is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
    the GNU General Public License for more details.
*/
#include "aqp.hpp"
#include "alt_key.hpp"
#ifndef USE_QTCONCURRENT
#include "convertimagetask.hpp"
#endif
#include "mainwindow.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QCompleter>
#include <QComboBox>
#include <QDirIterator>
#include <QDirModel>
#include <QGridLayout>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QThreadPool>
#include <QTimer>
#ifdef USE_QTCONCURRENT
#include <QtConcurrentRun>
#endif


const int PollTimeout = 100;

//事件会从一个线程切换到另一个线程
//QEvent能够处理
//详见下面的Event实现
#ifdef USE_QTCONCURRENT
struct ProgressEvent : public QEvent
{
    enum {EventId = QEvent::User};//每个自定义事件的唯一ID

    //ID    和     信息message
    explicit ProgressEvent(bool saved_, const QString &message_)
        : QEvent(static_cast<Type>(EventId)),
          saved(saved_), message(message_) {}

    const bool saved;
    const QString message;
};

//1.指向主窗口的指针，2.bool* stopped 用来观察是否取消处理线程
//3.从全部任务列表中得到的唯一文件列表 ，4.目标文件类型的后缀名
void convertImages(QObject *receiver, volatile bool *stopped,
        const QStringList &sourceFiles, const QString &targetType)
{
    foreach (const QString &source, sourceFiles) {
        if (*stopped)
            return;
        QImage image(source);
        QString target(source);
        target.chop(QFileInfo(source).suffix().length());
        target += targetType.toLower();
        if (*stopped)
            return;
        //save函数随便转换
        bool saved = image.save(target);

        QString message = saved
                ? QObject::tr("Saved '%1'")
                              .arg(QDir::toNativeSeparators(target))
                : QObject::tr("Failed to convert '%1'")
                              .arg(QDir::toNativeSeparators(source));
        //正在处理的文件的进度
        //sendEvents和postEvent的底层区别
        //ProgressEvent在上面定义了
        QApplication::postEvent(receiver,
                                new ProgressEvent(saved, message));//将成功标志和消息都传给主窗口
    }
}
#endif


#ifdef USE_CUSTOM_DIR_MODEL
// Taken from Qt's completer example
class DirModel : public QDirModel
{
public:
    explicit DirModel(QObject *parent=0) : QDirModel(parent) {}

    QVariant data(const QModelIndex &index,
                  int role=Qt::DisplayRole) const
    {
        if (role == Qt::DisplayRole && index.column() == 0) {
            QString path = QDir::toNativeSeparators(filePath(index));
            if (path.endsWith(QDir::separator()))
                path.chop(1);
            return path;
        }
        return QDirModel::data(index, role);
    }
};
#endif


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), stopped(true)
{
    createWidgets();
    createLayout();
    createConnections();

    AQP::accelerateWidget(this);
    updateUi();
    directoryEdit->setFocus();
    setWindowTitle(QApplication::applicationName());
}


void MainWindow::createWidgets()
{
    directoryLabel = new QLabel(tr("Path:"));
    QCompleter *directoryCompleter = new QCompleter(this);
#ifndef Q_WS_X11
    directoryCompleter->setCaseSensitivity(Qt::CaseInsensitive);
#endif
#ifdef USE_CUSTOM_DIR_MODEL
    directoryCompleter->setModel(new DirModel(directoryCompleter));
#else
    directoryCompleter->setModel(new QDirModel(directoryCompleter));
#endif
    directoryEdit = new QLineEdit(QDir::toNativeSeparators(
                                  QDir::homePath()));
    directoryEdit->setCompleter(directoryCompleter);
    directoryLabel->setBuddy(directoryEdit);

    sourceTypeLabel = new QLabel(tr("Source type:"));
    sourceTypeComboBox = new QComboBox;
    foreach (const QByteArray &ba,
             QImageReader::supportedImageFormats())
        sourceTypeComboBox->addItem(QString(ba).toUpper());
    sourceTypeComboBox->setCurrentIndex(0);
    sourceTypeLabel->setBuddy(sourceTypeComboBox);

    targetTypeLabel = new QLabel(tr("Target type:"));
    targetTypeComboBox = new QComboBox;
    targetTypeLabel->setBuddy(targetTypeComboBox);
    sourceTypeChanged(sourceTypeComboBox->currentText());

    logEdit = new QPlainTextEdit;
    logEdit->setReadOnly(true);
    logEdit->setPlainText(tr("Choose a path, source type and target "
                             "file type, and click Convert."));

    convertOrCancelButton = new QPushButton(tr("&Convert"));
    quitButton = new QPushButton(tr("Quit"));
}


void MainWindow::createLayout()
{
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(directoryLabel, 0, 0);
    layout->addWidget(directoryEdit, 0, 1, 1, 5);
    layout->addWidget(sourceTypeLabel, 1, 0);
    layout->addWidget(sourceTypeComboBox, 1, 1);
    layout->addWidget(targetTypeLabel, 1, 2);
    layout->addWidget(targetTypeComboBox, 1, 3);
    layout->addWidget(convertOrCancelButton, 1, 4);
    layout->addWidget(quitButton, 1, 5);
    layout->addWidget(logEdit, 2, 0, 1, 6);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    setCentralWidget(widget);
}


void MainWindow::createConnections()
{
    connect(directoryEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(updateUi()));
    connect(sourceTypeComboBox,
            SIGNAL(currentIndexChanged(const QString&)),
            this, SLOT(sourceTypeChanged(const QString&)));
    connect(sourceTypeComboBox, SIGNAL(activated(const QString&)),
            this, SLOT(sourceTypeChanged(const QString&)));
    connect(convertOrCancelButton, SIGNAL(clicked()),
            this, SLOT(convertOrCancel()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
}


void MainWindow::updateUi()
{
    if (stopped) {
        convertOrCancelButton->setText(tr("&Convert"));
        convertOrCancelButton->setEnabled(
                !directoryEdit->text().isEmpty());
    }
    else {
        convertOrCancelButton->setText(tr("&Cancel"));
        convertOrCancelButton->setEnabled(true);
    }
}


void MainWindow::sourceTypeChanged(const QString &sourceType)
{
    QStringList targetTypes;
    if (targetTypes.isEmpty()) {
        foreach (const QByteArray &ba,
                 QImageWriter::supportedImageFormats()) {
            const QString targetType = QString(ba).toUpper();
            if (targetType != sourceType)
                targetTypes << targetType.toUpper();
        }
    }
    targetTypes.sort();
    targetTypeComboBox->clear();
    targetTypeComboBox->addItems(targetTypes);
}


void MainWindow::convertOrCancel()
{
    //通知线程关闭，计算现在正在运行的线程in线程池
    //如果有就需要等待
    stopped = true;
    if (QThreadPool::globalInstance()->activeThreadCount())
        QThreadPool::globalInstance()->waitForDone();
    //如果用户已经取消，updateUi将Cancel变为Convert
    if (convertOrCancelButton->text() == tr("&Cancel")) {
        updateUi();
        return;
    }
    //如果点击了Convert
    QString sourceType = sourceTypeComboBox->currentText();
    QStringList sourceFiles;
    QDirIterator i(directoryEdit->text(), QDir::Files|QDir::Readable);
    while (i.hasNext())//读取文件
    {
        const QString &filenameAndPath = i.next();
        if (i.fileInfo().suffix().toUpper() == sourceType)//后缀名和所选源类型相匹配
            sourceFiles << filenameAndPath;
    }
    if (sourceFiles.isEmpty())
        AQP::warning(this, tr("No Images Error"),
                     tr("No matching files found"));
    else {
        logEdit->clear();
        convertFiles(sourceFiles);//调用convertFiles进行转换
    }
}

//每次convertFiles的时候开启QtConcurrent::run线程
//调用convertImages进行转换
//性能开销极大，如果图片很多的话，
//idealThreadCount()方法提供了计算程序运行所在平台上支持的辅助线程的最佳数目
void MainWindow::convertFiles(const QStringList &sourceFiles)
{
    stopped = false;
    updateUi();
    total = sourceFiles.count();
    done = 0;
    const QVector<int> sizes = AQP::chunkSizes(sourceFiles.count(),
            QThread::idealThreadCount());//这里得到了系统最佳的线程数

    int offset = 0;
    foreach (const int chunkSize, sizes)
    {
        //使用了QtConcurrent的时候run
        //mid是List容器内的函数，主要是用于确定开始位置和list数量
#ifdef USE_QTCONCURRENT
        //第一个为传入的线程函数，其他四项都是它的参数
        QtConcurrent::run(convertImages, this, &stopped,
                sourceFiles.mid(offset, chunkSize),//如果offset+count的和大于项的数目，全部使用offset的项
                targetTypeComboBox->currentText());
#else
        //ConvertImageTask(QRunnable的一个子类)
        ConvertImageTask *convertImageTask = new ConvertImageTask(
                this, &stopped, sourceFiles.mid(offset, chunkSize),
                targetTypeComboBox->currentText());
        //把convertImageTask的拥有权赋给Qt的全局线程池
        QThreadPool::globalInstance()->start(convertImageTask);
#endif
        offset += chunkSize;
    }
    //一旦所有辅助线程得到启动，就会调用checkIfDone()槽
    checkIfDone();
}

//检查数据处理是否完成
void MainWindow::checkIfDone()
{
    //这个if判断就是等待正在进行的线程结束
    //创建一个单触发计时器，每100ms就调用一次这个槽
    if (QThreadPool::globalInstance()->activeThreadCount())
        QTimer::singleShot(PollTimeout, this, SLOT(checkIfDone())); //轮询的方法不可取
    else {
        QString message;
        //所有线程结束
        if (done == total)
            message = tr("All %n image(s) converted", "", done);
        //用户取消或异常退出
        else
            message = tr("Converted %n/%1 image(s)", "", done)
                      .arg(total);
        logEdit->appendPlainText(message);
        stopped = true;
        updateUi();
    }
}

//槽函数，事件处理，QRunnable用
void MainWindow::announceProgress(bool saved, const QString &message)
{
    if (stopped)
        return;
    logEdit->appendPlainText(message);
    if (saved)
        ++done;
}
//在窗口的特定部件能够探测并处理自定义事件
//重新实现QWidget::Event
//给每个事件一个唯一的ID类型为QEvent::
#ifdef USE_QTCONCURRENT
bool MainWindow::event(QEvent *event)
{
    if (!stopped && event->type() ==
            static_cast<QEvent::Type>(ProgressEvent::EventId)) {
        ProgressEvent *progressEvent =
                static_cast<ProgressEvent*>(event);

        Q_ASSERT(progressEvent);
        logEdit->appendPlainText(progressEvent->message);
        if (progressEvent->saved)//如果存储成功，就增加已完成转换的文件数
            ++done;
        return true;//返回true表明事件已经得到处理
    }
    return QMainWindow::event(event);//如果处理过程已经停止，对于其他任务事件，还是使用原来的event
}
#endif


void MainWindow::closeEvent(QCloseEvent *event)
{
    stopped = true;
    if (QThreadPool::globalInstance()->activeThreadCount())
        QThreadPool::globalInstance()->waitForDone();
    event->accept();
}
