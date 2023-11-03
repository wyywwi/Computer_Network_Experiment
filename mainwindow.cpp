#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUdpSocket>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // udpSocket = new QUdpSocket(this);
        // connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    // 尝试打开log文件
    QString filePath = ".\\tftp.log";
    pf = new FileHandler(filePath);

    // 初始化进度条
    ui->progressBar->setValue(0);

    // 发送启动日志消息
    QDateTime currentTime = QDateTime::currentDateTime();
    QString logMessage = "\n---TFTP client v0.1--- Start running on " + currentTime.toString("yyyy-MM-dd HH:mm:ss") + "\n";
        onLogReceived(logMessage);

    // 初始化UDP
    // udpSocket = new QUdpSocket();
    pTrmanager = new TftpRequestManager();

    // 初始化transfermode
    QStringList transModeList = {"Default", "ASCII", "OCTET(bin)"};
    ui->TransferMode->addItems(transModeList);
    ui->TransferMode->setCurrentText("Default");

    // 初始化LocalNetwork
    pNetwork = new QMap<QString, QString>;
    *pNetwork = getIPAddress();
    QStringList networkList = pNetwork->values();  // 获取所有的 IP 地址作为列表
    QStringList networkNameList = pNetwork->keys();
    ui->LocalNetwork->addItems(networkNameList);
    ui->LocalNetwork->setCurrentText(networkNameList.first());
    ui->Port->setPlaceholderText("1037");

    // 初始化路径输入框
    QString DefaultDir = QCoreApplication::applicationDirPath();
    ui->SavePath->addItem(DefaultDir);
    ui->FilePath->setPlaceholderText("Click Browse Button to Choose File");
    ui->RemoteFilePath->setPlaceholderText("Click to Edit");

    // 连接IP输入框的editingFinished()信号与槽函数onServerIPEditFinished(),记录IP变化
       connect(ui->ServerIP, SIGNAL(editingFinished()), this, SLOT(onServerIPEditFinished()));
    // 连接RemoteFilePath输入框和log输出
       connect(ui->RemoteFilePath, SIGNAL(editingFinished()), this, SLOT(onRemoteFilePathEditFinished()));
    // 连接Port输入框和log输出
       connect(ui->Port, SIGNAL(editingFinished()), this, SLOT(onPortEditFinished()));
    // 连接pTrmanager发出的progressUpdated信号和progressBar，设置进度条
       connect(pTrmanager, SIGNAL(progressUpdated(int)), ui->progressBar, SLOT(setValue(int)));
    // 连接pTrmanager发出的信号与log
       connect(pTrmanager, SIGNAL(tftpMessage(QString)), this, SLOT(onLogReceived(QString)));
    // Interrupt处理
       connect(ui->InterruptButton, &QPushButton::clicked, pTrmanager, &TftpRequestManager::ResetAll);

    // SavePath、FilePath和RemoteFilePath三个QComboBox设置
    // 设置最大项数为10
    int maxItemCount = 10;
        ui->SavePath->setMaxCount(maxItemCount);
        ui->FilePath->setMaxCount(maxItemCount);

    // 当BrowseDir按钮点击时，弹出选择文件夹对话框
    connect(ui->BrowseDir, &QPushButton::clicked, this, [=]() {
           QString selectedPath = QFileDialog::getExistingDirectory(nullptr, "选择工作路径", "", QFileDialog::ShowDirsOnly);

           // 将选定的路径保存到历史路径记录，显示在 QComboBox 中，并记录日志
           ui->SavePath->addItem(selectedPath);
           ui->SavePath->setCurrentText(selectedPath);
           QString logStr = "Selected save path: " + selectedPath;
           onLogReceived(logStr);
       });
    // 当BrowseFile按钮点击时，弹出选择文件对话框
    connect(ui->BrowseFile, &QPushButton::clicked, this, [=]() {
            QString selectedFile = QFileDialog::getOpenFileName(nullptr, "选择文件", "", "All Files (*.*)");

            if (!selectedFile.isEmpty()) {
                // 将选定的文件路径保存到历史路径记录，显示在 QComboBox 中，并记录日志
                ui->FilePath->addItem(selectedFile);
                ui->FilePath->setCurrentText(selectedFile);
                QString logStr = "Selected File to be transferred: " + selectedFile;
                onLogReceived(logStr);
            }
       });
    // 当GET按钮被点击时，在log中记录请求信息并发送RRQ
    connect(ui->GetFile, &QPushButton::clicked, this, &MainWindow::onGetButtonClicked);
    // 当PUT按钮被点击时，在log中记录请求信息并发送WRQ
    connect(ui->PutFile, &QPushButton::clicked, this, &MainWindow::onPutButtonClicked);
}

