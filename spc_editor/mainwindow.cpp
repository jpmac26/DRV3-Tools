#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->listWidget->setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    if (!confirmUnsaved()) return;

    QString newFilename = QFileDialog::getOpenFileName(this, "Open SPC file", QString(), "SPC files (*.spc);;All files (*.*)");
    if (newFilename.isEmpty())
    {
        return;
    }

    openFile(newFilename);
}

void MainWindow::on_actionSave_triggered()
{
    QByteArray out_data;

    out_data.append(SPC_MAGIC.toUtf8());                    // SPC_MAGIC
    out_data.append(currentSpc.unk1);                       // unk1
    out_data.append(num_to_bytes(currentSpc.subfiles.count())); // file_count
    out_data.append(num_to_bytes(currentSpc.unk2));             // unk2
    out_data.append(0x10, 0x00);                            // padding
    out_data.append(SPC_TABLE_MAGIC.toUtf8());              // SPC_TABLE_MAGIC
    out_data.append(0x0C, 0x00);                            // padding

    for (SpcSubfile subfile : currentSpc.subfiles)
    {
        out_data.append(num_to_bytes(subfile.cmp_flag));         // cmp_flag
        out_data.append(num_to_bytes(subfile.unk_flag));         // unk_flag
        out_data.append(num_to_bytes(subfile.cmp_size));         // cmp_size
        out_data.append(num_to_bytes(subfile.dec_size));         // dec_size
        out_data.append(num_to_bytes(subfile.name_len));         // name_len
        out_data.append(0x10, 0x00);             // padding

        // Everything's aligned to multiples of 0x10
        uint name_padding = (0x10 - (subfile.name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - subfile.cmp_size % 0x10) % 0x10;

        out_data.append(subfile.filename.toUtf8());
        // Add the null terminator byte to the padding
        out_data.append(name_padding + 1, 0x00);

        out_data.append(subfile.data);                 // data
        out_data.append(data_padding, 0x00);     // data_padding
    }

    QString outName = currentSpc.filename;
    QFile f(outName);
    f.open(QFile::WriteOnly);
    f.write(out_data);
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

    QProgressDialog progressDlg(this);
    progressDlg.setWindowModality(Qt::WindowModal);
    progressDlg.setWindowFlags(progressDlg.windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
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

        QApplication::processEvents();

        // We also reach this code if we choose "Yes" or "YesToAll" on the overwrite confirmation dialog
        extractFile(outDir, subfile);
    }
    progressDlg.reset();
}

void MainWindow::on_actionExtractSelected_triggered()
{
    QFileDialog folderDlg;
    QString outDir = folderDlg.getExistingDirectory();
    if (outDir.isEmpty()) return;

    QModelIndexList selectedIndexes = ui->listWidget->selectionModel()->selectedIndexes();

    QProgressDialog progressDlg(this);
    progressDlg.setWindowModality(Qt::WindowModal);
    progressDlg.setWindowFlags(progressDlg.windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
    progressDlg.setCancelButton(0);
    progressDlg.setMaximum(selectedIndexes.count() - 1);
    progressDlg.show();

    bool overwriteAll = false;
    bool skipAll = false;
    for (int i = 0; i < selectedIndexes.count(); i++)
    {
        int index = ui->listWidget->selectionModel()->selectedIndexes().at(i).row();
        SpcSubfile subfile = currentSpc.subfiles.at(index);

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

        QApplication::processEvents();

        // We also reach this code if we choose "Yes" or "YesToAll" on the overwrite confirmation dialog
        extractFile(outDir, subfile);
    }
    progressDlg.reset();
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

void MainWindow::openFile(QString filename)
{
    currentSpc.filename = filename;
    QFile f(currentSpc.filename);
    f.open(QFile::ReadOnly);

    int pos = 0;
    QByteArray file_data = f.readAll();
    QString magic = bytes_to_str(file_data, pos, 4);
    if (magic == "$CMP")
    {
        return;
        file_data = srd_dec(file_data);
        pos = 0;
        magic = bytes_to_str(file_data, pos, 4);
    }

    if (magic != SPC_MAGIC)
    {
        QMessageBox::critical(this, "Error", "Invalid SPC file.");
        return;
    }

    currentSpc.unk1 = get_bytes(file_data, pos, 0x24);
    uint file_count = bytes_to_num<uint>(file_data, pos);
    currentSpc.unk2 = bytes_to_num<uint>(file_data, pos);
    pos += 0x10;   // padding

    QString table_magic = bytes_to_str(file_data, pos, 4);
    pos += 0x0C;

    if (table_magic != SPC_TABLE_MAGIC)
    {
        QMessageBox::critical(this, "Error", "Invalid SPC file.");
        return;
    }

    currentSpc.subfiles.clear();

    for (uint i = 0; i < file_count; i++)
    {
        SpcSubfile subfile;
        subfile.cmp_flag = bytes_to_num<ushort>(file_data, pos);
        subfile.unk_flag = bytes_to_num<ushort>(file_data, pos);
        subfile.cmp_size = bytes_to_num<uint>(file_data, pos);
        subfile.dec_size = bytes_to_num<uint>(file_data, pos);
        subfile.name_len = bytes_to_num<uint>(file_data, pos); // Null terminator excluded from count
        pos += 0x10;  // padding

        // Everything's aligned to multiples of 0x10
        // We don't actually want the null terminator byte, so pretend it's padding
        uint name_padding = (0x10 - (subfile.name_len + 1) % 0x10) % 0x10;
        uint data_padding = (0x10 - subfile.cmp_size % 0x10) % 0x10;

        subfile.filename = bytes_to_str(file_data, pos, subfile.name_len);
        pos += name_padding + 1;

        subfile.data = get_bytes(file_data, pos, subfile.cmp_size);
        pos += data_padding;

        currentSpc.subfiles.append(subfile);
    }
    f.close();

    this->setWindowTitle("SPC Editor: " + currentSpc.filename);
    reloadSubfileList();
}

void MainWindow::extractFile(QString outDir, const SpcSubfile &subfile)
{
    QByteArray outData;
    switch (subfile.cmp_flag)
    {
    case 0x01:  // Uncompressed, don't do anything
        outData = subfile.data;
        break;

    case 0x02:  // Compressed
        outData = spc_dec(subfile.data);

        if (outData.size() != subfile.dec_size)
        {
            qDebug() << "Error: Size mismatch, size was " << outData.size() << " but should be " << subfile.dec_size;
        }
        break;

    case 0x03:  // Load from external file
        QString ext_file_name = currentSpc.filename + "_" + subfile.filename;
        QFile ext_file(ext_file_name);
        ext_file.open(QFile::ReadOnly);
        QByteArray ext_data = ext_file.readAll();
        ext_file.close();
        outData = srd_dec(ext_data);
        break;
    }

    QFile outFile(outDir + QDir::separator() + subfile.filename);
    outFile.open(QFile::WriteOnly);
    outFile.write(outData);
    outFile.close();
}

void MainWindow::injectFile(QString name, const QByteArray &fileData)
{
    SpcSubfile injectFile;

    injectFile.filename = name;
    injectFile.data = fileData;
    injectFile.dec_size = injectFile.data.size();

    QMessageBox::StandardButton shouldCompress;
    shouldCompress = QMessageBox::question(this, "Compress Input File",
                                      "Does this file need to be re-compressed?\nIf unsure, choose \"Yes\".",
                                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);

    if (shouldCompress == QMessageBox::Cancel)
        return;

    int fileToOverwrite = -1;
    for (int i = 0; i < currentSpc.subfiles.count(); i++)
    {
        if (currentSpc.subfiles.at(i).filename == injectFile.filename)
        {
            QMessageBox::StandardButton shouldOverwrite;
            shouldOverwrite = QMessageBox::warning(this, "Confirm Overwrite File",
                                              injectFile.filename + " already exists.\nDo you want to replace it?",
                                              QMessageBox::Yes|QMessageBox::No);

            if (shouldOverwrite != QMessageBox::Yes)
                return;

            fileToOverwrite = i;
            break;
        }
    }

    if (shouldCompress == QMessageBox::Yes)
    {
        QProgressDialog progressDlg("Compressing file, please wait...", QString(), 0, 0, this);
        progressDlg.setWindowModality(Qt::WindowModal);
        progressDlg.setWindowFlags(progressDlg.windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);

        QFutureWatcher<QByteArray> cmp_watcher;
        QObject::connect(&cmp_watcher, &QFutureWatcher<void>::finished, &progressDlg, &QProgressDialog::reset);

        cmp_watcher.setFuture(QtConcurrent::run(&spc_cmp, injectFile.data));
        progressDlg.exec();
        cmp_watcher.waitForFinished();

        injectFile.cmp_flag = 0x02;
        injectFile.data = cmp_watcher.result();
    }
    else if (shouldCompress == QMessageBox::No)
    {
        injectFile.cmp_flag = 0x01;
    }
    injectFile.cmp_size = injectFile.data.size();
    injectFile.unk_flag = 0x01;
    injectFile.name_len = injectFile.filename.length();

    if (fileToOverwrite > -1)
        currentSpc.subfiles[fileToOverwrite] = injectFile;
    else
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

            if (currentSpc.filename.isEmpty())
                openFile(filepath);
            else
            {
                QFile f(filepath);
                f.open(QFile::ReadOnly);
                QString name = QFileInfo(f).fileName();
                QByteArray fileData = f.readAll();
                f.close();

                injectFile(name, fileData);
            }
        }

        event->acceptProposedAction();
    }
}
