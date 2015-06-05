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

#ifndef NIFSKOPE_H
#define NIFSKOPE_H

#include "message.h"

#include <QMainWindow>     // Inherited
#include <QObject>         // Inherited

#if QT_NO_DEBUG
#define NIFSKOPE_IPC_PORT 12583
#else
#define NIFSKOPE_IPC_PORT 12584
#endif

namespace Ui {
	class MainWindow;
}

class FileSelector;
class GLView;
class InspectView;
class KfmModel;
class NifModel;
class NifProxyModel;
class NifTreeView;
class ReferenceBrowser;
class SettingsDialog;
class SpellBook;

class QAction;
class QActionGroup;
class QComboBox;
class QGraphicsScene;
class QGraphicsView;
class QLocale;
class QModelIndex;
class QProgressBar;
class QStringList;
class QTimer;
class QUdpSocket;


//! @file nifskope.h NifSkope, IPCsocket

/*! The main application class for NifSkope.
 *
 * This class encapsulates the main NifSkope window. It has members for saving
 * and restoring settings, loading and saving NIF files, loading an XML
 * description, widgets for the various subwindows, menus, and a UDP socket
 * with which NifSkope can communicate with itself.
 */
class NifSkope final : public QMainWindow
{
	Q_OBJECT

public:
	NifSkope();
	~NifSkope();

	Ui::MainWindow * ui;

	//! Save Confirm dialog
	bool saveConfirm();
	//! Save NifSkope application settings.
	void saveUi() const;
	//! Restore NifSkope UI settings.
	void restoreUi();

	//! Returns path of currently open file
	QString getCurrentFile() const;

	/*! Create and initialize a new NifSkope application window.
	 *
	 * @param fname The name of the file to load in the new NifSkope window.
	 * @return		The newly created NifSkope instance.
	 */
	static NifSkope * createWindow( const QString & fname = QString() );

	static SettingsDialog * options();


	//! List of all supported file extensions
	static QStringList fileExtensions();

	//! Return a file filter for a single extension
	static QString fileFilter( const QString & );

	/*! Return a file filter for all supported extensions.
	 *
	 * @param allFiles If true, file filter will be prepended with "All Files (*.nif *.btr ...)"
	 *					so that all supported files will show at once. Used for Open File dialog.
	 */
	static QString fileFilters( bool allFiles = true );

	//! A map of all the currently support filetypes to their file extensions.
	static const QList<QPair<QString, QString>> filetypes;

signals:
	void beginLoading();
	void completeLoading( bool, QString & );
	void beginSave();
	void completeSave( bool, QString & );

public slots:
	void openFile( QString & );
	void openFiles( QStringList & );

	void enableUi();

	// Automatic slots

	//! Reparse the nif.xml and kfm.xml files.
	void on_aLoadXML_triggered();

	//! Reparse the nif.xml and kfm.xml files and reload the current file.
	void on_aReload_triggered();

	//! A slot that creates a new NifSkope application window.
	void on_aWindow_triggered();

	//! A slot for starting the XML checker.
	void on_aShredder_triggered();

    //! A slot for starting the Batch processor checker.
    void on_aBatchProcessor_triggered();

	//! Reset "block details"
	void on_aHeader_triggered();

	//! Select the font to use
	void on_aSelectFont_triggered();

	void on_tRender_actionTriggered( QAction * );

	void on_aViewTop_triggered( bool );
	void on_aViewFront_triggered( bool );
	void on_aViewLeft_triggered( bool );
	
	void on_aViewCenter_triggered();
	void on_aViewFlip_triggered( bool );
	void on_aViewPerspective_toggled( bool );
	void on_aViewWalk_triggered( bool );
	
	void on_aViewUser_toggled( bool );
	void on_aViewUserSave_triggered( bool );

	void on_aSettings_triggered();


protected slots:
	void openDlg();
	void saveAsDlg();

	void load();
	void save();

	void reload();

	void onLoadBegin();
	void onSaveBegin();

	void onLoadComplete( bool, QString & );
	void onSaveComplete( bool, QString & );