MainWindow::~MainWindow()
{
    delete pf;
    // delete udpSocket;
    delete pTrmanager;
    delete ui;
}

void MainWindow::onLogReceived(const QString& log){
    //显示报错及日志
    ui->logEdit->append(log);
    pf->writeToLogFile(log);
}

void MainWindow::onServerIPEditFinished()
{
    // 在文本框中显示输入框的文本，同时写入log
    QString text = ui->ServerIP->text();
    QString putstr = QString("Server IP Change to：%1").arg(text);
    onLogReceived(putstr);
}

void MainWindow::onRemoteFilePathEditFinished()
{
    // 在文本框中显示输入框的文本，同时写入log
    QString text = ui->RemoteFilePath->text();
    QString putstr = QString("Remote File Path Change to：%1").arg(text);
    onLogReceived(putstr);
}

void MainWindow::onPortEditFinished()
{
    // 在文本框中显示输入框的文本，同时写入log
    QString text = ui->Port->text();
    QString putstr = QString("Local Transfer Port Change to：%1").arg(text);
    onLogReceived(putstr);
}



void MainWindow::onGetButtonClicked()
{
    //状态检查
    if(pTrmanager->Rmode || pTrmanager->Wmode){
        onLogReceived("TFTP request is running");
        return;
    }
    ui->progressBar->setValue(0);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString StartStr = "\n------------START-READ-FILE-REQUEST------------\nTime: " + currentTime.toString("yyyy-MM-dd HH:mm:ss");
    onLogReceived(StartStr);
    // qDebug() << &StartStr;
    QString portText = ui->Port->text() == "" ? ui->Port->placeholderText() : ui->Port->text();
    QString currentConfig[5];
    currentConfig[0] = "Remote IP: " + ui->ServerIP->text();
    currentConfig[1] = "Local  IP: " + pNetwork->value(ui->LocalNetwork->currentText()) + "\nLocal Port: " + portText;
    currentConfig[2] = "Remote File: " + ui->RemoteFilePath->text();
    currentConfig[3] = "Save Path:   " + ui->SavePath->currentText();
    currentConfig[4] = "Transfer Mode: " + ui->TransferMode->currentText();
    for(int i=0; i<5; i++) onLogReceived(currentConfig[i]);
    // 参数检查
    if(ui->ServerIP->text() == "" ||
       ui->RemoteFilePath->text() == "" ||
       ui->SavePath->currentText() == ""){
        QString ErrorStr = "[Error]\nParameters incomplete, please check and provide the missing ones.";
        onLogReceived(ErrorStr);
        QString StartStr = "--------------END-READ-FILE-REQUEST------------\n";
        onLogReceived(StartStr);
        return;
    }
    // 其他参数检查
    static QRegularExpression ipRegex("^((\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])\\.){3}(\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])$");
    QString ip = ui->ServerIP->text();

    QRegularExpressionMatch match = ipRegex.match(ip);
    if (!match.hasMatch()) {
        // IP地址格式不正确
        QString ErrorStr = "[Error]\nServer IP does not match the required format, please check and rectify it.";
        onLogReceived(ErrorStr);
        QString StartStr = "--------------END-READ-FILE-REQUEST------------\n";
        onLogReceived(StartStr);
        return;
    }
    // 调用TFTP 发送ReadRequest
    pTrmanager->SetRRQparam(ui->SavePath->currentText(), ui->RemoteFilePath->text(), pNetwork->value(ui->LocalNetwork->currentText()), ui->ServerIP->text(), portText, ui->TransferMode->currentText());
    pTrmanager->TFTPReadFile();
}

