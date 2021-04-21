#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H


#include <QThread> // Inherited
#include <QWidget> // Inherited
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QWaitCondition>


class QCheckBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTextBrowser;

class TestMessage;
class FileSelector;

class FileQueue final
{
public:
	FileQueue() {}

	QString dequeue();

	bool isEmpty() { return count() == 0; }
	int count();

	void init( const QString & directory, const QStringList & extensions, bool recursive );
	void clear();

protected:
	QQueue<QString> make( const QString & directory, const QStringList & extensions, bool recursive );

	QMutex mutex;
	QQueue<QString> queue;
};

class TestThread final : public QThread
{
	Q_OBJECT

public:
	TestThread( QObject * o, FileQueue * q );
	~TestThread();

	QString blockMatch;
	quint32 verMatch = 0;
	bool reportAll = false;

signals:
	void sigStart( const QString & file );
	void sigReady( const QString & result );

protected:
	void run() override final;

	QList<TestMessage> checkLinks( const class NifModel * nif, const class QModelIndex & iParent, bool kf );

	FileQueue * queue;

	QMutex quit;
};

//! The XML checker widget.
class TestShredder final : public QWidget
{
	Q_OBJECT

public:
	TestShredder();
	~TestShredder();

	static TestShredder * create();

protected slots:
	void chooseBlock();
	void run();
	void xml();

	void threadStarted();
	void threadFinished();

	void renumberThreads( int );

protected:
	void closeEvent( QCloseEvent * ) override final;

	FileSelector * directory;
	QLineEdit * blockMatch;
	QCheckBox * recursive;
	QCheckBox * chkNif, * chkKf, * chkKfm;
	QCheckBox * repErr;
	QSpinBox * count;
	QLineEdit * verMatch;
	QTextBrowser * text;
	QProgressBar * progress;
	QLabel * label;
	QPushButton * btRun;

	FileQueue queue;

	QList<TestThread *> threads;

	QDateTime time;
};

#endif
