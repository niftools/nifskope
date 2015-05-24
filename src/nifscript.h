#ifndef NIFSCRIPT_H
#define NIFSCRIPT_H

#include "nifmodel.h"

#include <QMainWindow>
#include <QStringList>

namespace Ui {
class NifScript;
}

class NifScript : public QMainWindow
{
    Q_OBJECT

public:
    explicit NifScript(QWidget *parent = 0);
    ~NifScript();
private:
    Ui::NifScript *ui;
    void printOutput(QString output);
    void removeWhiteSpaces(QString *string);
    void chopWhiteSpaces(QString *string);

    NifModel *nif = new NifModel();

public slots:
    void okButtonPressed();
};

#endif // NIFSCRIPT_H
