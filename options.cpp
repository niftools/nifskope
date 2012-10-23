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

#include "options.h"

#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSpinBox>
#include <QStringListModel>
#include <QTimer>
#include <QTabWidget>
#include <QComboBox>
#include <QApplication>

#include "config.h"

#include "widgets/colorwheel.h"
#include "widgets/fileselect.h"
#include "widgets/floatslider.h"
#include "widgets/groupbox.h"

//! \file options.cpp SmallListView and Options implementation

//! Helper class for Options::TexFolderView
class SmallListView : public QListView
{
public:
	QSize sizeHint() const { return minimumSizeHint(); }
};

Options::Options()
{
   NIFSKOPE_QSETTINGS(cfg);
   cfg.beginGroup( "Render Settings" );

   showMeshes = cfg.value("Draw Meshes", true).toBool();

   tSave = new QTimer( this );
   tSave->setInterval( 5000 );
   tSave->setSingleShot( true );
   connect( this, SIGNAL( sigChanged() ), tSave, SLOT( start() ) );
   connect( tSave, SIGNAL( timeout() ), this, SLOT( save() ) );

   tEmit = new QTimer( this );
   tEmit->setInterval( 500 );
   tEmit->setSingleShot( true );
   connect( tEmit, SIGNAL( timeout() ), this, SIGNAL( sigChanged() ) );
   connect( this, SIGNAL( sigChanged() ), tEmit, SLOT( stop() ) );

   dialog = new GroupBox( "", Qt::Vertical );
   dialog->setWindowTitle(  tr("Settings")  );
   dialog->setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
   dialog->installEventFilter( this );

   aDrawAxes = new QAction( tr("Draw &Axes"), this );
   aDrawAxes->setToolTip( tr("draw xyz-Axes") );
   aDrawAxes->setCheckable( true );
   aDrawAxes->setChecked( cfg.value( "Draw AXes", true ).toBool() );
   connect( aDrawAxes, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aDrawNodes = new QAction( tr("Draw &Nodes"), this );
   aDrawNodes->setToolTip( tr("draw bones/nodes") );
   aDrawNodes->setCheckable( true );
   aDrawNodes->setChecked( cfg.value( "Draw Nodes", true ).toBool() );
   connect( aDrawNodes, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aDrawHavok = new QAction( tr("Draw &Havok"), this );
   aDrawHavok->setToolTip( tr("draw the havok shapes") );
   aDrawHavok->setCheckable( true );
   aDrawHavok->setChecked( cfg.value( "Draw Collision Geometry", true ).toBool() );
   connect( aDrawHavok, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aDrawConstraints = new QAction( tr("Draw &Constraints"), this );
   aDrawConstraints->setToolTip( tr("draw the havok constraints") );
   aDrawConstraints->setCheckable( true );
   aDrawConstraints->setChecked( cfg.value( "Draw Constraints", true ).toBool() );
   connect( aDrawConstraints, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aDrawFurn = new QAction( tr("Draw &Furniture"), this );
   aDrawFurn->setToolTip( tr("draw the furniture markers") );
   aDrawFurn->setCheckable( true );
   aDrawFurn->setChecked( cfg.value( "Draw Furniture Markers", true ).toBool() );
   connect( aDrawFurn, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aDrawHidden = new QAction( tr("Show Hid&den"), this );
   aDrawHidden->setToolTip( tr("always draw nodes and meshes") );
   aDrawHidden->setCheckable( true );
   aDrawHidden->setChecked( cfg.value( "Show Hidden Objects", false ).toBool() );
   connect( aDrawHidden, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aDrawStats = new QAction( tr("Show S&tats"), this );
   aDrawStats->setToolTip( tr("display some statistics about the selected node") );
   aDrawStats->setCheckable( true );
   aDrawStats->setChecked( cfg.value( "Show Stats", false ).toBool() );
   connect( aDrawStats, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

   aSettings = new QAction( tr("&Settings..."), this );
   aSettings->setToolTip( tr("show the settings dialog") );
   connect( aSettings, SIGNAL( triggered() ), dialog, SLOT( show() ) );

   cfg.endGroup();

   GroupBox *texPage;
   tab = new QTabWidget;

   if ( GroupBox *genPage = new GroupBox(Qt::Vertical) )
   {
      tab->addTab( genPage, tr("General") );

      genPage->pushLayout( Qt::Vertical );
      cfg.beginGroup( "Settings" );

      // Locale Settings
      genPage->pushLayout( tr("Regional and Language Settings"), Qt::Vertical, 1 );
      genPage->pushLayout( Qt::Horizontal );
      
      genPage->addWidget( new QLabel( "Language:" ) );
      RegionOpt = new QComboBox;
      genPage->addWidget( RegionOpt );

      QLocale localeInvariant("en");
      QString txtLang = QLocale::languageToString(localeInvariant.language());
      if (localeInvariant.country() != QLocale::AnyCountry)
         txtLang.append( " (" ).append(QLocale::countryToString(localeInvariant.country())).append(")");
      RegionOpt->addItem( txtLang, localeInvariant );

      QDir directory( QApplication::applicationDirPath() );
      // local copy
      if (!directory.cd("lang")) {
         // relative from nifskope/release
         if (!directory.cd("../lang")) {
            // linux
            if (!directory.cd("/usr/share/nifskope/lang")) {
               // no language directory found
            }
         }
      }

      QRegExp fileRe("NifSkope_(.*)\\.ts", Qt::CaseInsensitive);
      foreach( QString file, directory.entryList(QStringList("NifSkope_*.ts"), QDir::Files | QDir::NoSymLinks) ) {
         if ( fileRe.exactMatch(file) ) {
            QString localeText = fileRe.capturedTexts()[1];
            QLocale fileLocale(localeText);
            if ( RegionOpt->findData(fileLocale) < 0 ) {
               QString txtLang = QLocale::languageToString(fileLocale.language());
               if (fileLocale.country() != QLocale::AnyCountry)
                  txtLang.append( " (" ).append(QLocale::countryToString(fileLocale.country())).append(")");
               RegionOpt->addItem( txtLang, fileLocale );
            }
         }
      }
      QLocale locale = cfg.value( "Language", "en" ).toLocale();
      int localeIdx = RegionOpt->findData(locale);
      RegionOpt->setCurrentIndex( localeIdx > 0 ? localeIdx : 0 );

      connect( RegionOpt, SIGNAL( currentIndexChanged( int ) ), this, SIGNAL( sigChanged() ) );
      connect( RegionOpt, SIGNAL( currentIndexChanged( int ) ), this, SIGNAL( sigLocaleChanged() ) );

      genPage->popLayout();
      genPage->popLayout();

      //Misc Options
      genPage->pushLayout( tr("Misc. Settings"), Qt::Vertical, 1 );
      genPage->pushLayout( Qt::Horizontal );

      genPage->addWidget( new QLabel( tr("Startup Version") ) );
      genPage->addWidget( StartVer = new QLineEdit( cfg.value( "Startup Version", "20.0.0.5" ).toString() ) );
      StartVer->setToolTip( tr("This is the version that the initial 'blank' NIF file that is created when NifSkope opens will be.") );
      connect( StartVer, SIGNAL( textChanged( const QString & ) ), this, SIGNAL( sigChanged() ) );

      genPage->popLayout();

		/* if we want to make max string length more accessible
		genPage->pushLayout( Qt::Horizontal );
		genPage->addWidget( new QLabel( tr("Maximum String Length") ) );
		genPage->addWidget( StringLength = new QSpinBox );
		StringLength->setRange( 0, INT_MAX );
		StringLength->setValue( cfg.value( "Maximum String Length", 0x8000).toInt() );
		connect( StringLength, SIGNAL( valueChanged( int ) ), this, SIGNAL( sigChanged() ) );
		*/


      //More Misc options can be added here.
      genPage->popLayout();

      cfg.endGroup();
      genPage->popLayout();
   }


   tab->addTab( texPage = new GroupBox(Qt::Vertical), tr("&Rendering"));
   {
      cfg.beginGroup( "Render Settings" );

      texPage->pushLayout( tr("Texture Folders"), Qt::Vertical );
      texPage->pushLayout( Qt::Horizontal );

#ifdef Q_OS_WIN32
      texPage->pushLayout( tr("Auto Detect"), Qt::Vertical );
      QButtonGroup * tfgamegrp = new QButtonGroup( this );
      connect( tfgamegrp, SIGNAL( buttonClicked( int ) ), this, SLOT( textureFolderAutoDetect() ) );
      QPushButton * bt = new QPushButton( tr("Auto Detect\nGame Paths") );
      tfgamegrp->addButton( bt);
      texPage->addWidget( bt );

      texPage->popLayout();
#endif

      texPage->pushLayout( tr("Custom"), Qt::Vertical );
      texPage->pushLayout( Qt::Horizontal );

      QButtonGroup * tfactgrp = new QButtonGroup( this );
      connect( tfactgrp, SIGNAL( buttonClicked( int ) ), this, SLOT( textureFolderAction( int ) ) );
      int tfaid = 0;
      foreach ( QString tfaname, QStringList() << tr("Add Folder") << tr("Remove Folder") << tr("Move Up") << tr("Move Down") )
      {
         QPushButton * bt = new QPushButton( tfaname );
         TexFolderButtons[tfaid] = bt;
         tfactgrp->addButton( bt, tfaid++ );
         texPage->addWidget( bt );
      }

      texPage->popLayout();

      TexFolderModel = new QStringListModel( this );
      TexFolderModel->setStringList( cfg.value( "Texture Folders" ).toStringList() );
      connect( TexFolderModel, SIGNAL( rowsInserted( const QModelIndex &, int, int ) ), tEmit, SLOT( start() ) );
      connect( TexFolderModel, SIGNAL( rowsRemoved( const QModelIndex &, int, int ) ), tEmit, SLOT( start() ) );
      connect( TexFolderModel, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), tEmit, SLOT( start() ) );
      connect( TexFolderModel, SIGNAL( modelReset() ), tEmit, SLOT( start() ) );

      TexFolderView = new SmallListView;
      texPage->addWidget( TexFolderView, 0 );
      TexFolderView->setMinimumHeight( 105 );
      TexFolderView->setModel( TexFolderModel );
      TexFolderView->setCurrentIndex( TexFolderModel->index( 0, 0, QModelIndex() ) );

      texPage->pushLayout( Qt::Horizontal );

      TexFolderSelect = new FileSelector( FileSelector::Folder, "Folder", QBoxLayout::RightToLeft );
      texPage->addWidget( TexFolderSelect );

      QDataWidgetMapper * TexFolderMapper = new QDataWidgetMapper( this );
      TexFolderMapper->setModel( TexFolderModel );
      TexFolderMapper->addMapping( TexFolderSelect, 0 );
      TexFolderMapper->setCurrentModelIndex( TexFolderView->currentIndex() );
      connect( TexFolderView->selectionModel(), SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
         TexFolderMapper, SLOT( setCurrentModelIndex( const QModelIndex & ) ) );
      connect( TexFolderSelect, SIGNAL( sigActivated( const QString & ) ), TexFolderMapper, SLOT( submit() ) );

      connect( TexFolderView->selectionModel(), SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
         this, SLOT( textureFolderIndex( const QModelIndex & ) ) );
      textureFolderIndex( TexFolderView->currentIndex() );

      texPage->addWidget( TexAlternatives = new QCheckBox( tr("&Look for alternatives") ) );
      TexAlternatives->setToolTip( tr("If a texture was nowhere to be found<br>NifSkope will start looking for alternatives.<p style='white-space:pre'>texture.dds does not exist -> use texture.bmp instead</p>") );
      TexAlternatives->setChecked( cfg.value( "Texture Alternatives", true ).toBool() );
      connect( TexAlternatives, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

      texPage->popLayout();
      texPage->popLayout();
      texPage->popLayout();
      texPage->popLayout();
      texPage->pushLayout( Qt::Horizontal );
      texPage->pushLayout( tr("Render"), Qt::Vertical );


      texPage->addWidget( AntiAlias = new QCheckBox( tr("&Anti Aliasing") ) );
      AntiAlias->setToolTip( tr("Enable anti aliasing and anisotropic texture filtering if available.<br>You'll need to restart NifSkope for this setting to take effect.<br>") );
      AntiAlias->setChecked( cfg.value( "Anti Aliasing", true ).toBool() );
      connect( AntiAlias, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

      texPage->addWidget( Textures = new QCheckBox( tr("&Textures") ) );
      Textures->setToolTip( tr("Enable textures") );
      Textures->setChecked( cfg.value( "Texturing", true ).toBool() );
      connect( Textures, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

      texPage->addWidget( Shaders = new QCheckBox( tr("&Shaders") ) );
      Shaders->setToolTip( tr("Enable Shaders") );
      Shaders->setChecked( cfg.value( "Enable Shaders", false ).toBool() );
      connect( Shaders, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );


      texPage->popLayout();
      texPage->pushLayout( tr("Up Axis"), Qt::Vertical );


      texPage->addWidget( AxisX = new QRadioButton( tr("X") ) );
      texPage->addWidget( AxisY = new QRadioButton( tr("Y") ) );
      texPage->addWidget( AxisZ = new QRadioButton( tr("Z") ) );

      QButtonGroup * btgrp = new QButtonGroup( this );
      btgrp->addButton( AxisX );
      btgrp->addButton( AxisY );
      btgrp->addButton( AxisZ );
      btgrp->setExclusive( true );

      QString upax = cfg.value( "Up Axis", "Z" ).toString();
      if ( upax == "X" )
         AxisX->setChecked( true );
      else if ( upax == "Y" )
         AxisY->setChecked( true );
      else
         AxisZ->setChecked( true );

      connect( AxisX, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
      connect( AxisY, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
      connect( AxisZ, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );


      texPage->popLayout();
      texPage->pushLayout( tr("Culling"), Qt::Vertical, 1 );

      texPage->addWidget( CullNoTex = new QCheckBox( tr("Cull &Non Textured") ) );
      CullNoTex->setToolTip( tr("Hide all meshes without textures") );
      CullNoTex->setChecked( cfg.value( "Cull Non Textured", false ).toBool() );
      connect( CullNoTex, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

      texPage->addWidget( CullByID = new QCheckBox( tr("&Cull Nodes by Name") ) );
      CullByID->setToolTip( tr("Enabling this option hides some special nodes and meshes") );
      CullByID->setChecked( cfg.value( "Cull Nodes By Name", false ).toBool() );
      connect( CullByID, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

      texPage->addWidget( CullExpr = new QLineEdit( cfg.value( "Cull Expression", "^collidee|^shadowcaster|^\\!LoD_cullme|^footprint" ).toString() ) );
      CullExpr->setToolTip( tr("Enter a regular expression. Nodes which names match the expression will be hidden") );
// 	  leave RegExp input open as both rendering and collada cull sharing this
//    CullExpr->setEnabled( CullByID->isChecked() );
      CullExpr->setEnabled( true );
      connect( CullExpr, SIGNAL( textChanged( const QString & ) ), this, SIGNAL( sigChanged() ) );
//    connect( CullByID, SIGNAL( toggled( bool ) ), CullExpr, SLOT( setEnabled( bool ) ) );

      texPage->popLayout();
      texPage->popLayout();

      cfg.endGroup();
   }

   GroupBox *colorPage;
   tab->addTab( colorPage = new GroupBox(Qt::Vertical), tr("Colors"));
   {
      cfg.beginGroup( "Render Settings" );

      colorPage->pushLayout( tr("Light"), Qt::Vertical );
      colorPage->pushLayout( Qt::Horizontal );


      cfg.beginGroup( "Light0" );

      QStringList lightNames( QStringList() << tr("Ambient") << tr("Diffuse") << tr("Specular") );
      QList<QColor> lightDefaults( QList<QColor>() << QColor::fromRgbF( .4, .4, .4 ) << QColor::fromRgbF( .8, .8, .8 ) << QColor::fromRgbF( 1, 1, 1 ) );
      for ( int l = 0; l < 3; l++ )
      {
			ColorWheel * wheel = new ColorWheel( cfg.value( lightNames[l], lightDefaults[l] ).value<QColor>() );
			wheel->setSizeHint( QSize( 105, 105 ) );
			wheel->setAlpha( false );
			connect( wheel, SIGNAL( sigColorEdited( const QColor & ) ), this, SIGNAL( sigChanged() ) );
			LightColor[l] = wheel;
			
			colorPage->pushLayout( lightNames[l], Qt::Vertical );
			colorPage->addWidget( wheel );
			colorPage->popLayout();
      }

      colorPage->popLayout();
      colorPage->pushLayout( Qt::Horizontal );

      colorPage->addWidget( LightFrontal = new QCheckBox( tr("Frontal") ), 0 );
      LightFrontal->setToolTip( tr("Lock light to camera position") );
      LightFrontal->setChecked( cfg.value( "Frontal", true ).toBool() );
      connect( LightFrontal, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

      QWidget * pos = colorPage->pushLayout( tr("Position"), Qt::Horizontal, 1 );
      pos->setDisabled( LightFrontal->isChecked() );
      connect( LightFrontal, SIGNAL( toggled( bool ) ), pos, SLOT( setDisabled( bool ) ) );

      colorPage->addWidget( new QLabel( tr("Declination") ) );
      colorPage->addWidget( LightDeclination = new QSpinBox, 1 );
      LightDeclination->setMinimum( -180 );
      LightDeclination->setMaximum( +180 );
      LightDeclination->setSingleStep( 5 );
      LightDeclination->setWrapping( true );
      LightDeclination->setValue( cfg.value( "Declination", 0 ).toInt() );
      connect( LightDeclination, SIGNAL( valueChanged( int ) ), this, SIGNAL( sigChanged() ) );

      colorPage->addWidget( new QLabel( tr("Planar Angle") ) );
      colorPage->addWidget( LightPlanarAngle = new QSpinBox, 1 );
      LightPlanarAngle->setMinimum( -180 );
      LightPlanarAngle->setMaximum( +180 );
      LightPlanarAngle->setSingleStep( 5 );
      LightPlanarAngle->setWrapping( true );
      LightPlanarAngle->setValue( cfg.value( "Planar Angle", 0 ).toInt() );
      connect( LightPlanarAngle, SIGNAL( valueChanged( int ) ), this, SIGNAL( sigChanged() ) );

      cfg.endGroup();

      colorPage->popLayout();
      colorPage->pushLayout( tr("Presets"), Qt::Horizontal );

      QButtonGroup * grp = new QButtonGroup( this );
      connect( grp, SIGNAL( buttonClicked( int ) ), this, SLOT( activateLightPreset( int ) ) );
      int psid = 0;
      foreach ( QString psname, QStringList() << tr("Sunny Day") << tr("Dark Night") )
      {
         QPushButton * bt = new QPushButton( psname );
         grp->addButton( bt, psid++ );
         colorPage->addWidget( bt );
      }

      colorPage->popLayout();
      colorPage->popLayout();
      colorPage->popLayout();
      colorPage->pushLayout( tr("Colors"), Qt::Horizontal );


      QStringList colorNames( QStringList() << tr("Background") << tr("Foreground") << tr("Highlight") );
      QList<QColor> colorDefaults( QList<QColor>() << QColor::fromRgb( 0, 0, 0 ) << QColor::fromRgb( 255, 255, 255 ) << QColor::fromRgb( 255, 255, 0 ) );
      for ( int c = 0; c < 3; c++ )
      {
         colorPage->pushLayout( colorNames[c], Qt::Horizontal );

         ColorWheel * wheel = new ColorWheel( cfg.value( colorNames[c], colorDefaults[c] ).value<QColor>() );
         wheel->setSizeHint( QSize( 105, 105 ) );
         connect( wheel, SIGNAL( sigColorEdited( const QColor & ) ), this, SIGNAL( sigChanged() ) );
         colors[ c ] = wheel;
         colorPage->addWidget( wheel );

         if ( c != 0 )
         {
			alpha[ c ] = new AlphaSlider( Qt::Vertical );
			alpha[ c ]->setValue( cfg.value( colorNames[c], colorDefaults[c] ).value<QColor>().alphaF() );
			alpha[ c ]->setColor( wheel->getColor() );
			connect( alpha[ c ], SIGNAL( valueChanged( float ) ), this, SIGNAL( sigChanged() ) );
			connect( wheel, SIGNAL( sigColor( const QColor & ) ), alpha[ c ], SLOT( setColor( const QColor & ) ) );
			connect( alpha[ c ], SIGNAL( valueChanged( float ) ), wheel, SLOT( setAlphaValue( float ) ) );
			colorPage->addWidget( alpha[ c ] );
         }
		else
		{
			alpha[ c ] = 0;
			wheel->setAlpha( false );
		}

         colorPage->popLayout();
      }
      colorPage->popLayout();

      cfg.endGroup();
   }
   GroupBox *matPage;
   tab->addTab( matPage = new GroupBox(Qt::Vertical), tr("Materials"));
   {
      cfg.beginGroup( "Render Settings" );

      QWidget* overrideBox = matPage->pushLayout( tr("Material Overrides"), Qt::Vertical );
      overrideBox->setMaximumHeight(200);

      matPage->pushLayout( Qt::Horizontal );

      cfg.beginGroup( "MatOver" );

      QStringList names( QStringList() << tr("Ambient") << tr("Diffuse") << tr("Specular") << tr("Emissive") );
      QList<QColor> defaults( QList<QColor>() << QColor::fromRgbF( 1, 1, 1 ) << QColor::fromRgbF( 1, 1, 1 ) << QColor::fromRgbF( 1, 1, 1 ) << QColor::fromRgbF( 1, 1, 1 ) );
      for ( int l = 0; l < 4; l++ )
      {
         ColorWheel * wheel = new ColorWheel( cfg.value( names[l], defaults[l] ).value<QColor>() );
         wheel->setSizeHint( QSize( 105, 105 ) );
			wheel->setAlpha( false );
         connect( wheel, SIGNAL( sigColorEdited( const QColor & ) ), this, SIGNAL( materialOverridesChanged() ) );
         matColors[l] = wheel;

         QWidget* box = matPage->pushLayout( names[l], Qt::Vertical );
         box->setMaximumSize(150, 150);
         matPage->addWidget( wheel );
         matPage->popLayout();
      }

      matPage->popLayout();
      matPage->pushLayout( Qt::Horizontal );

      matPage->addWidget( overrideMatCheck = new QCheckBox( tr("Enable Material Color Overrides") ), 0 );
      overrideMatCheck->setToolTip( tr("Override colors used on Materials") );
      //overrideMaterials->setChecked( cfg.value( "Override", true ).toBool() );
      connect( overrideMatCheck, SIGNAL( toggled( bool ) ), this, SIGNAL( materialOverridesChanged() ) );

      cfg.endGroup();

      matPage->popLayout();
      matPage->popLayout();

      cfg.endGroup();
   }

   GroupBox *exportPage;
   tab->addTab( exportPage = new GroupBox(Qt::Vertical), tr("Export"));
   {
	   cfg.beginGroup( "Export Settings" );
	   exportPage->pushLayout( tr("Export Settings"), Qt::Vertical, 1 );
	   exportPage->addWidget( exportCull = new QCheckBox( tr("Use 'Cull Nodes by Name' rendering option to cull nodes on export") ),1,Qt::AlignTop);
	   exportCull->setChecked( cfg.value("export_culling",false).toBool() );
	   connect( exportCull, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	   exportPage->popLayout();
	   cfg.endGroup();
   }

   // set render page as default
   tab->setCurrentWidget( texPage );

   dialog->pushLayout( Qt::Vertical );

   dialog->addWidget( tab );

   dialog->popLayout();
}

Options::~Options()
{
	if ( tSave->isActive() )
		save();
}

bool Options::eventFilter( QObject * o, QEvent * e )
{
   if ( o == dialog && e->type() == QEvent::Close && tSave->isActive() )
   {
      save();
   }
   return false;
}

Options * Options::get()
{
	static Options * options = new Options();
	return options;
}

QList<QAction*> Options::actions()
{
	Options * opts = get();
	return QList<QAction*>()
		<< opts->aDrawAxes
		<< opts->aDrawNodes
		<< opts->aDrawHavok
		<< opts->aDrawConstraints
		<< opts->aDrawFurn
		<< opts->aDrawHidden
#ifdef USE_GL_QPAINTER
		<< opts->aDrawStats
#endif
		<< opts->aSettings;
}

void Options::save()
{
	tSave->stop();
	
	NIFSKOPE_QSETTINGS(cfg);

	// remove obsolete keys
	foreach ( QString key, QStringList()
		<< "texture folder"	<< "bg color" << "cull nodes by name" << "draw axis" << "draw furniture"
		<< "draw havok" << "draw hidden" << "draw nodes" << "draw only textured meshes" << "draw stats"
		<< "enable blending" << "enable shading" << "enable textures" << "highlight meshes" << "hl color"
		<< "rotate" << "Render Settings/Misc Settings/Startup Version" )
	{
		if ( cfg.contains( key ) )
			cfg.remove( key );
	}

	cfg.beginGroup( "Render Settings" );
	
	cfg.setValue( "Texture Folders", textureFolders() );
	cfg.setValue( "Texture Alternatives", textureAlternatives() );
	
	cfg.setValue( "Draw Axes", drawAxes() );
	cfg.setValue( "Draw Nodes", drawNodes() );
	cfg.setValue( "Draw Collision Geometry", drawHavok() );
	cfg.setValue( "Draw Constraints", drawConstraints() );
	cfg.setValue( "Draw Furniture Markers", drawFurn() );
	cfg.setValue( "Show Hidden Objects", drawHidden() );
	cfg.setValue( "Show Stats", drawStats() );

	cfg.setValue( "Background", bgColor() );
	cfg.setValue( "Foreground", nlColor() );
	cfg.setValue( "Highlight", hlColor() );
	
	cfg.setValue( "Anti Aliasing", antialias() );
	cfg.setValue( "Texturing", texturing() );
	cfg.setValue( "Enable Shaders", shaders() );
	
	cfg.setValue( "Cull Nodes By Name", CullByID->isChecked() );
	cfg.setValue( "Cull Expression", CullExpr->text() );
	cfg.setValue( "Cull Non Textured", onlyTextured() );
	
	cfg.setValue( "Up Axis", AxisX->isChecked() ? "X" : AxisY->isChecked() ? "Y" : "Z" );
	
	cfg.beginGroup( "Light0" );
	cfg.setValue( "Ambient", ambient() );
	cfg.setValue( "Diffuse", diffuse() );
	cfg.setValue( "Specular", specular() );
	cfg.setValue( "Frontal", lightFrontal() );
	cfg.setValue( "Declination", lightDeclination() );
	cfg.setValue( "Planar Angle", lightPlanarAngle() );
	cfg.endGroup();

	cfg.beginGroup( "MatOver" );
	cfg.setValue( "Ambient", overrideAmbient() );
	cfg.setValue( "Diffuse", overrideDiffuse() );
	cfg.setValue( "Specular", overrideSpecular() );
	cfg.setValue( "Emmissive", overrideEmissive() );
	cfg.endGroup();

	cfg.endGroup(); // Render Settings

	cfg.beginGroup( "Settings" );

	cfg.setValue( "Language", translationLocale() );
	cfg.setValue( "Startup Version", startupVersion() );
	// If we want to make this more accessible
	//cfg.setValue( "Maximum String Length", maxStringLength() );

	cfg.endGroup(); // Settings

	cfg.beginGroup( "Export Settings" );
	cfg.setValue( "export_culling", exportCullEnabled() );
	cfg.endGroup(); // Export Settings
}

void Options::textureFolderAutoDetect()
{
	//List to hold all games paths that were detected
	QStringList list;

	//Generic "same directory" path should always be added
	list.append( "./" );

	//String to hold the message box message
	QString game_list;

#ifdef Q_OS_WIN32

	// Oblivion
	{
		QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Oblivion", QSettings::NativeFormat );
		QDir dir( reg.value( "Installed Path" ).toString() );
		if ( dir.exists() && dir.cd( "Data" ) )
		{
			game_list.append( "TES4: Oblivion\n" );

			list.append( dir.path() );
			
			dir.setNameFilters( QStringList() << "*.bsa" );
			dir.setFilter( QDir::Dirs );
			foreach ( QString dn, dir.entryList() )
				list << dir.filePath( dn );
				
#ifndef FSENGINE
			if ( ! dir.cd( "Textures" ) )
			{
				QMessageBox::information( dialog, "NifSkope",
					tr("<p>The texture folder was not found.</p>"
					"<p>This may be because you haven't extracted the archive files yet.<br>"
					"<a href='http://cs.elderscrolls.com/constwiki/index.php/BSA_Unpacker_Tutorial'>Here</a>, it is explained how to do that.</p>") );
			}
#endif
		}
	}

	// Morrowind
	{
		QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Morrowind", QSettings::NativeFormat );
		QDir dir( reg.value( "Installed Path" ).toString() );
		if ( dir.exists() && dir.cd( "Data Files" ) )
		{
			game_list.append( "TES3: Morrowind\n" );

			list.append( dir.path() );
			list.append( dir.path() + "/Textures" );
			
			dir.setNameFilters( QStringList() << "*.bsa" );
			dir.setFilter( QDir::Dirs );
			foreach ( QString dn, dir.entryList() )
				list << dir.filePath( dn ) << dir.filePath( dn ) + "/Textures";
		}
	}

	// CIV IV
	{
		QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Firaxis Games\\Sid Meier's Civilization 4", QSettings::NativeFormat );
		QDir dir( reg.value( "INSTALLDIR" ).toString() );
		if ( dir.exists() && dir.cd( "Assets/Art/shared" ) )
		{
			game_list.append( "Sid Meier's Civilization IV\n" );

			list.append( dir.path() );
		}
	}

	// Freedom Force
	list.append( "./textures" );
	list.append( "./skins/standard" );
	{
		QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\FFVTTR", QSettings::NativeFormat );
		QDir dir( reg.value( "InstallDir" ).toString() );
		if ( dir.exists() && dir.cd( "/Data/Art/library/area_specific/_textures" ) )
		{
			game_list.append( "Freedom Force vs. the Third Reich\n" );
			list.append( dir.path() );
		}
	}
	{
		QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\Freedom Force", QSettings::NativeFormat );
		QDir dir( reg.value( "InstallDir" ).toString() );
		if ( dir.exists() && dir.cd( "Data/Art/library/area_specific/_textures" ) )
		{
			game_list.append( "Freedom Force\n" );

			list.append( dir.path() );
		}
	}
	{
		QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\Freedom Force Demo", QSettings::NativeFormat );
		QDir dir( reg.value( "InstallDir" ).toString() );
		if ( dir.exists() && dir.cd( "Data/Art/library/area_specific/_textures" ) ) {
			game_list.append( "Freedom Force Demo\n" );

			list.append( dir.path() );
		}
	}


   // Fallout 3
   {
      QSettings reg( "HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Fallout3", QSettings::NativeFormat );
      QDir dir( reg.value( "Installed Path" ).toString() );
      if ( dir.exists() && dir.cd( "Data" ) )
      {
         game_list.append( "Fallout 3\n" );

         list.append( dir.path() );

         dir.setNameFilters( QStringList() << "*.bsa" );
         dir.setFilter( QDir::Dirs );
         foreach ( QString dn, dir.entryList() )
            list << dir.filePath( dn );

#ifndef FSENGINE
         if ( ! dir.cd( "Textures" ) )
         {
            QMessageBox::information( dialog, "NifSkope",
               tr("<p>The texture folder was not found.</p>"
               "<p>This may be because you haven't extracted the archive files yet.<br>"
               "<a href='http://cs.elderscrolls.com/constwiki/index.php/BSA_Unpacker_Tutorial'>Here</a>, it is explained how to do that.</p>") );
         }
#endif
      }
   }

#endif
	//Set folder list box to contain the newly detected textures, along with the ones the user has already defined, ignoring any duplicates

	//Remove duplicates
	QStringList finalList = TexFolderModel->stringList();
	for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
		if ( finalList.contains( *it ) == false )
			finalList << *it;
	}

	TexFolderModel->setStringList( finalList );
	TexAlternatives->setChecked( false );
	TexFolderView->setCurrentIndex( TexFolderModel->index( 0, 0, QModelIndex() ) );

	//Announce result to user
	if ( game_list.size() == 0 )
	{
		game_list = tr("No supported games were detected.\nYour game may still work, you will just have to set the folders manually until an auto-detect routine is created.");
	}
	else
	{
		game_list = tr("Successfully detected the following games:\n") + game_list;
	}
	QMessageBox::information( dialog, "NifSkope", game_list );


}

void Options::textureFolderAction( int id )
{
	QModelIndex idx = TexFolderView->currentIndex();
	switch ( id )
	{
		case 0:
			// add folder
			TexFolderModel->insertRow( 0, QModelIndex() );
			TexFolderModel->setData( TexFolderModel->index( 0, 0, QModelIndex() ), tr("Choose a folder"), Qt::EditRole );
			TexFolderView->setCurrentIndex( TexFolderModel->index( 0, 0, QModelIndex() ) );
			break;
		case 1:
			if ( idx.isValid() )
			{	// remove folder
				TexFolderModel->removeRow( idx.row(), QModelIndex() );
			}
			break;
		case 2:
			if ( idx.isValid() && idx.row() > 0 )
			{	// move up
				QModelIndex xdi = idx.sibling( idx.row() - 1, 0 );
				QVariant v = TexFolderModel->data( idx, Qt::EditRole );
				TexFolderModel->setData( idx, TexFolderModel->data( xdi, Qt::EditRole ), Qt::EditRole );
				TexFolderModel->setData( xdi, v, Qt::EditRole );
				TexFolderView->setCurrentIndex( xdi );
			}
			break;
		case 3:
			if ( idx.isValid() && idx.row() < TexFolderModel->rowCount() - 1 )
			{	// move down
				QModelIndex xdi = idx.sibling( idx.row() + 1, 0 );
				QVariant v = TexFolderModel->data( idx, Qt::EditRole );
				TexFolderModel->setData( idx, TexFolderModel->data( xdi, Qt::EditRole ), Qt::EditRole );
				TexFolderModel->setData( xdi, v, Qt::EditRole );
				TexFolderView->setCurrentIndex( xdi );
			}
			break;
	}
}

void Options::textureFolderIndex( const QModelIndex & idx )
{
	TexFolderSelect->setEnabled( idx.isValid() );
	TexFolderButtons[0]->setEnabled( true );
	TexFolderButtons[1]->setEnabled( idx.isValid() );
	TexFolderButtons[2]->setEnabled( idx.isValid() && ( idx.row() > 0 ) );
	TexFolderButtons[3]->setEnabled( idx.isValid() && ( idx.row() < TexFolderModel->rowCount() - 1 ) );
}

void Options::activateLightPreset( int id )
{
	switch ( id )
	{
		case 0: // sunny day
			LightColor[0]->setColor( QColor::fromRgbF( 0.4, 0.4, 0.4 ) );
			LightColor[1]->setColor( QColor::fromRgbF( 0.8, 0.8, 0.8 ) );
			LightColor[2]->setColor( QColor::fromRgbF( 1.0, 1.0, 1.0 ) );
			break;
		case 1: // dark night
			LightColor[0]->setColor( QColor::fromRgbF( 0.2, 0.2, 0.25 ) );
			LightColor[1]->setColor( QColor::fromRgbF( 0.1, 0.1, 0.1 ) );
			LightColor[2]->setColor( QColor::fromRgbF( 0.1, 0.1, 0.1 ) );
			break;
		default:
			return;
	}
	emit sigChanged();
}

QStringList Options::textureFolders()
{
	return get()->TexFolderModel->stringList();
}

bool Options::textureAlternatives()
{
	return get()->TexAlternatives->isChecked();
}

Options::Axis Options::upAxis()
{
	return get()->AxisX->isChecked() ? XAxis : get()->AxisY->isChecked() ? YAxis : ZAxis;
}

bool Options::antialias()
{
	return get()->AntiAlias->isChecked();
}

bool Options::texturing()
{
	return get()->Textures->isChecked();
}

bool Options::shaders()
{
	return get()->Shaders->isChecked();
}


bool Options::drawAxes()
{
	return get()->aDrawAxes->isChecked();
}

bool Options::drawNodes()
{
	return get()->aDrawNodes->isChecked();
}

bool Options::drawHavok()
{
	return get()->aDrawHavok->isChecked();
}

bool Options::drawConstraints()
{
	return get()->aDrawConstraints->isChecked();
}

bool Options::drawFurn()
{
	return get()->aDrawFurn->isChecked();
}

bool Options::drawHidden()
{
	return get()->aDrawHidden->isChecked();
}

bool Options::drawStats()
{
	return get()->aDrawStats->isChecked();
}

bool Options::benchmark()
{
	return false;
}


QColor Options::bgColor()
{
	return get()->colors[ 0 ]->getColor();
}

QColor Options::nlColor()
{
	QColor c = get()->colors[ 1 ]->getColor();
	c.setAlphaF( get()->alpha[ 1 ]->value() );
	return c;
}

QColor Options::hlColor()
{
	QColor c = get()->colors[ 2 ]->getColor();
	c.setAlphaF( get()->alpha[ 2 ]->value() );
	return c;
}


QRegExp Options::cullExpression()
{
	return get()->CullByID->isChecked() ? QRegExp( get()->CullExpr->text() ) : QRegExp();
}

bool Options::onlyTextured()
{
	return get()->CullNoTex->isChecked();
}


QColor Options::ambient()
{
	return get()->LightColor[ 0 ]->getColor();
}

QColor Options::diffuse()
{
	return get()->LightColor[ 1 ]->getColor();
}

QColor Options::specular()
{
	return get()->LightColor[ 2 ]->getColor();
}

bool Options::lightFrontal()
{
	return get()->LightFrontal->isChecked();
}

int Options::lightDeclination()
{
	return get()->LightDeclination->value();
}

int Options::lightPlanarAngle()
{
	return get()->LightPlanarAngle->value();
}

QString Options::startupVersion()
{
	return get()->StartVer->text();	
};

bool Options::overrideMaterials()
{
   return get()->overrideMatCheck->isChecked();
}

QColor Options::overrideAmbient()
{
   return get()->matColors[0]->getColor();	
}

QColor Options::overrideDiffuse()
{
   return get()->matColors[1]->getColor();	
}

QColor Options::overrideSpecular()
{
   return get()->matColors[2]->getColor();	
}

QColor Options::overrideEmissive()
{
   return get()->matColors[3]->getColor();
}

/*!
 * This option is hidden in the registry and disabled by setting the key
 * Render Settings/Draw Meshes to false
 */
bool Options::drawMeshes()
{
	return get()->showMeshes;
}

QLocale Options::translationLocale()
{
   int idx = get()->RegionOpt->currentIndex();
   if (idx >= 0) {
      return get()->RegionOpt->itemData( idx ).toLocale();
   }
   return QLocale::system();
}

/* if we want to make this more accessible
int Options::maxStringLength()
{
	return get()->StringLength->value();
}
*/

bool Options::exportCullEnabled() {
	return get()->exportCull->isChecked();
}
