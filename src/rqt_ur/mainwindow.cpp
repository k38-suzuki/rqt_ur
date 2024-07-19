/**
   @author Kenta Suzuki
*/

#include "rqt_ur/mainwindow.h"

#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <control_msgs/FollowJointTrajectoryAction.h>
#include <control_msgs/FollowJointTrajectoryActionGoal.h>
#include <trajectory_msgs/JointTrajectory.h>
#include <trajectory_msgs/JointTrajectoryPoint.h>

#include <QAction>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QLabel>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtMath>

#include "rqt_ur/json_archive.h"

namespace {

const QStringList nameList = {
    "shoulder_pan_joint", "shoulder_lift_joint", "elbow_joint",
    "wrist_1_joint", "wrist_2_joint", "wrist_3_joint"
};

const QStringList labelList = {
    "joint 1 [deg]", "joint 2 [deg]", "joint 3 [deg]", "joint 4 [deg]",
    "joint 5 [deg]", "joint 6 [deg]", "duration [s]"
};

}

namespace rqt_ur {

class ActionConfigDialog : public QDialog
{
public:
    ActionConfigDialog(QWidget* parent = nullptr);

    void setDuration(const double& duration) { durationSpin->setValue(duration); }
    double duration() const { return durationSpin->value(); }

private:

    QDoubleSpinBox* durationSpin;
    QDialogButtonBox* buttonBox;
};

class MainWindow::Impl : public JsonArchive
{
public:
    MainWindow* self;

    Impl(MainWindow* self);

    void open();
    void save();
    void add();
    void remove();
    void play();
    void config();

    void on_positionsTree_itemChanged(QTreeWidgetItem* item, int column);

    void createActions();
    void createToolBars();

    virtual void read(const QJsonObject& json) override;
    virtual void write(QJsonObject& json) override;

    QAction* openAct;
    QAction* saveAct;
    QAction* addAct;
    QAction* removeAct;
    QAction* playAct;
    QAction* configAct;

    QTreeWidget* positionsTree;

