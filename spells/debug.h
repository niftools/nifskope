#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QProgressBar>
#include <QDateTime>

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

class NifModel;

class TestThread : public QThread
{
	Q_OBJECT
public:
	TestThread( QObject * o );
	~TestThread();
	
	QString filepath;
	
	QString result;

signals:
	void sigStart( const QString & file );
	void sigProgress( int, int );
	
protected:
	void run();
	
	NifModel * nif;
	
};

class ThreadBox : public QWidget
{
	Q_OBJECT
public:
	ThreadBox( TestThread * thread, QLayout * parentLayout );
	
	QSize sizeHint() const;
	
public slots:
	void sltProgress( int, int );
	
protected:
	QLabel * label;
	QProgressBar * prog;
	
	void resizeEvent( QResizeEvent * e );
};

class TestShredder : public QWidget
{
	Q_OBJECT
public:
	TestShredder();
	~TestShredder();
	
public slots:
	void setThreadNumber( int );
	void setThreadsVisible( bool );
	
protected:
	QLineEdit	* directory;
	QCheckBox * chkRecursive;
	QProgressBar * progMain;
	QPushButton * btRun;
	QTextEdit * text;
	QVBoxLayout * threadLayout;
	QGroupBox * threadGroup;
	QSpinBox * threadNumber;
	QCheckBox * threadsVisible;
	
protected slots:
	void browse();
	void run();
	void xml();
	
	void threadStarted();
	void threadFinished();
	
protected:
	QQueue<QString> queue;

	struct ThreadStruct
	{
		TestThread * thread;
		ThreadBox * box;
	};
	
	QList<ThreadStruct> threads;
	
	QDateTime time;
};

#endif
