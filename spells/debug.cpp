#include "../spellbook.h"

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
#include <QSpinBox>
#include <QTextEdit>
#include <QToolButton>
#include <QQueue>

#define NUM_THREADS 2

class spTestLoad : public Spell
{
public:
	QString name() const { return "Test Load"; }
	QString page() const { return "Debug"; }
	
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

REGISTER_SPELL( spTestLoad )

TestShredder::TestShredder()
	: QWidget()
{
	directory = new QLineEdit( this );
	directory->setText( "f:\\nif" );
	
	chkRecursive = new QCheckBox( "Recursive", this );
	chkRecursive->setChecked( true );
	chkRecursive->setToolTip( "Recurse into sub directories" );
	
	QAction * aBrowse = new QAction( "browse", this );
	connect( aBrowse, SIGNAL( triggered() ), this, SLOT( browse() ) );
	
	QToolButton * btBrowse = new QToolButton( this );
	btBrowse->setDefaultAction( aBrowse );
	
	text = new QTextEdit( this );
	text->setHidden( false );
	
	progMain = new QProgressBar( this );
	
	btRun = new QPushButton( "run", this );
	btRun->setCheckable( true );
	connect( btRun, SIGNAL( clicked() ), this, SLOT( run() ) );
	
	QPushButton * btClose = new QPushButton( "Close", this );
	connect( btClose, SIGNAL( clicked() ), this, SLOT( close() ) );
	
	threadGroup = new QGroupBox( "Threads", this );
	
	threadNumber = new QSpinBox();
	threadNumber->setRange( 1, 8 );
	connect( threadNumber, SIGNAL( valueChanged( int ) ), this, SLOT( setThreadNumber( int ) ) );
	
	QVBoxLayout * lay = new QVBoxLayout();
	setLayout( lay );
	
	QHBoxLayout * hbox = new QHBoxLayout();
	lay->addLayout( hbox );
	hbox->addWidget( new QLabel( "Dir" ) );
	hbox->addWidget( directory );
	hbox->addWidget( btBrowse );
	hbox->addWidget( chkRecursive );
	
	lay->addWidget( text );
	lay->addWidget( progMain );
	lay->addWidget( threadGroup );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( btRun );
	hbox->addWidget( btClose );
	
	threadGroup->setLayout( threadLayout = new QVBoxLayout() );
	threadLayout->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( new QLabel( "Thread Count" ) );
	hbox->addWidget( threadNumber );

	setThreadNumber( NUM_THREADS );
}

TestShredder::~TestShredder()
{
	queue.clear();
}

void TestShredder::setThreadNumber( int num )
{
	threadNumber->setValue( num );
	while ( threads.count() < num )
	{
		ThreadStruct thread;
		
		thread.label = new QLabel();
		thread.progbar = new ProgBar();
		thread.layout = new QHBoxLayout();
		thread.layout->addWidget( thread.label, 1 );
		thread.layout->addWidget( thread.progbar, 1 );
		threadLayout->addLayout( thread.layout );
		thread.label->setVisible( true );
		thread.progbar->setVisible( true );
		
		thread.thread = new TestThread( this );
		connect( thread.thread, SIGNAL( sigStart( const QString & ) ), thread.label, SLOT( setText( const QString & ) ) );
		connect( thread.thread, SIGNAL( sigProgress( int, int ) ), thread.progbar, SLOT( sltProgress( int, int ) ) );
		
		connect( thread.thread, SIGNAL( started() ), this, SLOT( threadStarted() ) );
		connect( thread.thread, SIGNAL( finished() ), this, SLOT( threadFinished() ) );
		
		threads.append( thread );
		
		if ( ! queue.isEmpty() && btRun->isChecked() )
		{
			thread.thread->filepath = queue.dequeue();
			thread.thread->start();
		}
	}
	while ( threads.count() > num )
	{
		if ( threads.isEmpty() )
			return;
		
		ThreadStruct thread = threads.takeLast();
		delete thread.label;
		delete thread.progbar;
		delete thread.layout;
		delete thread.thread;
	}
}

QQueue<QString> makeQueue( const QString & dname, bool recursive )
{
	QQueue<QString> queue;
	
	QDir dir( dname );
	if ( recursive )
	{
		dir.setFilter( QDir::Dirs );
		foreach ( QString d, dir.entryList() )
			if ( d != "." && d != ".." )
				queue += makeQueue( dir.filePath( d ), true );
	}
	
	dir.setFilter( QDir::Files );
	dir.setNameFilters( QStringList() << "*.nif" << "*.NIF" << "*.kf" << "*.KF" << "*.kfa" << "*.KFA" );
	foreach ( QString f, dir.entryList() )
		queue.enqueue( dir.filePath( f ) );
	return queue;
}

void TestShredder::run()
{
	queue.clear();
	
	if ( ! btRun->isChecked() )
		return;
	
	foreach ( ThreadStruct thread, threads )
		thread.thread->wait();
	
	text->clear();
	
	queue = makeQueue( directory->text(), chkRecursive->isChecked() );

	progMain->setRange( 0, queue.count() );
	progMain->setValue( 0 );
	
	foreach ( ThreadStruct thread, threads )
	{
		if ( ! queue.isEmpty() )
		{
			thread.thread->filepath = queue.dequeue();
			thread.thread->start();
		}
	}
}

void TestShredder::threadStarted()
{
	progMain->setValue( progMain->maximum() - queue.count() );
}

void TestShredder::threadFinished()
{
	Q_ASSERT( sender() );
	TestThread * thread = qobject_cast<TestThread*>( sender() );
	if ( thread && ! thread->result.isEmpty() )
		text->append( thread->result );

	if ( queue.isEmpty() )
	{
		btRun->setChecked( false );
	}
	else if ( thread )
	{
		thread->filepath = queue.dequeue();
		thread->start();
	}
}

void TestShredder::browse()
{
	QString d = QFileDialog::getExistingDirectory( this, "Choose a folder" );
	if ( ! d.isEmpty() )
	{
		directory->setText( d );
	}
}

/*
 *  Thread
 */

TestThread::TestThread( QObject * o )
	: QThread( o )
{
	nif = new NifModel( this );
	connect( nif, SIGNAL( sigProgress( int, int ) ), this, SIGNAL( sigProgress( int, int ) ) );
	connect( nif, SIGNAL( sigMessage( const Message & ) ), this, SLOT( sltMessage( const Message & ) ) );
}

TestThread::~TestThread()
{
	wait();
}

void TestThread::run()
{
	emit sigStart( filepath );
	result.clear();
	nif->load( filepath );
	result.prepend( QString( "<b>%1</b> (%2)<br>" ).arg( filepath ).arg( nif->getVersion() ) );
}

void TestThread::sltMessage( const Message & msg )
{
	if ( msg.type() != QtDebugMsg )
	{
		result += msg + "<br>";
	}
}

void ProgBar::sltProgress( int x, int y )
{
	setRange( 0, y );
	setValue( x );
}
