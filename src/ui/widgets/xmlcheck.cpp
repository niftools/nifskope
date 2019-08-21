#include "xmlcheck.h"

#include "message.h"
#include "model/kfmmodel.h"
#include "model/nifmodel.h"
#include "ui/widgets/fileselect.h"

#include "spells/sanitize.h"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
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
#include <QComboBox>
#include <QQueue>

#define NUM_THREADS 4


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
	QSettings settings;
	settings.beginGroup( "XML Checker" );

	directory = new FileSelector( FileSelector::Folder, "Dir", QBoxLayout::RightToLeft );
	directory->setText( settings.value( "Directory" ).toString() );

	recursive = new QCheckBox( tr( "Recursive" ), this );
	recursive->setChecked( settings.value( "Recursive", true ).toBool() );
	recursive->setToolTip( tr( "Recurse into sub directories" ) );

	chkNif = new QCheckBox( tr( "*.nif" ), this );
	chkNif->setChecked( settings.value( "Check NIF", true ).toBool() );
	chkNif->setToolTip( tr( "Check .nif files" ) );

	chkKf = new QCheckBox( tr( "*.kf(a)" ), this );
	chkKf->setChecked( settings.value( "Check KF", true ).toBool() );
	chkKf->setToolTip( tr( "Check .kf files" ) );

	chkKfm = new QCheckBox( tr( "*.kfm" ), this );
	chkKfm->setChecked( settings.value( "Check KFM", true ).toBool() );
	chkKfm->setToolTip( tr( "Check .kfm files" ) );

	QAction * aChoose = new QAction( tr( "Block Match" ), this );
	connect( aChoose, &QAction::triggered, this, &TestShredder::chooseBlock );
	QToolButton * btChoose = new QToolButton( this );
	btChoose->setDefaultAction( aChoose );

	blockMatch = new QLineEdit( this );
	valueName = new QLineEdit( this );
	valueMatch = new QLineEdit( this );
	valueOps = new QComboBox( this );
	for ( const auto & o : ops_ord )
		valueOps->insertItem(ops[o].first, o );
	valueOps->setCurrentIndex(0);

	QString op_tip;
	for ( const auto t : ops_ord )
		op_tip += QString("%1\t%2\r\n").arg(t).arg(ops[t].second);
	valueOps->setToolTip(op_tip);

	chkCheckErrors = new QCheckBox(tr("Error Checking"), this);
	chkCheckErrors->setChecked(settings.value("Error Checking", true).toBool());

	hdrOnly = new QCheckBox( tr( "Header Only" ), this );
	hdrOnly->setChecked( settings.value( "Header Only", false ).toBool() );

	repErr = new QCheckBox( tr( "List Matches Only" ), this );
	repErr->setChecked( settings.value( "List Matches Only", true ).toBool() );

	count = new QSpinBox();
	count->setRange( 1, 16 );
	count->setValue( settings.value( "Threads", NUM_THREADS ).toInt() );
	connect( count, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &TestShredder::renumberThreads );

	//Version Check
	verMatch = new QLineEdit( this );

	text = new QTextBrowser();
	text->setHidden( false );
	text->setReadOnly( true );
	text->setOpenExternalLinks( true );

	progress = new QProgressBar( this );

	label = new QLabel( this );
	label->setHidden( true );

	btRun = new QPushButton( tr( "Run" ), this );
	btRun->setCheckable( true );
	connect( btRun, &QPushButton::clicked, this, &TestShredder::run );

	QPushButton * btXML = new QPushButton( tr( "Reload XML" ), this );
	connect( btXML, &QPushButton::clicked, this, &TestShredder::xml );

	QPushButton * btClose = new QPushButton( tr( "Close" ), this );
	connect( btClose, &QPushButton::clicked, this, &TestShredder::close );

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
	hbox->addWidget( new QLabel( tr( "Threads:" ) ) );
	hbox->addWidget( count );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( new QLabel( tr( "Value:" ) ) );
	hbox->addWidget( valueName );
	hbox->addWidget( valueOps );
	hbox->addWidget( valueMatch );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( chkCheckErrors );
	hbox->addWidget( repErr );
	hbox->addWidget( hdrOnly );

	lay->addLayout( hbox = new QHBoxLayout() );
	hbox->addWidget( new QLabel( tr( "Version Match:" ) ) );
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

	settings.endGroup();
}

