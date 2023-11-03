#include "tftprequestmanager.h"

TftpRequestManager::TftpRequestManager(QObject *parent) : QObject(parent)
{
    // 初始化操作
    pSaveFile = new QFile(this);
    pSendFile = new QFile(this);
    udpSocket = new QUdpSocket(this);
    // 其他初始化逻辑
    transMode = 0;

    //初始化timer

    timer = new QTimer();
    timer->setInterval(2000); // 设置超时时长为2000ms
    timer->setSingleShot(true); // 设置为单次触发模式
    retriesLeft = 5;

    // 连接信号和槽
    // connect(this, &TftpRequestManager::progressUpdated, this, &TftpRequestManager::handleProgressUpdate);
    connect(udpSocket, &QUdpSocket::readyRead, this, &TftpRequestManager::processPendingDatagrams);
    connect(timer, &QTimer::timeout, this, &TftpRequestManager::SendDatagramwithTimer);
    connect(this, SIGNAL(transError(int,QString)), this, SLOT(processErrorMessage(int,QString)));
}

TftpRequestManager::~TftpRequestManager()
{
    // 在析构函数中进行清理操作
    delete pSaveFile;
    delete pSendFile;
    delete udpSocket;
}



void TftpRequestManager::SetRRQparam(QString inSavePath, QString inRemoteFilePath, QString inhostip, QString inserverip, QString inhostport, QString intransmode)
{
    // 设置参数值
    // 这里假设SavePath、RemoteFilePath、hostport 和 transmode 都是合法的字符串
    this->SavePath = inSavePath;
    this->RemoteFile = inRemoteFilePath;
    this->hostip = QHostAddress(inhostip);
    this->serverip = QHostAddress(inserverip);
    this->hostPort = inhostport.toInt();
    this->transMode = (intransmode == "OCTET(bin)") ? 1 : 0;

    // 参数合法性检查
    if(1){
        this->Rmode = 1;
    }

    // 输出设置信息
        qDebug() << "SavePath: " << SavePath;
        qDebug() << "RemoteFile: " << RemoteFile;
        qDebug() << "hostip: " << hostip;
        qDebug() << "serverip: " << serverip;
        qDebug() << "hostport: " << hostPort;
        qDebug() << "transmode: " << transMode;
}

void TftpRequestManager::SetWRQparam(QString inFilePath, QString inhostip, QString inserverip, QString inhostport, QString intransmode)
{
    // 设置参数值
    // 这里假设 FilePath、hostip、serverip、hostport 和 transmode 都是合法的字符串
    this->pSendFile = new QFile(inFilePath);
    this->LocalFile = inFilePath;
    this->hostip = QHostAddress(inhostip);
    this->serverip = QHostAddress(inserverip);
    this->hostPort = inhostport.toInt();
    this->transMode = (intransmode == "OCTET(bin)") ? 1 : 0;

    // 参数合法性检查
    if (1) {
        this->Wmode = 1;
    }

    // 输出设置信息
    qDebug() << "FilePath: " << inFilePath;
    qDebug() << "hostip: " << hostip;
    qDebug() << "serverip: " << serverip;
    qDebug() << "hostport: " << hostPort;
    qDebug() << "transmode: " << transMode;
}

int TftpRequestManager::TFTPReadFile(){
    // 允许发送检查
    if(!this->Rmode){
        qDebug() << "Read not allowed";
        return 4;
    }
    // 其他TFTP合法性检查

    // 依据savepath和remotefile中的文件名打开（新建）文件用于保存接收到的文件
    QString fileName = QFileInfo(RemoteFile).fileName();
    QString baseName = QFileInfo(RemoteFile).baseName();
    QString suffix = QFileInfo(RemoteFile).suffix();

    QString newFileName = fileName;
    int counter = 1;

    // 检查文件是否已存在
    while (QFile::exists(SavePath + "\\" + newFileName)) {
        // 文件已存在，修改文件名
        if (!suffix.isEmpty()) {
            // 如果文件名包含后缀，则在后缀之前插入 (i)
            newFileName = baseName + "(" + QString::number(counter) + ")." + suffix;
        } else {
            // 如果文件名没有后缀，则直接在末尾添加 (i)
            newFileName = baseName + "(" + QString::number(counter) + ")";
        }
        counter++;
    }

    // 使用新的文件名
    pSaveFile->setFileName(SavePath + "\\" + newFileName);

    if (!pSaveFile->open(QIODevice::ReadWrite | QIODevice::Truncate | QIODevice::NewOnly)) {
        // 文件打开失败，返回错误码1
        qDebug() << "File open failed";
        return 1;
    }

    // 关闭文件是良好美德
    pSaveFile->close();

    // socket交互
    // 添加超时重传机制
    SendReadRequest();
    return 0;
}

