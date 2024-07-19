/**
   @author Kenta Suzuki
*/

#ifndef rqt_ur__mainwindow_H
#define rqt_ur__mainwindow_H

#include <QMainWindow>

namespace rqt_ur {

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    class Impl;
    Impl* impl;
};

}

#endif // rqt_ur__mainwindow_H
