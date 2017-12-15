/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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
#include "ui_nifskope.h"

#include "glview.h"
#include "message.h"
#include "spellbook.h"
#include "version.h"
#include "gl/glscene.h"
#include "model/kfmmodel.h"
#include "model/nifmodel.h"
#include "model/nifproxymodel.h"
#include "ui/widgets/fileselect.h"
#include "ui/widgets/nifview.h"
#include "ui/widgets/refrbrowser.h"
#include "ui/widgets/inspect.h"
#include "ui/about_dialog.h"
#include "ui/settingsdialog.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QCryptographicHash>

#include <QListView>
#include <QTreeView>
#include <QStandardItemModel>

#include <fsengine/bsa.h>
#include <fsengine/fsmanager.h>

#ifdef WIN32
#  define WINDOWS_LEAN_AND_MEAN
#  include "windows.h"
#endif


//! @file nifskope.cpp The main file for %NifSkope

SettingsDialog * NifSkope::options;

const QList<QPair<QString, QString>> NifSkope::filetypes = {
	// NIF types
	{ "NIF", "nif" }, { "Bethesda Terrain", "btr" }, { "Bethesda Terrain Object", "bto" },
	// KF types
	{ "Keyframe", "kf" }, { "Keyframe Animation", "kfa" }, { "Keyframe Motion", "kfm" },
	// Miscellaneous NIF types
	{ "NIFCache", "nifcache" }, { "TEXCache", "texcache" }, { "PCPatch", "pcpatch" }, { "JMI", "jmi" }
};

QStringList NifSkope::fileExtensions()
{
	QStringList fileExts;
	for ( int i = 0; i < filetypes.size(); i++ ) {
		fileExts << filetypes.at( i ).second;
	}

	return fileExts;
}

QString NifSkope::fileFilter( const QString & ext )
{
	QString filter;

	for ( int i = 0; i < filetypes.size(); i++ ) {
		auto& ft = filetypes.at(i);
		if ( ft.second == ext )
			filter = QString( "%1 (*.%2)" ).arg( ft.first ).arg( ft.second );
	}

	return filter;
}

QString NifSkope::fileFilters( bool allFiles )
{
	QStringList filters;

	if ( allFiles ) {
		filters << QString( "All Files (*.%1)" ).arg( fileExtensions().join( " *." ) );
	}

	for ( int i = 0; i < filetypes.size(); i++ ) {
		filters << QString( "%1 (*.%2)" ).arg( filetypes.at( i ).first ).arg( filetypes.at( i ).second );
	}

	return filters.join( ";;" );
}


/*
 * main GUI window
 */