int TftpRequestManager::TFTPWriteFile(){
    // 允许发送检查
    if(!this->Wmode){
        qDebug() << "Write not allowed";
        return 4;
    }

    // 尝试打开待发送文件
    // 待发送文件此时已经被pSendFile指向，尝试打开；如果能够正常采用读取模式打开则关闭，不能则报错返回错误码1

    // 尝试打开待发送文件
    if (!pSendFile->open(QIODevice::ReadOnly)){
        // 文件打开失败，返回错误码1
        qDebug() << "File open failed";
        return 1;
    }

    // 关闭文件是良好的习惯
    pSendFile->close();

    // socket 交互
    // 添加超时重传机制
    SendWriteRequest();
    return 0;
}

void TftpRequestManager::ResetTimer(){
    // 停止计时器
    timer->stop();

    // 删除计时器对象
    delete timer;

    // 创建新的计时器对象
    timer = new QTimer(this);
    timer->setInterval(2000); // 设置超时时长为3000ms
    timer->setSingleShot(true); // 设置为单次触发模式

    // 连接超时信号与槽函数
    connect(timer, &QTimer::timeout, this, &TftpRequestManager::SendDatagramwithTimer);
}

// 统一超时重传函数
void TftpRequestManager::SendDatagramwithTimer(){
    ResetTimer();
    retriesLeft--;
    if(retriesLeft){
        // qDebug() << serverPort;
        udpSocket->writeDatagram(LastDatagram, serverip, serverPort);
        emit tftpMessage("Retransmission " + QString::number(5-retriesLeft) + ".");
        timer->start();
    }
    else{
        emit tftpMessage("[Error]\n    Timeout. Session closed.\n--------------END--REQUEST----------------------\n");
        retriesLeft = 5;
        Wmode = Rmode = 0;
        LastPacket = 0;
        ACKNum = 0;
        DATANum = 1;
        serverPort = 69;
    }
}

int TftpRequestManager::SendReadRequest(){
    // 构造RRQ请求包
    QByteArray requestData;
    QDataStream out(&requestData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_4);
    out << quint16(1);  // TFTP opcode for RRQ

    QString tmpfileName = QFileInfo(RemoteFile).fileName();
    QByteArray fileNameAscii = tmpfileName.toLatin1();  // 将文件名转换为 ASCII 编码的字节数组
    // 写入文件名
    out.writeRawData(fileNameAscii.constData(), fileNameAscii.size() + 1);  // 包括结尾的空字符

    // 写入传输模式
    QString tmpmode = (transMode == 1) ? "octet" : "netascii";
    QByteArray modeAscii = tmpmode.toLatin1();  // 将传输模式转换为 ASCII 编码的字节数组
    out.writeRawData(modeAscii.constData(), modeAscii.size() + 1);  // 包括结尾的空字符

    // 发送RRQ请求
    // 检查绑定状态
    if (udpSocket->waitForConnected()) {
        // 解除连接
        udpSocket->disconnectFromHost();
    }
    udpSocket->bind(hostip, hostPort);

    // 输出前先记录
    LastDatagram = requestData;
    qDebug() << LastDatagram;
    udpSocket->writeDatagram(requestData, serverip, 69);
    timer->start();

    return 0;
}

