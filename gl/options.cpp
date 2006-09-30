/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
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
#include <QDialog>
#include <QGroupBox>
#include <QLayout>
#include <QLineEdit>
#include <QSettings>
#include <QStack>
#include <QTimer>

#include "widgets/colorwheel.h"

class GroupBox : public QGroupBox
{
	QStack<QBoxLayout*> lay;
public:
	GroupBox( const QString & title, Qt::Orientation o ) : QGroupBox( title )
	{
		lay.push( new QBoxLayout( o2d( o ), this ) );
	}
	
	void addWidget( QWidget * widget, int stretch = 0, Qt::Alignment alignment = 0 )
	{
		lay.top()->addWidget( widget, stretch, alignment );
	}
	
	void pushLayout( Qt::Orientation o )
	{
		QBoxLayout * l = new QBoxLayout( o2d( o ) );
		lay.top()->addLayout( l );
		lay.push( l );
	}
	
	void popLayout()
	{
		if ( lay.count() > 1 )
			lay.pop();
	}
	
	QBoxLayout::Direction o2d( Qt::Orientation o )
	{
		switch ( o )
		{
			case Qt::Vertical:
				return QBoxLayout::TopToBottom;
			case Qt::Horizontal:
			default:
				return QBoxLayout::LeftToRight;
		}
	}
};

GLOptions * GLOptions::get()
{
	static GLOptions * options = new GLOptions();
	return options;
}

QList<QAction*> GLOptions::actions()
{
	GLOptions * opts = get();
	return QList<QAction*>() << opts->aDrawAxes << opts->aDrawNodes << opts->aDrawHavok << opts->aDrawFurn << opts->aDrawHidden << opts->aDrawStats << opts->aSettings;
}

