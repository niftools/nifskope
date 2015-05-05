#include "nifscript.h"
#include "ui_nifscript.h"

#include <QFile>
#include <QTextStream>

NifScript::NifScript(QWidget *parent) : QMainWindow(parent), ui(new Ui::NifScript)
{
    //Init
    ui->setupUi(this);
    setFixedSize(500, 250);
    QFile scriptFile(QApplication::arguments()[1]);
    setWindowTitle("NifScope script \"" + scriptFile.fileName() + "\"");
    ui->okButton->setEnabled(false);
    QStringList commands;


    connect(ui->okButton, SIGNAL(clicked(bool)), this, SLOT(okButtonPressed()));
    printOutput("<font color=\"blue\">##### Opened script " + scriptFile.fileName() + " #####</font>");

    //Load commands
    scriptFile.open(QIODevice::ReadOnly);
    QTextStream scriptContent(&scriptFile);
    while(!scriptContent.atEnd()){
        QString line = scriptContent.readLine();
        //Cleanup
        removeWhiteSpaces(&line);
        if(line.indexOf("//") != -1)
            line = line.mid(0, line.indexOf("//"));
        if(line.length() == 0)
            continue;
        commands.append(line);
    }
    ui->progressBar->setMaximum(commands.length());

    //Execute every line
    foreach (QString line, commands) {
        //Loads a .nif: LOAD <path>
        if(line.indexOf("LOAD") == 0){
            line.remove(0, 4);
            removeWhiteSpaces(&line);
            chopWhiteSpaces(&line);
            printOutput("Loaded file: " + line);
        }
        //Imports a model: IMPORT <path>
        else if(line.indexOf("IMPORT") == 0) {
            line.remove(0, 6);
            removeWhiteSpaces(&line);
            chopWhiteSpaces(&line);
            printOutput("Imported file: " + line);
        }
        //Update progress bar
        ui->progressBar->setValue(ui->progressBar->value()+1);
    }
}

NifScript::~NifScript()
{
    delete ui;
}

void NifScript::printOutput(QString output)
{
    ui->output->append(output);
}

void NifScript::removeWhiteSpaces(QString *string)
{
    while((*string)[0] == ' ')
        string->remove(0, 1);
}

void NifScript::chopWhiteSpaces(QString *string)
{
    while((*string)[string->length()-1] == ' ')
        string->chop(1);
}


//###################
//SLOTS
//###################

void NifScript::okButtonPressed()
{
    close();
}
