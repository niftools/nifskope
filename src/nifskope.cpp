/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "nifskope.h"
#include "version.h"
#include "options.h"

#include "ui_nifskope.h"
#include "ui/about_dialog.h"

#include "glview.h"
#include "kfmmodel.h"
#include "nifmodel.h"
#include "nifproxy.h"
#include "spellbook.h"
#include "widgets/copyfnam.h"
#include "widgets/fileselect.h"
#include "widgets/nifview.h"
#include "widgets/refrbrowser.h"
#include "widgets/inspect.h"
#include "widgets/xmlcheck.h"


#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTranslator>
#include <QUdpSocket>
#include <QUrl>

#include <QListView>
#include <QTreeView>

#ifdef FSENGINE
#include <fsengine/fsmanager.h>
#endif

#ifdef WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  include "windows.h"
#endif


//! \file nifskope.cpp The main file for NifSkope


/*
 * main GUI window
 */

NifSkope::NifSkope()
	: QMainWindow(), selecting( false ), initialShowEvent( true ), ui( new Ui::MainWindow )
{
	// Init UI
	ui->setupUi( this );
	
	// Init Dialogs
	aboutDialog = new AboutDialog( this );

	// Migrate settings from older versions of NifSkope
	migrateSettings();

	// Create models
	/* ********************** */

	nif = new NifModel( this );
	proxy = new NifProxyModel( this );
	proxy->setModel( nif );

	kfm = new KfmModel( this );

	book = new SpellBook( nif, QModelIndex(), this, SLOT( select( const QModelIndex & ) ) );

	// Setup Views
	/* ********************** */

	// Block List
	list = ui->list;
	list->setModel( proxy );
	list->setItemDelegate( nif->createDelegate( book ) );
	list->installEventFilter( this );

	// Block Details
	tree = ui->tree;
	tree->setModel( nif );
	tree->setItemDelegate( nif->createDelegate( book ) );
	tree->installEventFilter( this );

	// KFM
	kfmtree = ui->kfmtree;
	kfmtree->setModel( kfm );
	kfmtree->setItemDelegate( kfm->createDelegate() );
	kfmtree->installEventFilter( this );

	// Help Browser
	refrbrwsr = ui->refrBrowser;
	refrbrwsr->setNifModel( nif );

	// Connect models with views
	/* ********************** */

	connect( nif, &NifModel::sigMessage, this, &NifSkope::dispatchMessage );
	connect( kfm, &KfmModel::sigMessage, this, &NifSkope::dispatchMessage );
	connect( list, &NifTreeView::sigCurrentIndexChanged, this, &NifSkope::select );
	connect( list, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );
	connect( tree, &NifTreeView::sigCurrentIndexChanged, this, &NifSkope::select );
	connect( tree, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );
	connect( tree, &NifTreeView::sigCurrentIndexChanged, refrbrwsr, &ReferenceBrowser::browse );
	connect( kfmtree, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );

#ifdef EDIT_ON_ACTIVATE
	// TODO: Determine necessity of this, appears redundant
	//connect( list, &NifTreeView::activated, list, static_cast<void (NifTreeView::*)(const QModelIndex&)>(&NifTreeView::edit) );
	//connect( tree, &NifTreeView::activated, tree, static_cast<void (NifTreeView::*)(const QModelIndex&)>(&NifTreeView::edit) );
	//connect( kfmtree, &NifTreeView::activated, kfmtree, static_cast<void (NifTreeView::*)(const QModelIndex&)>(&NifTreeView::edit) );
#endif

	// Create GLView
	/* ********************** */

	ogl = GLView::create();
	ogl->setObjectName( "OGL1" );
	ogl->setNif( nif );

	// Create InspectView
	/* ********************** */
	
	inspect = new InspectView;
	inspect->setNifModel( nif );
	inspect->setScene( ogl->getScene() );


	/*
	 * UI Init
	 * **********************
	 */

	setCentralWidget( ogl );

	// Set Actions
	initActions();

	// Dock Widgets
	initDockWidgets();

	// Toolbars
	initToolBars();

	// Menus
	initMenu();


	connect( Options::get(), &Options::sigLocaleChanged, this, &NifSkope::sltLocaleChanged );
}

NifSkope::~NifSkope()
{
	delete ui;
}

void NifSkope::closeEvent( QCloseEvent * e )
{
	saveUi();

	QMainWindow::closeEvent( e );
}


