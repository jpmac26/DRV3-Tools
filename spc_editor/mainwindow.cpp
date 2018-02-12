#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
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

void MainWindow::reloadSubfileList()
{
    QStringList items;
    for (SpcSubfile subfile : currentSpc.subfiles)
    {
        items.append(subfile.filename);
    }

    ui->listWidget->clear();
    ui->listWidget->addItems(items);
    //ui->listWidget->updateGeometry();
    ui->listWidget->repaint();
}

void MainWindow::extractFile(QString outDir, SpcSubfile subfile)
{
    switch (subfile.cmp_flag)
    {
    case 0x01:  // Uncompressed, don't do anything
        break;

    case 0x02:  // Compressed
        subfile.data = spc_dec(subfile.data);

        if (subfile.data.size() != subfile.dec_size)
        {
            qDebug() << "Error: Size mismatch, size was " << subfile.data.size() << " but should be " << subfile.dec_size;
        }
        break;

    case 0x03:  // Load from external file
        QString ext_file_name = currentSpc.filename + "_" + subfile.filename;
        QFile ext_file(ext_file_name);
        ext_file.open(QFile::ReadOnly);
        BinaryData ext_data(ext_file.readAll());
        ext_file.close();
        subfile.data = srd_dec(ext_data);
        break;
    }

    QFile out(outDir + QDir::separator() + subfile.filename);
    out.open(QFile::WriteOnly);
    out.write(subfile.data.Bytes);
    out.close();
}