int TftpRequestManager::SendWriteRequest(){
    // 构造RRQ请求包
    QByteArray requestData;
    QDataStream out(&requestData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_4);
    out << quint16(2);  // TFTP opcode for WRQ

    QString tmpfileName = QFileInfo(LocalFile).fileName();
    QByteArray fileNameAscii = tmpfileName.toLatin1();  // 将文件名转换为 ASCII 编码的字节数组
    // 写入文件名
    out.writeRawData(fileNameAscii.constData(), fileNameAscii.size() + 1);  // 包括结尾的空字符

    // 写入传输模式
    QString tmpmode = (transMode == 1) ? "octet" : "netascii";
    QByteArray modeAscii = tmpmode.toLatin1();  // 将传输模式转换为 ASCII 编码的字节数组
    out.writeRawData(modeAscii.constData(), modeAscii.size() + 1);  // 包括结尾的空字符

    // 发送WRQ请求
    // 检查绑定状态
    if (udpSocket->waitForConnected()) {
        // 解除连接
        udpSocket->disconnectFromHost();
    }
    udpSocket->bind(hostip, hostPort);

    // 输出前先记录
    LastDatagram = requestData;
    udpSocket->writeDatagram(requestData, serverip, 69);
    timer->start();

    return 0;
}

void TftpRequestManager::processPendingDatagrams()
{
    // 接收到包，重置计时器，重置重传次数
    // 该操作放在之后进行
//    ResetTimer();
//    retriesLeft = 5;

    // 获取包内容
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 解析操作码，端口初始化
        QByteArray opcodeBytes = datagram.left(2);
        quint16 opcode = static_cast<quint16>((static_cast<uchar>(opcodeBytes[0]) << 8) |
                                              static_cast<uchar>(opcodeBytes[1]));
        // qDebug() << opcode;

        if((ACKNum == 0 && opcode == 3) || (DATANum == 1 && opcode == 4) || (opcode == 5)) serverPort = senderPort;

        // 处理接收到的数据包
        // 对 datagram 进行解析和处理
        if (sender == serverip && senderPort == serverPort) {

            // 数据报来源合法，根据 TFTP 规则判断数据包类型并进行处理
            if (opcode == 3 && Rmode == 1) {  // DATA 数据包
                handleDataPacket(datagram);
            } else if (opcode == 4 && Wmode == 1) {  // ACK 数据包
                handleAckPacket(datagram);
            } else if (opcode == 5) {  // ERROR 数据包
                handleErrorPacket(datagram);
            }
            else{
                emit tftpMessage("Unexpected Datagram received");
            }
        }

    }
}

// Client发送完整文件，ascii问题由Server处理
void TftpRequestManager::handleAckPacket(const QByteArray& datagram){
    // 解析 ACK 数据包
    // 提取数据块编号等信息
    QByteArray data = datagram.mid(2); // 去除数据包类型 Code
    int blockNumber = 0; // 从 ACK 数据包中提取的数据块编号
    QByteArray blockNumberBytes = data.left(2);
    data = data.mid(2);
    blockNumber = static_cast<quint16>((static_cast<uchar>(blockNumberBytes[0]) << 8) |
                  static_cast<uchar>(blockNumberBytes[1]));
    // qDebug() << "ACK Seq: " + QString::number(blockNumber);
    // qDebug() << "ACK Num: " + QString::number(ACKNum);

    // 检查是否为最后一个数据报的ACK
    if(!LastPacket){
        // 检查期望
        if (blockNumber >= ACKNum){
            if(blockNumber > ACKNum){
                ACKNum = blockNumber;
                emit tftpMessage("Received unexpected ACK number " + QString::number(blockNumber) + ", reset ACK position.");
            }
            // 设置DataBlock
            QByteArray dataPacket;

            // 打开文件，转移到指定位置，读取文件
            if (pSendFile->open(QIODevice::ReadOnly)){
                pSendFile->seek(ACKNum*512);
                dataPacket = pSendFile->read(512);
                pSendFile->close();
            }
            else {
                // 文件打开失败，处理错误
                int errcode = 3;
                QString errmessage = "Disk full or allocation exceeded";
                emit transError(errcode, errmessage);
            }

            // 设置数据报并发送
            dataPacket.prepend((ACKNum+1) & 0xFF); // 数据块编号低位
            dataPacket.prepend(((ACKNum+1) >> 8) & 0xFF); // 数据块编号高位
            dataPacket.prepend('\x03'); // ACK 类型码
            dataPacket.prepend('\x00'); // ACK 类型
            LastDatagram = dataPacket;
            udpSocket->writeDatagram(dataPacket, serverip, serverPort);

            progress = (ACKNum*51200) / pSendFile->size();
            // qDebug()<< progress;
            emit progressUpdated(progress);
            ACKNum++;
            ResetTimer();
            retriesLeft = 5;
            timer->start();

            // 判断是否为最后一个数据包
            if(dataPacket.size() != 516) LastPacket = dataPacket.size() - 4;
        }
        else{
            // 接收到非期望ACK编号, 若ACK小于当前包，重发
            // 为避免SAS错误，修正为不进行重发，仅等待超时（因为为停等协议，之前的DATA包肯定已经传输）
            // udpSocket->writeDatagram(LastDatagram, serverip, serverPort);
            // emit tftpMessage("Received unexpected ACK number " + QString::number(blockNumber) + ", resend DATA.");
            // timer->start();
            // retriesLeft = 5;
            emit tftpMessage("Received unexpected ACK number " + QString::number(blockNumber) + ", wait for the correct one.");
        }
    }
    else{
        // 结束会话
        progress = 100;
        emit progressUpdated(progress);
        emit tftpMessage("Sent " + QString::number(ACKNum) + " packets, " + QString::number((ACKNum-1)*512 + LastPacket) + " bytes.");
        LastPacket = 0;
        ACKNum = 0;
        serverPort = 69;
        Wmode = 0;
        emit tftpMessage("All data has been sent. Session closed.\n--------------END-WRITE-FILE-REQUEST------------\n");
        ResetTimer();
        retriesLeft = 5;
    }
}

