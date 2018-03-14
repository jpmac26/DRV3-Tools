#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTableWidget>
#include <QtConcurrent/QtConcurrent>
#include "../utils/binarydata.h"
#include "../utils/data_formats.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSaveAs_triggered();
    void on_actionExit_triggered();
    void on_comboBox_SelectLabel_currentIndexChanged(int index);
    void on_tableWidget_Strings_itemChanged(QTableWidgetItem *item);
    void on_tableWidget_Code_itemChanged(QTableWidgetItem *item);
    void on_tableWidget_Flags_itemChanged(QTableWidgetItem *item);
    void on_toolButton_CmdAdd_clicked();
    void on_toolButton_CmdDel_clicked();
    void on_toolButton_CmdUp_clicked();
    void on_toolButton_CmdDown_clicked();
    void on_toolButton_StringAdd_clicked();
    void on_toolButton_StringDel_clicked();
    void on_toolButton_StringUp_clicked();
    void on_toolButton_StringDown_clicked();
    void on_toolButton_FlagAdd_clicked();
    void on_toolButton_FlagDel_clicked();
    void on_toolButton_FlagUp_clicked();
    void on_toolButton_FlagDown_clicked();

private:
    bool confirmUnsaved();
    void openFile(QString filepath);
    void reloadAllLists();
    void reloadLabelList();
    void reloadCodeList(const int index = -1);
    void reloadStringList();
    void reloadFlagList();
    void updateHexHeaders(QTableWidget *widget);
    void updateLabelCodeWidget();

    Ui::MainWindow *ui;
    WrdFile currentWrd;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