void NifSkope::select( const QModelIndex & index )
{
	if ( selecting )
		return;

	QModelIndex idx = index;

	if ( idx.model() == proxy )
		idx = proxy->mapTo( index );

	if ( idx.isValid() && idx.model() != nif )
		return;

	selecting = true;

	// TEST: Cast sender to GLView
	//auto s = qobject_cast<GLView *>(sender());
	//if ( s )
	//	qDebug() << sender()->objectName();

	if ( sender() != ogl ) {
		ogl->setCurrentIndex( idx );
	}

	if ( sender() != list ) {
		if ( list->model() == proxy ) {
			QModelIndex pidx = proxy->mapFrom( nif->getBlock( idx ), list->currentIndex() );
			list->setCurrentIndex( pidx );
		} else if ( list->model() == nif ) {
			list->setCurrentIndex( nif->getBlockOrHeader( idx ) );
		}
	}

	if ( sender() != tree ) {
		if ( dList->isVisible() ) {
			QModelIndex root = nif->getBlockOrHeader( idx );

			if ( tree->rootIndex() != root )
				tree->setRootIndex( root );

			tree->setCurrentIndex( idx.sibling( idx.row(), 0 ) );

			// Expand BSShaderTextureSet by default
			//if ( root.child( 1, 0 ).data().toString() == "Textures" )
			//	tree->expandAll();

		} else {
			if ( tree->rootIndex() != QModelIndex() )
				tree->setRootIndex( QModelIndex() );

			tree->setCurrentIndex( idx.sibling( idx.row(), 0 ) );
		}
	}

	selecting = false;
}

void NifSkope::setListMode()
{
	QModelIndex idx = list->currentIndex();
	QAction * a = gListMode->checkedAction();

	if ( !a || a == aList ) {
		if ( list->model() != nif ) {
			// switch to list view
			QHeaderView * head = list->header();
			int s0 = head->sectionSize( head->logicalIndex( 0 ) );
			int s1 = head->sectionSize( head->logicalIndex( 1 ) );
			list->setModel( nif );
			list->setItemsExpandable( false );
			list->setRootIsDecorated( false );
			list->setCurrentIndex( proxy->mapTo( idx ) );
			list->setColumnHidden( NifModel::NameCol, false );
			list->setColumnHidden( NifModel::TypeCol, true );
			list->setColumnHidden( NifModel::ValueCol, false );
			list->setColumnHidden( NifModel::ArgCol, true );
			list->setColumnHidden( NifModel::Arr1Col, true );
			list->setColumnHidden( NifModel::Arr2Col, true );
			list->setColumnHidden( NifModel::CondCol, true );
			list->setColumnHidden( NifModel::Ver1Col, true );
			list->setColumnHidden( NifModel::Ver2Col, true );
			list->setColumnHidden( NifModel::VerCondCol, true );
			head->resizeSection( 0, s0 );
			head->resizeSection( 1, s1 );
		}
	} else {
		if ( list->model() != proxy ) {
			// switch to hierarchy view
			QHeaderView * head = list->header();
			int s0 = head->sectionSize( head->logicalIndex( 0 ) );
			int s1 = head->sectionSize( head->logicalIndex( 1 ) );
			list->setModel( proxy );
			list->setItemsExpandable( true );
			list->setRootIsDecorated( true );
			QModelIndex pidx = proxy->mapFrom( idx, QModelIndex() );
			list->setCurrentIndex( pidx );
			// proxy model has only two columns (see columnCount in nifproxy.h)
			list->setColumnHidden( 0, false );
			list->setColumnHidden( 1, false );
			head->resizeSection( 0, s0 );
			head->resizeSection( 1, s1 );
		}
	}
}

void NifSkope::load( const QString & filepath )
{
	lineLoad->setText( filepath );
	QTimer::singleShot( 0, this, SLOT( load() ) );
}

