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
#include "config.h"
#include "version.h"

#include "widgets/colorwheel.h"
#include "widgets/fileselect.h"
#include "widgets/floatslider.h"
#include "widgets/groupbox.h"

#include <QListView> // Inherited
#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QDataWidgetMapper>
#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>
#include <QStringListModel>
#include <QTimer>
#include <QTabWidget>
#include <QComboBox>
#include <QApplication>


//! \file options.cpp SmallListView and Options implementation

//! Helper class for Options::TexFolderView
class SmallListView final : public QListView
{
public:
	QSize sizeHint() const override final { return minimumSizeHint(); }
};

Options::Options()
{
	version = new NifSkopeVersion( NIFSKOPE_VERSION );

	QSettings cfg;
	cfg.beginGroup( "Render Settings" );

	showMeshes = cfg.value( "Draw Meshes", true ).toBool();

	// Cast QTimer slot
	auto tStart = static_cast<void (QTimer::*)()>(&QTimer::start);
	// Cast QButtonGroup signal
	auto bgClicked = static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked);

	tSave = new QTimer( this );
	tSave->setInterval( 5000 );
	tSave->setSingleShot( true );
	connect( this, &Options::sigChanged, tSave, tStart );
	connect( tSave, &QTimer::timeout, this, &Options::save );

	tEmit = new QTimer( this );
	tEmit->setInterval( 500 );
	tEmit->setSingleShot( true );
	connect( tEmit, &QTimer::timeout, this, &Options::sigChanged );
	connect( this, &Options::sigChanged, tEmit, &QTimer::stop );

	dialog = new GroupBox( "", Qt::Vertical );
	dialog->setWindowTitle( tr( "Settings" ) );
	dialog->setWindowFlags( Qt::Tool | Qt::WindowStaysOnTopHint );
	dialog->installEventFilter( this );

	aDrawAxes = new QAction( tr( "Draw &Axes" ), this );
	aDrawAxes->setToolTip( tr( "draw xyz-Axes" ) );
	aDrawAxes->setCheckable( true );
	aDrawAxes->setChecked( cfg.value( "Draw Axes", true ).toBool() );
	connect( aDrawAxes, &QAction::toggled, this, &Options::sigChanged );

	aDrawNodes = new QAction( tr( "Draw &Nodes" ), this );
	aDrawNodes->setToolTip( tr( "draw bones/nodes" ) );
	aDrawNodes->setCheckable( true );
	aDrawNodes->setChecked( cfg.value( "Draw Nodes", true ).toBool() );
	connect( aDrawNodes, &QAction::toggled, this, &Options::sigChanged );

	aDrawHavok = new QAction( tr( "Draw &Havok" ), this );
	aDrawHavok->setToolTip( tr( "draw the havok shapes" ) );
	aDrawHavok->setCheckable( true );
	aDrawHavok->setChecked( cfg.value( "Draw Collision Geometry", true ).toBool() );
	connect( aDrawHavok, &QAction::toggled, this, &Options::sigChanged );

	aDrawConstraints = new QAction( tr( "Draw &Constraints" ), this );
	aDrawConstraints->setToolTip( tr( "draw the havok constraints" ) );
	aDrawConstraints->setCheckable( true );
	aDrawConstraints->setChecked( cfg.value( "Draw Constraints", true ).toBool() );
	connect( aDrawConstraints, &QAction::toggled, this, &Options::sigChanged );

	aDrawFurn = new QAction( tr( "Draw &Furniture" ), this );
	aDrawFurn->setToolTip( tr( "draw the furniture markers" ) );
	aDrawFurn->setCheckable( true );
	aDrawFurn->setChecked( cfg.value( "Draw Furniture Markers", true ).toBool() );
	connect( aDrawFurn, &QAction::toggled, this, &Options::sigChanged );

	aDrawHidden = new QAction( tr( "Show Hid&den" ), this );
	aDrawHidden->setToolTip( tr( "always draw nodes and meshes" ) );
	aDrawHidden->setCheckable( true );
	aDrawHidden->setChecked( cfg.value( "Show Hidden Objects", false ).toBool() );
	connect( aDrawHidden, &QAction::toggled, this, &Options::sigChanged );

	aDrawStats = new QAction( tr( "Show S&tats" ), this );
	aDrawStats->setToolTip( tr( "display some statistics about the selected node" ) );
	aDrawStats->setCheckable( true );
	aDrawStats->setChecked( cfg.value( "Show Stats", false ).toBool() );
	connect( aDrawStats, &QAction::toggled, this, &Options::sigChanged );

	aSettings = new QAction( tr( "&Settings..." ), this );
	aSettings->setToolTip( tr( "show the settings dialog" ) );
	connect( aSettings, &QAction::triggered, dialog, &GroupBox::show );

	cfg.endGroup();

	GroupBox * texPage;
	tab = new QTabWidget;

	if ( GroupBox * genPage = new GroupBox( Qt::Vertical ) ) {
		tab->addTab( genPage, tr( "General" ) );

		genPage->pushLayout( Qt::Vertical );
		cfg.beginGroup( "Settings" );

		// Locale Settings
		genPage->pushLayout( tr( "Regional and Language Settings" ), Qt::Vertical, 1 );
		genPage->pushLayout( Qt::Horizontal );

		genPage->addWidget( new QLabel( "Language:" ) );
		RegionOpt = new QComboBox;
		genPage->addWidget( RegionOpt );

		QLocale localeInvariant( "en" );
		QString txtLang = QLocale::languageToString( localeInvariant.language() );

		if ( localeInvariant.country() != QLocale::AnyCountry )
			txtLang.append( " (" ).append( QLocale::countryToString( localeInvariant.country() ) ).append( ")" );

		RegionOpt->addItem( txtLang, localeInvariant );

		QDir directory( QApplication::applicationDirPath() );

		if ( !directory.cd( "lang" ) ) {
#ifdef Q_OS_LINUX

			if ( !directory.cd( "/usr/share/nifskope/lang" ) ) {
			}

#endif
		}

		QRegularExpression fileRe( "NifSkope_(.*)\\.qm", QRegularExpression::CaseInsensitiveOption );

		foreach ( const QString file, directory.entryList( QStringList( "NifSkope_*.qm" ), QDir::Files | QDir::NoSymLinks ) ) {
			QRegularExpressionMatch fileReMatch = fileRe.match( file );
			if ( fileReMatch.hasMatch() ) {
				QString localeText = fileReMatch.capturedTexts()[1];
				QLocale fileLocale( localeText );

				if ( RegionOpt->findData( fileLocale ) < 0 ) {
					QString txtLang = QLocale::languageToString( fileLocale.language() );

					if ( fileLocale.country() != QLocale::AnyCountry )
						txtLang.append( " (" ).append( QLocale::countryToString( fileLocale.country() ) ).append( ")" );

					RegionOpt->addItem( txtLang, fileLocale );
				}
			}
		}
		QLocale locale = cfg.value( "Language", "en" ).toLocale();
		int localeIdx  = RegionOpt->findData( locale );
		RegionOpt->setCurrentIndex( localeIdx > 0 ? localeIdx : 0 );

		connect( RegionOpt, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),this, &Options::sigChanged );
		connect( RegionOpt, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &Options::sigLocaleChanged );

		genPage->popLayout();
		genPage->popLayout();

		//Misc Options
		genPage->pushLayout( tr( "Misc. Settings" ), Qt::Vertical, 1 );
		genPage->pushLayout( Qt::Horizontal );

		genPage->addWidget( new QLabel( tr( "Startup Version" ) ) );
		genPage->addWidget( StartVer = new QLineEdit( cfg.value( "Startup Version", "20.0.0.5" ).toString() ) );
		StartVer->setToolTip( tr( "This is the version that the initial 'blank' NIF file that is created when NifSkope opens will be." ) );
		connect( StartVer, &QLineEdit::textChanged, this, &Options::sigChanged );

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


	tab->addTab( texPage = new GroupBox( Qt::Vertical ), tr( "&Rendering" ) );
	{
		cfg.beginGroup( "Render Settings" );

		texPage->pushLayout( tr( "Texture Folders" ), Qt::Vertical );
		texPage->pushLayout( Qt::Horizontal );

#ifdef Q_OS_WIN32
		texPage->pushLayout( tr( "Auto Detect" ), Qt::Vertical );
		QButtonGroup * tfgamegrp = new QButtonGroup( this );
		connect( tfgamegrp, bgClicked, this, &Options::textureFolderAutoDetect );
		QPushButton * bt = new QPushButton( tr( "Auto Detect\nGame Paths" ) );
		tfgamegrp->addButton( bt );
		texPage->addWidget( bt );

		texPage->popLayout();
#endif

		texPage->pushLayout( tr( "Custom" ), Qt::Vertical );
		texPage->pushLayout( Qt::Horizontal );

		QButtonGroup * tfactgrp = new QButtonGroup( this );
		connect( tfactgrp, bgClicked, this, &Options::textureFolderAction );
		int tfaid = 0;
		for ( const QString& tfaname : QStringList{ tr( "Add Folder" ), tr( "Remove Folder" ), tr( "Move Up" ), tr( "Move Down" ) } )
		{
			QPushButton * bt = new QPushButton( tfaname );
			TexFolderButtons[tfaid] = bt;
			tfactgrp->addButton( bt, tfaid++ );
			texPage->addWidget( bt );
		}

		texPage->popLayout();

		

		TexFolderModel = new QStringListModel( this );
		TexFolderModel->setStringList( cfg.value( "Texture Folders" ).toStringList() );

		connect( TexFolderModel, &QStringListModel::rowsInserted, tEmit, tStart );
		connect( TexFolderModel, &QStringListModel::rowsRemoved, tEmit, tStart );
		connect( TexFolderModel, &QStringListModel::dataChanged, tEmit, tStart );
		connect( TexFolderModel, &QStringListModel::modelReset, tEmit, tStart );
		connect( TexFolderModel, &QStringListModel::rowsInserted, this, &Options::sigFlush3D );
		connect( TexFolderModel, &QStringListModel::rowsRemoved, this, &Options::sigFlush3D );
		connect( TexFolderModel, &QStringListModel::dataChanged, this, &Options::sigFlush3D );
		connect( TexFolderModel, &QStringListModel::modelReset, this, &Options::sigFlush3D );


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
		connect( TexFolderView->selectionModel(), &QItemSelectionModel::currentChanged,
			TexFolderMapper, &QDataWidgetMapper::setCurrentModelIndex );
		connect( TexFolderSelect, &FileSelector::sigActivated, TexFolderMapper, &QDataWidgetMapper::submit );

		connect( TexFolderView->selectionModel(), &QItemSelectionModel::currentChanged,
			this, &Options::textureFolderIndex );
		textureFolderIndex( TexFolderView->currentIndex() );

		texPage->addWidget( TexAlternatives = new QCheckBox( tr( "&Look for alternatives" ) ) );
		TexAlternatives->setToolTip( tr( "If a texture was nowhere to be found<br>NifSkope will start looking for alternatives.<p style='white-space:pre'>texture.dds does not exist -> use texture.bmp instead</p>" ) );
		TexAlternatives->setChecked( cfg.value( "Texture Alternatives", true ).toBool() );
		connect( TexAlternatives, &QCheckBox::toggled, this, &Options::sigChanged );
		connect( TexAlternatives, &QCheckBox::toggled, this, &Options::sigFlush3D );

		texPage->popLayout();
		texPage->popLayout();
		texPage->popLayout();
		texPage->popLayout();
		texPage->pushLayout( Qt::Horizontal );
		texPage->pushLayout( tr( "Render" ), Qt::Vertical );


		texPage->addWidget( AntiAlias = new QCheckBox( tr( "&Anti Aliasing" ) ) );
		AntiAlias->setToolTip( tr( "Enable anti aliasing and anisotropic texture filtering if available.<br>You'll need to restart NifSkope for this setting to take effect.<br>" ) );
		AntiAlias->setChecked( cfg.value( "Anti Aliasing", true ).toBool() );
		connect( AntiAlias, &QCheckBox::toggled, this, &Options::sigChanged );

		texPage->addWidget( Textures = new QCheckBox( tr( "&Textures" ) ) );
		Textures->setToolTip( tr( "Enable textures" ) );
		Textures->setChecked( cfg.value( "Texturing", true ).toBool() );
		connect( Textures, &QCheckBox::toggled, this, &Options::sigChanged );
		connect( Textures, &QCheckBox::toggled, this, &Options::sigFlush3D );

		texPage->addWidget( Shaders = new QCheckBox( tr( "&Shaders" ) ) );
		Shaders->setToolTip( tr( "Enable Shaders" ) );
		Shaders->setChecked( cfg.value( "Enable Shaders", true ).toBool() );
		connect( Shaders, &QCheckBox::toggled, this, &Options::sigChanged );
		connect( Shaders, &QCheckBox::toggled, this, &Options::sigFlush3D );

		texPage->popLayout();
		texPage->pushLayout( tr( "Up Axis" ), Qt::Vertical );


		texPage->addWidget( AxisX = new QRadioButton( tr( "X" ) ) );
		texPage->addWidget( AxisY = new QRadioButton( tr( "Y" ) ) );
		texPage->addWidget( AxisZ = new QRadioButton( tr( "Z" ) ) );

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

		connect( AxisX, &QRadioButton::toggled, this, &Options::sigChanged );
		connect( AxisY, &QRadioButton::toggled, this, &Options::sigChanged );
		connect( AxisZ, &QRadioButton::toggled, this, &Options::sigChanged );


		texPage->popLayout();
		texPage->pushLayout( tr( "Culling" ), Qt::Vertical, 1 );

		texPage->addWidget( CullNoTex = new QCheckBox( tr( "Cull &Non Textured" ) ) );
		CullNoTex->setToolTip( tr( "Hide all meshes without textures" ) );
		CullNoTex->setChecked( cfg.value( "Cull Non Textured", false ).toBool() );
		connect( CullNoTex, &QCheckBox::toggled, this, &Options::sigChanged );

		texPage->addWidget( CullByID = new QCheckBox( tr( "&Cull Nodes by Name" ) ) );
		CullByID->setToolTip( tr( "Enabling this option hides some special nodes and meshes" ) );
		CullByID->setChecked( cfg.value( "Cull Nodes By Name", false ).toBool() );
		connect( CullByID, &QCheckBox::toggled, this, &Options::sigChanged );

		texPage->addWidget( CullExpr = new QLineEdit( cfg.value( "Cull Expression", "^collidee|^shadowcaster|^\\!LoD_cullme|^footprint" ).toString() ) );
		CullExpr->setToolTip( tr( "Enter a regular expression. Nodes which names match the expression will be hidden" ) );
//    leave RegExp input open as both rendering and collada cull sharing this
//    CullExpr->setEnabled( CullByID->isChecked() );
		CullExpr->setEnabled( true );
		connect( CullExpr, &QLineEdit::textChanged, this, &Options::sigChanged );
//    connect( CullByID, SIGNAL( toggled( bool ) ), CullExpr, SLOT( setEnabled( bool ) ) );

		texPage->popLayout();
		texPage->popLayout();

		cfg.endGroup();
	}

	GroupBox * colorPage;
	tab->addTab( colorPage = new GroupBox( Qt::Vertical ), tr( "Colors" ) );
	{
		cfg.beginGroup( "Render Settings" );

		colorPage->pushLayout( tr( "Light" ), Qt::Vertical );
		colorPage->pushLayout( Qt::Horizontal );


		cfg.beginGroup( "Light0" );

		QStringList lightNames{ tr( "Ambient" ), tr( "Diffuse" ), tr( "Specular" ) };
		QList<QColor> lightDefaults{
			QColor::fromRgbF( .4, .4, .4 ),
			QColor::fromRgbF( .8, .8, .8 ),
			QColor::fromRgbF( 1, 1, 1 )
		};

		for ( int l = 0; l < 3; l++ ) {
			ColorWheel * wheel = new ColorWheel( cfg.value( lightNames[l], lightDefaults[l] ).value<QColor>() );
			wheel->setSizeHint( QSize( 105, 105 ) );
			wheel->setAlpha( false );
			connect( wheel, &ColorWheel::sigColorEdited, this, &Options::sigChanged );
			LightColor[l] = wheel;

			colorPage->pushLayout( lightNames[l], Qt::Vertical );
			colorPage->addWidget( wheel );
			colorPage->popLayout();
		}

		colorPage->popLayout();
		colorPage->pushLayout( Qt::Horizontal );

		colorPage->addWidget( LightFrontal = new QCheckBox( tr( "Frontal" ) ), 0 );
		LightFrontal->setToolTip( tr( "Lock light to camera position" ) );
		LightFrontal->setChecked( cfg.value( "Frontal", true ).toBool() );
		connect( LightFrontal, &QCheckBox::toggled, this, &Options::sigChanged );

		QWidget * pos = colorPage->pushLayout( tr( "Position" ), Qt::Horizontal, 1 );
		pos->setDisabled( LightFrontal->isChecked() );
		connect( LightFrontal, &QCheckBox::toggled, pos, &QWidget::setDisabled );

		colorPage->addWidget( new QLabel( tr( "Declination" ) ) );
		colorPage->addWidget( LightDeclination = new QSpinBox, 1 );
		LightDeclination->setMinimum( -180 );
		LightDeclination->setMaximum( +180 );
		LightDeclination->setSingleStep( 5 );
		LightDeclination->setWrapping( true );
		LightDeclination->setValue( cfg.value( "Declination", 0 ).toInt() );
		connect( LightDeclination, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Options::sigChanged );

		colorPage->addWidget( new QLabel( tr( "Planar Angle" ) ) );
		colorPage->addWidget( LightPlanarAngle = new QSpinBox, 1 );
		LightPlanarAngle->setMinimum( -180 );
		LightPlanarAngle->setMaximum( +180 );
		LightPlanarAngle->setSingleStep( 5 );
		LightPlanarAngle->setWrapping( true );
		LightPlanarAngle->setValue( cfg.value( "Planar Angle", 0 ).toInt() );
		connect( LightPlanarAngle, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Options::sigChanged );

		cfg.endGroup();

		colorPage->popLayout();
		colorPage->pushLayout( tr( "Presets" ), Qt::Horizontal );

		QButtonGroup * grp = new QButtonGroup( this );
		connect( grp, bgClicked, this, &Options::activateLightPreset );
		int psid = 0;
		for ( const QString& psname : QStringList{ tr( "Sunny Day" ), tr( "Dark Night" ) } )
		{
			QPushButton * bt = new QPushButton( psname );
			grp->addButton( bt, psid++ );
			colorPage->addWidget( bt );
		}

		colorPage->popLayout();
		colorPage->popLayout();
		colorPage->popLayout();
		colorPage->pushLayout( tr( "Colors" ), Qt::Horizontal );


		QStringList colorNames{ tr( "Background" ), tr( "Foreground" ), tr( "Highlight" ) };
		QList<QColor> colorDefaults{
			QColor::fromRgb( 0, 0, 0 ),
			QColor::fromRgb( 255, 255, 255 ),
			QColor::fromRgb( 255, 255, 0 )
		};

		for ( int c = 0; c < 3; c++ ) {
			colorPage->pushLayout( colorNames[c], Qt::Horizontal );

			ColorWheel * wheel = new ColorWheel( cfg.value( colorNames[c], colorDefaults[c] ).value<QColor>() );
			wheel->setSizeHint( QSize( 105, 105 ) );
			connect( wheel, &ColorWheel::sigColorEdited, this, &Options::sigChanged );
			colors[ c ] = wheel;
			colorPage->addWidget( wheel );

			if ( c != 0 ) {
				alpha[ c ] = new AlphaSlider( Qt::Vertical );
				alpha[ c ]->setValue( cfg.value( colorNames[c], colorDefaults[c] ).value<QColor>().alphaF() );
				alpha[ c ]->setColor( wheel->getColor() );
				connect( alpha[c], &AlphaSlider::valueChanged, this, &Options::sigChanged );
				connect( wheel, &ColorWheel::sigColor, alpha[ c ], &AlphaSlider::setColor );
				connect( alpha[c], &AlphaSlider::valueChanged, wheel, &ColorWheel::setAlphaValue );
				colorPage->addWidget( alpha[ c ] );
			} else {
				alpha[c] = nullptr;
				wheel->setAlpha( false );
			}

			colorPage->popLayout();
		}

		colorPage->popLayout();

		cfg.endGroup();
	}
	GroupBox * matPage;
	tab->addTab( matPage = new GroupBox( Qt::Vertical ), tr( "Materials" ) );
	{
		cfg.beginGroup( "Render Settings" );

		QWidget * overrideBox = matPage->pushLayout( tr( "Material Overrides" ), Qt::Vertical );
		overrideBox->setMaximumHeight( 200 );

		matPage->pushLayout( Qt::Horizontal );

		cfg.beginGroup( "MatOver" );

		QStringList names{ tr( "Ambient" ), tr( "Diffuse" ), tr( "Specular" ), tr( "Emissive" ) };
		QList<QColor> defaults{
			QColor::fromRgbF( 1, 1, 1 ),
			QColor::fromRgbF( 1, 1, 1 ),
			QColor::fromRgbF( 1, 1, 1 ),
			QColor::fromRgbF( 1, 1, 1 )
		};

		for ( int l = 0; l < 4; l++ ) {
			ColorWheel * wheel = new ColorWheel( cfg.value( names[l], defaults[l] ).value<QColor>() );
			wheel->setSizeHint( QSize( 105, 105 ) );
			wheel->setAlpha( false );
			connect( wheel, &ColorWheel::sigColorEdited, this, &Options::materialOverridesChanged );
			matColors[l] = wheel;

			QWidget * box = matPage->pushLayout( names[l], Qt::Vertical );
			box->setMaximumSize( 150, 150 );
			matPage->addWidget( wheel );
			matPage->popLayout();
		}

		matPage->popLayout();
		matPage->pushLayout( Qt::Horizontal );

		matPage->addWidget( overrideMatCheck = new QCheckBox( tr( "Enable Material Color Overrides" ) ), 0 );
		overrideMatCheck->setToolTip( tr( "Override colors used on Materials" ) );
		//overrideMaterials->setChecked( cfg.value( "Override", true ).toBool() );
		connect( overrideMatCheck, &QCheckBox::toggled, this, &Options::materialOverridesChanged );

		cfg.endGroup();

		matPage->popLayout();
		matPage->popLayout();

		cfg.endGroup();
	}

	GroupBox * exportPage;
	tab->addTab( exportPage = new GroupBox( Qt::Vertical ), tr( "Export" ) );
	{
		cfg.beginGroup( "Export Settings" );
		exportPage->pushLayout( tr( "Export Settings" ), Qt::Vertical, 1 );
		exportPage->addWidget( exportCull = new QCheckBox( tr( "Use 'Cull Nodes by Name' rendering option to cull nodes on export" ) ), 1, Qt::AlignTop );
		exportCull->setChecked( cfg.value( "Export Culling", false ).toBool() );
		connect( exportCull, &QCheckBox::toggled, this, &Options::sigChanged );
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
	if ( o == dialog && e->type() == QEvent::Close && tSave->isActive() ) {
		save();
	}

	return false;
}

