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

#include <QMainWindow>     // Inherited
#include <QObject>         // Inherited
#include <QFileInfo>
#include <QLocale>
#include <QModelIndex>
#include <QSet>
#include <QUndoCommand>

#include <memory>

#if QT_NO_DEBUG
#define NIFSKOPE_IPC_PORT 12583
#else
#define NIFSKOPE_IPC_PORT 12584
#endif

namespace Ui {
	class MainWindow;
}

namespace nstypes
{
	QString operator"" _uip( const char * str, size_t sz );
}

class GLView;
class GLGraphicsView;
class InspectView;
class KfmModel;
class NifModel;
class NifProxyModel;
class NifTreeView;
class ReferenceBrowser;
class SettingsDialog;
class SpellBook;
class FSArchiveHandler;
class BSA;
class BSAModel;
class BSAProxyModel;
class QStandardItemModel;
class QAction;
class QActionGroup;
class QComboBox;
class QGraphicsScene;
class QProgressBar;
class QStringList;
class QTimer;
class QTreeView;
class QUdpSocket;

namespace nstheme
{
	enum WindowColor { Base, BaseAlt, Text, Highlight, HighlightText, BrightText };
	enum WindowTheme { ThemeDark, ThemeLight, ThemeWindows, ThemeWindowsXP };
	enum ToolbarSize { ToolbarSmall, ToolbarLarge };
}


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

	static SettingsDialog * getOptions();

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

	//! Sets application locale and loads translation files
	static void SetAppLocale( QLocale curLocale );
	//! Application-wide debug and warning message handler
	static void MessageOutput( QtMsgType type, const QMessageLogContext & context, const QString & str );

	//! A map of all the currently support filetypes to their file extensions.
	static const QList<QPair<QString, QString>> filetypes;

	enum { NumRecentFiles = 10 };

	static QColor defaultsDark[6];
	static QColor defaultsLight[6];

	static void reloadTheme();

signals:
	void beginLoading();
	void completeLoading( bool, QString & );
	void beginSave();
	void completeSave( bool, QString & );

public slots:
	void openFile( QString & );
	void openFiles( QStringList & );

	void openArchive( const QString & );
	void openArchiveFile( const QModelIndex & );
	void openArchiveFileString( BSA *, const QString & );

	void enableUi();

	void updateSettings();

	//! Select a NIF index
	void select( const QModelIndex & );

	// Automatic slots

	//! Reparse the nif.xml and kfm.xml files.
	void on_aLoadXML_triggered();

	//! Reparse the nif.xml and kfm.xml files and reload the current file.
	void on_aReload_triggered();

	//! A slot that creates a new NifSkope application window.
	void on_aWindow_triggered();

	//! A slot for starting the XML checker.
	void on_aShredder_triggered();

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

	void on_mTheme_triggered( QAction * action );


protected slots:
	void openDlg();
	void saveAsDlg();

	void archiveDlg();

	void load();
	void save();

	void reload();

	void exitRequested();

	void onLoadBegin();
	void onSaveBegin();

	void onLoadComplete( bool, QString & );
	void onSaveComplete( bool, QString & );

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
	void checkFile( QFileInfo fInfo, QByteArray filehash );

	void openRecentFile();
	void setCurrentFile( const QString & );
	void clearCurrentFile();
	void updateRecentFileActions();
	void updateAllRecentFileActions();

	void openRecentArchive();
	void openRecentArchiveFile();
	void setCurrentArchive( BSA * );
	void setCurrentArchiveFile( const QString & );
	void clearCurrentArchive();
	void updateRecentArchiveActions();
	void updateRecentArchiveFileActions();

	//! Disconnect and reconnect the models to the views
	void swapModels();

	QMenu * lightingWidget();
	QWidget * filePathWidget( QWidget * );

	void setViewFont( const QFont & );

	//! Load the theme
	void loadTheme();
	//! Sync the theme actions in the UI
	void setThemeActions();
	//! Set the toolbar size
	void setToolbarSize();
	//! Set the theme
	void setTheme( nstheme::WindowTheme theme );

	//! Migrate settings from older versions of NifSkope.
	void migrateSettings() const;

	//! All QActions in the UI
	QSet<QAction *> allActions;

	nstheme::WindowTheme theme = nstheme::ThemeDark;
	nstheme::ToolbarSize toolbarSize = nstheme::ToolbarLarge;

	QString currentFile;
	BSA * currentArchive = nullptr;

	QByteArray filehash;

	//! Stores the NIF file in memory.
	NifModel * nif;
	//! A hierarchical proxy for the NIF file.
	NifProxyModel * proxy;
	//! Stores the KFM file in memory.
	KfmModel * kfm;

	NifModel * nifEmpty;
	NifProxyModel * proxyEmpty;
	KfmModel * kfmEmpty;

	//! This view shows the block list.
	NifTreeView * list;
	//! This view shows the block details.
	NifTreeView * tree;
	//! This view shows the file header.
	NifTreeView * header;
	//! This view shows the archive browser files.
	QTreeView * bsaView;

	//! This view shows the KFM file, if any.
	NifTreeView * kfmtree;

	//! Spellbook instance
	std::shared_ptr<SpellBook> book;

	std::shared_ptr<FSArchiveHandler> archiveHandler;

	static SettingsDialog * options;

	//! Help browser
	ReferenceBrowser * refrbrwsr;

	//! Transform inspect view
	InspectView * inspect;

	//! The main window
	GLView * ogl;

	QGraphicsScene * graphicsScene;
	GLGraphicsView * graphicsView;

	QComboBox * animGroups;
	QAction * animGroupsAction;

	bool selecting = false;
	bool initialShowEvent = true;
	
	QProgressBar * progress = nullptr;

	QDockWidget * dList;
	QDockWidget * dTree;
	QDockWidget * dHeader;
	QDockWidget * dKfm;
	QDockWidget * dRefr;
	QDockWidget * dInsp;
	QDockWidget * dBrowser;

	QToolBar * tool;

	QAction * aSanitize;

	QAction * undoAction;
	QAction * redoAction;

	QActionGroup * selectActions;
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

	QAction * recentFileActs[NumRecentFiles];
	QAction * recentArchiveActs[NumRecentFiles];
	QAction * recentArchiveFileActs[NumRecentFiles];

	bool isResizing;
	QTimer * resizeTimer;
	QImage viewBuffer;

	struct Settings
	{
		QLocale locale;
		bool suppressSaveConfirm;
	} cfg;

	//! The currently selected index
	QModelIndex currentIdx;

	QUndoStack * indexStack;
	//QAction * idxForwardAction;
	//QAction * idxBackAction;

	BSAModel * bsaModel;
	BSAProxyModel * bsaProxyModel;
	QStandardItemModel * emptyModel;

	QMenu * mRecentArchiveFiles;
};


class SelectIndexCommand : public QUndoCommand
{
public:
	SelectIndexCommand( NifSkope *, const QModelIndex &, const QModelIndex & );
	void redo() override;
	void undo() override;
private:
	QModelIndex curIdx, prevIdx;

	NifSkope * nifskope;
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

	void openNif( const QUrl & );

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
