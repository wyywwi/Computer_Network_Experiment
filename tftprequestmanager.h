#ifndef TFTPREQUESTMANAGER_H
#define TFTPREQUESTMANAGER_H

#include <QObject>
#include <QUdpSocket>
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDialog>
#include <QNetworkInterface>
#include <QDebug>
#include <QFileInfo>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QTimer>


class TftpRequestManager : public QObject
{
    Q_OBJECT

public:
    TftpRequestManager(QObject *parent = nullptr);
    ~TftpRequestManager();

    void SetRRQparam(QString inSavePath, QString inRemoteFilePath, QString inhostip, QString inserverip, QString inhostport, QString intransmode);
    void SetWRQparam(QString inFilePath, QString inhostip, QString inserverip, QString inhostport, QString intransmode);
    int TFTPReadFile();
    int TFTPWriteFile();

    bool Rmode = 0;
    bool Wmode = 0; // 合法性检查参数，由父对象中的合法性检查进行设置

public slots:
    void ResetAll();

signals:
    void progressUpdated(int progress);
    void transError(int ErrorCode, QString ErrorMessage);
    // void RRQSent(int times);
    void tftpMessage(QString Message);

private slots:
    void processPendingDatagrams();
    void SendDatagramwithTimer();
    void processErrorMessage(int ErrorCode, QString ErrorMessage);

private:
    QFile* pSaveFile; //用于指向接收时生成的文件
    QFile* pSendFile; //用于指向待发送的文件
    QString SavePath = ""; //接收文件时的保存路径
    QString RemoteFile = ""; //远程文件地址
    QString LocalFile = ""; // 本地待发送文件地址
    QUdpSocket* udpSocket;
    QHostAddress hostip;
    QHostAddress serverip;
    int hostPort = 1037; //默认为1037
    int serverPort; // 记录server处包的发出port
    int transMode; // 0 for netascii, 1 for octet(bin)，用于发送RQ包时设置


    int LastPacket = 0;// 最后一个数据报检查，DATA专用
    int progress = 0;//发送/接收进度

    int ACKNum = 0;//  ACK序号，起始为0
    int DATANum = 1;// 数据报序号，起始为1

    int retriesLeft = 5; //计数器
    QTimer* timer; // 计时器

    QByteArray LastDatagram; // 用于重发数据报

    int SendReadRequest();
    int SendWriteRequest();
    int SendData(int dataNum);
    void handleDataPacket(const QByteArray& datagram);
    void handleAckPacket(const QByteArray& datagram);
    void handleErrorPacket(const QByteArray& datagram);
    void ResetTimer();

};

#endif // TFTPREQUESTMANAGER_H