void TftpRequestManager::handleDataPacket(const QByteArray& datagram)
{
    // 解析 DATA 数据包
    // 提取数据块编号等信息

    //初始化数据块编号
    QByteArray data = datagram.mid(2); // 去除数据包类型 Code
    int blockNumber = 0; // 从 DATA 数据包中提取的数据块编号
    QByteArray blockNumberBytes = data.left(2);
    data = data.mid(2);
    blockNumber = static_cast<quint16>((static_cast<uchar>(blockNumberBytes[0]) << 8) |
                  static_cast<uchar>(blockNumberBytes[1]));
    // qDebug() << "Block Seq: " + QString::number(blockNumber);
    // qDebug() << "Data  Num: " + QString::number(DATANum);


    // 检查数据块编号是否与期望一致
    if (blockNumber == DATANum) {
        // 将数据保存到文件
        if (pSaveFile->open(QIODevice::Append)) {
            pSaveFile->write(data);
            pSaveFile->close();

            // 发送 ACK 数据包，大端模式
            QByteArray ackPacket;
            ackPacket.append('\x00');
            ackPacket.append('\x04'); // ACK 类型码
            ackPacket.append(((DATANum) >> 8) & 0xFF); // 数据块编号高位
            ackPacket.append((DATANum) & 0xFF); // 数据块编号低位

            LastDatagram = ackPacket;
            udpSocket->writeDatagram(ackPacket, serverip, serverPort);

            // 更新期望的数据块编号
            // qDebug() << data.length();
            if(data.length() == 512){
                progress = DATANum / 328; //按最大传输65536个块，每个块512B计算的虚拟（最慢）进度 的两倍
                // qDebug()<< progress;
                emit progressUpdated(progress);
                DATANum++; // 正常续传
                ResetTimer();
                retriesLeft = 5;
                timer->start();
            }
            else{
                progress = 100;
                emit progressUpdated(progress);
                emit tftpMessage("Received " + QString::number(DATANum) + " packets, " + QString::number((DATANum-1)*512 + data.length()) + " bytes.");
                DATANum = 1; //结束回 1
                serverPort = 69;
                Rmode = 0;
                emit tftpMessage("All data has been received. Session closed.\n--------------END-READ-FILE-REQUEST------------\n");
                // 不再续timer
                ResetTimer();
                retriesLeft = 5;
            }
        } else {
            // 文件打开失败，处理错误
            int errcode = 3;
            QString errmessage = "Disk full or allocation exceeded";
            emit transError(errcode, errmessage);
            ResetTimer();
            retriesLeft = 5;
        }
    } else {
        // 接收到非期望的数据块编号
        // 如果数据块编号较当前编号低大于一，为无用重传数据，不作任何处理；如果比当前数据块编号低一，为避免多次重复造成数据报指数增长，直接等待重传；如果高于当前数据块，属于错误状态
        // 总结来说，就是等待超时重传，除此之外不作任何处理。
//        if (blockNumber == DATANum-1){
//            udpSocket->writeDatagram(LastDatagram, serverip, serverPort);
//            emit tftpMessage("Received unexpected block number " + QString::number(blockNumber) + ", resend ACK.");
//        }
//        else emit tftpMessage("Received unexpected block number " + QString::number(blockNumber) + ", no reply.");
        emit tftpMessage("Received unexpected block number " + QString::number(blockNumber) + ", no reply.");
//        ResetTimer();
//        retriesLeft = 5;
//        timer->start();
    }
}