Options * Options::get()
{
	static Options * options = new Options();
	return options;
}

QList<QAction *> Options::actions()
{
	Options * opts = get();
	return { opts->aDrawAxes,
			opts->aDrawNodes,
			opts->aDrawHavok ,
			opts->aDrawConstraints,
			opts->aDrawFurn,
			opts->aDrawHidden,
#ifdef USE_GL_QPAINTER
			opts->aDrawStats,
#endif
			opts->aSettings
	};
}

void Options::save()
{
	tSave->stop();

	QSettings cfg;

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

		// Light0 group
		cfg.setValue( "Light0/Ambient", ambient() );
		cfg.setValue( "Light0/Diffuse", diffuse() );
		cfg.setValue( "Light0/Specular", specular() );
		cfg.setValue( "Light0/Frontal", lightFrontal() );
		cfg.setValue( "Light0/Declination", lightDeclination() );
		cfg.setValue( "Light0/Planar Angle", lightPlanarAngle() );

		// MatOver group
		cfg.setValue( "MatOver/Ambient", overrideAmbient() );
		cfg.setValue( "MatOver/Diffuse", overrideDiffuse() );
		cfg.setValue( "MatOver/Specular", overrideSpecular() );
		cfg.setValue( "MatOver/Emissive", overrideEmissive() );

	cfg.endGroup(); // Render Settings

	// Settings group
	cfg.setValue( "Settings/Language", translationLocale() );
	cfg.setValue( "Settings/Startup Version", startupVersion() );
	// If we want to make this more accessible
	//cfg.setValue( "Settings/Maximum String Length", maxStringLength() );

	// Export Settings group
	cfg.setValue( "Export Settings/Export Culling", exportCullEnabled() );
}