void NifSkope::load()
{
	setEnabled( false );

	QFileInfo niffile( QDir::fromNativeSeparators( lineLoad->text() ) );
	niffile.makeAbsolute();

	if ( niffile.suffix().compare( "kfm", Qt::CaseInsensitive ) == 0 ) {
		lineLoad->rstState();
		lineSave->rstState();

		if ( !kfm->loadFromFile( niffile.filePath() ) ) {
			qWarning() << tr( "failed to load kfm from '%1'" ).arg( niffile.filePath() );
			lineLoad->setState( FileSelector::stError );
		} else {
			lineLoad->setState( FileSelector::stSuccess );
			lineLoad->setText( niffile.filePath() );
			lineSave->setText( niffile.filePath() );
		}

		niffile.setFile( kfm->getFolder(),
			kfm->get<QString>( kfm->getKFMroot(), "NIF File Name" ) );
	}

	ui->tAnim->setEnabled( false );

	if ( !niffile.isFile() ) {
		nif->clear();
		lineLoad->setState( FileSelector::stError );
	} else {
		progress = new QProgressBar( this );
		progress->setRange( 0, 1 );
		progress->setValue( 0 );
		progress->setMaximumSize( 200, 18 );

		// Process progress events
		connect( nif, &NifModel::sigProgress, [this]( int c, int m ) {
		
			progress->setRange( 0, m );
			progress->setValue( c );
			qApp->processEvents();
		} );

		ui->statusbar->addPermanentWidget( progress );

		lineLoad->rstState();
		lineSave->rstState();

		if ( !nif->loadFromFile( niffile.filePath() ) ) {
			qWarning() << tr( "failed to load nif from '%1'" ).arg( niffile.filePath() );
			lineLoad->setState( FileSelector::stError );
		} else {
			lineLoad->setState( FileSelector::stSuccess );
			lineLoad->setText( niffile.filePath() );
			lineSave->setText( niffile.filePath() );
		}

		setWindowTitle( niffile.fileName() );
	}

	ui->mRender->setEnabled( true );

	//ui->tAnim->setEnabled( true );
	ui->tRender->setEnabled( true );

	ogl->setOrientation( GLView::viewFront );
	ogl->center();

	// Expand BSShaderTextureSet by default
	auto indices = nif->match( nif->index( 0, 0 ), Qt::DisplayRole, "Textures", -1, Qt::MatchRecursive );
	for ( auto i : indices ) {
		tree->expand( i );
	}

	// Scroll panel back to top
	tree->scrollTo( nif->index( 0, 0 ) );

	setEnabled( true );
}

void NifSkope::save()
{
	// write to file
	setEnabled( false );

	QString nifname = lineSave->text();

	if ( nifname.endsWith( ".KFM", Qt::CaseInsensitive ) ) {
		lineSave->rstState();

		if ( !kfm->saveToFile( nifname ) ) {
			qWarning() << tr( "failed to write kfm file" ) << nifname;
			lineSave->setState( FileSelector::stError );
		} else {
			lineSave->setState( FileSelector::stSuccess );
		}
	} else {
		lineSave->rstState();

		if ( aSanitize->isChecked() ) {
			QModelIndex idx = SpellBook::sanitize( nif );

			if ( idx.isValid() )
				select( idx );
		}

		if ( !nif->saveToFile( nifname ) ) {
			qWarning() << tr( "failed to write nif file " ) << nifname;
			lineSave->setState( FileSelector::stError );
		} else {
			lineSave->setState( FileSelector::stSuccess );
		}

		// TODO: nif->getFileInfo() returns stale data
		// Instead create tmp QFileInfo from lineSave text
		// Future: updating file info stored in nif
		QFileInfo finfo( nifname );
		setWindowTitle( finfo.fileName() );
	}

	setEnabled( true );
}

void NifSkope::copyFileNameLoadSave()
{
	if ( lineLoad->text().isEmpty() ) {
		return;
	}

	lineSave->replaceText( lineLoad->text() );
}

void NifSkope::copyFileNameSaveLoad()
{
	if ( lineSave->text().isEmpty() ) {
		return;
	}

	lineLoad->replaceText( lineSave->text() );
}



void NifSkope::openURL()
{
	if ( !sender() )
		return;

	QAction * aURL = qobject_cast<QAction *>( sender() );

	if ( !aURL )
		return;

	QUrl URL(aURL->toolTip());

	if ( !URL.isValid() )
		return;

	QDesktopServices::openUrl( URL );
}


void NifSkope::dispatchMessage( const Message & msg )
{
	switch ( msg.type() ) {
	case QtCriticalMsg:
		qCritical() << msg;
		break;
	case QtFatalMsg:
		qFatal( QString( msg ).toLatin1().data() );
		break;
	case QtWarningMsg:
		qWarning() << msg;
		break;
	case QtDebugMsg:
	default:
		qDebug() << msg;
		break;
	}
}

QTextEdit * msgtarget = nullptr;


#ifdef Q_OS_WIN32
//! Windows mutex handling
class QDefaultHandlerCriticalSection
{
	CRITICAL_SECTION cs;

public:
	QDefaultHandlerCriticalSection() { InitializeCriticalSection( &cs ); }
	~QDefaultHandlerCriticalSection() { DeleteCriticalSection( &cs ); }
	void lock() { EnterCriticalSection( &cs ); }
	void unlock() { LeaveCriticalSection( &cs ); }
};

