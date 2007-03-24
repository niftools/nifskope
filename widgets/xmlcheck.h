#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QWaitCondition>

#include "../message.h"

class QCheckBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTextBrowser;
class QVBoxLayout;

class FileSelector;

class FileQueue
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

class TestThread : public QThread
{
	Q_OBJECT
public:
	TestThread( QObject * o, FileQueue * q );
	~TestThread();
	
	QString blockMatch;
	bool reportAll;

signals:
	void sigStart( const QString & file );
	void sigReady( const QString & result );
	
protected:
	void run();
	
	QList<Message> checkLinks( const class NifModel * nif, const class QModelIndex & iParent, bool kf );
	
	FileQueue * queue;
	
	QMutex quit;
};

class TestShredder : public QWidget
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
	void	closeEvent( QCloseEvent * );
	
	FileSelector * directory;
	QLineEdit    * blockMatch;
	QCheckBox    * recursive;
	QCheckBox    * chkNif, * chkKf, * chkKfm;
	QCheckBox    * repErr;
	QSpinBox     * count;
	QTextBrowser * text;
	QProgressBar * progress;
	QLabel       * label;
	QPushButton  * btRun;
	
	FileQueue queue;

	QList<TestThread*> threads;
	
	QDateTime time;
};

#endif
