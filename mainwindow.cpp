#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_base_image_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                tr("打开PDF文件"), "",
                tr("PDF文件 (*.pdf *.PDF)"));

    if (fileName.length() == 0) {
        return;
    }

    QProcess process;
    QString command = "identify -format \"%n\n\" \"" + fileName + "\"";

//    cerr << command.toStdString() << endl;

    process.start(command);
    process.waitForFinished(TIMEOUT/2);

    QString output = QString(process.readAllStandardOutput());
//    cerr << output.toStdString() << endl;
    ui->page_end->setValue(output.split("\n").first().toInt());

    baseFile = fileName;
    ui->base_label->setText(fileName.split("/").last());
    ui->textBrowser->append("要修改的图像: " + fileName);
}



void MainWindow::on_paste_image_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                tr("打开照片"), "",
                tr("照片 (*.png *.PNG *.jpg *.JPG *.jpeg *.JPEG *.bmp *.BMP *.ppm *.PPM *.xbm *.XBM)"));

    if (fileName.length() == 0) {
        return;
    }

    overlay = QPixmap(fileName);
    if (overlay.isNull()) {
        if (!loadPasteImg(fileName)) {
            ui->textBrowser->append("<br><b>错误: 无法打开 " + fileName + " 或不支持的格式！</b>");
            return;
        }
    }

    pasteImage = fileName;
    ui->paste_label->setText(fileName.split("/").last());
    ui->textBrowser->append("要粘贴的图像: " + fileName);
}

void MainWindow::on_merge_button_clicked()
{
    if (pasteImage.isEmpty() || baseFile.isEmpty()) {
        QString error = (pasteImage.isEmpty()) ? "没有要粘贴的图像！" : "没有要修改的图像！";
        ui->textBrowser->append(error);
        return;
    }

    QString baseName = ui->base_label->text().split(".").first();
    ui->textBrowser->append("在把 " + ui->base_label->text() + " 转换为照片 ...");

    QProcess process;
    QString command = "pdftopng.exe -r 300 \"" + baseFile + "\" \"" + baseName + ".png\"";
//    cerr << command.toStdString() << endl;

    process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    process.start(command);
    process.waitForFinished(TIMEOUT * 120);

    QString errors = QString(process.readAllStandardError());
//    cout << process.readAllStandardOutput().toStdString() << endl;

    if (errors.toLower().contains("invalid") || errors.toLower().contains("unable to open")) {
        ui->textBrowser->append("<br>这个操作有错误:");
        ui->textBrowser->append(errors);

        deletePNG();
        return;
    }
    else if (!errors.isEmpty()) {
        ui->textBrowser->append("<br>这个操作有警告:");
        ui->textBrowser->append(errors);
    }

    QStringList filters;
    filters << (baseName + "*.png");
    QDir dir(QCoreApplication::applicationDirPath());
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files|QDir::NoDotAndDotDot);

    if (files.size() < ui->page_end->value()) {
        ui->textBrowser->append("<br><b>错误: 页面结束比 " + baseFile + " 页数大！</b>");
        deletePNG();
        return;
    }

    ui->textBrowser->append("<br>在改照片 ...");
    for(size_t i = ui->page_start->value() - 1; i < ui->page_end->value(); ++i) {
//            cerr << files[i].fileName().toStdString() << endl;
        ui->textBrowser->append(files[i].fileName() + " ...");
        pasteTo(QCoreApplication::applicationDirPath() + "\\" + files[i].fileName());
    }

    QString newName = baseFile.split(".").first() + "_NEW.pdf";
    ui->textBrowser->append("<br>在把照片转换为 <b>" + newName + "</b> ...");
    command = "magick \"" + baseName + "*.png\" -units PixelsPerInch -quality 100 -density 300 \"" + newName + "\"";
    process.start(command);
    process.waitForFinished(TIMEOUT * 60);

    errors = QString(process.readAllStandardError());
    //    cout << process.readAllStandardOutput().toStdString() << endl;

    if (errors.toLower().contains("invalid") || errors.toLower().contains("unable to open")) {
        ui->textBrowser->append("<br>这个操作有错误:");
        ui->textBrowser->append(errors);

        deletePNG();
        return;
    }
    else if (!errors.isEmpty()) {
        ui->textBrowser->append("<br>这个操作有警告:");
        ui->textBrowser->append(errors);
    }

    ui->textBrowser->append("<br>完成。<br>");

    if (!(ui->checkBox->isChecked())) {
        deletePNG();
    }
}

void MainWindow::on_convert_button_clicked()
{
    if (baseFile.isEmpty()) {
        ui->textBrowser->append("没有要修改的图像！");
        return;
    }

    QString baseName = baseFile.split(".").first();
    ui->textBrowser->append("在把 " + ui->base_label->text() + " 转换为照片 ...");

    QProcess process;
    QString command = "pdftopng.exe -r 300 -f " + QString::number(ui->page_start->value()) + " -l " + QString::number(ui->page_end->value());
    command += " \"" + baseFile + "\" \"" + baseName + ".png\"";
//    cerr << command.toStdString() << endl;

    process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    process.start(command);
    process.waitForFinished(TIMEOUT * 120);

    QString errors = QString(process.readAllStandardError());
//    cout << process.readAllStandardOutput().toStdString() << endl;

    if (errors.toLower().contains("invalid") || errors.toLower().contains("unable to open")) {
        ui->textBrowser->append("<br>这个操作有错误:");
        ui->textBrowser->append(errors);
        return;
    }
    else if (!errors.isEmpty()) {
        ui->textBrowser->append("<br>这个操作有警告:");
        ui->textBrowser->append(errors);
    }

    ui->textBrowser->append("<br>图像保存到: <b>" + baseFile.left(baseFile.size() - ui->base_label->text().size()) + "</b><br>");
}


bool MainWindow::loadPasteImg(const QString& fileName) {
    for (size_t i = 0; i < supportedFormats.size(); ++i) {
        overlay = QPixmap(fileName, supportedFormats[i]);
        if (!overlay.isNull()) {
            return true;
        }
    }
    return false;
}

void MainWindow::pasteTo(const QString& baseImage) {
//    cerr << baseImage.toStdString() << endl;
    QPixmap base (baseImage, "PNG");
//    cerr << (base.isNull() == true) << endl;
    QPainter painter(&base);
    painter.drawPixmap(ui->x_coord->value(), ui->y_coord->value(), overlay);
    base.save(baseImage, "PNG");

}

void MainWindow::deletePNG() {
    QProcess process;
    process.setWorkingDirectory(QCoreApplication::applicationDirPath());
    process.start("cmd.exe");
    process.write("del *.png\n\r");
    process.write("exit\n\r");
    process.waitForFinished();
}