void MainWindow::onPutButtonClicked()
{
    //状态检查
    if(pTrmanager->Rmode || pTrmanager->Wmode){
        onLogReceived("TFTP request is running");
        return;
    }
    ui->progressBar->setValue(0);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString StartStr = "\n------------START-WRITE-FILE-REQUEST------------\nTime: " + currentTime.toString("yyyy-MM-dd HH:mm:ss");
    onLogReceived(StartStr);
    // qDebug() << &StartStr;
    QString portText = ui->Port->text() == "" ? ui->Port->placeholderText() : ui->Port->text();
    QString currentConfig[4];
    currentConfig[0] = "Remote IP: " + ui->ServerIP->text();
    currentConfig[1] = "Local  IP: " + pNetwork->value(ui->LocalNetwork->currentText()) + "\nLocal Port: " + portText;
    currentConfig[2] = "Local File: " + ui->FilePath->currentText();
    currentConfig[3] = "Transfer Mode: " + ui->TransferMode->currentText();
    for(int i=0; i<4; i++) onLogReceived(currentConfig[i]);
    // 参数检查
    if(ui->ServerIP->text() == "" ||
       ui->FilePath->currentText() == "" ||
       ui->TransferMode->currentText() == ""){
        QString ErrorStr = "[Error] Parameters incomplete, please check and provide the missing ones.";
        onLogReceived(ErrorStr);
        QString StartStr = "--------------END-WRITE-FILE-REQUEST------------\n";
        onLogReceived(StartStr);
        return;
    }
    // 其他参数检查
    static QRegularExpression ipRegex("^((\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])\\.){3}(\\d{1,2}|1\\d{2}|2[0-4]\\d|25[0-5])$");
    QString ip = ui->ServerIP->text();

    QRegularExpressionMatch match = ipRegex.match(ip);
    if (!match.hasMatch()) {
        // IP地址格式不正确
        QString ErrorStr = "[Error]\nServer IP does not match the required format, please check and rectify it.";
        onLogReceived(ErrorStr);
        QString StartStr = "--------------END-READ-FILE-REQUEST------------\n";
        onLogReceived(StartStr);
        return;
    }
    // 调用TFTP 发送ReadRequest
    pTrmanager->SetWRQparam(ui->FilePath->currentText(), pNetwork->value(ui->LocalNetwork->currentText()), ui->ServerIP->text(), portText, ui->TransferMode->currentText());
    pTrmanager->TFTPWriteFile();
}



void MainWindow::onLogOpenFailed(const QString& errorMessage){
    pf->writeToLogFile(errorMessage);
}

QMap<QString, QString> MainWindow::getIPAddress()
{
    QString ipAddress;
    QMap<QString, QString> ipMap; // 用于存储 IP 地址和对应的接口标识

    // 获取所有网络接口
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    // 遍历网络接口
    for (const QNetworkInterface& interface : interfaces)
    {
        // 过滤回环和无效接口
        if (interface.flags().testFlag(QNetworkInterface::IsLoopBack) || !interface.isValid())
            continue;

        // 获取接口的 IP 地址列表
        QList<QNetworkAddressEntry> addressEntries = interface.addressEntries();

        // 遍历 IP 地址列表
        for (const QNetworkAddressEntry& entry : addressEntries)
        {
            // 获取 IPv4 地址
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
            {
                ipAddress = entry.ip().toString();

                // 过滤以 "169.254" 开头的 IP 地址
                if (ipAddress.startsWith("169.254")) break;

                // 存储 IP 地址和接口标识的键值对
                ipMap.insert(interface.humanReadableName(), ipAddress);
                break;
            }
        }
    }

    // 输出 IP 地址和对应接口标识到 qDebug
    qDebug() << "IP Addresses and Interface Identifiers:";
    for (auto it = ipMap.begin(); it != ipMap.end(); ++it)
    {
        qDebug() << "IP Address:" << it.value() << "Interface Identifier:" << it.key();
    }

    return ipMap;
}
