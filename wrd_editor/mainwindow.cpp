#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QTableView>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //ui->tableCode->setColumnWidth(1, 180);

    QStringList args = QApplication::arguments();
    if (args.count() <= 1)
        return;

    for (int i = 1; i < args.count(); ++i)
    {
        if (QFileInfo(args[i]).exists() && args[i].endsWith(".wrd", Qt::CaseInsensitive))
        {
            openFile(args[i]);
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    openFile();
}

void MainWindow::on_actionSave_triggered()
{
    saveFile(currentWrd.filename);
}

void MainWindow::on_actionSaveAs_triggered()
{
    saveFile();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsaved())
        event->ignore();
    else
        event->accept();
}

bool MainWindow::confirmUnsaved()
{
    if (!unsavedChanges) return true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Unsaved Changes",
                                  "Would you like to save your changes?",
                                  QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

    if (reply == QMessageBox::Yes)
    {
        on_actionSave_triggered();
        return true;
    }
    else if (reply == QMessageBox::No)
    {
        return true;
    }
    return false;
}

bool MainWindow::openFile(QString newFilepath)
{
    if (!confirmUnsaved()) return false;

    if (newFilepath.isEmpty())
    {
        newFilepath = QFileDialog::getOpenFileName(this, "Open WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    }
    if (newFilepath.isEmpty()) return false;

    QFile f(newFilepath);
    if (!f.open(QFile::ReadOnly)) return false;

    currentWrd = wrd_from_bytes(f.readAll(), newFilepath);
    f.close();

    this->setWindowTitle("WRD Editor: " + QFileInfo(newFilepath).fileName());

    //reloadLabelList();
    WrdUiModel *code = new WrdUiModel(this, &currentWrd, 0);
    WrdUiModel *params = new WrdUiModel(this, &currentWrd, 1);
    WrdUiModel *strings = new WrdUiModel(this, &currentWrd, 2);
    QObject::connect(code, &WrdUiModel::editCompleted, this, &MainWindow::on_editCompleted);
    QObject::connect(params, &WrdUiModel::editCompleted, this, &MainWindow::on_editCompleted);
    QObject::connect(strings, &WrdUiModel::editCompleted, this, &MainWindow::on_editCompleted);
    ui->tableCode->setModel(code);
    ui->tableParams->setModel(params);
    ui->tableStrings->setModel(strings);

    ui->centralWidget->setEnabled(true);
    ui->tableCode->scrollToTop();
    ui->tableParams->scrollToTop();
    ui->tableStrings->scrollToTop();

    // Manually trigger this to check for undocumented command params right away
    //on_comboBox_SelectLabel_currentIndexChanged(0);
    return true;
}

bool MainWindow::saveFile(QString newFilepath)
{
    if (newFilepath.isEmpty())
    {
        newFilepath = QFileDialog::getSaveFileName(this, "Save WRD file", QString(), "WRD files (*.wrd);;All files (*.*)");
    }
    if (newFilepath.isEmpty()) return false;


    /*
    currentWrd.labels.clear();
    for (int i = 0; i < ui->comboBox_SelectLabel->count(); i++)
    {
        currentWrd.labels.append(ui->comboBox_SelectLabel->itemText(i));
    }
    */

    QFile f(newFilepath);
    if (!f.open(QFile::WriteOnly)) return false;

    const QByteArray out_data = wrd_to_bytes(currentWrd);
    f.write(out_data);
    f.close();

    // If the strings are internal, we've saved them already in wrd_to_bytes()
    if (currentWrd.external_strings && currentWrd.strings.count() > 0)
    {
        // Strings are stored in the "(current spc name)_text_(region).spc" file,
        // within an STX file with the same name as the current WRD file.
        QString stx_file = QFileInfo(newFilepath).absolutePath();
        if (stx_file.endsWith(".SPC", Qt::CaseInsensitive))
            stx_file.chop(4);

        QString region = "_US";
        if (stx_file.right(3).startsWith("_"))
        {
            region = stx_file.right(3);
            stx_file.chop(3);
        }

        stx_file.append("_text" + region + ".SPC");
        stx_file.append(QDir::separator());
        stx_file.append(QFileInfo(newFilepath).fileName());
        stx_file.replace(".wrd", ".stx");

        const QByteArray stx_data = repack_stx_strings(currentWrd.strings);
        QFile f2(stx_file);
        f2.open(QFile::WriteOnly);
        f2.write(stx_data);
        f2.close();
    }

    currentWrd.filename = newFilepath;
    this->setWindowTitle("WRD Editor: " + QFileInfo(newFilepath).fileName());

    unsavedChanges = false;
    return true;
}

/*
void MainWindow::reloadLabelList()
{
    ui->comboBox_SelectLabel->blockSignals(true);
    ui->tableCode->blockSignals(true);
    ui->comboBox_SelectLabel->clear();

    for (QString label_name : currentWrd.labels)
    {
        ui->comboBox_SelectLabel->addItem(label_name);
    }

    ui->tableCode->blockSignals(false);
    ui->comboBox_SelectLabel->blockSignals(false);
}
*/

/*
void MainWindow::on_comboBox_SelectLabel_currentIndexChanged(int index)
{
    ui->tableCode->setModel(new WrdUiModel(this, &currentWrd, 0, index));
    ui->tableCode->scrollToTop();

    for (int i = 0; i < currentWrd.code.at(index).count(); i++)
    {
        const WrdCmd cmd = currentWrd.code.at(index).at(i);

        if (cmd.arg_types.count() != cmd.args.count())
        {
            if (cmd.opcode != 0x01 && cmd.opcode != 0x03) // IFF/IFW command can have a variable number of parameters
            {
                QMessageBox errorMsg(QMessageBox::Information,
                                     "Unexpected Command Parameters",
                                     "Opcode " + num_to_hex(cmd.opcode, 2) + " expected " + QString::number(cmd.arg_types.count()) + " args, but found " + QString::number(cmd.args.count()) + ".",
                                     QMessageBox::Ok);
                errorMsg.exec();
            }

            for (int j = cmd.arg_types.count(); j < cmd.args.count(); j++)
            {
                currentWrd.code[index][i].arg_types.append(0);
            }
        }
    }
}
*/



void MainWindow::on_editCompleted(const QString & /*str*/)
{
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Add_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    table->model()->insertRow(row + 1);
    table->selectRow(row + 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Del_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    table->model()->removeRow(row);
    if (row - 1 >= 0)
        table->selectRow(row - 1);
    else
        table->selectRow(row);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Up_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    if (row - 1 < 0)
        return;

    table->model()->moveRow(QModelIndex(), row, QModelIndex(), row - 1);
    table->selectRow(row - 1);
    unsavedChanges = true;
}

void MainWindow::on_toolButton_Down_clicked()
{
    QTableView *table = ui->tabWidget->currentWidget()->findChild<QTableView *>(QString(), Qt::FindDirectChildrenOnly);
    int row = table->currentIndex().row();
    if (row + 1 >= table->model()->rowCount())
        return;

    table->model()->moveRow(QModelIndex(), row, QModelIndex(), row + 1);
    table->selectRow(row + 1);
    unsavedChanges = true;
}



void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.count(); i++)
        {
            QString filepath = urlList.at(i).toLocalFile();
            if (openFile(filepath))
            {
                event->acceptProposedAction();
                break;
            }
        }
    }
}