	//! Select a NIF index
	void select( const QModelIndex & );

	//! Display a context menu at the specified position
	void contextMenu( const QPoint & pos );

	//! Set the list mode
	void setListMode();

	//! Override the view font
	void overrideViewFont();

	/*! Sets Import/Export menus
	 *
	 * @see importex/importex.cpp
	 */
	void fillImportExportMenus();
	//! Perform Import or Export
	void sltImportExport( QAction * action );

	//! Open a URL using the system handler
	void openURL();

	//! Change system locale and notify user that restart may be required
	void sltLocaleChanged();

	//! Called after window resizing has stopped
	void resizeDone();

protected:
	void closeEvent( QCloseEvent * e ) override final;
	//void resizeEvent( QResizeEvent * event ) override final;
	bool eventFilter( QObject * o, QEvent * e ) override final;

private:
	void initActions();
	void initDockWidgets();
	void initToolBars();
	void initMenu();
	void initConnections();

	void loadFile( const QString & );
	void saveFile( const QString & );

	void openRecentFile();
	void setCurrentFile( const QString & );
	void clearCurrentFile();
	void updateRecentFileActions();
	void updateAllRecentFileActions();

	QString strippedName( const QString & ) const;

	QMenu * lightingWidget();
	QWidget * filePathWidget( QWidget * );

	void setViewFont( const QFont & );

	//! Migrate settings from older versions of NifSkope.
	void migrateSettings() const;

	//! "About NifSkope" dialog.
	QWidget * aboutDialog;

	SettingsDialog * settingsDlg;

	QString currentFile;

	//! Stores the NIF file in memory.
	NifModel * nif;
	//! A hierarchical proxy for the NIF file.
	NifProxyModel * proxy;
	//! Stores the KFM file in memory.
	KfmModel * kfm;

	//! This view shows the block list.
	NifTreeView * list;
	//! This view shows the block details.
	NifTreeView * tree;
	//! This view shows the file header.
	NifTreeView * header;

	//! This view shows the KFM file, if any.
	NifTreeView * kfmtree;

	//! Spellbook instance
	SpellBook * book;

	//! Help browser
	ReferenceBrowser * refrbrwsr;

	//! Transform inspect view
	InspectView * inspect;

	//! The main window
	GLView * ogl;

	QGraphicsScene * graphicsScene;
	QGraphicsView * graphicsView;

	QComboBox * animGroups;
	QAction * animGroupsAction;

	bool selecting;
	bool initialShowEvent;
	
	QProgressBar * progress = nullptr;

	QDockWidget * dList;
	QDockWidget * dTree;
	QDockWidget * dHeader;
	QDockWidget * dKfm;
	QDockWidget * dRefr;
	QDockWidget * dInsp;

	QToolBar * tool;

	QAction * aSanitize;

#ifdef FSENGINE
	QAction * aResources;
#endif

	QAction * undoAction;
	QAction * redoAction;

	QActionGroup * showActions;
	QActionGroup * shadingActions;

	QActionGroup * gListMode;
	QAction * aList;
	QAction * aHierarchy;
	QAction * aCondition;
	QAction * aRCondition;

	QAction * aSelectFont;

	QMenu * mExport;
	QMenu * mImport;

	QAction * aRecentFilesSeparator;

	enum { NumRecentFiles = 10 };
	QAction * recentFileActs[NumRecentFiles];

	bool isResizing;
	QTimer * resizeTimer;
	QImage buf;
};


//! UDP communication between instances
class IPCsocket final : public QObject
{
	Q_OBJECT

public:
	//! Creates a socket
	static IPCsocket * create( int port );

	//! Sends a command
	static void sendCommand( const QString & cmd, int port );

public slots:
	//! Acts on a command
	void execCommand( const QString & cmd );

	//! Opens a NIF from a URL
	void openNif( const QString & );

protected slots:
	void processDatagram();

protected:
	IPCsocket( QUdpSocket * );
	~IPCsocket();

	QUdpSocket * socket;
};

#endif
