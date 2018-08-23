#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QString>
#include <QString>
#include <QDir>
#include <QFileInfoList>
#include <QPixmap>
#include <QPainter>
#include <QProcess>

#include <vector>

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
    void on_base_image_clicked();

    void on_paste_image_clicked();

    void on_merge_button_clicked();

private:
    std::vector<char *> supportedFormats = { "PNG", "JPG", "JPEG", "BMP", "PPM", "XBM" };

    Ui::MainWindow *ui;

    QString baseFile;
    QString pasteImage;

    QPixmap overlay;

    bool loadPasteImg();
    void pasteTo(const QString& baseImage);
    void deletePNG();
};

#endif // MAINWINDOW_H
