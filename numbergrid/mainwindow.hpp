#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP
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

#include "surrogateitem.hpp"
#include "matchform.hpp"
#include "threadsafeerrorinfo.hpp"
#include <QAbstractItemView>
#include <QFutureWatcher>
#include <QList>
#include <QMainWindow>


class QAction;
class QCloseEvent;
class QProgressBar;
class QStandardItem;
class QStandardItemModel;
class QTableView;


struct Results
{
    explicit Results() : count(0), sum(0.0) {}

    int count;
    long double sum;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent=0);

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    void editCount();
    void editSelect();
    void editApplyScript();
    void finishedCounting();
    void finishedSelecting();
    void finishedApplyingScript();
    void setDirty(bool on=true);
    void updateUi();

protected:
    void closeEvent(QCloseEvent *event);

private:
    void createModelAndView();
    void createActions();
    void createMenusAndToolBar();
    void createConnections();
    bool okToClearData();

    //为了确保里面的QList<SurrogateItem>不被修改
    //前面的const指返回类型为const
    //后面加const表示this指针为const，对于类中的参数都不可修改

    const QList<SurrogateItem> allSurrogateItems() const;
    QList<SurrogateItem> selectedSurrogateItems() const;

    //stop()没有加const的必要

    void stop();
    bool applyDespiteErrors();

    template<typename T>
    void setUpProgressBar(QFutureWatcher<T> &futureWatcher);

    QAction *fileNewAction;
    QAction *fileOpenAction;
    QAction *fileSaveAction;
    QAction *fileSaveAsAction;
    QAction *fileQuitAction;
    QAction *editCountAction;
    QAction *editSelectAction;
    QAction *editApplyScriptAction;
    QAction *editStopAction;
    QTableView *view;
    QProgressBar *progressBar;

    QStandardItemModel *model;
    //QFutureWatcher allows monitoring a QFuture using signals and slots
    QFutureWatcher<SurrogateItem> selectWatcher;
    QFutureWatcher<Results> countWatcher;
    QFutureWatcher<SurrogateItem> applyScriptWatcher;
    MatchCriteria countCriteria;
    bool applyToAll;
    mutable bool cacheIsDirty;
    QString script;
    ThreadSafeErrorInfo errorInfo;
    QString filename;
    QAbstractItemView::EditTriggers editTriggers;
};

#endif // MAINWINDOW_HPP