NifSkope::NifSkope()
	: QMainWindow(), ui( new Ui::MainWindow )
{
	// Init UI
	ui->setupUi( this );

	qApp->installEventFilter( this );
	
	// Init Dialogs
	
	if ( !options )
		options = new SettingsDialog;

	// Migrate settings from older versions of NifSkope
	migrateSettings();

	// Update Settings struct from registry
	updateSettings();

	// Create models
	/* ********************** */

	nif = new NifModel( this );
	proxy = new NifProxyModel( this );
	proxy->setModel( nif );

	nifEmpty = new NifModel( this );
	proxyEmpty = new NifProxyModel( this );

	nif->setMessageMode( BaseModel::UserMessage );

	// Setup QUndoStack
	nif->undoStack = new QUndoStack( this );

	indexStack = new QUndoStack( this );

	// Setup Window Modified on data change
	connect( nif, &NifModel::dataChanged, [this]( const QModelIndex &, const QModelIndex & ) {
		// Only if UI is enabled (prevents asterisk from flashing during save/load)
		if ( !windowTitle().isEmpty() && isEnabled() )
			setWindowModified( true );
	} );

	kfm = new KfmModel( this );
	kfmEmpty = new KfmModel( this );

	book = SpellBookPtr( new SpellBook( nif, QModelIndex(), this, SLOT( select( const QModelIndex & ) ) ) );

	// Setup Views
	/* ********************** */

	// Block List
	list = ui->list;
	list->setModel( proxy );
	list->setSortingEnabled( false );
	list->setItemDelegate( nif->createDelegate( this, book ) );
	list->installEventFilter( this );
	list->header()->resizeSection( NifModel::NameCol, 250 );

	// Block Details
	tree = ui->tree;
	tree->setModel( nif );
	tree->setSortingEnabled( false );
	tree->setItemDelegate( nif->createDelegate( this, book ) );
	tree->installEventFilter( this );
	tree->header()->moveSection( 1, 2 );
	tree->header()->resizeSection( NifModel::NameCol, 135 );
	tree->header()->resizeSection( NifModel::ValueCol, 250 );
	// Allow multi-row paste
	//	Note: this has some side effects such as vertex selection 
	//	in viewport being wrong if you attempt to select many rows.
	tree->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// Header Details
	header = ui->header;
	header->setModel( nif );
	header->setItemDelegate( nif->createDelegate( this, book ) );
	header->installEventFilter( this );
	header->header()->moveSection( 1, 2 );
	header->header()->resizeSection( NifModel::NameCol, 135 );
	header->header()->resizeSection( NifModel::ValueCol, 250 );

	// KFM
	kfmtree = ui->kfmtree;
	kfmtree->setModel( kfm );
	kfmtree->setItemDelegate( kfm->createDelegate( this ) );
	kfmtree->installEventFilter( this );

	// Help Browser
	refrbrwsr = ui->refrBrowser;
	refrbrwsr->setNifModel( nif );

	// Archive Browser
	bsaView = ui->bsaView;
	connect( bsaView, &QTreeView::doubleClicked, this, &NifSkope::openArchiveFile );

	bsaModel = new BSAModel( this );
	bsaProxyModel = new BSAProxyModel( this );

	// Empty Model for swapping out before model fill
	emptyModel = new QStandardItemModel( this );

	// Connect models with views
	/* ********************** */

	connect( list, &NifTreeView::sigCurrentIndexChanged, this, &NifSkope::select );
	connect( list, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );
	connect( tree, &NifTreeView::sigCurrentIndexChanged, this, &NifSkope::select );
	connect( tree, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );
	connect( tree, &NifTreeView::sigCurrentIndexChanged, refrbrwsr, &ReferenceBrowser::browse );
	connect( header, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );
	connect( kfmtree, &NifTreeView::customContextMenuRequested, this, &NifSkope::contextMenu );

	// Create GLView
	/* ********************** */

	ogl = GLView::create( this );
	ogl->setObjectName( "OGL1" );
	ogl->setNif( nif );
	ogl->installEventFilter( this );

	// Create InspectView
	/* ********************** */
	
	inspect = new InspectView;
	inspect->setNifModel( nif );
	inspect->setScene( ogl->getScene() );

	// Create Progress Bar
	/* ********************** */
	progress = new QProgressBar( ui->statusbar );
	progress->setMaximumSize( 200, 18 );
	progress->setVisible( false );

	// Process progress events
	connect( nif, &NifModel::sigProgress, [this]( int c, int m ) {
		progress->setRange( 0, m );
		progress->setValue( c );
		qApp->processEvents();
	} );

	/*
	 * UI Init
	 * **********************
	 */

	// Init Scene and View
	graphicsScene = new QGraphicsScene( this );
	graphicsView = new GLGraphicsView( this );
	graphicsView->setScene( graphicsScene );
	graphicsView->setRenderHint( QPainter::Antialiasing );
	graphicsView->setRenderHint( QPainter::SmoothPixmapTransform );
	graphicsView->setCacheMode( QGraphicsView::CacheNone );
	graphicsView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	graphicsView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	//graphicsView->setOptimizationFlags( QGraphicsView::DontSavePainterState | QGraphicsView::DontAdjustForAntialiasing );

	graphicsView->setViewport( ogl );
	graphicsView->setViewportUpdateMode( QGraphicsView::FullViewportUpdate );

	// Set central widget and viewport
	setCentralWidget( graphicsView );
	
	setContextMenuPolicy( Qt::NoContextMenu );

	// Resize timer for eventFilter()
	isResizing = false;
	resizeTimer = new QTimer( this );
	resizeTimer->setSingleShot( true );
	connect( resizeTimer, &QTimer::timeout, this, &NifSkope::resizeDone );

	// Set Actions
	initActions();

	// Dock Widgets
	initDockWidgets();

	// Toolbars
	initToolBars();

	// Menus
	initMenu();

	// Connections (that are required to load after all other inits)
	initConnections();

	connect( options, &SettingsDialog::saveSettings, this, &NifSkope::updateSettings );
	connect( options, &SettingsDialog::localeChanged, this, &NifSkope::sltLocaleChanged );

	connect( qApp, &QApplication::lastWindowClosed, this, &NifSkope::exitRequested );
}

