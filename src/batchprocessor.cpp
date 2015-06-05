#include "batchprocessor.h"
#include "ui_batchprocessor.h"

#include <QAction>
#include <QFileDialog>

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
            foreach(QMenu * menu, addActionMenu->findChildren<QMenu*>()){
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
        QMap<QString, QVariant::Type> contentType;
        contentType.insert("test Value", QVariant::Int);
        ui->actionView->addItem(actionName, contentType);
}

void BatchProcessor::on_addFilesButton_pressed()
{
    QStringList files = QFileDialog::getOpenFileNames(this, tr("Add Files"), QString(), tr("NIF (*.nif)"));
    ui->fileList->addItems(files);
}

void BatchProcessor::on_deleteFileButton_pressed()
{
    QList<QListWidgetItem*> list = ui->fileList->selectedItems();
    foreach(QListWidgetItem *item, list){
        delete item;
    }
}

void BatchProcessor::on_runButton_pressed()
{
    ui->progressBar->setTextVisible(true);
    ui->progressBar->setEnabled(true);
    ui->runButton->setEnabled(false);
}
