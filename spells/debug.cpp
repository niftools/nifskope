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

class spThreadLoad : public Spell
{
public:
	QString name() const { return "Thread Load"; }
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

REGISTER_SPELL( spThreadLoad )

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
	
	QPushButton * btXML = new QPushButton( "Reload XML", this );
	connect( btXML, SIGNAL( clicked() ), this, SLOT( xml() ) );
	
	QPushButton * btClose = new QPushButton( "Close", this );
	connect( btClose, SIGNAL( clicked() ), this, SLOT( close() ) );
	
	threadGroup = new QGroupBox( "Threads", this );
	
	threadNumber = new QSpinBox();
	threadNumber->setRange( 1, 8 );
	connect( threadNumber, SIGNAL( valueChanged( int ) ), this, SLOT( setThreadNumber( int ) ) );
	
	threadsVisible = new QCheckBox( "show threads" );
	threadsVisible->setChecked( true );
	connect( threadsVisible, SIGNAL( toggled( bool ) ), this, SLOT( setThreadsVisible( bool ) ) );
	
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
	hbox->addWidget( btXML );
	hbox->addWidget( btClose );
	
	threadGroup->setLayout( threadLayout = new QVBoxLayout() );
	threadLayout->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( new QLabel( "Thread Count" ) );
	hbox->addWidget( threadNumber );
	hbox->addWidget( threadsVisible );

	setThreadNumber( NUM_THREADS );
}

TestShredder::~TestShredder()
{
	queue.clear();
}

void TestShredder::xml()
{
	btRun->setChecked( false );
	queue.clear();
	foreach ( ThreadStruct thread, threads )
		thread.thread->wait();
	NifModel::loadXML();
}

void TestShredder::setThreadNumber( int num )
{
	threadNumber->setValue( num );
	while ( threads.count() < num )
	{
		ThreadStruct thread;
		
		thread.thread = new TestThread( this );
		connect( thread.thread, SIGNAL( started() ), this, SLOT( threadStarted() ) );
		connect( thread.thread, SIGNAL( finished() ), this, SLOT( threadFinished() ) );
		
		thread.box = new ThreadBox( thread.thread, threadLayout );
		thread.box->setVisible( threadsVisible->isChecked() );
		
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
		if ( thread.box )
			delete thread.box;
		delete thread.thread;
	}
}

void TestShredder::setThreadsVisible( bool x )
{
	threadsVisible->setChecked( x );
	foreach ( ThreadStruct thread, threads )
	{
		thread.box->setVisible( x );
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
	
	time = QDateTime::currentDateTime();

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
		//text->append( QString( "<br>run time %1 [s]<br>" ).arg( time.secsTo( QDateTime::currentDateTime() ) ) );
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
	nif->setMessageMode( NifModel::CollectMessages );
	connect( nif, SIGNAL( sigProgress( int, int ) ), this, SIGNAL( sigProgress( int, int ) ) );
}

TestThread::~TestThread()
{
	wait();
}

void TestThread::run()
{
	emit sigStart( filepath );
	nif->load( filepath );
	result = QString( "<b>%1</b> (%2)<br>" ).arg( filepath ).arg( nif->getVersion() );
	foreach ( Message msg, nif->getMessages() )
	{
		if ( msg.type() != QtDebugMsg )
		{
			result += msg + "<br>";
		}
	}
}

/*
 *  Thread Box
 */
 
ThreadBox::ThreadBox( TestThread * thread, QLayout * parentLayout )
{
	prog = new QProgressBar( this );
	label = new QLabel( this );

	parentLayout->addWidget( this );
	//label->setVisible( true );
	//prog->setVisible( true );
	//setVisible( true );

	connect( thread, SIGNAL( sigStart( const QString & ) ), label, SLOT( setText( const QString & ) ) );
	connect( thread, SIGNAL( sigProgress( int, int ) ), this, SLOT( sltProgress( int, int ) ) );
}

QSize ThreadBox::sizeHint() const
{
	QSize sl = label->sizeHint();
	QSize sp = prog->sizeHint();
	
	return QSize( sl.width() + sp.width(), qMax( sl.height(), sp.height() ) );
}

void ThreadBox::resizeEvent( QResizeEvent * )
{
	label->setGeometry( 0, 0, width() / 2, height() );
	prog->setGeometry( width() / 2, 0, width() / 2, height() );
}

void ThreadBox::sltProgress( int x, int y )
{
	prog->setRange( 0, y );
	prog->setValue( x );
}
