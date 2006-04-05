#include "../spellbook.h"

#include "../kfmmodel.h"

#include "debug.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>
#include <QToolButton>
#include <QQueue>

#define NUM_THREADS 2

class spThreadLoad : public Spell
{
public:
	QString name() const { return "XML Checker"; }
	QString page() const { return ""; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( ! nif || ! index.isValid() );
	}
	
	QModelIndex cast( NifModel *, const QModelIndex & index )
	{
		TestShredder * shredder = new TestShredder();
		shredder->setAttribute( Qt::WA_DeleteOnClose );
		shredder->show();
		return index;
	}
};

REGISTER_SPELL( spThreadLoad )

TestShredder::TestShredder()
	: QWidget()
{
	QSettings settings( "NifTools", "NifSkope" );
	settings.beginGroup( "XML Checker" );

	directory = new QLineEdit( this );
	directory->setText( settings.value( "Directory" ).toString() );
	
	recursive = new QCheckBox( "Recursive", this );
	recursive->setChecked( settings.value( "Recursive", true ).toBool() );
	recursive->setToolTip( "Recurse into sub directories" );
	
	QAction * aBrowse = new QAction( "Dir", this );
	connect( aBrowse, SIGNAL( triggered() ), this, SLOT( browse() ) );
	QToolButton * btBrowse = new QToolButton( this );
	btBrowse->setDefaultAction( aBrowse );
	
	count = new QSpinBox();
	count->setRange( 1, 8 );
	count->setValue( settings.value( "Threads", NUM_THREADS ).toInt() );
	count->setPrefix( "threads " );
	connect( count, SIGNAL( valueChanged( int ) ), this, SLOT( renumberThreads( int ) ) );
	
	text = new QTextEdit( this );
	text->setHidden( false );
	
	progress = new QProgressBar( this );
	
	btRun = new QPushButton( "run", this );
	btRun->setCheckable( true );
	connect( btRun, SIGNAL( clicked() ), this, SLOT( run() ) );
	
	QPushButton * btXML = new QPushButton( "Reload XML", this );
	connect( btXML, SIGNAL( clicked() ), this, SLOT( xml() ) );
	
	QPushButton * btClose = new QPushButton( "Close", this );
	connect( btClose, SIGNAL( clicked() ), this, SLOT( close() ) );
	
	QVBoxLayout * lay = new QVBoxLayout();
	setLayout( lay );
	
	QHBoxLayout * hbox = new QHBoxLayout();
	lay->addLayout( hbox );
	hbox->addWidget( btBrowse );
	hbox->addWidget( directory );
	hbox->addWidget( recursive );
	hbox->addWidget( count );
	
	lay->addWidget( text );
	lay->addWidget( progress );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( btRun );
	hbox->addWidget( btXML );
	hbox->addWidget( btClose );
	
	renumberThreads( count->value() );
}

TestShredder::~TestShredder()
{
	QSettings settings( "NifTools", "NifSkope" );
	settings.beginGroup( "XML Checker" );
	
	settings.setValue( "Directory", directory->text() );
	settings.setValue( "Recursive", recursive->isChecked() );
	settings.setValue( "Threads", count->value() );

	queue.clear();
}

void TestShredder::xml()
{
	btRun->setChecked( false );
	queue.clear();
	foreach ( TestThread * thread, threads )
		thread->wait();
	NifModel::loadXML();
	KfmModel::loadXML();
}

void TestShredder::renumberThreads( int num )
{
	while ( threads.count() < num )
	{
		TestThread * thread = new TestThread( this, &queue );
		connect( thread, SIGNAL( sigStart( const QString & ) ), this, SLOT( threadStarted() ) );
		connect( thread, SIGNAL( sigReady( const QString & ) ), text, SLOT( append( const QString & ) ) );
		connect( thread, SIGNAL( finished() ), this, SLOT( threadFinished() ) );
		threads.append( thread );
		
		if ( btRun->isChecked() )
		{
			thread->start();
		}
	}
	while ( threads.count() > num )
	{
		TestThread * thread = threads.takeLast();
		delete thread;
	}
}