void NifSkope::exitRequested()
{
	qApp->removeEventFilter( this );
	// Must disconnect from this signal as it's set once for each widget for some reason
	disconnect( qApp, &QApplication::lastWindowClosed, this, &NifSkope::exitRequested );

	FSManager::del();

	if ( options ) {
		delete options;
		options = nullptr;
	}
}

NifSkope::~NifSkope()
{
	delete ui;
}

void NifSkope::swapModels()
{
	// Swap out the models with empty versions while loading the file
	// This is so that the views do not update while loading the file
	if ( tree->model() == nif ) {
		list->setModel( proxyEmpty );
		tree->setModel( nifEmpty );
		header->setModel( nifEmpty );
		kfmtree->setModel( kfmEmpty );
	} else {
		list->setModel( proxy );
		tree->setModel( nif );
		header->setModel( nif );
		kfmtree->setModel( kfm );
	}
}

void NifSkope::updateSettings()
{
	QSettings settings;

	settings.beginGroup( "Settings" );

	cfg.locale = settings.value( "Locale", "en" ).toLocale();
	cfg.suppressSaveConfirm = settings.value( "UI/Suppress Save Confirmation", false ).toBool();

	settings.endGroup();
}

SettingsDialog * NifSkope::getOptions()
{
	return options;
}



void NifSkope::closeEvent( QCloseEvent * e )
{
	saveUi();

	if ( saveConfirm() )
		e->accept();
	else
		e->ignore();
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

	QModelIndex prevIdx = currentIdx;
	currentIdx = idx;

	selecting = true;

	// Push to index stack only if there is a sender
	//	Must also come AFTER selecting=true
	//	Both of these things prevent infinite recursion
	if ( sender() && !currentIdx.parent().isValid() ) {
		// Skips index selection in Block Details
		// NOTE: QUndoStack::push() calls the redo() command which calls NifSkope::select()
		//	therefore infinite recursion is possible.
		indexStack->push( new SelectIndexCommand( this, currentIdx, prevIdx ) );
	}

	// TEST: Cast sender to GLView
	//auto s = qobject_cast<GLView *>(sender());
	//if ( s )
	//	qDebug() << sender()->objectName();

	if ( sender() != ogl ) {
		ogl->setCurrentIndex( idx );
	}

	if ( sender() == ogl ) {
		if ( dList->isVisible() )
			dList->raise();
	}

	// Switch to Block Details tab if not selecting inside Header tab
	if ( sender() != header ) {
		if ( dTree->isVisible() )
			dTree->raise();
	}

	if ( sender() != list ) {
		if ( list->model() == proxy ) {
			QModelIndex idxProxy = proxy->mapFrom( nif->getBlock( idx ), list->currentIndex() );

			// Fix for NiDefaultAVObjectPalette (et al.) bug
			//	mapFrom() stops at the first result for the given block number,
			//	thus when clicking in the viewport, the actual NiTriShape is not selected
			//	but the reference to it in NiDefaultAVObjectPalette or other non-NiAVObjects.

			// The true parent of the NIF block
			QModelIndex blockParent = nif->index( nif->getParent( idx ) + 1, 0 );
			QModelIndex blockParentProxy = proxy->mapFrom( blockParent, list->currentIndex() );
			QString blockParentString = blockParentProxy.data( Qt::DisplayRole ).toString();

			// The parent string for the proxy result (possibly incorrect)
			QString proxyIdxParentString = idxProxy.parent().data( Qt::DisplayRole ).toString();

			// Determine if proxy result is incorrect
			if ( proxyIdxParentString != blockParentString ) {
				// Find ALL QModelIndex which match the display string
				for ( const QModelIndex & i : list->model()->match( list->model()->index( 0, 0 ), Qt::DisplayRole, idxProxy.data( Qt::DisplayRole ),
					100, Qt::MatchRecursive ) )
				{
					// Skip if child of NiDefaultAVObjectPalette, et al.
					if ( i.parent().data( Qt::DisplayRole ).toString() != blockParentString )
						continue;

					list->setCurrentIndex( i );
				}
			} else {
				// Proxy parent is already an ancestor of NiAVObject
				list->setCurrentIndex( idxProxy );
			}

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
			// proxy model has only two columns (see columnCount in nifproxymodel.h)
			list->setColumnHidden( 0, false );
			list->setColumnHidden( 1, false );
			head->resizeSection( 0, s0 );
			head->resizeSection( 1, s1 );
		}
	}
}

