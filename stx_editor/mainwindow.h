#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFontMetrics>
#include <QMessageBox>
#include <QStringLiteral>
#include <QtWidgets>
#include <QTextStream>
#include "../utils/binarydata.h"
#include "../utils/stx.h"

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
    void closeEvent(QCloseEvent *event);
    void on_textBox_textChanged();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    bool confirmUnsaved();
    bool openFile(QString newFilepath = QString());
    void reloadStrings();


    Ui::MainWindow *ui;
    QFileDialog openStx;
    QString currentFilename;
    QByteArray currentStx;
    QFrame *textBoxFrame;
    //QList<QPlainTextEdit *> textBoxes;
    bool unsavedChanges = false;
};

#endif // MAINWINDOW_H