void MainWindow::on_actionUS_toggled(bool checked)
{
    if (checked == false)
    {
        // Make sure at least one checkbox is active at all times
        if (!ui->actionJP->isChecked() && !ui->actionFR->isChecked()
                && !ui->actionZH->isChecked() && !ui->actionCN->isChecked())
        {
            ui->actionUS->setChecked(true);
        }
    }
    else
    {
        ui->actionJP->setChecked(false);
        ui->actionFR->setChecked(false);
        ui->actionZH->setChecked(false);
        ui->actionCN->setChecked(false);
    }
}

void MainWindow::on_actionJP_toggled(bool checked)
{
    if (checked == false)
    {
        // Make sure at least one checkbox is active at all times
        if (!ui->actionUS->isChecked() && !ui->actionFR->isChecked()
                && !ui->actionZH->isChecked() && !ui->actionCN->isChecked())
        {
            ui->actionJP->setChecked(true);
        }
    }
    else
    {
        ui->actionUS->setChecked(false);
        ui->actionFR->setChecked(false);
        ui->actionZH->setChecked(false);
        ui->actionCN->setChecked(false);
    }
}

void MainWindow::on_actionFR_toggled(bool checked)
{
    if (checked == false)
    {
        // Make sure at least one checkbox is active at all times
        if (!ui->actionJP->isChecked() && !ui->actionUS->isChecked()
                && !ui->actionZH->isChecked() && !ui->actionCN->isChecked())
        {
            ui->actionFR->setChecked(true);
        }
    }
    else
    {
        ui->actionJP->setChecked(false);
        ui->actionUS->setChecked(false);
        ui->actionZH->setChecked(false);
        ui->actionCN->setChecked(false);
    }
}

void MainWindow::on_actionZH_toggled(bool checked)
{
    if (checked == false)
    {
        // Make sure at least one checkbox is active at all times
        if (!ui->actionJP->isChecked() && !ui->actionFR->isChecked()
                && !ui->actionUS->isChecked() && !ui->actionCN->isChecked())
        {
            ui->actionZH->setChecked(true);
        }
    }
    else
    {
        ui->actionJP->setChecked(false);
        ui->actionFR->setChecked(false);
        ui->actionUS->setChecked(false);
        ui->actionCN->setChecked(false);
    }
}

void MainWindow::on_actionCN_toggled(bool checked)
{
    if (checked == false)
    {
        // Make sure at least one checkbox is active at all times
        if (!ui->actionJP->isChecked() && !ui->actionFR->isChecked()
                && !ui->actionZH->isChecked() && !ui->actionUS->isChecked())
        {
            ui->actionCN->setChecked(true);
        }
    }
    else
    {
        ui->actionJP->setChecked(false);
        ui->actionFR->setChecked(false);
        ui->actionZH->setChecked(false);
        ui->actionUS->setChecked(false);
    }
}