TestShredder::~TestShredder()
{
	QSettings settings;
	settings.beginGroup( "XML Checker" );

	settings.setValue( "Directory", directory->text() );
	settings.setValue( "Recursive", recursive->isChecked() );
	settings.setValue( "Check NIF", chkNif->isChecked() );
	settings.setValue( "Check KF", chkKf->isChecked() );
	settings.setValue( "Check KFM", chkKfm->isChecked() );
	settings.setValue( "List Matches Only", repErr->isChecked() );
	settings.setValue( "Header Only", hdrOnly->isChecked() );
	settings.setValue( "Error Checking", chkCheckErrors->isChecked() );
	settings.setValue( "Threads", count->value() );

	settings.endGroup();

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
	while ( threads.count() < num ) {
		TestThread * thread = new TestThread( this, &queue );
		connect( thread, &TestThread::sigStart, this, &TestShredder::threadStarted );
		connect( thread, &TestThread::sigReady, text, &QTextBrowser::append );
		connect( thread, &TestThread::finished, this, &TestShredder::threadFinished );
		threads.append( thread );

		thread->blockMatch = blockMatch->text();
		thread->verMatch  = NifModel::version2number( verMatch->text() );
		thread->reportAll = !repErr->isChecked();
		thread->headerOnly = hdrOnly->isChecked();
		thread->checkFile = chkCheckErrors->isChecked();

		thread->valueName = valueName->text();
		thread->valueMatch = valueMatch->text();
		thread->op = OpType(valueOps->currentIndex());

		if ( btRun->isChecked() ) {
			thread->start();
		}
	}

	while ( threads.count() > num ) {
		TestThread * thread = threads.takeLast();
		delete thread;
	}
}

void TestShredder::run()
{
	progress->setMaximum( progress->maximum() - queue.count() );
	queue.clear();

	if ( !btRun->isChecked() )
		return;

	for ( TestThread * thread : threads ) {
		thread->wait();
	}

	text->clear();
	label->setHidden( true );

	QStringList extensions;

	if ( chkNif->isChecked() )
		extensions << "*.nif" << "*.nifcache" << "*.texcache" << "*.pcpatch" << "*.bto" << "*.btr" << "*.item" << "*.nif_wii";
	if ( chkKf->isChecked() )
		extensions << "*.kf" << "*.kfa";
	if ( chkKfm->isChecked() )
		extensions << "*.kfm";

	queue.init( directory->text(), extensions, recursive->isChecked() );

	time = QDateTime::currentDateTime();

	progress->setRange( 0, queue.count() );
	progress->setValue( 0 );

	for ( TestThread * thread : threads ) {
		thread->verMatch = NifModel::version2number( verMatch->text() );
		thread->blockMatch = blockMatch->text();
		thread->reportAll  = !repErr->isChecked();
		thread->headerOnly = hdrOnly->isChecked();
		thread->checkFile = chkCheckErrors->isChecked();
		thread->valueName = valueName->text();
		thread->valueMatch = valueMatch->text();
		thread->op = OpType(valueOps->currentIndex());

		thread->start();
	}
}

void TestShredder::threadStarted()
{
	progress->setValue( progress->maximum() - queue.count() );
}

void TestShredder::threadFinished()
{
	if ( queue.isEmpty() ) {
		for ( TestThread * thread : threads ) {
			if ( thread->isRunning() )
				return;
		}

		btRun->setChecked( false );

		label->setText( tr( "%1 files in %2 seconds" ).arg( progress->maximum() ).arg( time.secsTo( QDateTime::currentDateTime() ) ) );
		label->setVisible( true );
	}
}

void TestShredder::chooseBlock()
{
	QStringList ids = NifModel::allNiBlocks();
	ids.sort();

	QMap<QString, QMenu *> map;
	for ( const QString& id : ids ) {
		QString x( "Other" );

		if ( id.startsWith( "Ni" ) )
			x = QString( "Ni&" ) + id.mid( 2, 1 ) + "...";
		if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
			x = "Havok";
		if ( id.startsWith( "BS" ) || id == "AvoidNode" || id == "RootCollisionNode" )
			x = "Bethesda";
		if ( id.startsWith( "Fx" ) )
			x = "Firaxis";
		if ( !map.contains( x ) )
			map[ x ] = new QMenu( x );

		map[ x ]->addAction( id );
	}

	QMenu menu;
	for ( QMenu * m : map ) {
		menu.addMenu( m );
	}

	QAction * act = menu.exec( QCursor::pos() );

	if ( act )
		blockMatch->setText( act->text() );
}