//! Application-wide debug and warning message handler internals
static void qDefaultMsgHandler( QtMsgType t, const char * str )
{
	Q_UNUSED( t );
	// OutputDebugString is not threadsafe.
	// cannot use QMutex here, because qWarning()s in the QMutex
	// implementation may cause this function to recurse
	static QDefaultHandlerCriticalSection staticCriticalSection;

	if ( !str )
		str = "(null)";

	staticCriticalSection.lock();
	QString s( QString::fromLocal8Bit( str ) );
	s += QLatin1String( "\n" );
	OutputDebugStringW( (TCHAR *)s.utf16() );
	staticCriticalSection.unlock();
}
#else
// Doxygen won't find this unless you undef Q_OS_WIN32
//! Application-wide debug and warning message handler internals
void qDefaultMsgHandler( QtMsgType t, const char * str )
{
	if ( !str )
		str = "(null)";

	printf( "%s\n", str );
}
#endif

//! Application-wide debug and warning message handler
void myMessageOutput( QtMsgType type, const QMessageLogContext &, const QString & str )
{
	QByteArray msg = str.toLocal8Bit();
	static const QString editFailed( "edit: editing failed" );
	static const QString accessWidgetRect( "QAccessibleWidget::rect" );

	switch ( type ) {
	case QtDebugMsg:
		qDefaultMsgHandler( type, msg.constData() );
		break;
	case QtWarningMsg:

		// workaround for Qt 4.2.2
		if ( editFailed == msg )
			return;
		else if ( QString( msg ).startsWith( accessWidgetRect ) )
			return;

	case QtCriticalMsg:

		if ( !msgtarget ) {
			msgtarget = new QTextEdit;
			msgtarget->setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
		}

		if ( !msgtarget->isVisible() ) {
			msgtarget->clear();
			msgtarget->show();
		}

		msgtarget->append( msg );
		qDefaultMsgHandler( type, msg.constData() );
		break;
	case QtFatalMsg:
		qDefaultMsgHandler( type, msg.constData() );
		QMessageBox::critical( 0, QMessageBox::tr( "Fatal Error" ), msg );
		// TODO: the above causes stack overflow when
		// "ASSERT: "testAttribute(Qt::WA_WState_Created)" in file kernel\qapplication_win.cpp, line 3699"
		abort();
	}
}


/*
 *  IPC socket
 */

IPCsocket * IPCsocket::create()
{
	QUdpSocket * udp = new QUdpSocket();

	if ( udp->bind( QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT, QUdpSocket::DontShareAddress ) ) {
		IPCsocket * ipc = new IPCsocket( udp );
		QDesktopServices::setUrlHandler( "nif", ipc, "openNif" );
		return ipc;
	}

	return 0;
}

void IPCsocket::sendCommand( const QString & cmd )
{
	QUdpSocket udp;
	udp.writeDatagram( (const char *)cmd.data(), cmd.length() * sizeof( QChar ), QHostAddress( QHostAddress::LocalHost ), NIFSKOPE_IPC_PORT );
}

IPCsocket::IPCsocket( QUdpSocket * s ) : QObject(), socket( s )
{
	QObject::connect( socket, &QUdpSocket::readyRead, this, &IPCsocket::processDatagram );
}

IPCsocket::~IPCsocket()
{
	delete socket;
}

void IPCsocket::processDatagram()
{
	while ( socket->hasPendingDatagrams() ) {
		QByteArray data;
		data.resize( socket->pendingDatagramSize() );
		QHostAddress host;
		quint16 port = 0;

		socket->readDatagram( data.data(), data.size(), &host, &port );

		if ( host == QHostAddress( QHostAddress::LocalHost ) && ( data.size() % sizeof( QChar ) ) == 0 ) {
			QString cmd;
			cmd.setUnicode( (QChar *)data.data(), data.size() / sizeof( QChar ) );
			execCommand( cmd );
		}
	}
}

void IPCsocket::execCommand( const QString & cmd )
{
	if ( cmd.startsWith( "NifSkope::open" ) ) {
		openNif( cmd.right( cmd.length() - 15 ) );
	}
}

void IPCsocket::openNif( const QString & url )
{
	NifSkope::createWindow( url );
}


