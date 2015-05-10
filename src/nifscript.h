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
    explicit NifScript();
    ~NifScript();
private:
    Ui::NifScript *ui;
    void printOutput(QString output);
    void removeWhiteSpaces(QString *string);

    bool loadCommand(QString path);
    bool importCommand(int block, QString path);
    bool saveCommand(QString path);
    bool spellCommand(QString spell, int block);

    NifModel *nif;
    QString scriptPath;

public slots:
    void okButtonPressed();
    void executeScript();
};

#endif // NIFSCRIPT_H