// 'Recent Files' Helpers

QString strippedName( const QString & fullFileName )
{
	return QFileInfo( fullFileName ).fileName();
}

int updateRecentActions( QAction * acts[], const QStringList & files )
{
	int numRecentFiles = std::min( files.size(), (int)NifSkope::NumRecentFiles );

	for ( int i = 0; i < numRecentFiles; ++i ) {
		QString text = QString( "&%1 %2" ).arg( i + 1 ).arg( strippedName( files[i] ) );
		acts[i]->setText( text );
		acts[i]->setData( files[i] );
		acts[i]->setStatusTip( files[i] );
		acts[i]->setVisible( true );
	}
	for ( int j = numRecentFiles; j < NifSkope::NumRecentFiles; ++j )
		acts[j]->setVisible( false );

	return numRecentFiles;
}

void updateRecentFiles( QStringList & files, const QString & file )
{
	files.removeAll( file );
	files.prepend( file );
	while ( files.size() > NifSkope::NumRecentFiles )
		files.removeLast();
}
// End Helpers


void NifSkope::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value( "File/Recent File List" ).toStringList();

	int numRecentFiles = ::updateRecentActions( recentFileActs, files );

	aRecentFilesSeparator->setVisible( numRecentFiles > 0 );
	ui->mRecentFiles->setEnabled( numRecentFiles > 0 );
}

void NifSkope::updateAllRecentFileActions()
{
	for ( QWidget * widget : QApplication::topLevelWidgets() ) {
		NifSkope * win = qobject_cast<NifSkope *>(widget);
		if ( win ) {
			win->updateRecentFileActions();
			win->updateRecentArchiveActions();
			win->updateRecentArchiveFileActions();
		}
	}
}

QString NifSkope::getCurrentFile() const
{
	return currentFile;
}

void NifSkope::setCurrentFile( const QString & filename )
{
	currentFile = QDir::fromNativeSeparators( filename );

	nif->refreshFileInfo( currentFile );

	setWindowFilePath( currentFile );

	// Avoid adding files opened from BSAs to Recent Files
	QFileInfo file( currentFile );
	if ( !file.exists() && !file.isAbsolute() ) {
		setCurrentArchiveFile( filename );
		return;
	}

	QSettings settings;
	QStringList files = settings.value( "File/Recent File List" ).toStringList();
	::updateRecentFiles( files, currentFile );

	settings.setValue( "File/Recent File List", files );

	updateAllRecentFileActions();
}

void NifSkope::setCurrentArchiveFile( const QString & filepath )
{
	QString bsa = filepath.split( "/" ).first();
	if ( !bsa.endsWith( ".bsa", Qt::CaseInsensitive ) && !bsa.endsWith( ".ba2", Qt::CaseInsensitive ) )
		return;

	// Strip BSA name from beginning of path
	QString path = filepath;
	path.replace( bsa + "/", "" );

	QSettings settings;
	QHash<QString, QVariant> hash = settings.value( "File/Recent Archive Files" ).toHash();

	// Retrieve and update existing Recent Files for BSA
	QStringList filepaths = hash.value( bsa ).toStringList();
	::updateRecentFiles( filepaths, path );

	// Replace BSA's Recent Files
	hash[bsa] = filepaths;

	settings.setValue( "File/Recent Archive Files", hash );

	updateAllRecentFileActions();
}

void NifSkope::clearCurrentFile()
{
	QSettings settings;
	QStringList files = settings.value( "File/Recent File List" ).toStringList();
	files.removeAll( currentFile );
	settings.setValue( "File/Recent File List", files );

	updateAllRecentFileActions();
}

void NifSkope::setCurrentArchive( BSA * bsa )
{
	currentArchive = bsa;

	QString file = currentArchive->path();

	QSettings settings;
	QStringList files = settings.value( "File/Recent Archive List" ).toStringList();
	::updateRecentFiles( files, file );

	settings.setValue( "File/Recent Archive List", files );

	updateAllRecentFileActions();
}

void NifSkope::clearCurrentArchive()
{
	QSettings settings;
	QStringList files = settings.value( "File/Recent Archive List" ).toStringList();

	files.removeAll( currentArchive->path() );
	settings.setValue( "File/Recent Archive List", files );

	updateAllRecentFileActions();
}

