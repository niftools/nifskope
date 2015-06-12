#include "batchprocessor.h"
#include "ui_batchprocessor.h"

#include <QAction>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProgressDialog>
#include <QProgressBar>

BatchProcessor::BatchProcessor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BatchProcessor)
{
    ui->setupUi(this);

    //Create menu for all spells
    addActionMenu = new QMenu();
    for(Spell *spell : SpellBook::spells()){
        if(spell->page().isEmpty()){
            addActionMenu->addAction(spell->name());
        } else {
            bool actionMade = false;
            for(QMenu * menu : addActionMenu->findChildren<QMenu*>()){
                if(menu->title() == spell->page()){
                    QAction *action = new QAction(spell->name(), menu);
                    action->setProperty("page", spell->page());
                    menu->addAction(action);
                    actionMade = true;
                    break;
                }
            }
            if(actionMade) continue;
            QMenu *subMenu = new QMenu(spell->page(), addActionMenu);
            QAction *action = new QAction(spell->name(), subMenu);
            action->setProperty("page", spell->page());
            subMenu->addAction(action);
            addActionMenu->addMenu(subMenu);
        }
    }

    ui->addActionButton->setMenu(addActionMenu);
    connect(addActionMenu, SIGNAL(triggered(QAction*)), this, SLOT(actionTriggered(QAction*)));
}

BatchProcessor::~BatchProcessor()
{
    delete ui;
}

BatchProcessor *BatchProcessor::create()
{
    BatchProcessor * batchProcessor = new BatchProcessor();
    batchProcessor->setAttribute( Qt::WA_DeleteOnClose );
    batchProcessor->show();
    return batchProcessor;
}

void BatchProcessor::actionTriggered(QAction *action)
{
    QString actionName;
    if(action->property("page") != QVariant()){
        actionName = action->property("page").toString() + "/" + action->text();
    } else {
        actionName = action->text();
    }
        ui->actionView->addItem(actionName, SpellBook::lookup(actionName)->batchProperties());
}

void BatchProcessor::on_addFilesButton_pressed()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Add Files"), QString(), tr("NIF (*.nif)"));
    for(QString file : files){
        bool found = false;
        for(int index = 0; index < ui->fileList->count(); index++){
            if(ui->fileList->item(index)->text() == file)
                found = true;
        }
        if(!found)
            ui->fileList->addItem(file);
    }
}

void BatchProcessor::on_deleteFileButton_pressed()
{
    for(QListWidgetItem *item : ui->fileList->selectedItems()){
        delete item;
    }
}

void BatchProcessor::on_runButton_pressed()
{
    QProgressDialog progress(this);
    progress.setModal(true);
    progress.setLabelText("Applying spells to files");
    progress.setMinimumDuration(0);
    progress.setWindowFlags(progress.windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
    QProgressBar *bar = new QProgressBar(&progress);
    bar->setFormat("%v of %m");
    bar->setRange(0, ui->fileList->count());
    bar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    progress.setBar(bar);
    progress.show();

    for(int fileIndex = 0; fileIndex < ui->fileList->count(); fileIndex++){
        qApp->processEvents();
        if(progress.wasCanceled())
            break;
        NifModel nifModel;
        nifModel.loadFromFile(ui->fileList->item(fileIndex)->text());

        for(int actionIndex = 0; actionIndex < ui->actionView->count(); actionIndex++){
            SpellBook::lookup(ui->actionView->title(actionIndex))->castProperties(&nifModel, nifModel.getBlock(ui->actionView->data(actionIndex)[0].name == "Block:" ? ui->actionView->data(actionIndex)[0].value.toInt() : 0), ui->actionView->data(actionIndex));
        }
        progress.setValue(fileIndex+1);
        nifModel.saveToFile(ui->fileList->item(fileIndex)->text());
    }
}

void BatchProcessor::on_saveButton_pressed()
{
    QString path = QFileDialog::getSaveFileName(this, "Save", QString(), QString("NifSkope batch files(*.nss)"));
    if(path.isEmpty())
        return;
    QJsonDocument doc;
    QJsonArray actionArray;
    for(int index = 0; index < ui->actionView->count(); index++){
        QJsonObject item;
        for(BatchProperty property : ui->actionView->data(index)){
            item[property.name] = QJsonValue::fromVariant(property.value);
        }
        item["title"] = ui->actionView->title(index);
        actionArray.append(item);
    }
    doc.setArray(actionArray);

    QFile file(path);
    file.open(QIODevice::WriteOnly);
    QTextStream fileStream(&file);
    fileStream << doc.toJson();
    file.close();
}

void BatchProcessor::on_loadButton_pressed()
{
    QString path = QFileDialog::getOpenFileName(this, "Load", QString(), QString("NifSkope batch files(*.nss)"));
    if(path.isEmpty())
        return;
    ui->actionView->clear();
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray actionArray = doc.array();
    for(int index = 0; index < actionArray.count(); index++){
        QJsonObject item = actionArray[index].toObject();
        ui->actionView->addItem(item["title"].toString(), SpellBook::lookup(item["title"].toString())->batchProperties());
        for(BatchProperty property : ui->actionView->data(index)){
            ui->actionView->setData(index, property.name, item.value(property.name).toVariant());
        }
    }
}