void TestShredder::closeEvent( QCloseEvent * e )
{
	for ( TestThread * thread : threads ) {
		if ( thread->isRunning() ) {
			e->ignore();
			queue.clear();
		}
	}
}

/*
 *  File Queue
 */

QQueue<QString> FileQueue::make( const QString & dname, const QStringList & extensions, bool recursive )
{
	QQueue<QString> paths;

	QDir dir( dname );

	if ( recursive ) {
		dir.setFilter( QDir::Dirs );
		for ( const QString& d : dir.entryList() ) {
			if ( d != "." && d != ".." )
				paths += make( dir.filePath( d ), extensions, true );
		}
	}

	dir.setFilter( QDir::Files );
	dir.setNameFilters( extensions );
	for ( const QString& f : dir.entryList() ) {
		paths.enqueue( dir.filePath( f ) );
	}

	return paths;
}

void FileQueue::init( const QString & dname, const QStringList & extensions, bool recursive )
{
	QQueue<QString> paths = make( dname, extensions, recursive );

	mutex.lock();
	this->queue = paths;
	mutex.unlock();
}

QString FileQueue::dequeue()
{
	QMutexLocker lock( &mutex );

	if ( queue.isEmpty() )
		return QString();

	return queue.dequeue();
}

int FileQueue::count()
{
	QMutexLocker lock( &mutex );
	return queue.count();
}

void FileQueue::clear()
{
	QMutexLocker lock( &mutex );
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
	if ( isRunning() ) {
		quit.lock();
		wait();
		quit.unlock();
	}
}