void NifSkope::updateRecentArchiveActions()
{
	QSettings settings;
	QStringList files = settings.value( "File/Recent Archive List" ).toStringList();

	int numRecentFiles = ::updateRecentActions( recentArchiveActs, files );

	ui->mRecentArchives->setEnabled( numRecentFiles > 0 );
}

void NifSkope::updateRecentArchiveFileActions()
{
	QSettings settings;
	QHash<QString, QVariant> hash = settings.value( "File/Recent Archive Files" ).toHash();

	if ( !currentArchive )
		return;

	QString key = currentArchive->name();

	QStringList files = hash.value( key ).toStringList();

	int numRecentFiles = ::updateRecentActions( recentArchiveFileActs, files );

	mRecentArchiveFiles->setEnabled( numRecentFiles > 0 );
}

QByteArray fileChecksum( const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm )
{
	QFile f( fileName );
	if ( f.open( QFile::ReadOnly ) ) {
		QCryptographicHash hash( hashAlgorithm );
		if ( hash.addData( &f ) ) {
			return hash.result();
		}
	}
	return QByteArray();
}

void NifSkope::checkFile( QFileInfo fInfo, QByteArray hash )
{
	QString fname = fInfo.fileName();
	QString fpath = fInfo.filePath();
	QDir::temp().mkdir( "NifSkope" );
	QString tmpDir = QDir::tempPath() + "/NifSkope";
	QDir tmp( tmpDir );
	QString tmpFile = tmpDir + "/" + fInfo.fileName();

	emit beginSave();
	bool saved = nif->saveToFile( tmpFile );
	if ( saved ) {
		auto filehash2 = fileChecksum( tmpFile, QCryptographicHash::Md5 );

		if ( hash == filehash2 ) {
			tmp.remove( fname );
		} else {
			QString err = "An MD5 hash comparison indicates this file will not be 100% identical upon saving. This could indicate underlying issues with the data in this file.";
			Message::warning( this, err, fpath );
#ifdef QT_NO_DEBUG
			tmp.remove( fname );
#endif
		}
	}
	emit completeSave( saved, fpath );
}

void NifSkope::openArchive( const QString & archive )
{
	// Clear memory from previously opened archives
	bsaModel->clear();
	bsaProxyModel->clear();
	bsaProxyModel->setSourceModel( emptyModel );
	bsaView->setModel( emptyModel );
	bsaView->setSortingEnabled( false );

	archiveHandler.reset();

	archiveHandler = FSArchiveHandler::openArchive( archive );
	if ( !archiveHandler ) {
		qCWarning( nsIo ) << "The BSA could not be opened.";
		return;
	}

	auto bsa = archiveHandler->getArchive<BSA *>();
	if ( bsa ) {

		setCurrentArchive( bsa );

		// Models
		bsaModel->init();

		// Populate model from BSA
		bsa->fillModel( bsaModel, "meshes" );

		if ( bsaModel->rowCount() == 0 ) {
			qCWarning( nsIo ) << "The BSA does not contain any meshes.";
			clearCurrentArchive();
			return;
		}

		// Set proxy and view only after filling source model
		bsaProxyModel->setSourceModel( bsaModel );
		bsaView->setModel( bsaProxyModel );
		bsaView->setSortingEnabled( true );

		bsaView->hideColumn( 1 );
		bsaView->setColumnWidth( 0, 300 );
		bsaView->setColumnWidth( 2, 50 );

		// Sort proxy after model/view is populated
		bsaProxyModel->sort( 0, Qt::AscendingOrder );
		bsaProxyModel->setFiletypes( { ".nif", ".bto", ".btr" } );
		bsaProxyModel->resetFilter();

		// Set filename label
		ui->bsaName->setText( currentArchive->name() );

		ui->bsaFilter->setEnabled( true );
		ui->bsaFilenameOnly->setEnabled( true );

		// Bring tab to front
		dBrowser->raise();

		// Filter
		auto filterTimer = new QTimer( this );
		filterTimer->setSingleShot( true );

		connect( ui->bsaFilter, &QLineEdit::textChanged, [filterTimer]() { filterTimer->start( 300 ); } );
		connect( filterTimer, &QTimer::timeout, [this]() {
			auto text = ui->bsaFilter->text();

			bsaProxyModel->setFilterRegExp( QRegExp( text, Qt::CaseInsensitive, QRegExp::Wildcard ) );
			bsaView->expandAll();

			if ( text.isEmpty() ) {
				bsaView->collapseAll();
				bsaProxyModel->resetFilter();
			}
				
		} );

		connect( ui->bsaFilenameOnly, &QCheckBox::toggled, bsaProxyModel, &BSAProxyModel::setFilterByNameOnly );

		// Update filter when switching open archives
		filterTimer->start( 0 );
	}
}