bool regTexturePath( QStringList & gamePaths, QString & gameList, // Out Params
                     const QString & regPath, const QString & regValue,
                     const QString & gameFolder, const QString & gameName,
                     QStringList gameSubDirs = QStringList(),
                     QStringList gameArchiveFilters = QStringList(),
                     QString confirmPath = QString() )
{
	QSettings reg( regPath, QSettings::NativeFormat );
	QDir dir( reg.value( regValue ).toString() );

	if ( dir.exists() && dir.cd( gameFolder ) ) {
		gameList.append( gameName + "\n" );

		gamePaths.append( dir.path() );
		if ( !gameSubDirs.isEmpty() ) {
			foreach ( QString sd, gameSubDirs ) {
				gamePaths.append( dir.path() + sd );
			}
		}

		if ( !gameArchiveFilters.isEmpty() ) {
			dir.setNameFilters( gameArchiveFilters );
			dir.setFilter( QDir::Dirs );
			foreach( QString dn, dir.entryList() ) {
				gamePaths << dir.filePath( dn );

				if ( !gameSubDirs.isEmpty() ) {
					foreach ( QString sd, gameSubDirs ) {
						gamePaths << dir.filePath( dn ) + sd;
					}
				}
			}
		}
		return true;
	}
	return false;
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

	// Skyrim
	{
		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Skyrim",
			"Installed Path",
			"Data",
			"TES V: Skyrim",
			{},        /* No subdirs */
			{ ".bsa" },
			"Textures" /* Confirm Textures if no FSENGINE */
		);
	}

	// Fallout: New Vegas
	{
		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\FalloutNV",
			"Installed Path",
			"Data",
			"Fallout: New Vegas",
			{},        /* No subdirs */
			{ ".bsa" },
			"Textures" /* Confirm Textures if no FSENGINE */
		);
	}

	// Fallout 3
	{
		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Fallout3",
			"Installed Path",
			"Data",
			"Fallout 3",
			{},        /* No subdirs */
			{ ".bsa" },
			"Textures" /* Confirm Textures if no FSENGINE */
		);
	}

	// Oblivion
	{
		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Oblivion",
			"Installed Path",
			"Data",
			"TES IV: Oblivion",
			{},        /* No subdirs */
			{ ".bsa" },
			"Textures" /* Confirm Textures if no FSENGINE */
		);
	}

	// Morrowind
	{
		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Bethesda Softworks\\Morrowind",
			"Installed Path",
			"Data",
			"TES III: Morrowind",
			{ "/Textures" },
			{ ".bsa" }
		);
	}

	// CIV IV
	{
		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Firaxis Games\\Sid Meier's Civilization 4",
			"INSTALLDIR",
			"Assets/Art/shared",
			"Sid Meier's Civilization IV"
		);
	}

	// Freedom Force
	{
		QStringList ffSubDirs{ "./textures", "./skins/standard" };

		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\FFVTTR",
			"InstallDir",
			"Data/Art/library/area_specific/_textures",
			"Freedom Force vs. the Third Reich",
			ffSubDirs
		);

		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\Freedom Force",
			"InstallDir",
			"Data/Art/library/area_specific/_textures",
			"Freedom Force",
			ffSubDirs
		);

		regTexturePath( list, game_list,
			"HKEY_LOCAL_MACHINE\\SOFTWARE\\Irrational Games\\Freedom Force Demo",
			"InstallDir",
			"Data/Art/library/area_specific/_textures",
			"Freedom Force Demo",
			ffSubDirs
		);
	}

