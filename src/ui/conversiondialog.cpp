#include "conversiondialog.h"
#include "ui_conversiondialog.h"

#include <QProgressDialog>
#include <QProgressBar>
#include <QFuture>
#include <QFutureWatcher>
#include <QThread>
#include <QtConcurrent>
#include <QObject>

#include <QLayout>
#include <QLabel>

ConversionDialog::ConversionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConversionDialog)
{
    ui->setupUi(this);
}

ConversionDialog::~ConversionDialog()
{
    delete ui;
}

void ConversionDialog::on_buttonBox_accepted()
{
    // get the number of iterations
    int iterations = ui->spinBoxIterations->text().toInt();

    progressDialog(iterations);
}

ProgressReceiver::ProgressReceiver(QWidget *parent) : parent(parent)
{
    //
}

void ProgressReceiver::addBar(Qt::HANDLE tID, int range, const QString & barText)
{
    QMutexLocker locker(&mu);
    QProgressBar * bar = bars.value(tID, nullptr);

    if (bar) {
        bar->setFormat("");
    } else {
        bar = new QProgressBar(parent);

        if (!bar) {
            return;
        }

        bars[tID] = bar;

        bar->setAlignment(Qt::AlignCenter);

        if (parent->layout()) {
            parent->layout()->addWidget(bar);
        }
    }

    bar->setValue(0);
    bar->setRange(0, range);
    bar->setFormat(barText);

    bar->show();

    emit barReady(tID, bar);
}

void ProgressReceiver::update(Qt::HANDLE tID, int i)
{
    QProgressBar * bar = bars.value(tID, nullptr);

    if (!bar) {
        return;
    }

    bar->setValue(i);
}

void ProgressReceiver::update2(QProgressBar *bar, int i)
{
    if (bar) {
        bar->setValue(i);
    }
}

void ConversionDialog::on_pushButton_clicked()
{
//    ui->progressBar->
//    QThread::msleep(1000);
    progressDialog(100);
}

void ConversionDialog::progressDialog(int iterations)
{
    // Prepare the vector.
    QVector<int> vector;
    for (int i = 0; i < iterations; ++i)
        vector.append(i);

    // Create a QFutureWatcher and connect signals and slots.
    // Monitor progress changes of the future
    QFutureWatcher<void> futureWatcher;

    // Create a progress dialog.
    ProgressGrid dialog(futureWatcher);

    ProgressReceiver receiver = ProgressReceiver(&dialog);

    qRegisterMetaType<Qt::HANDLE>("Qt::HANDLE");

    // Start the computation.
    futureWatcher.setFuture(QtConcurrent::map(
            vector,
            [&](int & iteration) {
                const int work = 100;
                volatile int v = 0;

                ProgressUpdater updater(QThread::currentThreadId(), receiver, work);

                for (int j = 0; j <= work; ++j) {
                    if (futureWatcher.future().isCanceled()) {
                        return;
                    }

                    ++v;
                    QThread::msleep(10);

                    emit updater.progress(QThread::currentThreadId(), j);
                }

                qDebug() << "iteration" << iteration << "in thread" << QThread::currentThreadId();
            }));

    // Display the dialog and start the event loop.
    dialog.exec();

    futureWatcher.waitForFinished();

    // Query the future to check if was canceled.
    qDebug() << "Canceled?" << futureWatcher.future().isCanceled();
}

ProgressGrid::ProgressGrid(QFutureWatcher<void> & futureWatcher)
{
    this->setLabelText(QString("Converting..."));

    QVBoxLayout * layout = new QVBoxLayout;
    new QLabel("Thread(s):", this);

    for (QObject *obj : this->children()) {
        QWidget *widget = qobject_cast<QWidget *>(obj);
        if (widget) {
            layout->addWidget(widget);
        }
    }

    this->setLayout(layout);

    QObject::connect(&futureWatcher, SIGNAL(finished()),
                     this,           SLOT(reset()));
    QObject::connect(this,           SIGNAL(canceled()),
                     &futureWatcher, SLOT(cancel()));

    QObject::connect(&futureWatcher, SIGNAL(progressRangeChanged(int,int)),
                     this,           SLOT(setRange(int,int)));
    QObject::connect(&futureWatcher, SIGNAL(progressValueChanged(int)),
                     this,           SLOT(setValue(int)));
}

void ProgressGrid::resizeEvent(QResizeEvent *event)
{
    //
}

ProgressUpdater::ProgressUpdater(Qt::HANDLE threadID, ProgressReceiver & receiver, int range, QString barText)
    : threadID(threadID)
{
    QObject::connect(this,      &ProgressUpdater::start,
                     &receiver, &ProgressReceiver::addBar, Qt::QueuedConnection);
//    QObject::connect(this,      &ProgressUpdater::progress,
//                     &receiver, &ProgressReceiver::update, Qt::QueuedConnection);

    QObject::connect(&receiver, &ProgressReceiver::barReady,
                     this,      &ProgressUpdater::receiveBarPointer, Qt::QueuedConnection);

    QObject::connect(this,      &ProgressUpdater::progress2,
                     &receiver, &ProgressReceiver::update2, Qt::QueuedConnection);

    bar = nullptr;

    emit start(threadID, range, barText);
}

void ProgressUpdater::progress(int i)
{
    if (!bar) {
        QCoreApplication::processEvents();

        if (!bar) {
            return;
        }
    }

    emit progress2(bar, i);
}

void ProgressUpdater::receiveBarPointer(Qt::HANDLE tID, QProgressBar *newBar)
{
    if (tID != threadID) {
        return;
    }

    // Bar pointer already set
    if (bar) {
        return;
    }

    bar = newBar;
}