void NifSkope::openArchiveFile( const QModelIndex & index )
{
	QString filepath = index.sibling( index.row(), 1 ).data( Qt::EditRole ).toString();

	if ( !filepath.isEmpty() )
		openArchiveFileString( currentArchive, filepath );
}

void NifSkope::openArchiveFileString( BSA * bsa, const QString & filepath )
{
	if ( bsa->hasFile( filepath ) ) {
		if ( !saveConfirm() )
			return;

		// Read data from BSA
		QByteArray data;
		bsa->fileContents( filepath, data );

		// Format like "BSANAME.BSA/path/to/file.nif"
		QString path = bsa->name() + "/" + filepath;

		QBuffer buf;
		buf.setData( data );
		if ( buf.open( QBuffer::ReadOnly ) ) {

			emit beginLoading();

			bool loaded = nif->load( buf );
			if ( loaded )
				setCurrentFile( path );

			emit completeLoading( loaded, path );

			//if ( loaded ) {
			//	QCryptographicHash hash( QCryptographicHash::Md5 );
			//	hash.addData( data );
			//	filehash = hash.result();
			//
			//	QFileInfo f( path );
			//	
			//	checkFile( f, filehash );
			//}

			buf.close();
		}
	}
}


void NifSkope::openFile( QString & file )
{
	if ( !saveConfirm() )
		return;

	loadFile( file );
}

void NifSkope::openRecentFile()
{
	if ( !saveConfirm() )
		return;

	QAction * action = qobject_cast<QAction *>(sender());
	if ( action )
		loadFile( action->data().toString() );
}

void NifSkope::openRecentArchive()
{
	QAction * action = qobject_cast<QAction *>(sender());
	if ( action )
		openArchive( action->data().toString() );
}

void NifSkope::openRecentArchiveFile()
{
	QAction * action = qobject_cast<QAction *>(sender());
	if ( action )
		openArchiveFileString( currentArchive, action->data().toString() );
}


void NifSkope::openFiles( QStringList & files )
{
	// Open first file in current window if blank
	//	or only one file selected.
	if ( getCurrentFile().isEmpty() || files.count() == 1 ) {
		QString first = files.takeFirst();
		if ( !first.isEmpty() )
			loadFile( first );
	}

	for ( const QString & file : files ) {
		NifSkope::createWindow( file );
	}
}

void NifSkope::saveFile( const QString & filename )
{
	setCurrentFile( filename );
	save();
}

void NifSkope::loadFile( const QString & filename )
{
	QApplication::setOverrideCursor( Qt::WaitCursor );

	setCurrentFile( filename );
	QTimer::singleShot( 0, this, SLOT( load() ) );
}

void NifSkope::reload()
{
	QTimer::singleShot( 0, this, SLOT( load() ) );
}

void NifSkope::load()
{
	emit beginLoading();

	QFileInfo f( QDir::fromNativeSeparators( currentFile ) );
	f.makeAbsolute();

	QString fname = f.filePath();

	// TODO: This is rather poor in terms of file validation

	if ( f.suffix().compare( "kfm", Qt::CaseInsensitive ) == 0 ) {
		emit completeLoading( kfm->loadFromFile( fname ), fname );

		f.setFile( kfm->getFolder(), kfm->get<QString>( kfm->getKFMroot(), "NIF File Name" ) );

		return;
	}

	bool loaded = nif->loadFromFile( fname );

	emit completeLoading( loaded, fname );

	//if ( loaded ) {
	//	filehash = fileChecksum( fname, QCryptographicHash::Md5 );
	//
	//	checkFile( f, filehash );
	//}
}

void NifSkope::save()
{
	// Assure file path is absolute
	// If not absolute, it is loaded from a BSA
	QFileInfo curFile( currentFile );
	if ( !curFile.isAbsolute() ) {
		saveAsDlg();
		return;
	}

	emit beginSave();

	QString fname = currentFile;

	// TODO: This is rather poor in terms of file validation

	if ( fname.endsWith( ".KFM", Qt::CaseInsensitive ) ) {
		emit completeSave( kfm->saveToFile( fname ), fname );
	} else {
		if ( aSanitize->isChecked() ) {
			QModelIndex idx = SpellBook::sanitize( nif );
			if ( idx.isValid() )
				select( idx );
		}

		emit completeSave( nif->saveToFile( fname ), fname );
	}
}