// TODO: This class was not used. QSystemLocale became private in Qt 5.
// It appears this class was going to handle display of numbers.
/*//! System locale override
/**
 * Qt does not use the System Locale consistency so this basically forces all floating
 * numbers into C format but leaves all other local specific settings.
 *//*
class NifSystemLocale : QSystemLocale
{
    virtual QVariant query(QueryType type, QVariant in) const
    {
        switch (type)
        {
        case DecimalPoint:
            return QVariant( QLocale::c().decimalPoint() );
        case GroupSeparator:
            return QVariant( QLocale::c().groupSeparator() );
        default:
            return QVariant();
        }
    }
};*/

static QTranslator * mTranslator = nullptr;

//! Sets application locale and loads translation files
static void SetAppLocale( QLocale curLocale )
{
	QDir directory( QApplication::applicationDirPath() );

	if ( !directory.cd( "lang" ) ) {
#ifdef Q_OS_LINUX
	if ( !directory.cd( "/usr/share/nifskope/lang" ) ) {}
#endif
	}

	QString fileName = directory.filePath( "NifSkope_" ) + curLocale.name();

	if ( !QFile::exists( fileName + ".qm" ) )
		fileName = directory.filePath( "NifSkope_" ) + curLocale.name().section( '_', 0, 0 );

	if ( !QFile::exists( fileName + ".qm" ) ) {
		if ( mTranslator ) {
			qApp->removeTranslator( mTranslator );
			delete mTranslator;
			mTranslator = nullptr;
		}
	} else {
		if ( !mTranslator ) {
			mTranslator = new QTranslator();
			qApp->installTranslator( mTranslator );
		}

		mTranslator->load( fileName );
	}
}

void NifSkope::sltLocaleChanged()
{
	SetAppLocale( Options::get()->translationLocale() );

	QMessageBox mb( "NifSkope",
	                tr( "NifSkope must be restarted for this setting to take full effect." ),
	                QMessageBox::Information, QMessageBox::Ok + QMessageBox::Default, 0, 0,
	                qApp->activeWindow()
	);
	mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
	mb.exec();
}

QString NifSkope::getLoadFileName()
{
	return lineLoad->text();
}

/*
 *  main
 */

//! The main program
int main( int argc, char * argv[] )
{
	// set up the Qt Application
	QApplication app( argc, argv );
	app.setOrganizationName( "NifTools" );
	app.setOrganizationDomain( "niftools.org" );
	app.setApplicationName( "NifSkope " + NifSkopeVersion::rawToMajMin( NIFSKOPE_VERSION ) );
	app.setApplicationVersion( NIFSKOPE_VERSION );
	app.setApplicationDisplayName( "NifSkope " + NifSkopeVersion::rawToDisplay( NIFSKOPE_VERSION, true ) );

	// install message handler
	qRegisterMetaType<Message>( "Message" );
#ifdef QT_NO_DEBUG
	qInstallMessageHandler( myMessageOutput );
#endif

	// if there is a style sheet present then load it
	QDir qssDir( QApplication::applicationDirPath() );
	QStringList qssList( QStringList()
	                     << "style.qss"
#ifdef Q_OS_LINUX
	                     << "/usr/share/nifskope/style.qss"
#endif
	);
	QString qssName;
	for ( const QString& str : qssList ) {
		if ( qssDir.exists( str ) ) {
			qssName = qssDir.filePath( str );
			break;
		}
	}

	// load the style sheet if present
	if ( !qssName.isEmpty() ) {
		QFile style( qssName );

		if ( style.open( QFile::ReadOnly ) ) {
			app.setStyleSheet( style.readAll() );
			style.close();
		}
	}

	QSettings cfg;
	cfg.beginGroup( "Settings" );
	SetAppLocale( cfg.value( "Language", "en" ).toLocale() );
	cfg.endGroup();

	NifModel::loadXML();
	KfmModel::loadXML();

	QStack<QString> fnames;
	bool reuseSession = true;

	// EXE is being passed arguments
	for ( int i = 1; i < argc; ++i ) {
		char * arg = argv[i];

		if ( arg && arg[0] == '-' ) {
			// Command line arguments
			// TODO: See QCommandLineParser for future
			// expansion of command line abilities.
			switch ( arg[1] ) {
			case 'i':
			case 'I':
				// TODO: Figure out the point of this
				reuseSession = false;
				break;
			}
		} else {
			//qDebug() << "arg " << i << ": " << arg;
			QString fname = QDir::current().filePath( arg );

			if ( QFileInfo( fname ).exists() ) {
				fnames.push( fname );
			}
		}
	}

	// EXE is being opened directly
	if ( fnames.isEmpty() ) {
		fnames.push( QString() );
	}

	if ( fnames.count() > 0 ) {

		// TODO: Figure out the point of this
		if ( !reuseSession ) {
			//qDebug() << "NifSkope createWindow";
			NifSkope::createWindow( fnames.pop() );
			return app.exec();
		}

		if ( IPCsocket * ipc = IPCsocket::create() ) {
			//qDebug() << "IPCSocket exec";
			ipc->execCommand( QString( "NifSkope::open %1" ).arg( fnames.pop() ) );

			while ( !fnames.isEmpty() ) {
				IPCsocket::sendCommand( QString( "NifSkope::open %1" ).arg( fnames.pop() ) );
			}

			return app.exec();
		} else {
			//qDebug() << "IPCSocket send";
			IPCsocket::sendCommand( QString( "NifSkope::open %1" ).arg( fnames.pop() ) );
			return 0;
		}

	}
}


