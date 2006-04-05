#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QProgressBar>
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
class QTextEdit;
class QVBoxLayout;

class FileQueue
{
public:
	FileQueue() {}

	QString dequeue();
	
	bool isEmpty() { return count() == 0; }
	int count();
	
	void init( const QString & directory, bool recursive );
	void clear();

protected:
	QQueue<QString> make( const QString & directory, bool recursive );

	QMutex mutex;
	QQueue<QString> queue;
};

class TestThread : public QThread
{
	Q_OBJECT
public:
	TestThread( QObject * o, FileQueue * q );
	~TestThread();
	
signals:
	void sigStart( const QString & file );
	void sigReady( const QString & result );
	
protected:
	void run();
	
	FileQueue * queue;

	QMutex quit;
};

class TestShredder : public QWidget
{
	Q_OBJECT
public:
	TestShredder();
	~TestShredder();
	
protected slots:
	void browse();
	void run();
	void xml();
	
	void threadStarted();
	void threadFinished();
	
	void renumberThreads( int );

protected:
	void	closeEvent( QCloseEvent * );
	
	QLineEdit	* directory;
	QCheckBox	* recursive;
	QSpinBox	* count;
	QTextEdit	* text;
	QProgressBar * progress;
	QPushButton * btRun;
	
	FileQueue queue;

	QList<TestThread*> threads;
	
	QDateTime time;
};

#endif