//! Opens website links using the QAction's tooltip text
void NifSkope::openURL()
{
	// Note: This method may appear unused but this slot is
	//	utilized in the nifskope.ui file.

	if ( !sender() )
		return;

	QAction * aURL = qobject_cast<QAction *>( sender() );
	if ( !aURL )
		return;

	// Sender is an action, grab URL from tooltip
	QUrl URL(aURL->toolTip());
	if ( !URL.isValid() )
		return;

	QDesktopServices::openUrl( URL );
}


/*
 *	SelectIndexCommand
 *		Manages cycling between previously selected indices like a browser Back/Forward button
 */

SelectIndexCommand::SelectIndexCommand( NifSkope * wnd, const QModelIndex & cur, const QModelIndex & prev )
{
	nifskope = wnd;

	curIdx = cur;
	prevIdx = prev;
}

void SelectIndexCommand::redo()
{
	nifskope->select( curIdx );
}

void SelectIndexCommand::undo()
{
	nifskope->select( prevIdx );
}


//! Application-wide debug and warning message handler
void NifSkope::MessageOutput( QtMsgType type, const QMessageLogContext & context, const QString & str )
{
	switch ( type ) {
	case QtDebugMsg:
		fprintf( stderr, "[Debug] %s\n", qPrintable( str ) );
		break;
	case QtWarningMsg:
		fprintf( stderr, "[Warning] %s\n", qPrintable( str ) );
		Message::message( qApp->activeWindow(), str, &context, QMessageBox::Warning );
		break;
	case QtCriticalMsg:
		fprintf( stderr, "[Critical] %s\n", qPrintable( str ) );
		Message::message( qApp->activeWindow(), str, &context, QMessageBox::Critical );
		break;
	case QtFatalMsg:
		fprintf( stderr, "[Fatal] %s\n", qPrintable( str ) );
		break;
	case QtInfoMsg:
		fprintf( stderr, "[Info] %s\n", qPrintable( str ) );
		break;
	}
}


static QTranslator * mTranslator = nullptr;

//! Sets application locale and loads translation files
void NifSkope::SetAppLocale( QLocale curLocale )
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

	QLocale::setDefault( QLocale::C );
}

void NifSkope::sltLocaleChanged()
{
	SetAppLocale( cfg.locale );

	QMessageBox mb( "NifSkope",
	                tr( "NifSkope must be restarted for this setting to take full effect." ),
	                QMessageBox::Information, QMessageBox::Ok + QMessageBox::Default, 0, 0,
	                qApp->activeWindow()
	);
	mb.setIconPixmap( QPixmap( ":/res/nifskope.png" ) );
	mb.exec();

	// TODO: Retranslate dynamically
	//ui->retranslateUi( this );
}