void NifSkope::migrateSettings() const
{
	// IMPORTANT:
	//	Do not make any calls to Options:: until after all migration code.
	//	Static calls to Options:: still create the options instance and inits
	//	the various widgets with incorrect values. Once you close the app,
	//	the settings you migrated get overwritten with the default values.

	// Load current NifSkope settings
	QSettings cfg;
	// Load pre-1.2 NifSkope settings
	QSettings cfg1_1( "NifTools", "NifSkope" );

	// Current version strings
	QString curVer = NIFSKOPE_VERSION;
	QString curQtVer = QT_VERSION_STR;
	QString curDisplayVer = NifSkopeVersion::rawToDisplay( NIFSKOPE_VERSION, true );

	bool doMigration = false;

	// New Install, no need to migrate anything
	if ( !cfg.value( "Version" ).isValid() && !cfg1_1.value( "version" ).isValid() ) {
		// QSettings constructor creates an empty folder, so clear it.
		cfg1_1.clear();

		// Set version values
		cfg.setValue( "Version", curVer );
		cfg.setValue( "Qt Version", curQtVer );
		cfg.setValue( "Display Version", curDisplayVer );

		return;
	}

	// Forward dec for prevVer which either comes from `cfg` or `cfg1_1`
	QString prevVer;
	QString prevQtVer = cfg.value( "Qt Version" ).toString();
	QString prevDisplayVer = cfg.value( "Display Version" ).toString();

	// Set full granularity for version comparisons
	NifSkopeVersion::setNumParts( 7 );

	// Check for Existing 1.1 Migration
	if ( !cfg1_1.value( "migrated" ).isValid() ) {
		// Old install has not been migrated yet

		// Get prevVer from pre-1.2 settings
		prevVer = cfg1_1.value( "version" ).toString();

		NifSkopeVersion tmp( prevVer );
		if ( tmp < "1.2.0" )
			doMigration = true;
	} else {
		// Get prevVer from post-1.2 settings
		prevVer = cfg.value( "Version" ).toString();
	}

	NifSkopeVersion oldVersion( prevVer );
	NifSkopeVersion newVersion( curVer );

	// Migrate from 1.1.x to 1.2
	if ( doMigration && (oldVersion < "1.2.0") ) {
		// Port old key values to new key names
		QHash<QString, QString>::const_iterator i;
		for ( i = migrateTo1_2.begin(); i != migrateTo1_2.end(); ++i ) {
			QVariant val = cfg1_1.value( i.key() );

			if ( val.isValid() ) {
				cfg.setValue( i.value(), val );
			}
		}

		// Set `migrated` flag in legacy QSettings
		cfg1_1.setValue( "migrated", true );
	}

	// Check NifSkope Version
	//	Assure full granularity here
	NifSkopeVersion::setNumParts( 7 );
	if ( oldVersion != newVersion ) {
		// Set new Version
		cfg.setValue( "Version", curVer );

		if ( prevDisplayVer != curDisplayVer )
			cfg.setValue( "Display Version", curDisplayVer );
	}

	// Check Qt Version
	if ( curQtVer != prevQtVer ) {
		// Check all keys and delete all QByteArrays
		// to prevent portability problems between Qt versions
		QStringList keys = cfg.allKeys();

		for ( const auto& key : keys ) {
			if ( cfg.value( key ).type() == QVariant::ByteArray ) {
				qDebug() << "Removing Qt version-specific settings" << key
					<< "while migrating settings from previous version";
				cfg.remove( key );
			}
		}

		cfg.setValue( "Qt Version", curQtVer );
	}
}
