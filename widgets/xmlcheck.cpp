#include "xmlcheck.h"

#include "../kfmmodel.h"
#include "../nifmodel.h"
#include "../config.h"

#include "fileselect.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDebug>
#include <QDir>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextBrowser>
#include <QToolButton>
#include <QQueue>

#define NUM_THREADS 2


TestShredder * TestShredder::create()
{
	TestShredder * shredder = new TestShredder();
	shredder->setAttribute( Qt::WA_DeleteOnClose );
	shredder->show();
	return shredder;
}


TestShredder::TestShredder()
	: QWidget()
{
	NIFSKOPE_QSETTINGS(settings);
	settings.beginGroup( "XML Checker" );

	directory = new FileSelector( FileSelector::Folder, "Dir", QBoxLayout::RightToLeft );
	directory->setText( settings.value( "Directory" ).toString() );
	
	recursive = new QCheckBox( tr("Recursive"), this );
	recursive->setChecked( settings.value( "Recursive", true ).toBool() );
	recursive->setToolTip( tr("Recurse into sub directories") );
	
	chkNif = new QCheckBox( tr("*.nif"), this );
	chkNif->setChecked( settings.value( "check nif", true ).toBool() );
	chkNif->setToolTip( tr("Check .nif files") );
	
	chkKf = new QCheckBox( tr("*.kf(a)"), this );
	chkKf->setChecked( settings.value( "check kf", true ).toBool() );
	chkKf->setToolTip( tr("Check .kf files") );
	
	chkKfm = new QCheckBox( tr("*.kfm"), this );
	chkKfm->setChecked( settings.value( "check kfm", true ).toBool() );
	chkKfm->setToolTip( tr("Check .kfm files") );
	
	QAction * aChoose = new QAction( tr("Block Match"), this );
	connect( aChoose, SIGNAL( triggered() ), this, SLOT( chooseBlock() ) );
	QToolButton * btChoose = new QToolButton( this );
	btChoose->setDefaultAction( aChoose );
	
	blockMatch = new QLineEdit( this );

	repErr = new QCheckBox( tr("report errors only"), this );
	repErr->setChecked( settings.value( "report errors only", true ).toBool() );
	
	count = new QSpinBox();
	count->setRange( 1, 8 );
	count->setValue( settings.value( "Threads", NUM_THREADS ).toInt() );
	connect( count, SIGNAL( valueChanged( int ) ), this, SLOT( renumberThreads( int ) ) );

	//Version Check
	verMatch = new QLineEdit(this);

	text = new QTextBrowser();
	text->setHidden( false );
	text->setReadOnly( true );
	text->setOpenExternalLinks( true );
	
	progress = new QProgressBar( this );
	
	label = new QLabel( this );
	label->setHidden( true );
	
	btRun = new QPushButton( tr("run"), this );
	btRun->setCheckable( true );
	connect( btRun, SIGNAL( clicked() ), this, SLOT( run() ) );
	
	QPushButton * btXML = new QPushButton( tr("Reload XML"), this );
	connect( btXML, SIGNAL( clicked() ), this, SLOT( xml() ) );
	
	QPushButton * btClose = new QPushButton( tr("Close"), this );
	connect( btClose, SIGNAL( clicked() ), this, SLOT( close() ) );
	
	QVBoxLayout * lay = new QVBoxLayout();
	setLayout( lay );
	
	QHBoxLayout * hbox = new QHBoxLayout();
	lay->addLayout( hbox );
	hbox->addWidget( directory );
	hbox->addWidget( recursive );
	hbox->addWidget( chkNif );
	hbox->addWidget( chkKf );
	hbox->addWidget( chkKfm );
	
	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( btChoose );
	hbox->addWidget( blockMatch );
	hbox->addWidget( repErr );
	hbox->addWidget( new QLabel( tr("Threads:") ) );
	hbox->addWidget( count );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( new QLabel( tr("Version Match:") ) );
	hbox->addWidget( verMatch );
	
	lay->addWidget( text );
	
	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( progress );
	hbox->addWidget( label );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( btRun );
	hbox->addWidget( btXML );
	hbox->addWidget( btClose );
	
	renumberThreads( count->value() );
}