GLOptions::GLOptions()
{
	QSettings cfg( "NifTools", "NifSkope" );
	cfg.beginGroup( "Render Settings" );
	
	aDrawAxes = new QAction( "Draw &Axes", this );
	aDrawAxes->setToolTip( "draw xyz-Axes" );
	aDrawAxes->setCheckable( true );
	aDrawAxes->setChecked( cfg.value( "Draw AXes", true ).toBool() );
	connect( aDrawAxes, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	
	aDrawNodes = new QAction( "Draw &Nodes", this );
	aDrawNodes->setToolTip( "draw bones/nodes" );
	aDrawNodes->setCheckable( true );
	aDrawNodes->setChecked( cfg.value( "Draw Nodes", true ).toBool() );
	connect( aDrawNodes, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	
	aDrawHavok = new QAction( "Draw &Havok", this );
	aDrawHavok->setToolTip( "draw the havok shapes" );
	aDrawHavok->setCheckable( true );
	aDrawHavok->setChecked( cfg.value( "Draw Collision Geometry", true ).toBool() );
	connect( aDrawHavok, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );

	aDrawFurn = new QAction( "Draw &Furniture", this );
	aDrawFurn->setToolTip( "draw the furniture markers" );
	aDrawFurn->setCheckable( true );
	aDrawFurn->setChecked( cfg.value( "Draw Furniture Markers", true ).toBool() );
	connect( aDrawFurn, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	
	aDrawHidden = new QAction( "Show Hid&den", this );
	aDrawHidden->setToolTip( "always draw nodes and meshes" );
	aDrawHidden->setCheckable( true );
	aDrawHidden->setChecked( cfg.value( "Show Hidden Objects", false ).toBool() );
	connect( aDrawHidden, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	
	aDrawStats = new QAction( "Show S&tats", this );
	aDrawStats->setToolTip( "display some statistics about the selected node" );
	aDrawStats->setCheckable( true );
	aDrawStats->setChecked( cfg.value( "Show Stats", false ).toBool() );
	connect( aDrawStats, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	
	
	dialog = new GroupBox( "", Qt::Vertical );
	dialog->setWindowTitle(  "Render Settings"  );
	
	aSettings = new QAction( "&Settings...", this );
	aSettings->setToolTip( "show the advanced settings dialog" );
	connect( aSettings, SIGNAL( triggered() ), dialog, SLOT( show() ) );
	
	GroupBox * grp;
	
	dialog->pushLayout( Qt::Horizontal );
	
	
	dialog->addWidget( grp = new GroupBox( "Render", Qt::Vertical ) );

	AntiAlias = new QCheckBox( "&Anti Aliasing" );
	AntiAlias->setToolTip( "Enable anti aliasing if available.<br>You'll need to restart NifSkope for this setting to take effect." );
	AntiAlias->setChecked( cfg.value( "Anti Aliasing", true ).toBool() );
	connect( AntiAlias, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	grp->addWidget( AntiAlias );
	
	Textures = new QCheckBox( "&Textures" );
	Textures->setToolTip( "Enable textures" );
	Textures->setChecked( cfg.value( "Texturing", true ).toBool() );
	connect( Textures, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	grp->addWidget( Textures );
	
	Shaders = new QCheckBox( "&Shaders" );
	Shaders->setToolTip( "Enable Shaders" );
	Shaders->setChecked( cfg.value( "Enable Shaders", true ).toBool() );
	connect( Shaders, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	grp->addWidget( Shaders );
	
	
	dialog->addWidget( grp = new GroupBox( "Up Axis", Qt::Vertical ) );
	
	grp->addWidget( AxisX = new QCheckBox( "X" ) );
	grp->addWidget( AxisY = new QCheckBox( "Y" ) );
	grp->addWidget( AxisZ = new QCheckBox( "Z" ) );
	
	
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
	
	dialog->popLayout();
	

	dialog->addWidget( grp = new GroupBox( "Culling", Qt::Vertical ) );
	
	CullNoTex = new QCheckBox( "Cull &Non Textured" );
	CullNoTex->setToolTip( "Hide all meshes without textures" );
	CullNoTex->setChecked( cfg.value( "Cull Non Textured", false ).toBool() );
	connect( CullNoTex, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	grp->addWidget( CullNoTex );
	
	grp->pushLayout( Qt::Horizontal );
	CullByID = new QCheckBox( "&Cull Nodes by Name" );
	CullByID->setToolTip( "Enabling this option hides some special nodes and meshes" );
	CullByID->setChecked( cfg.value( "Cull Nodes By Name", false ).toBool() );
	connect( CullByID, SIGNAL( toggled( bool ) ), this, SIGNAL( sigChanged() ) );
	grp->addWidget( CullByID );
	
	CullExpr = new QLineEdit( cfg.value( "Cull Expression", "^collidee|^shadowcaster|^\\!LoD_cullme|^footprint" ).toString() );
	CullExpr->setToolTip( "Enter a regular expression. Nodes which names match the expression will be hidden" );
	CullExpr->setEnabled( CullByID->isChecked() );
	connect( CullExpr, SIGNAL( textChanged( const QString & ) ), this, SIGNAL( sigChanged() ) );
	connect( CullByID, SIGNAL( toggled( bool ) ), CullExpr, SLOT( setEnabled( bool ) ) );
	grp->addWidget( CullExpr );
	grp->popLayout();
	
	
	dialog->addWidget( grp = new GroupBox( "Colors", Qt::Horizontal ) );
	
	QStringList colorNames( QStringList() << "Background" << "Foreground" << "Highlight" );
	QList<QColor> colorDefaults( QList<QColor>() << QColor::fromRgb( 0, 0, 0 ) << QColor::fromRgb( 255, 255, 255 ) << QColor::fromRgb( 255, 255, 0 ) );
	for ( int c = 0; c < 3; c++ )
	{
		GroupBox * g = new GroupBox( colorNames.value( c ), Qt::Vertical );
		grp->addWidget( g );
		ColorWheel * wheel = new ColorWheel( cfg.value( colorNames[c], colorDefaults.value( c ) ).value<QColor>() );
		g->addWidget( wheel );
		wheel->setSizeHint( QSize( 105, 105 ) );
		connect( wheel, SIGNAL( sigColorEdited( const QColor & ) ), this, SIGNAL( sigChanged() ) );
		colors[ c ] = wheel;
	}
	
	dialog->popLayout();
	dialog->popLayout();
	

	tSave = new QTimer( this );
	tSave->setInterval( 5000 );
	tSave->setSingleShot( true );
	connect( this, SIGNAL( sigChanged() ), tSave, SLOT( start() ) );
	connect( tSave, SIGNAL( timeout() ), this, SLOT( save() ) );
}

GLOptions::~GLOptions()
{
	if ( tSave->isActive() )
		save();
}


void GLOptions::save()
{
	tSave->stop();
	
	QSettings cfg( "NifTools", "NifSkope" );
	cfg.beginGroup( "Render Settings" );
	
	cfg.setValue( "Draw Axes", drawAxes() );
	cfg.setValue( "Draw Nodes", drawNodes() );
	cfg.setValue( "Draw Collision Geometry", drawHavok() );
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
}

GLOptions::Axis GLOptions::upAxis()
{
	return get()->AxisX->isChecked() ? XAxis : get()->AxisY->isChecked() ? YAxis : ZAxis;
}

bool GLOptions::antialias()
{
	return get()->AntiAlias->isChecked();
}

bool GLOptions::texturing()
{
	return get()->Textures->isChecked();
}

bool GLOptions::shaders()
{
	return get()->Shaders->isChecked();
}


bool GLOptions::drawAxes()
{
	return get()->aDrawAxes->isChecked();
}

bool GLOptions::drawNodes()
{
	return get()->aDrawNodes->isChecked();
}

bool GLOptions::drawHavok()
{
	return get()->aDrawHavok->isChecked();
}

bool GLOptions::drawFurn()
{
	return get()->aDrawFurn->isChecked();
}

bool GLOptions::drawHidden()
{
	return get()->aDrawHidden->isChecked();
}

bool GLOptions::drawStats()
{
	return get()->aDrawStats->isChecked();
}

bool GLOptions::benchmark()
{
	return false;
}


QColor GLOptions::bgColor()
{
	return get()->colors[ colorBack ]->getColor();
}

QColor GLOptions::hlColor()
{
	return get()->colors[ colorHigh ]->getColor();
}

QColor GLOptions::nlColor()
{
	return get()->colors[ colorNorm ]->getColor();
}


QRegExp GLOptions::cullExpression()
{
	return get()->CullByID->isChecked() ? QRegExp( get()->CullExpr->text() ) : QRegExp();
}

bool GLOptions::onlyTextured()
{
	return get()->CullNoTex->isChecked();
}

