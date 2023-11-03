#include "tftpset.h"
#include "ui_tftpset.h"

tftpset::tftpset(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tftpset)
{
    ui->setupUi(this);
}

tftpset::~tftpset()
{
    delete ui;
}