TestShredder::~TestShredder()
{
	NIFSKOPE_QSETTINGS(settings);
	settings.beginGroup( "XML Checker" );
	
	settings.setValue( "Directory", directory->text() );
	settings.setValue( "Recursive", recursive->isChecked() );
	settings.setValue( "check nif", chkNif->isChecked() );
	settings.setValue( "check kf", chkKf->isChecked() );
	settings.setValue( "check kfm", chkKfm->isChecked() );
	settings.setValue( "report errors only", repErr->isChecked() );
	settings.setValue( "Threads", count->value() );

	queue.clear();
}

void TestShredder::xml()
{
	//btRun->setChecked( false );
	//queue.clear();
	//foreach ( TestThread * thread, threads )
	//	thread->wait();
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
		
		thread->blockMatch = blockMatch->text();
		thread->verMatch = NifModel::version2number( verMatch->text() );
		thread->reportAll = ! repErr->isChecked();
		
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
	progress->setMaximum( progress->maximum() - queue.count() );
	queue.clear();
	
	if ( ! btRun->isChecked() )
		return;
	
	foreach ( TestThread * thread, threads )
		thread->wait();
	
	text->clear();
	label->setHidden( true );
	
	QStringList extensions;
	if ( chkNif->isChecked() )
		extensions << "*.nif" << "*.nifcache" << "*.texcache" << "*.pcpatch";
	if ( chkKf->isChecked() )
		extensions << "*.kf" << "*.kfa";
	if ( chkKfm->isChecked() )
		extensions << "*.kfm";
	
	queue.init( directory->text(), extensions, recursive->isChecked() );
	
	time = QDateTime::currentDateTime();

	progress->setRange( 0, queue.count() );
	progress->setValue( 0 );
	
	foreach ( TestThread * thread, threads )
	{
		thread->verMatch = NifModel::version2number( verMatch->text() );
		thread->blockMatch = blockMatch->text();
		thread->reportAll = ! repErr->isChecked();
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
		
		label->setText( tr( "%1 files in %2 seconds" ).arg( progress->maximum() ).arg( time.secsTo( QDateTime::currentDateTime() ) ) );
		label->setVisible( true );
	}
}

