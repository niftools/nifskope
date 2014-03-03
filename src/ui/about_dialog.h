#ifndef ABOUT_H
#define ABOUT_H

#include "ui_about_dialog.h"

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    AboutDialog(QWidget *parent = 0);

private:
    Ui::AboutDialog ui;
};


#endif // ABOUT_H
