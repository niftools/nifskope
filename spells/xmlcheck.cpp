#include "xmlcheck.h"

#include "../spellbook.h"

#include "../kfmmodel.h"
#include "../nifskope.h"

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
#include <QMenu>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
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

	QAction * aBrowse = new QAction( "Dir", this );
	connect( aBrowse, SIGNAL( triggered() ), this, SLOT( browse() ) );
	QToolButton * btBrowse = new QToolButton( this );
	btBrowse->setDefaultAction( aBrowse );
	
	directory = new QLineEdit( this );
	directory->setText( settings.value( "Directory" ).toString() );
	
	recursive = new QCheckBox( "Recursive", this );
	recursive->setChecked( settings.value( "Recursive", true ).toBool() );
	recursive->setToolTip( "Recurse into sub directories" );
	
	chkNif = new QCheckBox( "*.nif", this );
	chkNif->setChecked( settings.value( "check nif", true ).toBool() );
	chkNif->setToolTip( "Check .nif files" );
	
	chkKf = new QCheckBox( "*.kf(a)", this );
	chkKf->setChecked( settings.value( "check kf", true ).toBool() );
	chkKf->setToolTip( "Check .kf files" );
	
	chkKfm = new QCheckBox( "*.kfm", this );
	chkKfm->setChecked( settings.value( "check kfm", true ).toBool() );
	chkKfm->setToolTip( "Check .kfm files" );
	
	QAction * aChoose = new QAction( "Block Match", this );
	connect( aChoose, SIGNAL( triggered() ), this, SLOT( chooseBlock() ) );
	QToolButton * btChoose = new QToolButton( this );
	btChoose->setDefaultAction( aChoose );
	
	blockMatch = new QLineEdit( this );
	
	count = new QSpinBox();
	count->setRange( 1, 8 );
	count->setValue( settings.value( "Threads", NUM_THREADS ).toInt() );
	count->setPrefix( "threads " );
	connect( count, SIGNAL( valueChanged( int ) ), this, SLOT( renumberThreads( int ) ) );
	
	text = new Browser();
	text->setHidden( false );
	text->setReadOnly( true );
	connect( text, SIGNAL( sigAnchorClicked( const QString & ) ), this, SLOT( sltOpenNif( const QString & ) ) );
	
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
	hbox->addWidget( chkNif );
	hbox->addWidget( chkKf );
	hbox->addWidget( chkKfm );
	
	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( btChoose );
	hbox->addWidget( blockMatch );
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
	settings.setValue( "check nif", chkNif->isChecked() );
	settings.setValue( "check kf", chkKf->isChecked() );
	settings.setValue( "check kfm", chkKfm->isChecked() );
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
	
	QStringList extensions;
	if ( chkNif->isChecked() )
		extensions << "*.nif";
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
		thread->blockMatch = blockMatch->text();
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

void TestShredder::chooseBlock()
{
	QStringList ids = NifModel::allNiBlocks();
	ids.sort();
	
	QMap< QChar, QMenu *> map;
	foreach ( QString id, ids )
	{
		QChar x;
		if ( id.startsWith( "Ni" ) )
			x = id[ 2 ].toUpper();
		else
			x = '*';
		if ( ! map.contains( x ) )
			map.insert( x, new QMenu( x ) );
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

void TestShredder::sltOpenNif( const QString & fname )
{
	NifSkope::createWindow( fname );
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
		
		{	// lock the XML lock
			QReadLocker lck( lock );
			
			if ( blockMatch.isEmpty() || ( model == &nif && NifModel::checkForBlock( filepath, blockMatch ) ) )
			{
				bool loaded = model->loadFromFile( filepath );
				
				QString result = QString( "<a href=\"%1\">%2</a> (%3)<br>" ).arg(filepath).arg( filepath ).arg( model->getVersion() );
				QList<Message> messages = model->getMessages();
				
				if ( loaded && model == & nif )
					for ( int b = 0; b < nif.getBlockCount(); b++ )
						messages += checkLinks( &nif, nif.getBlock( b ) );
				
				foreach ( Message msg, messages )
				{
					if ( msg.type() != QtDebugMsg )
						result += msg + "<br>";
				}
				emit sigReady( result );
			}
		}
		
		if ( quit.tryLock() )
			quit.unlock();
		else
			break;
		
		filepath = queue->dequeue();
	}	
}

QString linkId( QModelIndex idx )
{
	QString id = QString( "%1 (%2)" ).arg( idx.data().toString() ).arg( idx.sibling( idx.row(), NifModel::TempCol ).data().toString() );
	while ( idx.parent().isValid() )
	{
		idx = idx.parent();
		id.prepend( QString( "%1/" ).arg( idx.data().toString() ) );
	}
	return id;
}

QList<Message> TestThread::checkLinks( const NifModel * nif, const QModelIndex & iParent )
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
				if ( ! child )
					messages.append( Message() << "unassigned parent link" << linkId( idx ) );
			}
			else if ( l >= nif->getBlockCount() )
				messages.append( Message() << "invalid link" << linkId( idx ) );
			else
			{
				QString tmplt = idx.sibling( idx.row(), NifModel::TempCol ).data( Qt::DisplayRole ).toString();
				if ( ! tmplt.isEmpty() )
				{
					QModelIndex iBlock = nif->getBlock( l );
					if ( ! nif->inherits( iBlock, tmplt ) )
						messages.append( Message() << "link" << linkId( idx ) << "points to wrong block type" << iBlock.data().toString() );
				}
			}
		}
		if ( nif->rowCount( idx ) > 0 )
			messages += checkLinks( nif, idx );
	}
	return messages;
}

// Browser

Browser::Browser()
{
}

void Browser::mousePressEvent( QMouseEvent * e )
{
	pressPos = e->pos();
	QTextEdit::mousePressEvent( e );
}

void Browser::mouseReleaseEvent( QMouseEvent * e )
{
	if ( ( pressPos - e->pos() ).manhattanLength() < QApplication::startDragDistance() )
	{
		QString anchor = anchorAt( e->pos() );
		if ( ! anchor.isEmpty() )
			emit sigAnchorClicked( anchor );
	}
	QTextEdit::mouseReleaseEvent( e );
}