void TftpRequestManager::handleErrorPacket(const QByteArray& datagram){
    ResetTimer();
    retriesLeft = 5;
    // 解析 ACK 数据包
    // 提取数据块编号等信息
    QByteArray data = datagram.mid(2); // 去除数据包类型 Code
    int ErrorNum = 0; // 错误
    QByteArray ErrorNumBytes = data.left(2);
    data = data.mid(2);
    ErrorNum = static_cast<quint16>((static_cast<uchar>(ErrorNumBytes[0]) << 8) |
               static_cast<uchar>(ErrorNumBytes[1]));
    QString ErrorMessage = QString::fromLatin1(data);//data剩余部分为ASCII字符，转译成QString
    emit transError(ErrorNum, "[From Server] " + ErrorMessage);
}

void TftpRequestManager::processErrorMessage(int ErrorCode, QString ErrorMessage){
    QString LogMessage = "[Error]\n    Error code " + QString::number(ErrorCode) + ": ";
    switch(ErrorCode){
        case 0: LogMessage.append("Not defined, see error message. "); break;
        case 1: LogMessage.append("File not found. "); break;
        case 2: LogMessage.append("Access violation. "); break;
        case 3: LogMessage.append("Disk full or allocation exceeded. "); break;
        case 4: LogMessage.append("Illegal TFTP operation. "); break;
        case 5: LogMessage.append("Unknown transfer ID. "); break;
        case 6: LogMessage.append("File already exists. "); break;
        case 7: LogMessage.append("No such user. "); // 虽然但是很奇怪，TFTP不是没有用户吗
    }
    LogMessage.append("\n    Error Message: ");
    LogMessage.append(ErrorMessage);
    emit tftpMessage(LogMessage);
    emit tftpMessage("Session closed.\n--------------END-REQUEST----------------------\n");
    ResetTimer();
    retriesLeft = 5;
    Wmode = Rmode = 0;
    LastPacket = 0;
    ACKNum = 0;
    DATANum = 1;
    serverPort = 69;
}

void TftpRequestManager::ResetAll(){
    progress = 0;
    emit progressUpdated(progress);

    QByteArray ErrorPacket;
    ErrorPacket.append('\x00');
    ErrorPacket.append('\x05'); // ACK 类型码
    ErrorPacket.append('\x00'); // 数据块编号高位
    ErrorPacket.append('\x00'); // 数据块编号低位
    ErrorPacket.append("Not defined, see error message");
    udpSocket->writeDatagram(ErrorPacket, serverip, serverPort);
    udpSocket->writeDatagram(ErrorPacket, serverip, serverPort);
    udpSocket->writeDatagram(ErrorPacket, serverip, serverPort);
    udpSocket->writeDatagram(ErrorPacket, serverip, serverPort);
    udpSocket->writeDatagram(ErrorPacket, serverip, serverPort);

    ResetTimer();
    retriesLeft = 5;
    serverPort = 69;
    Rmode = 0;
    Wmode = 0;
    LastPacket = 0;
    ACKNum = 0;
    DATANum = 1;
    emit tftpMessage("Interrupt Button clicked. Session closed.\n--------------END-REQUEST----------------------\n");
}