void TestShredder::run()
{
	queue.clear();
	
	if ( ! btRun->isChecked() )
		return;
	
	foreach ( TestThread * thread, threads )
		thread->wait();
	
	text->clear();
	
	queue.init( directory->text(), recursive->isChecked() );
	
	time = QDateTime::currentDateTime();

	progress->setRange( 0, queue.count() );
	progress->setValue( 0 );
	
	foreach ( TestThread * thread, threads )
	{
		thread->start();
	}
}

void TestShredder::threadStarted()
{
	progress->setValue( progress->maximum() - queue.count() );
}

void TestShredder::threadFinished()
{
	if ( queue.isEmpty() )
	{
		foreach ( TestThread * thread, threads )
			if ( thread->isRunning() )
				return;
		
		btRun->setChecked( false );
		text->append( QString::number( time.secsTo( QDateTime::currentDateTime() ) ) );
	}
}

void TestShredder::browse()
{
	QString d = QFileDialog::getExistingDirectory( this, "Choose a folder", directory->text() );
	if ( ! d.isEmpty() )
	{
		directory->setText( d );
	}
}

void TestShredder::closeEvent( QCloseEvent * e )
{
	foreach ( TestThread * thread, threads )
		if ( thread->isRunning() )
		{
			e->ignore();
			queue.clear();
		}
}

/*
 *  File Queue
 */
 
QQueue<QString> FileQueue::make( const QString & dname, bool recursive )
{
	QQueue<QString> queue;
	
	QDir dir( dname );
	if ( recursive )
	{
		dir.setFilter( QDir::Dirs );
		foreach ( QString d, dir.entryList() )
			if ( d != "." && d != ".." )
				queue += make( dir.filePath( d ), true );
	}
	
	dir.setFilter( QDir::Files );
	dir.setNameFilters( QStringList() << "*.kfm" << "*.KFM" << "*.nif" << "*.NIF" << "*.kf" << "*.KF" << "*.kfa" << "*.KFA" );
	foreach ( QString f, dir.entryList() )
		queue.enqueue( dir.filePath( f ) );
	
	return queue;
}

void FileQueue::init( const QString & dname, bool recursive )
{
	QQueue<QString> queue = make( dname, recursive );
	
	mutex.lock();
	this->queue = queue;
	mutex.unlock();
}

QString FileQueue::dequeue()
{
	QMutexLocker lock( & mutex );
	if ( queue.isEmpty() )
		return QString();
	else
		return queue.dequeue();
}

int FileQueue::count()
{
	QMutexLocker lock( & mutex );
	return queue.count();
}

void FileQueue::clear()
{
	QMutexLocker lock( & mutex );
	queue.clear();
}

/*
 *  Thread
 */

TestThread::TestThread( QObject * o, FileQueue * q )
	: QThread( o ), queue( q )
{
}

TestThread::~TestThread()
{
	if ( isRunning() )
	{
		quit.lock();
		wait();
		quit.unlock();
	}
}

void TestThread::run()
{
	NifModel nif;
	nif.setMessageMode( BaseModel::CollectMessages );
	KfmModel kfm;
	kfm.setMessageMode( BaseModel::CollectMessages );

	QString filepath = queue->dequeue();
	while ( ! filepath.isEmpty() )
	{
		emit sigStart( filepath );
		
		BaseModel * model = & nif;
		if ( filepath.endsWith( ".KFM", Qt::CaseInsensitive ) )
			model = & kfm;
		
		model->loadFromFile( filepath );
		
		QString result = QString( "<b>%1</b> (%2)<br>" ).arg( filepath ).arg( model->getVersion() );
		foreach ( Message msg, model->getMessages() )
		{
			if ( msg.type() != QtDebugMsg )
				result += msg + "<br>";
		}
		emit sigReady( result );
		
		if ( quit.tryLock() )
			quit.unlock();
		else
			break;
		
		filepath = queue->dequeue();
	}
}

