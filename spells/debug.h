#ifndef SPELL_DEBUG_H
#define SPELL_DEBUG_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QProgressBar>

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
	
protected slots:
	void sltMessage( const Message & msg );
	
protected:
	void run();
	
	NifModel * nif;
	
};

class ProgBar : public QProgressBar
{
	Q_OBJECT
public:
	ProgBar() {}
	
public slots:
	void sltProgress( int, int );
};

class TestShredder : public QWidget
{
	Q_OBJECT
public:
	TestShredder();
	~TestShredder();
	
public slots:
	void setThreadNumber( int );
	
protected:
	QLineEdit	* directory;
	QCheckBox * chkRecursive;
	QProgressBar * progMain;
	QPushButton * btRun;
	QTextEdit * text;
	QVBoxLayout * threadLayout;
	QGroupBox * threadGroup;
	QSpinBox * threadNumber;
	
protected slots:
	void browse();
	void run();
	
	void threadStarted();
	void threadFinished();
	
protected:
	void addThread();
	void delThread();
	
	QQueue<QString> queue;

	struct ThreadStruct
	{
		TestThread * thread;
		QHBoxLayout * layout;
		QLabel * label;
		ProgBar * progbar;
	};
	
	QList<ThreadStruct> threads;	
};

#endif