void NifSkope::migrateSettings() const
{
	// Load current NifSkope settings
	QSettings settings;
	// Load pre-1.2 NifSkope settings
	QSettings cfg1_1( "NifTools", "NifSkope" );
	// Load NifSkope 1.2 settings
	QSettings cfg1_2( "NifTools", "NifSkope 1.2" );

	// Current version strings
	QString curVer = NIFSKOPE_VERSION;
	QString curQtVer = QT_VERSION_STR;
	QString curDisplayVer = NifSkopeVersion::rawToDisplay( NIFSKOPE_VERSION, true );

	// New Install, no need to migrate anything
	if ( !settings.value( "Version" ).isValid() && !cfg1_1.value( "version" ).isValid() ) {
		// QSettings constructor creates an empty folder, so clear it.
		cfg1_1.clear();

		// Set version values
		settings.setValue( "Version", curVer );
		settings.setValue( "Qt Version", curQtVer );
		settings.setValue( "Display Version", curDisplayVer );

		return;
	}

	QString prevVer = curVer;
	QString prevQtVer = settings.value( "Qt Version" ).toString();
	QString prevDisplayVer = settings.value( "Display Version" ).toString();

	// Set full granularity for version comparisons
	NifSkopeVersion::setNumParts( 7 );

	// Test migration lambda
	//	Note: Sets value of prevVer
	auto testMigration = [&prevVer]( QSettings & migrateFrom, const char * migrateTo ) {
		if ( migrateFrom.value( "version" ).isValid() && !migrateFrom.value( "migrated" ).isValid() ) {
			prevVer = migrateFrom.value( "version" ).toString();

			NifSkopeVersion tmp( prevVer );
			if ( tmp < migrateTo )
				return true;
		}
		return false;
	};

	// Migrate lambda
	//	Using a QHash of registry keys (stored in version.h), migrates from one version to another.
	auto migrate = []( QSettings & migrateFrom, QSettings & migrateTo, const QHash<QString, QString> & migration ) {
		QHash<QString, QString>::const_iterator i;
		for ( i = migration.begin(); i != migration.end(); ++i ) {
			QVariant val = migrateFrom.value( i.key() );

			if ( val.isValid() ) {
				migrateTo.setValue( i.value(), val );
			}
		}

		migrateFrom.setValue( "migrated", true );
	};

	// NOTE: These set `prevVer` and must come before setting `oldVersion`
	bool migrateFrom1_1 = testMigration( cfg1_1, "1.2.0" );
	bool migrateFrom1_2 = testMigration( cfg1_2, "2.0" );

	if ( !migrateFrom1_1 && !migrateFrom1_2 ) {
		prevVer = settings.value( "Version" ).toString();
	}

	NifSkopeVersion oldVersion( prevVer );
	NifSkopeVersion newVersion( curVer );

	// Check NifSkope Version
	//	Assure full granularity here
	NifSkopeVersion::setNumParts( 7 );
	if ( oldVersion != newVersion ) {

		// Migrate from 1.1.x to 1.2
		if ( migrateFrom1_1 ) {
			qDebug() << "Migrating from 1.1 to 1.2";
			migrate( cfg1_1, cfg1_2, migrateTo1_2 );
		}

		// Migrate from 1.2.x to 2.0
		if ( migrateFrom1_2 ) {
			qDebug() << "Migrating from 1.2 to 2.0";
			migrate( cfg1_2, settings, migrateTo2_0 );
		}

		// Set new Version
		settings.setValue( "Version", curVer );

		if ( prevDisplayVer != curDisplayVer )
			settings.setValue( "Display Version", curDisplayVer );

		// Migrate to new Settings
		if ( oldVersion <= NifSkopeVersion( "2.0.dev1" ) ) {
			qDebug() << "Migrating to new Settings";

			// Sanitize backslashes
			auto sanitize = []( QVariant oldVal ) {
				QStringList sanitized;
				for ( const QString & archive : oldVal.toStringList() ) {
					if ( archive == "AUTO" ) {
						sanitized.append( FSManager::autodetectArchives() );
						continue;
					}

					sanitized.append( QDir::fromNativeSeparators( archive ) );
				}

				return sanitized;
			};

			QVariant foldersVal = settings.value( "Settings/Resources/Folders" );
			if ( foldersVal.toStringList().isEmpty() ) {
				QVariant oldVal = settings.value( "Render Settings/Texture Folders" );
				if ( !oldVal.isNull() ) {
					settings.setValue( "Settings/Resources/Folders", sanitize( oldVal ) );
				}
			}

			QVariant archivesVal = settings.value( "Settings/Resources/Archives" );
			if ( archivesVal.toStringList().isEmpty() ) {
				QVariant oldVal = settings.value( "FSEngine/Archives" );
				if ( !oldVal.isNull() ) {
					settings.setValue( "Settings/Resources/Archives", sanitize( oldVal ) );
				}
			}

			// Update archive handler
			FSManager::get()->initialize();
			
			// Remove old keys

			settings.remove( "FSEngine" );
			settings.remove( "Render Settings" );
			settings.remove( "Settings/Language" );
			settings.remove( "Settings/Startup Version" );
		}
	}

#ifdef QT_NO_DEBUG
	// Check Qt Version
	if ( curQtVer != prevQtVer ) {
		// Check all keys and delete all QByteArrays
		// to prevent portability problems between Qt versions
		QStringList keys = settings.allKeys();

		for ( const auto& key : keys ) {
			if ( settings.value( key ).type() == QVariant::ByteArray ) {
				qDebug() << "Removing Qt version-specific settings" << key
					<< "while migrating settings from previous version";
				settings.remove( key );
			}
		}

		settings.setValue( "Qt Version", curQtVer );
	}
#endif
}
