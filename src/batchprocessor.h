#ifndef BATCHPROCESSING_H
#define BATCHPROCESSING_H

#include "spellbook.h"

#include <QDialog>

namespace Ui {
class BatchProcessor;
}

class BatchProcessor : public QDialog
{
    Q_OBJECT

public:
    explicit BatchProcessor(QWidget *parent = 0);
    ~BatchProcessor();
    static BatchProcessor *create();

private:
    Ui::BatchProcessor *ui;
    QMenu *addActionMenu;

public slots:
    void actionTriggered(QAction *action);
    void on_addFilesButton_pressed();
    void on_deleteFileButton_pressed();
    void on_runButton_pressed();
};

#endif // BATCHPROCESSING_H
