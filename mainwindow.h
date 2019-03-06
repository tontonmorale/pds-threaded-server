#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <myserver.h>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool showWindow;

public slots:
    void serverError();

private:
    Ui::MainWindow *ui;
    MyServer server;
};

#endif // MAINWINDOW_H
