#ifndef TFTPSET_H
#define TFTPSET_H

#include <QDialog>

namespace Ui {
class tftpset;
}

class tftpset : public QDialog
{
    Q_OBJECT

public:
    explicit tftpset(QWidget *parent = nullptr);
    ~tftpset();

private:
    Ui::tftpset *ui;
};

#endif // TFTPSET_H