#endif
	//Set folder list box to contain the newly detected textures, along with the ones the user has already defined, ignoring any duplicates
	list.removeDuplicates();

	TexFolderModel->setStringList( list );
	TexAlternatives->setChecked( false );
	TexFolderView->setCurrentIndex( TexFolderModel->index( 0, 0, QModelIndex() ) );

	//Announce result to user
	if ( game_list.size() == 0 ) {
		game_list = tr( "No supported games were detected.\nYour game may still work, you will just have to set the folders manually until an auto-detect routine is created." );
	} else {
		game_list = tr( "Successfully detected the following games:\n" ) + game_list;
	}

	QMessageBox::information( dialog, "NifSkope", game_list );
}

void Options::textureFolderAction( int id )
{
	QModelIndex idx = TexFolderView->currentIndex();

	switch ( id ) {
	case 0:
		// add folder
		TexFolderModel->insertRow( 0, QModelIndex() );
		TexFolderModel->setData( TexFolderModel->index( 0, 0, QModelIndex() ), tr( "Choose a folder" ), Qt::EditRole );
		TexFolderView->setCurrentIndex( TexFolderModel->index( 0, 0, QModelIndex() ) );
		break;
	case 1:

		if ( idx.isValid() ) {
			// remove folder
			TexFolderModel->removeRow( idx.row(), QModelIndex() );
		}
		break;
	case 2:

		if ( idx.isValid() && idx.row() > 0 ) {
			// move up
			QModelIndex xdi = idx.sibling( idx.row() - 1, 0 );
			QVariant v = TexFolderModel->data( idx, Qt::EditRole );
			TexFolderModel->setData( idx, TexFolderModel->data( xdi, Qt::EditRole ), Qt::EditRole );
			TexFolderModel->setData( xdi, v, Qt::EditRole );
			TexFolderView->setCurrentIndex( xdi );
		}
		break;
	case 3:

		if ( idx.isValid() && idx.row() < TexFolderModel->rowCount() - 1 ) {
			// move down
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
	switch ( id ) {
	case 0:     // sunny day
		LightColor[0]->setColor( QColor::fromRgbF( 0.4, 0.4, 0.4 ) );
		LightColor[1]->setColor( QColor::fromRgbF( 0.8, 0.8, 0.8 ) );
		LightColor[2]->setColor( QColor::fromRgbF( 1.0, 1.0, 1.0 ) );
		break;
	case 1:     // dark night
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


QRegularExpression Options::cullExpression()
{
	return get()->CullByID->isChecked() ? QRegularExpression( get()->CullExpr->text() ) : QRegularExpression();
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

	if ( idx >= 0 ) {
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

bool Options::exportCullEnabled()
{
	return get()->exportCull->isChecked();
}


QString Options::getDisplayVersion()
{
	return get()->version->displayVersion;
}
