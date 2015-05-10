#include "nifscript.h"
#include "ui_nifscript.h"
#include "nifmodel.h"
#include "spellbook.h"

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QModelIndex>
#include <QTimer>

void importObj( NifModel * nif, const QModelIndex & index, QString fname = QString());
void import3ds( NifModel * nif, const QModelIndex & index, QString fname = QString());


NifScript::NifScript() : QMainWindow(), ui(new Ui::NifScript)
{
    ui->setupUi(this);
    setFixedSize(500, 250);
    scriptPath = QApplication::arguments()[1];
    nif = new NifModel(this);
    //Needed to execute the script directly after construction.
    //Otherwise the window will be shown when the process is done.
    QTimer::singleShot(10, this, SLOT(executeScript()));
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

bool NifScript::importCommand(int block, QString path)
{
    QFileInfo info(path);
    if(!info.isAbsolute())
        info = QDir(QFileInfo(scriptPath).absolutePath()).absoluteFilePath(path);
    if(info.isFile()){
        if(info.absoluteFilePath().endsWith(".obj", Qt::CaseInsensitive)){
            importObj(nif, nif->getBlock(block), info.filePath());
            printOutput("Imported file: " + path);
            return true;
        }
        if(info.absoluteFilePath().endsWith(".3ds", Qt::CaseInsensitive)){
            import3ds(nif, nif->getBlock(block), info.filePath());
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

bool NifScript::spellCommand(QString spell, int block)
{
    Spell* remove = SpellBook::lookup(Spell::tr(spell.toLatin1().data()));
    remove->cast(nif, nif->getBlock(block));
    return true;
}

//###################
//SLOTS
//###################

void NifScript::okButtonPressed()
{
    close();
}

void NifScript::executeScript()
{
    QFile scriptFile(scriptPath);
    setWindowTitle("NifScope script \"" + scriptFile.fileName() + "\"");
    ui->okButton->setEnabled(false);
    QStringList commands;
    QStringList files;


    connect(ui->okButton, SIGNAL(clicked(bool)), this, SLOT(okButtonPressed()));
    printOutput("<font color=\"blue\">##### Opened script " + scriptFile.fileName() + " #####</font>");
    //Load commands
    scriptFile.open(QIODevice::ReadOnly);
    QTextStream scriptContent(&scriptFile);
    while(!scriptContent.atEnd()){
        QString line = scriptContent.readLine();
        //Cleanup
        removeWhiteSpaces(&line);
        if(line.indexOf("DO_FOR") == 0){
            line.remove(0, 6);
            removeWhiteSpaces(&line);
            //QRegExp fileType(line);
            QDir scriptDir(QDir(QFileInfo(scriptPath).absolutePath()));
            QStringList dirObjects = scriptDir.entryList(QStringList(line));
            foreach(QString item, dirObjects){
                item = scriptDir.absoluteFilePath(item);
                if(QFileInfo(item).isFile()){
                    files.append(item);
                    printOutput(item);
                }
            }
            continue;
        }
        if(line.indexOf("//") != -1)
            line = line.left(line.indexOf("//"));
        if(line.length() == 0)
            continue;
        commands.append(line);
    }
    if(files.length() == 0) files.append(QString());
    ui->progressBar->setMaximum(commands.length()*files.length());

    //Execute every line of the script for every file
    foreach (QString file, files) {
        //Create command stack for this file
        QStringList fileCommands = commands;
        fileCommands.replaceInStrings("$(FILENAME)", QFileInfo(file).fileName());
        fileCommands.replaceInStrings("$(FILEPATH)", QFileInfo(file).filePath());
        fileCommands.replaceInStrings("$(DIRECTORY)", QFileInfo(file).absolutePath());

        foreach (QString line, fileCommands) {
            //Loads a .nif: LOAD <path>
            if(line.indexOf("LOAD") == 0){
                line.remove(0, 4);
                removeWhiteSpaces(&line);
                if(!loadCommand(line)) break;
            }
            //Imports a model: IMPORT <block> <path>
            else if(line.indexOf("IMPORT") == 0) {
                line.remove(0, 6);
                removeWhiteSpaces(&line);

                int block;
                bool ok;
                block = line.mid(0, line.indexOf(' ')).toInt(&ok);
                if(!ok){
                    printOutput("<font color=\"red\">Invalid block number</font>");
                    break;
                }

                line = line.mid(block+1);
                removeWhiteSpaces(&line);

                if(!importCommand(block, line)) break;
            }
            //Saves as .nif: SAVE <path>
            else if(line.indexOf("SAVE") == 0) {
                line.remove(0, 4);
                removeWhiteSpaces(&line);
                if(!saveCommand(line)) break;
            }
            //Removes block: REMOVEB <block>
            else if(line.indexOf("REMOVEB") == 0) {
                line.remove(0, 7);
                removeWhiteSpaces(&line);

                int block;
                bool ok;
                block = line.toInt(&ok);
                if(!ok){
                    printOutput("<font color=\"red\">Invalid block number</font>");
                    break;
                }

                if(!spellCommand("Block/Remove Branch", block)) break;
                else
                    printOutput("Removed branch: " + QString::number(block));
            }
            //Copy branch: COPYB <block>
            else if(line.indexOf("COPYB") == 0) {
                line.remove(0, 5);
                removeWhiteSpaces(&line);

                int block;
                bool ok;
                block = line.toInt(&ok);
                if(!ok){
                    printOutput("<font color=\"red\">Invalid block number</font>");
                    break;
                }

                if(!spellCommand("Block/Copy Branch", block)) break;
                else
                    printOutput("Copied branch: " + QString::number(block));
            }
            //Paste branch: PASTEB <block>
            else if(line.indexOf("PASTEB") == 0) {
                line.remove(0, 6);
                removeWhiteSpaces(&line);

                int block;
                bool ok;
                block = line.toInt(&ok);
                if(!ok){
                    printOutput("<font color=\"red\">Invalid block number</font>");
                    break;
                }

                if(!spellCommand("Block/Paste Branch", block)) break;
                else
                    printOutput("Pasted branch to: " + QString::number(block));
            }
            //Paste branch: SPELL <block> <command>
            else if(line.indexOf("SPELL") == 0) {
                line.remove(0, 5);
                removeWhiteSpaces(&line);

                int block;
                bool ok;
                block = line.left(line.indexOf(' ')).toInt(&ok);
                if(!ok){
                    printOutput("<font color=\"red\">Invalid block number</font>");
                    break;
                }
                line.remove(0, QString(block).length()+1);
                removeWhiteSpaces(&line);
                if(!spellCommand(line, block)) break;
                else
                    printOutput("Applied \"" + line + "\" to block " + QString::number(block));
            }
            //Update progress bar
            ui->progressBar->setValue(ui->progressBar->value()+1);
        }
        printOutput("<font color=\"blue\">Done!</font>");
    }

    //Finish
    ui->okButton->setEnabled(true);
}