void MainWindow::on_actionOpen_triggered()
{
    if (!confirmUnsaved()) return;

    QString newFilename = QFileDialog::getOpenFileName(this, "Open SPC file", QString(), "SPC files (*.spc);;All files (*.*)");
    if (newFilename.isEmpty())
    {
        return;
    }
    currentSpc.filename = newFilename;
    QFile f(currentSpc.filename);
    f.open(QFile::ReadOnly);

    BinaryData fileData(f.readAll());
    fileData.Position = 0;
    QString magic = fileData.get_str(4);
    if (magic == "$CMP")
    {
        return;
        fileData = srd_dec(fileData);
        fileData.Position = 0;
        magic = fileData.get_str(4);
    }

    if (magic != SPC_MAGIC)
    {
        QMessageBox::critical(this, "Error", "Invalid SPC file.");
        return;
    }

    currentSpc.unk1 = fileData.get(0x24);
    uint file_count = fileData.get_u32();
    currentSpc.unk2 = fileData.get_u32();
    fileData.Position += 0x10;   // padding

    QString table_magic = fileData.get_str(4);
    fileData.Position += 0x0C;

    if (table_magic != SPC_TABLE_MAGIC)
    {
        QMessageBox::critical(this, "Error", "Invalid SPC file.");
        return;
    }

    currentSpc.subfiles.clear();

    for (uint i = 0; i < file_count; i++)
    {
        SpcSubfile subfile;
        subfile.cmp_flag = fileData.get_u16();
        subfile.unk_flag = fileData.get_u16();
        subfile.cmp_size = fileData.get_u32();
        subfile.dec_size = fileData.get_u32();
        subfile.name_len = fileData.get_u32(); // Null terminator excluded from count
        fileData.Position += 0x10;  // padding

        // Everything's aligned to multiples of 0x10
        // We don't actually want the null terminator byte, so pretend it's padding
        uint name_padding = (0x10 - (subfile.name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - subfile.cmp_size % 0x10) % 0x10;

        subfile.filename = fileData.get_str(subfile.name_len);
        fileData.Position += name_padding + 1;

        subfile.data = BinaryData(fileData.get(subfile.cmp_size));
        fileData.Position += data_padding;

        currentSpc.subfiles.append(subfile);
    }
    f.close();

    this->setWindowTitle("SPC Editor: " + currentSpc.filename);
    reloadSubfileList();
}

void MainWindow::on_actionSave_triggered()
{
    QByteArray outData;

    outData.append(SPC_MAGIC.toUtf8());                     // SPC_MAGIC
    outData.append(currentSpc.unk1);                        // unk1
    outData.append(from_u32(currentSpc.subfiles.count()));  // file_count
    outData.append(from_u32(currentSpc.unk2));              // unk2
    outData.append(QByteArray(0x10, 0x00));                 // padding
    outData.append(SPC_TABLE_MAGIC.toUtf8());               // SPC_TABLE_MAGIC
    outData.append(QByteArray(0x0C, 0x00));                 // padding

    for (SpcSubfile subfile : currentSpc.subfiles)
    {
        outData.append(from_u16(subfile.cmp_flag));         // cmp_flag
        outData.append(from_u16(subfile.unk_flag));         // unk_flag
        outData.append(from_u32(subfile.cmp_size));         // cmp_size
        outData.append(from_u32(subfile.dec_size));         // dec_size
        outData.append(from_u32(subfile.name_len));         // name_len
        outData.append(QByteArray(0x10, 0x00));             // padding

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (subfile.name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - subfile.cmp_size % 0x10) % 0x10;

        outData.append(subfile.filename.toUtf8());
        // Add the null terminator byte to the padding
        outData.append(QByteArray(name_padding + 1, 0x00));

        outData.append(subfile.data.Bytes);                 // data
        outData.append(QByteArray(data_padding, 0x00));     // data_padding
    }

    QString outName = currentSpc.filename;
    QFile f(outName);
    f.open(QFile::WriteOnly);
    f.write(outData);
    f.close();
    unsavedChanges = false;
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString newFilename = QFileDialog::getSaveFileName(this, "Save SPC file", QString(), "SPC files (*.spc);;All files (*.*)");
    if (newFilename.isEmpty())
        return;

    currentSpc.filename = newFilename;
    on_actionSave_triggered();
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionExtractAll_triggered()
{
    QString outDir = QFileDialog::getExistingDirectory();
    if (outDir.isEmpty()) return;

    QProgressDialog progressDlg;
    progressDlg.setWindowFlag(Qt::WindowCloseButtonHint, false);
    progressDlg.setCancelButton(0);
    progressDlg.setMaximum(currentSpc.subfiles.count() - 1);
    progressDlg.show();

    bool overwriteAll = false;
    bool skipAll = false;
    for (int i = 0; i < currentSpc.subfiles.count(); i++)
    {
        SpcSubfile subfile = currentSpc.subfiles[i];

        progressDlg.setLabelText("Extracting file " + QString::number(i + 1) + "/" + QString::number(currentSpc.subfiles.count()) + ": " + subfile.filename);
        progressDlg.setValue(i);

        if (QFile(outDir + QDir::separator() + subfile.filename).exists())
        {
            QMessageBox::StandardButton reply;
            if (!overwriteAll && !skipAll)
            {
                reply = QMessageBox::question(&progressDlg, "Confirm overwrite",
                                              subfile.filename + " already exists in this location. Would you like to overwrite it?",
                                              QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::No|QMessageBox::NoToAll|QMessageBox::Cancel);

                if (reply == QMessageBox::Cancel) break;

                overwriteAll = (reply == QMessageBox::YesToAll);
                skipAll = (reply == QMessageBox::NoToAll);
            }

            if (skipAll || reply == QMessageBox::No)
            {
                continue;
            }
        }

        // We also reach this code if we choose "Yes" or "YesToAll" on the overwrite confirmation dialog
        extractFile(outDir, subfile);
    }
    progressDlg.close();
}

void MainWindow::on_actionExtractSelected_triggered()
{
    QFileDialog folderDlg;
    QString outDir = folderDlg.getExistingDirectory();
    if (outDir.isEmpty()) return;

    QModelIndexList selectedIndexes = ui->listWidget->selectionModel()->selectedIndexes();

    QProgressDialog progressDlg;
    progressDlg.setWindowFlag(Qt::WindowCloseButtonHint, false);
    progressDlg.setCancelButton(0);
    progressDlg.setMaximum(selectedIndexes.count());
    progressDlg.show();

    bool overwriteAll = false;
    bool skipAll = false;
    for (int i = 0; i < selectedIndexes.count(); i++)
    {
        int index = ui->listWidget->selectionModel()->selectedIndexes()[i].row();
        SpcSubfile subfile = currentSpc.subfiles[index];

        progressDlg.setLabelText("Extracting file " + QString::number(i + 1) + "/" + QString::number(selectedIndexes.count()) + ": " + subfile.filename);
        progressDlg.setValue(i);

        if (QFile(outDir + QDir::separator() + subfile.filename).exists())
        {
            QMessageBox::StandardButton reply;
            if (!overwriteAll && !skipAll)
            {
                reply = QMessageBox::question(&progressDlg, "Confirm overwrite",
                                              subfile.filename + " already exists in this location. Would you like to overwrite it?",
                                              QMessageBox::Yes|QMessageBox::YesToAll|QMessageBox::No|QMessageBox::NoToAll|QMessageBox::Cancel);

                if (reply == QMessageBox::Cancel) break;

                overwriteAll = (reply == QMessageBox::YesToAll);
                skipAll = (reply == QMessageBox::NoToAll);
            }

            if (skipAll || reply == QMessageBox::No)
            {
                continue;
            }
        }

        // We also reach this code if we choose "Yes" or "YesToAll" on the overwrite confirmation dialog
        extractFile(outDir, subfile);
    }
    progressDlg.close();
}

void MainWindow::on_actionInjectFile_triggered()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    QString injectName = dialog.getOpenFileName(this, "Select file to inject", QString(), "All files (*.*);;SPC files (*.spc);;STX files (*.stx);;SRD files (*.srd);;WRD files (*.wrd)");
    if (injectName.isEmpty())
    {
        return;
    }

    QFile file(injectName);
    file.open(QFile::ReadOnly);
    QByteArray fileData = file.readAll();
    file.close();

    injectFile(QFileInfo(file).fileName(), fileData);
}

void MainWindow::injectFile(QString name, QByteArray fileData)
{
    SpcSubfile injectFile;

    injectFile.filename = name;
    injectFile.data = BinaryData(fileData);
    injectFile.data.Position = 0;

    for (int i = 0; i < currentSpc.subfiles.size(); i++)
    {
        if (currentSpc.subfiles[i].filename == injectFile.filename)
        {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::warning(this, "Confirm Import File",
                                          injectFile.filename + " already exists.\nDo you want to replace it?",
                                          QMessageBox::Yes|QMessageBox::No);

            if (reply != QMessageBox::Yes) return;

            currentSpc.subfiles[i].data = injectFile.data;
            currentSpc.subfiles[i].dec_size = currentSpc.subfiles[i].data.size();

            reply = QMessageBox::question(this, "Compress Input File",
                                          "Does this file need to be re-compressed?\nIf unsure, choose \"Yes\".",
                                          QMessageBox::Yes|QMessageBox::No);

            if (reply == QMessageBox::Yes)
            {
                currentSpc.subfiles[i].cmp_flag = 0x02;
                currentSpc.subfiles[i].data = spc_cmp(currentSpc.subfiles[i].data);
            }
            else
            {
                currentSpc.subfiles[i].cmp_flag = 0x01;
            }
            currentSpc.subfiles[i].cmp_size = currentSpc.subfiles[i].data.size();

            unsavedChanges = true;
            reloadSubfileList();
            return;
        }
    }


    injectFile.dec_size = injectFile.data.size();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Compress Input File",
                                  "Does this file need to be re-compressed?\nIf unsure, choose \"Yes\".",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        injectFile.cmp_flag = 0x02;
        injectFile.data = spc_cmp(injectFile.data);
    }
    else
    {
        injectFile.cmp_flag = 0x01;
    }
    injectFile.cmp_size = injectFile.data.size();
    injectFile.unk_flag = 0x01;
    injectFile.name_len = injectFile.filename.length();

    currentSpc.subfiles.append(injectFile);
    unsavedChanges = true;
    reloadSubfileList();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!confirmUnsaved())
        event->ignore();
    else
        event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();

        for (auto it = urlList.begin(); it != urlList.end(); it++)
        {
            QFile f(((QUrl)*it).toLocalFile());
            f.open(QFile::ReadOnly);
            QString name = QFileInfo(f).fileName();
            QByteArray fileData = f.readAll();
            f.close();

            injectFile(name, fileData);
        }

        event->acceptProposedAction();
    }
}