void TestThread::run()
{
	NifModel nif;
	KfmModel kfm;

	QString filepath = queue->dequeue();

	while ( !filepath.isEmpty() ) {
		emit sigStart( filepath );

		BaseModel * model = &nif;
		QReadWriteLock * lock = &nif.XMLlock;

		if ( filepath.endsWith( ".KFM", Qt::CaseInsensitive ) ) {
			model = &kfm;
			lock  = &kfm.XMLlock;
		}

		bool kf = ( filepath.endsWith( ".KF", Qt::CaseInsensitive ) || filepath.endsWith( ".KFA", Qt::CaseInsensitive ) );

		{
			// lock the XML lock
			QReadLocker lck( lock );

			QString result;
			if ( model == &nif && nif.earlyRejection( filepath, blockMatch, verMatch ) ) {
				bool loaded = (headerOnly) ? nif.loadHeaderOnly(filepath) : model->loadFromFile(filepath);

				result = QString( "<a href=\"nif:%1\">%1</a> (%2, %3, %4)" )
					.arg( filepath, model->getVersion() ).arg( nif.getUserVersion() ).arg( nif.getUserVersion2() );
				QList<TestMessage> messages = model->getMessages();

				bool blk_match = false;
				bool val_match = false;

				if ( !headerOnly && loaded && model == &nif ) {
					for ( int b = 0; b < nif.getBlockCount(); b++ ) {
						auto blk = nif.getBlock( b );
						bool current_match = !blockMatch.isEmpty() && nif.inherits(nif.getBlockName(blk), blockMatch);
						blk_match |= current_match;

						NifValue value;
						if ( (blockMatch.isEmpty() || current_match) && !valueName.isEmpty() && !valueMatch.isEmpty() ) {
							auto nameIdx = nif.getIndex(blk, valueName);
							bool hasName = nameIdx.isValid();
							if ( hasName ) {
								value = nif.getValue(nameIdx);

								bool isInt = value.isCount() && !value.isFloat();
								bool isStr = value.isString() || value.type() == NifValue::tStringIndex || value.isFloat();

								auto asInt = value.toCount();
								auto asStr = value.toString();
								if ( value.type() == NifValue::tStringIndex )
									asStr = nif.string(nameIdx);

								bool current_match = false;

								switch ( op ) {
								case OP_EQ:
									if ( isInt )
										current_match = (asInt == valueMatch.toInt(nullptr, 0));
									else if ( isStr )
										current_match = (asStr == valueMatch);
									break;
								case OP_NEQ:
									if ( isInt )
										current_match = (asInt != valueMatch.toInt(nullptr, 0));
									else if ( isStr )
										current_match = (asStr != valueMatch);
									break;
								case OP_AND:
									if ( !isInt )
										break;
									current_match = (asInt & valueMatch.toInt(nullptr, 0));
									break;
								case OP_AND_S:
									if ( !isInt )
										break;
									current_match = (asInt & (1 << valueMatch.toInt(nullptr, 0)));
									break;
								case OP_NAND:
									if ( !isInt )
										break;
									current_match = !(asInt & valueMatch.toInt(nullptr, 0));
									break;
								case OP_STR_S:
									current_match = asStr.startsWith(valueMatch, Qt::CaseInsensitive);
									break;
								case OP_STR_E:
									current_match = asStr.endsWith(valueMatch, Qt::CaseInsensitive);
									break;
								case OP_STR_NS:
									current_match = !asStr.startsWith(valueMatch, Qt::CaseInsensitive);
									break;
								case OP_STR_NE:
									current_match = !asStr.endsWith(valueMatch, Qt::CaseInsensitive);
									break;
								case OP_CONT:
									current_match = asStr.contains(valueMatch, Qt::CaseInsensitive);
									break;
								default:
									current_match = false;
									break;
								}

								if ( current_match )
									messages += TestMessage( QtInfoMsg ) <<
												QString( "[%1] Found Match: %2 %3 %4 | Value: %5" ).arg(b)
												.arg(valueName).arg(ops_ord[int(op)].toHtmlEscaped())
												.arg(valueMatch).arg(asStr);
							} else {
								current_match = false;
							}

							val_match |= current_match;
						}

						if ( checkFile ) {
							messages += checkLinks(&nif, blk, kf);
						}
					}

					if ( checkFile ) {
						for ( auto checker : SpellBook::checkers() )
							checker->castIfApplicable(&nif, {});
						messages += nif.getMessages();
					}

				}

				bool rep = reportAll || (blk_match && valueMatch.isEmpty());

				// Don't show anything if block match is on but the requested type wasn't found & we're in block match mode
				if ( blockMatch.isEmpty() == true || blk_match == true || val_match == true ) {
					for ( const TestMessage& msg : messages ) {
						if ( msg.type() != QtDebugMsg ) {
							result += "<br>" + msg;
							rep |= true;
						}
					}

					if ( rep )
						emit sigReady( result );
				}
			} else if ( !blockMatch.isEmpty() && !verMatch ) {
				// Do not silently fail on unrecognized NIFs
				result += QString("Did not recognize file as a NIF: %1").arg(filepath);
				emit sigReady(result);
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
	QString id = QString( "%1 (%2)" ).arg( nif->itemName( idx ), nif->itemTmplt( idx ) );

	while ( idx.parent().isValid() ) {
		idx = idx.parent();
		id.prepend( QString( "%1/" ).arg( nif->itemName( idx ) ) );
	}

	return id;
}

QList<TestMessage> TestThread::checkLinks( const NifModel * nif, const QModelIndex & iParent, bool kf )
{
	QList<TestMessage> messages;

	for ( int r = 0; r < nif->rowCount( iParent ); r++ ) {
		QModelIndex idx = iParent.child( r, 0 );
		bool child;

		if ( nif->isLink( idx, &child ) ) {
			qint32 l = nif->getLink( idx );

			if ( l < 0 ) {
				// This is not really an error
				// if ( ! child && ! kf )
				//	messages.append( Message() << tr("unassigned parent link") << linkId( nif, idx ) );
			} else if ( l >= nif->getBlockCount() ) {
				messages.append( TestMessage() << tr( "invalid link" ) << linkId( nif, idx ) );
			} else {
				QString tmplt = nif->itemTmplt( idx );

				if ( !tmplt.isEmpty() ) {
					QModelIndex iBlock = nif->getBlock( l );

					if ( !nif->inherits( iBlock, tmplt ) )
						messages.append( TestMessage() << tr( "link" ) << linkId( nif, idx ) << tr( "points to wrong block type" ) << nif->itemName( iBlock ) );
				}
			}
		}

		if ( nif->rowCount( idx ) > 0 )
			messages += checkLinks( nif, idx, kf );
	}

	return messages;
}
