#ifndef CONVERSIONDIALOG_H
#define CONVERSIONDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QProgressBar>
#include <QProgressDialog>
#include <QMap>
#include <QFutureWatcher>

namespace Ui {
class ConversionDialog;
}

class ConversionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConversionDialog(QWidget *parent = nullptr);
    ~ConversionDialog();

private slots:
    void on_buttonBox_accepted();
    void on_pushButton_clicked();

private:
    Ui::ConversionDialog *ui;
    void progressDialog(int iterations);
};

class ProgressReceiver : public QObject
{
    Q_OBJECT

public:
    ProgressReceiver(QWidget *parent = nullptr);
public slots:
    void addBar(Qt::HANDLE tID, int range, const QString & barText);
    void update(Qt::HANDLE tID, int i);
    void update2(QProgressBar * bar, int i);
signals:
    void barReady(Qt::HANDLE tID, QProgressBar *bar);
private:
    QMap<Qt::HANDLE, QProgressBar*> bars;
    QWidget * parent;
    QMutex mu;
};

class ProgressUpdater : public QObject
{
    Q_OBJECT

public:
    ProgressUpdater(Qt::HANDLE threadID, ProgressReceiver & receiver, int range, QString barText = "%p% - %v out of %m processed");
    void progress(int i);
public slots:
    void receiveBarPointer(Qt::HANDLE tID, QProgressBar *newBar);
signals:
    void start(Qt::HANDLE tID, int range, const QString & barText);
    void progress(Qt::HANDLE tID, int i);
    void progress2(QProgressBar *bar, int i);
private:
    Qt::HANDLE threadID;
    QProgressBar * bar;
};

class ProgressGrid : public QProgressDialog
{
    Q_OBJECT

public:
    ProgressGrid(QFutureWatcher<void> & futureWatcher);
public slots:
    void resizeEvent(QResizeEvent * event);
};

#endif // CONVERSIONDIALOG_H