    double duration;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    impl = new Impl(this);
}

MainWindow::Impl::Impl(MainWindow* self)
    : self(self)
    , duration(30.0)
{
    QWidget* widget = new QWidget;
    self->setCentralWidget(widget);

    createActions();
    createToolBars();

    self->setWindowTitle("Universal Robots");

    positionsTree = new QTreeWidget;
    positionsTree->setHeaderLabels(labelList);
    positionsTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    self->connect(positionsTree, &QTreeWidget::itemChanged,
        [&](QTreeWidgetItem* item, int column){ on_positionsTree_itemChanged(item, column); });

    auto layout = new QVBoxLayout;
    layout->addWidget(positionsTree);
    widget->setLayout(layout);
}

MainWindow::~MainWindow()
{
    delete impl;
}

void MainWindow::Impl::open()
{
    static QString dir = "/home";
    QString fileName = QFileDialog::getOpenFileName(self, "Open File",
        dir,
        "JSON Files (*.json);;All Files (*)");

    if(fileName.isEmpty()) {
        return;
    } else {
        QFileInfo info(fileName);
        dir = info.absolutePath();
        loadFile(fileName);
    }
}

void MainWindow::Impl::save()
{
    static QString dir = "/home";
    QString fileName = QFileDialog::getSaveFileName(self, "Save File",
        dir,
        "JSON Files (*.json);;All Files (*)");

    if(fileName.isEmpty()) {
        return;
    } else {
        QFileInfo info(fileName);
        dir = info.absolutePath();
        saveFile(fileName);
    }
}

void MainWindow::Impl::add()
{
    int index = positionsTree->indexOfTopLevelItem(positionsTree->currentItem());

    QTreeWidgetItem* item = new QTreeWidgetItem;
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    for(int i = 0; i < 6; ++i) {
        item->setText(i, "0");
    }
    item->setText(6, "5");

    QList<QTreeWidgetItem*> selectedItems = positionsTree->selectedItems();
    if(selectedItems.size() == 0) {
        positionsTree->addTopLevelItem(item);
    } else {
        positionsTree->insertTopLevelItem(index, item);
    }
    positionsTree->setCurrentItem(item);
}

void MainWindow::Impl::remove()
{
    QList<QTreeWidgetItem*> selectedItems = positionsTree->selectedItems();
    for(int i = 0; i < selectedItems.size(); ++i) {
        QTreeWidgetItem* item = selectedItems.at(i);
        int index = positionsTree->indexOfTopLevelItem(item);
        positionsTree->takeTopLevelItem(index);
    }
}

void MainWindow::Impl::play()
{
    // create the action client
    // true causes the client to spin its own thread
    actionlib::SimpleActionClient<control_msgs::FollowJointTrajectoryAction> ac("/scaled_pos_joint_traj_controller/follow_joint_trajectory", true);

    ROS_INFO("Waiting for action server to start.");
    // wait for the action server to start
    ac.waitForServer();

    ROS_INFO("Action server started, sending goal.");
    // send a goal to the action
    control_msgs::FollowJointTrajectoryGoal goal;
    for(int i = 0; i < nameList.size(); ++i) {
        goal.trajectory.joint_names.push_back(nameList.at(i).toStdString());
    }
    int count = positionsTree->topLevelItemCount();
    goal.trajectory.points.resize(count);
    for(int i = 0; i < count; ++i) {
        QTreeWidgetItem* item = positionsTree->topLevelItem(i);
        // trajectory_msgs::JointTrajectoryPoint 
        goal.trajectory.points[i].positions.resize(6);
        goal.trajectory.points[i].velocities.resize(6);
        for(int j = 0; j < 6; ++j) {
            goal.trajectory.points[i].positions[j] = item->text(j).toDouble() * M_PI / 180.0;
            goal.trajectory.points[i].velocities[j] = 0.0;
        }
        goal.trajectory.points[i].time_from_start = ros::Duration(item->text(6).toDouble());
    }
    ac.sendGoal(goal);

    //wait for the action to return
    bool finished_before_timeout = ac.waitForResult(ros::Duration(duration));

    if(finished_before_timeout) {
        actionlib::SimpleClientGoalState state = ac.getState();
        ROS_INFO("Action finished: %s",state.toString().c_str());
    } else {
        ROS_INFO("Action did not finish before the time out.");
    }
    // exit
}

void MainWindow::Impl::config()
{
    ActionConfigDialog dialog(self);
    dialog.setDuration(duration);

    if(dialog.exec()) {
        duration = dialog.duration();
    }
}

void MainWindow::Impl::on_positionsTree_itemChanged(QTreeWidgetItem* item, int column)
{
    bool ok;
    double d = item->text(column).toDouble(&ok);
    if(ok) {
        if(column < 6) {
            d = d > 363.0 ? 363.0 : d;
            d = d < -363.0 ? -363.0 : d;
        } else {
            d = d < 0.0 ? 0.0 : d;
        }
    } else {
        d = 0.0;
    }
    
    QString text = QString("%1").arg(d);
    positionsTree->blockSignals(true);
    item->setText(column, text);
    positionsTree->blockSignals(false);
}

void MainWindow::Impl::createActions()
{
    const QIcon openIcon = QIcon::fromTheme("document-open");
    openAct = new QAction(openIcon, "&Open...", self);
    openAct->setStatusTip("Open an existing file");
    self->connect(openAct, &QAction::triggered, [&](){ open(); });

    const QIcon saveIcon = QIcon::fromTheme("document-save");
    saveAct = new QAction(saveIcon, "&Save", self);
    saveAct->setStatusTip("Save the document to disk");
    self->connect(saveAct, &QAction::triggered, [&](){ save(); });

    const QIcon addIcon = QIcon::fromTheme("list-add");
    addAct = new QAction(addIcon, "&Add", self);
    addAct->setStatusTip("Add a waypoint");
    self->connect(addAct, &QAction::triggered, [&](){ add(); });

    const QIcon removeIcon = QIcon::fromTheme("list-remove");
    removeAct = new QAction(removeIcon, "&Remove", self);
    removeAct->setStatusTip("Remove the waypoint");
    self->connect(removeAct, &QAction::triggered, [&](){ remove(); });

    const QIcon playIcon = QIcon::fromTheme("media-playback-start");
    playAct = new QAction(playIcon, "&Play", self);
    playAct->setStatusTip("Play the action");
    self->connect(playAct, &QAction::triggered, [&](){ play(); });

    const QIcon configIcon = QIcon::fromTheme("preferences-system");
    configAct = new QAction(configIcon, "&Config", self);
    configAct->setStatusTip("Show the config dialog");
    self->connect(configAct, &QAction::triggered, [&](){ config(); });
}

void MainWindow::Impl::createToolBars()
{
    QToolBar* urToolBar = self->addToolBar("UR");
    urToolBar->addAction(openAct);
    urToolBar->addAction(saveAct);
    urToolBar->addSeparator();
    urToolBar->addAction(addAct);
    urToolBar->addAction(removeAct);
    urToolBar->addAction(playAct);
    urToolBar->addAction(configAct);
    urToolBar->setObjectName("URToolBar");
}

void MainWindow::Impl::read(const QJsonObject& json)
{
    duration = get(json, "duration", 30.0);
    int size = get(json, "size", 0);

    for(int i = 0; i < size; ++i) {
        QString key = QString("positions_%1").arg(i);
        if(json.contains(key) && json[key].isArray()) {
            QJsonArray waypointArray = json[key].toArray();
            QTreeWidgetItem* item = new QTreeWidgetItem(positionsTree);
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            for(int j = 0; j < 7; ++j) {
                item->setText(j, QString("%1").arg(waypointArray[j].toDouble()));
            }
        }
    }
}

void MainWindow::Impl::write(QJsonObject& json)
{
    json["duration"] = duration;
    int size = positionsTree->topLevelItemCount();
    json["size"] = size;

    for(int i = 0; i < size; ++i) {
        QString key = QString("positions_%1").arg(i);
        QTreeWidgetItem* item = positionsTree->topLevelItem(i);
        QJsonArray waypointArray;
        for(int j = 0; j < 7; ++j) {
            waypointArray.append(item->text(j).toDouble());
        }
        json[key] = waypointArray;
    }
}

ActionConfigDialog::ActionConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    durationSpin = new QDoubleSpinBox;

    auto formLayout = new QFormLayout;
    formLayout->addRow("Duration [s]", durationSpin);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, [&](){ accept(); });
    connect(buttonBox, &QDialogButtonBox::rejected, [&](){ reject(); });

    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    setWindowTitle("Action Config");
}

}
