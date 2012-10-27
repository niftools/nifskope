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

#ifndef NIFSKOPE_H
#define NIFSKOPE_H

#include <QMainWindow>
#include <QProgressDialog>

#define NIFSKOPE_IPC_PORT 12583

class NifModel;
class NifProxyModel;
class KfmModel;
class NifTreeView;
class ReferenceBrowser;
class InspectView;

class GLView;
class FileSelector;
 
class QModelIndex;

class QAction;
class QActionGroup;
class QSettings;
class QSlider;
class QSpinBox;
class QTextEdit;
class QTranslator;
class QLocale;

class QUdpSocket;

#include "message.h"

#include "ui/about_dialog.h"

//! \file nifskope.h The main header for NifSkope

//! The main application class for NifSkope. 
/*!
 * This class encapsulates the main NifSkope window. It has members for saving
 * and restoring settings, loading and saving nif files, loading an xml
 * description, widgets for the various subwindows, menu's, and a socket by
 * which NifSkope can communicate with itself.
 */
class NifSkope : public QMainWindow
{
Q_OBJECT
public:
	//! Constructor
	NifSkope();
	//! Destructor
	~NifSkope();
	
	//! Create and initialize a new NifSkope application window.
	/*!
	 * \param fname The name of the file to load in the new NifSkope window.
	 * \return The newly created NifSkope instance.
	 */
	static NifSkope * createWindow( const QString & fname = QString() );

	//! Save NifSkope application settings.
	/*!
	 * \param settings The QSettings object used to store the settings.
	 */
	void save( QSettings & settings ) const;

	//! Restore NifSkope application settings. 
	/*!
	 * \param settings The QSettings object to restore the settings from.
	 */
	void restore( const QSettings & settings );
	//! Get Loaded filename
	/*!
	 * \return QString of loaded filename
	 */
	QString getLoadFileName();
	
public slots:
	//! Set the lineLoad string and load a nif, kf, or kfm file.
	/*!
	 * \param filepath The file to load.
	 */
	void load( const QString & filepath );

	//! Load a nif, kf, or kfm file, taking the file path from the lineLoad widget.
	void load();

	//! Save a nif, kf, or kfm file, taking the file path from the lineSave widget.
	void save();
	
	//! Reparse the nif.xml and kfm.xml files.
	void loadXML();

	//! Reparse the nif.xml and kfm.xml files and reload the current file.
	void reload();
	
	//! A slot that creates a new NifSkope application window.
	void sltWindow();

	//! A slot for starting the XML checker.
	void sltShredder();
	
	//! Display the "About NifSkope" window.
	void about();

	//! Reset "block details"
	void sltResetBlockDetails();
	
protected slots:
	//! Select a NIF index
	void select( const QModelIndex & );
	
	//! Display a context menu at the specified position
	void contextMenu( const QPoint & pos );
	
	//! Set the list mode
	void setListMode();
	
	//! Select the font to use
	void sltSelectFont();
	
	//! Send a Message
	void dispatchMessage( const Message & msg );
	
	//! Override the view font
	void overrideViewFont();
	
	//! Copy file name from load to save
	void copyFileNameLoadSave();
	//! Copy file name from save to load
	void copyFileNameSaveLoad();
	
	//! Sets Import/Export menus
	/*!
	 * see importex/importex.cpp
	 */
	void fillImportExportMenus();
	//! Perform Import or Export
	void sltImportExport( QAction * action );
	
	//! Open a URL using the system handler
	void openURL();
	
	//! Change system locale and notify user that restart may be required
	void sltLocaleChanged();
	
protected:
	void closeEvent( QCloseEvent * e );
	bool eventFilter( QObject * o, QEvent * e );
	
private:
	void initActions();
	void initDockWidgets();
	void initToolBars();
	void initMenu();

    QWidget *aboutDialog;
	
	void setViewFont( const QFont & );

	//! Copy settings from one config to another, without overwriting keys.
	/*!
	 * This is a helper function for migrateSettings().
	 */
	void copySettings(QSettings & cfg, const QSettings & oldcfg, const QString name) const;

	//! Migrate settings from older versions of nifskope.
	void migrateSettings() const;

	//! Stores the nif file in memory.
	NifModel * nif;
	//! A hierarchical proxy for the nif file.
	NifProxyModel * proxy;
	//! Stores the kfm file in memory.
	KfmModel * kfm;
	
	//! This view shows the block list.
	NifTreeView * list;
	//! This view shows the whole nif file or the block details.
	NifTreeView * tree;
	//! This view shows the KFM file, if any
	NifTreeView * kfmtree;

	//! Help browser
	ReferenceBrowser * refrbrwsr;

	//! Transform inspect view
	InspectView * inspect;
	
	//! The main window
	GLView * ogl;
	
	bool selecting;
	bool initialShowEvent;
	
	FileSelector * lineLoad;
	FileSelector * lineSave;
	
	QDockWidget * dList;
	QDockWidget * dTree;
	QDockWidget * dKfm;
	QDockWidget * dRefr;
	QDockWidget * dInsp;
	
	QToolBar * tool;
	
	QAction * aSanitize;
	QAction * aLoadXML;
	QAction * aReload;
	QAction * aWindow;
	QAction * aShredder;
	QAction * aQuit;
	
	QAction * aLineLoad;
	QAction * aLineSave;
	QAction * aCpFileName;
	
#ifdef FSENGINE
	QAction * aResources;
#endif
	
	QActionGroup * gListMode;
	QAction * aList;
	QAction * aHierarchy;
	QAction * aCondition;
	QAction * aRCondition;
	
	QAction * aSelectFont;
	
	QAction * aHelpWebsite;
	QAction * aHelpForum;
	QAction * aNifToolsWebsite;
	QAction * aNifToolsDownloads;

	QAction * aNifSkope;
	QAction * aAboutQt;

	QMenu * mExport;
	QMenu * mImport;
};

//! UDP communication between instances
class IPCsocket : public QObject
{
	Q_OBJECT
public:
	//! Creates a socket
	static IPCsocket * create();
	
	//! Sends a command
	static void sendCommand( const QString & cmd );

public slots:
	//! Acts on a command
	void execCommand( const QString & cmd );
	
	//! Opens a NIF from a URL
	void openNif( const QUrl & );

protected slots:
	void processDatagram();
	
protected:
	IPCsocket( QUdpSocket * );
	~IPCsocket();
	
	QUdpSocket * socket;
};

//! Progress dialog
class ProgDlg : public QProgressDialog
{
	Q_OBJECT
public:
	//! Constructor
	ProgDlg() {}

public slots:
	//! Update progress
	/*!
	 * \param x The amount done
	 * \param y The total amount
	 */
	void sltProgress( int x, int y );
};

#endif
