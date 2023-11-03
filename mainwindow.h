#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileDialog>
#include <QNetworkInterface>
#include <QDebug>
#include <QRegularExpression>

#include "tftprequestmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class FileHandler : public QObject
{
    Q_OBJECT

public:
    FileHandler(const QString& filePath);
    ~FileHandler();

    void writeToLogFile(const QString& content);

signals:
    void fileOpenErrorOccurred(const QString& errorMessage);

private:
    QFile file;
    QTextStream stream;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QMap<QString, QString> getIPAddress();

private slots:
    void onLogOpenFailed(const QString& errorMessage);
    void onLogReceived(const QString& log);
    void onServerIPEditFinished();
    void onRemoteFilePathEditFinished();
    void onPortEditFinished();
    void onGetButtonClicked();
    void onPutButtonClicked();

private:
    Ui::MainWindow *ui;
    // QUdpSocket* udpSocket;
    FileHandler* pf;
    QMap<QString, QString>* pNetwork;
    TftpRequestManager* pTrmanager;
};

#endif // MAINWINDOW_H