void TestShredder::chooseBlock()
{
	QStringList ids = NifModel::allNiBlocks();
	ids.sort();
	
	QMap< QString, QMenu *> map;
	foreach ( QString id, ids )
	{
		QString x( "Other" );
		
		if ( id.startsWith( "Ni" ) )
			x = QString("Ni&") + id.mid( 2, 1 ) + "...";
		if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
			x = "Havok";
		if ( id.startsWith( "BS" ) || id == "AvoidNode" || id == "RootCollisionNode" )
			x = "Bethesda";
		if ( id.startsWith( "Fx" ) )
			x = "Firaxis";
		
		if ( ! map.contains( x ) )
			map[ x ] = new QMenu( x );
		map[ x ]->addAction( id );
	}

	QMenu menu;
	foreach ( QMenu * m, map )
		menu.addMenu( m );
	
	QAction * act = menu.exec( QCursor::pos() );
	if ( act )
		blockMatch->setText( act->text() );
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
 
QQueue<QString> FileQueue::make( const QString & dname, const QStringList & extensions, bool recursive )
{
	QQueue<QString> queue;
	
	QDir dir( dname );
	if ( recursive )
	{
		dir.setFilter( QDir::Dirs );
		foreach ( QString d, dir.entryList() )
			if ( d != "." && d != ".." )
				queue += make( dir.filePath( d ), extensions, true );
	}
	
	dir.setFilter( QDir::Files );
	dir.setNameFilters( extensions );
	foreach ( QString f, dir.entryList() )
		queue.enqueue( dir.filePath( f ) );
	
	return queue;
}

void FileQueue::init( const QString & dname, const QStringList & extensions, bool recursive )
{
	QQueue<QString> queue = make( dname, extensions, recursive );
	
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
	reportAll = true;
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
		QReadWriteLock * lock = &nif.XMLlock;
		
		if ( filepath.endsWith( ".KFM", Qt::CaseInsensitive ) )
		{
			model = & kfm;
			lock = & kfm.XMLlock;
		}
		
		bool kf = ( filepath.endsWith( ".KF", Qt::CaseInsensitive ) || filepath.endsWith( ".KFA", Qt::CaseInsensitive ) );
		
		{	// lock the XML lock
			QReadLocker lck( lock );
			
			if (  model == &nif && nif.earlyRejection( filepath, blockMatch, verMatch ) )
			{
				bool loaded = model->loadFromFile( filepath );
				
				QString result = QString( "<a href=\"nif:%1\">%1</a> (%2)" ).arg( filepath ).arg( model->getVersion() );
				QList<Message> messages = model->getMessages();
				
				bool blk_match = false;
				if ( loaded && model == & nif )
					for ( int b = 0; b < nif.getBlockCount(); b++ )
					{
						//In case early rejection failed, such as if this is an older file without the block types in the header
						//note if any of these blocks types match the specified one.
						if ( blockMatch.isEmpty() == false && nif.inherits( nif.getBlockName( nif.getBlock(b) ), blockMatch ) ) {
							blk_match = true;
						}
						messages += checkLinks( &nif, nif.getBlock( b ), kf );
					}
				
				bool rep = reportAll;

				//Don't show anything if block match is on but the requested type wasn't found & we're in block match mode

				if ( blockMatch.isEmpty() == true || blk_match == true )
				{
					foreach ( Message msg, messages )
					{
						if ( msg.type() != QtDebugMsg )
						{
							result += "<br>" + msg;
							rep |= true;
						}
					}
					if ( rep )
						emit sigReady( result );
				}
			}
		}
		
		if ( quit.tryLock() )
			quit.unlock();
		else
			break;
		
		filepath = queue->dequeue();
	}	
}

static QString linkId( const NifModel * nif, QModelIndex idx )
{
	QString id = QString( "%1 (%2)" ).arg( nif->itemName( idx ) ).arg( nif->itemTmplt( idx ) );
	while ( idx.parent().isValid() )
	{
		idx = idx.parent();
		id.prepend( QString( "%1/" ).arg( nif->itemName( idx ) ) );
	}
	return id;
}

QList<Message> TestThread::checkLinks( const NifModel * nif, const QModelIndex & iParent, bool kf )
{
	QList<Message> messages;
	for ( int r = 0; r < nif->rowCount( iParent ); r++ )
	{
		QModelIndex idx = iParent.child( r, 0 );
		bool child;
		if ( nif->isLink( idx, &child ) )
		{
			qint32 l = nif->getLink( idx );
			if ( l < 0 )
			{
				//This is not really an error
				//if ( ! child && ! kf )
				//	messages.append( Message() << tr("unassigned parent link") << linkId( nif, idx ) );
			}
			else if ( l >= nif->getBlockCount() )
				messages.append( Message() << tr("invalid link") << linkId( nif, idx ) );
			else
			{
				QString tmplt = nif->itemTmplt( idx );
				if ( ! tmplt.isEmpty() )
				{
					QModelIndex iBlock = nif->getBlock( l );
					if ( ! nif->inherits( iBlock, tmplt ) )
						messages.append( Message() << tr("link") << linkId( nif, idx ) << tr("points to wrong block type") << nif->itemName( iBlock ) );
				}
			}
		}
		if ( nif->rowCount( idx ) > 0 )
			messages += checkLinks( nif, idx, kf );
	}
	return messages;
}
