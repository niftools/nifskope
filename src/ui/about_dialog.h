#ifndef ABOUT_H
#define ABOUT_H

#include "ui_about_dialog.h"

#include <QDialog>


class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	AboutDialog( QWidget * parent = nullptr );

private:
	Ui::AboutDialog ui;
};


#endif // ABOUT_H
