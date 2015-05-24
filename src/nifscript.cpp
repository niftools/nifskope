#include "nifscript.h"
#include "ui_nifscript.h"

#include <QFile>
#include <QTextStream>
#include <QDir>


NifScript::NifScript() : QMainWindow(), ui(new Ui::NifScript)
{
    //Init
    ui->setupUi(this);
    setFixedSize(500, 250);
    scriptPath = QApplication::arguments()[1];
    QFile scriptFile(scriptPath);
    setWindowTitle("NifScope script \"" + scriptFile.fileName() + "\"");
    ui->okButton->setEnabled(false);
    QStringList commands;
    nif = new NifModel(this);


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
            if(!loadCommand(line)) break;
        }
        //Imports a model: IMPORT <path>
        else if(line.indexOf("IMPORT") == 0) {
            line.remove(0, 6);
            removeWhiteSpaces(&line);
            if(!importCommand(line)) break;
        }
        //Saves as .nif: SAVE <path>
        else if(line.indexOf("SAVE") == 0) {
            line.remove(0, 4);
            removeWhiteSpaces(&line);
            if(!saveCommand(line)) break;
        }
        //Update progress bar
        ui->progressBar->setValue(ui->progressBar->value()+1);
    }

    //Finish
    ui->okButton->setEnabled(true);
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
    while(string->length() > 0 && (*string)[0] == ' ')
        string->remove(0, 1);
    while(string->length() > 0 && (*string)[string->length()-1] == ' ')
        string->chop(1);
}

//###################
//COMMANDS
//###################

bool NifScript::loadCommand(QString path)
{
    QFileInfo info(path);
    if(!info.isAbsolute())
        info = QDir(QFileInfo(scriptPath).absolutePath()).absoluteFilePath(path);

    if(info.isFile() && nif->loadFromFile(info.filePath())){
        printOutput("Loaded file: " + info.fileName());
        return true;
    }
    printOutput("<font color=\"red\">Can't load nif: " + path + "</font>");
    return false;
}

bool NifScript::importCommand(QString path)
{
    QFileInfo info(QDir(QFileInfo(scriptPath).absolutePath()).absoluteFilePath(path));
    if(info.isFile()){
        if(info.absoluteFilePath().endsWith(".obj", Qt::CaseInsensitive)){
            printOutput("Imported file: " + path);
            return true;
        }
        if(info.absoluteFilePath().endsWith(".3ds", Qt::CaseInsensitive)){
            printOutput("Imported file: " + path);
            return true;
        }
    }
    printOutput("<font color=\"red\">Can't load file: " + path + "</font>");
    return false;
}

bool NifScript::saveCommand(QString path)
{
    QFileInfo info(path);
    if(!info.isAbsolute())
        info = QDir(QFileInfo(scriptPath).absolutePath()).absoluteFilePath(path);

    if(info.absoluteFilePath().endsWith(".nif", Qt::CaseInsensitive) && nif->saveToFile(info.filePath())){
        printOutput("Saved to: " + info.fileName());
        return true;
    }
    printOutput("<font color=\"red\">Can't save. Invalid file path' " + info.filePath() + " </font>");
    return false;
}

//###################
//SLOTS
//###################

void NifScript::okButtonPressed()
{
    close();
}
