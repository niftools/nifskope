#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QProgressBar>
#include <QDateTime>
#include <QTextEdit>
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

signals:
	void sigStart( const QString & file );
	void sigReady( const QString & result );
	
protected:
	void run();
	
	QList<Message> checkLinks( const class NifModel * nif, const class QModelIndex & iParent );
	
	FileQueue * queue;
	
	QMutex quit;
};

class Browser : public QTextEdit
{
	Q_OBJECT
public:
	Browser();

signals:
	void sigAnchorClicked( const QString & );
	
protected:
	void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
	
	QPoint pressPos;
};

class TestShredder : public QWidget
{
	Q_OBJECT
public:
	TestShredder();
	~TestShredder();
	
protected slots:
	void browse();
	void chooseBlock();
	void run();
	void xml();
	
	void threadStarted();
	void threadFinished();
	
	void renumberThreads( int );
	
	void sltOpenNif( const QString & );

protected:
	void	closeEvent( QCloseEvent * );
	
	QLineEdit	* directory;
	QLineEdit	* blockMatch;
	QCheckBox	* recursive;
	QCheckBox	* chkNif, * chkKf, * chkKfm;
	QSpinBox	* count;
	Browser		* text;
	QProgressBar * progress;
	QPushButton * btRun;
	
	FileQueue queue;

	QList<TestThread*> threads;
	
	QDateTime time;
};

#endif
