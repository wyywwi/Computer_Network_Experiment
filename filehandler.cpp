#include "mainwindow.h"

FileHandler::FileHandler(const QString& filePath)
    : file(filePath)
{
    if (file.exists())
    {
        if (file.open(QIODevice::Append | QIODevice::Text))
        {
            // 文件存在，以追加模式打开成功
            stream.setDevice(&file);
        }
        else
        {
            // 处理文件打开失败的情况
            QString errorMessage = "Failed to open tftp.log!";
            emit fileOpenErrorOccurred(errorMessage);
        }
    }
    else
    {
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            // 文件不存在，以写入模式打开成功
            stream.setDevice(&file);
        }
        else
        {
            // 处理文件打开失败的情况
            QString errorMessage = "Failed to create tftp.log!";
            emit fileOpenErrorOccurred(errorMessage);
        }
    }
}

FileHandler::~FileHandler()
{
    file.close(); // 在析构函数中关闭文件
}

void FileHandler::writeToLogFile(const QString& content)
{
    QString str = content + '\n';
    stream << str;
    stream.flush(); // 刷新缓冲区以确保内容写入文件
}
