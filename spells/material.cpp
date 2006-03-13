#include "../spellbook.h"

#include "material.h"
#include "color.h"

#include <QGroupBox>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTimer>

/* XPM */
static char * mat42_xpm[] = {
"64 64 43 1",
" 	c None",
".	c #866A36","+	c #856E33","@	c #817333","#	c #836F45","$	c #8B6D3E",
"%	c #8C6F34","&	c #917231","*	c #77812C","=	c #7D765D","-	c #987147",
";	c #6C8D29",">	c #93792F",",	c #7A7B71","'	c #A0734E",")	c #71828B",
"!	c #5DA11E","~	c #9B822B","{	c #56AD16","]	c #6E889D","^	c #AD795F",
"/	c #698DB0","(	c #48BE13","_	c #6691BD",":	c #43C311","<	c #A58D28",
"[	c #6396CE","}	c #1BE30C","|	c #38D700","1	c #BD7F71","2	c #11F003",
"3	c #C78581","4	c #0CFF00","5	c #B7A51C","6	c #DA8C95","7	c #C3B618",
"8	c #ED93AA","9	c #F898B9","0	c #D6CD0A","a	c #FF9BC4","b	c #E6DF00",
"c	c #F6F500","d	c #FFFF00",
"                                                                ",
"                                                                ",
"                                  .......                       ",
"                               ................                 ",
"                             .......................            ",
"                           ...........................          ",
"                          ...............................       ",
"                        .................................       ",
"                       ...................................      ",
"                     ......................................     ",
"                    ........................................    ",
"                   ...........................@***+.........    ",
"                  ................%~5<%......;2444}:*........   ",
"                 ................&0ddd0&....@}4444442;........  ",
"                ................%7ddddd7....;44444444|+.......  ",
"               .................5ddddddb>...{444444442@.......  ",
"              .................%0ddddddc<..+|444444442@.......  ",
"             ..................%bddddddc<..@|444444442@.......  ",
"            ...................%bddddddc<..@|44444444|........  ",
"           ............'^1^-...%bddddddc>..@|44444444!........  ",
"          ............39aaa8'..%bddddddb%..+:4444442!.........  ",
"         ............^9aaaaa6$.%7dddddd0....*}4444};..........  ",
"        .............6aaaaaa9^..<cddddd5.....*(||{@...........  ",
"        ............$8aaaaaaa1...7ddddb>.....................   ",
"       .............-9aaaaaaa1...%7cdb~......................   ",
"      ..............-9aaaaaaa1.....>~&......................    ",
"      ..............-8aaaaaaa1..............................    ",
"     ........#)]],..$8aaaaaa9^..............................    ",
"     .......=_[[[[,..^9aaaaa6..............................     ",
"    .......$/[[[[[]$..^9aaa8'.............................      ",
"    .......)[[[[[[_#...'363'..............................      ",
"    ......#_[[[[[[_#....................................        ",
"    ......=[[[[[[[/#..................................          ",
"    ......,[[[[[[[_#.................................           ",
"    ......,[[[[[[[_#...............................             ",
"    ......,[[[[[[[]$.............................               ",
"    ......=_[[[[[_#............................                 ",
"    .......][[[[_,............................                  ",
"    .......#][[_,...........................                    ",
"    .........==#..........................                      ",
"    .....................................                       ",
"    ....................................                        ",
"     ...................................                        ",
"     ..................................                         ",
"     .................................                          ",
"      ................................                          ",
"      ...............................                           ",
"      ..................  ...........                           ",
"      ................      .........                           ",
"       ..............        ........                           ",
"        .............        ........                           ",
"        ............         .......                            ",
"         ...........         .......                            ",
"         ............        .......                            ",
"          ...........       ........                            ",
"           ...........     .........                            ",
"           ........................                             ",
"            .......................                             ",
"             .....................                              ",
"               .................                                ",
"                .............                                   ",
"                  ........                                      ",
"                                                                ",
"                                                                "};

QIcon * mat42_xpm_icon = 0;

class spMaterialEdit : public Spell
{
public:
	QString name() const { return "Material"; }
	QString page() const { return ""; }
	bool instant() const { return true; }
	QIcon icon() const
	{
		if ( ! mat42_xpm_icon ) mat42_xpm_icon = new QIcon( mat42_xpm );
		return *mat42_xpm_icon;
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iBlock = nif->getBlock( index, "NiMaterialProperty" );
		QModelIndex sibling = index.sibling( index.row(), 0 );
		return index.isValid() && ( iBlock == sibling || nif->getIndex( iBlock, "Name" ) == sibling );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		MaterialEdit * me = new MaterialEdit( nif, nif->getBlock( index ) );
		me->setAttribute( Qt::WA_DeleteOnClose );
		me->show();
		return index;
	}
};

REGISTER_SPELL( spMaterialEdit )

static const char * colorName[4] = { "Ambient Color", "Diffuse Color", "Specular Color", "Emissive Color" };
static const char * valueName[2] = { "Alpha", "Glossiness" };
static const float valueMax[2] = { 1.0, 128.0 };

MaterialEdit::MaterialEdit( NifModel * n, const QModelIndex & i )
	: QDialog(), nif( n ), iMaterial( i )
{
	connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
		this, SLOT( nifDataChanged( const QModelIndex &, const QModelIndex & ) ) );
	connect( nif, SIGNAL( modelReset() ), this, SLOT( sltReset() ) );
	connect( nif, SIGNAL( destroyed() ), this, SLOT( nifDestroyed() ) );
	
	QVBoxLayout * vbox = new QVBoxLayout;
	setLayout( vbox );
	
	{
		matName = new QLineEdit;
		connect( matName, SIGNAL( textEdited( const QString & ) ), this, SLOT( sltApply() ) );
		
		QGroupBox * group = new QGroupBox( "Name" );
		QVBoxLayout * lay = new QVBoxLayout;
		group->setLayout( lay );
		lay->addWidget( matName );
		
		vbox->addWidget( group );
	}
	
	QGridLayout * grid = new QGridLayout;
	vbox->addLayout( grid );
	
	for ( int i = 0; i < 4; i++ )
	{
		color[i] = new ColorWheel;
		color[i]->setSizeHint( QSize( 140, 140 ) );
		connect( color[i], SIGNAL( sigColorEdited( const QColor & ) ), this, SLOT( sltApply() ) );
		
		QGroupBox * group = new QGroupBox( colorName[i] );
		QVBoxLayout * lay = new QVBoxLayout;
		group->setLayout( lay );
		lay->addWidget( color[i] );
		
		grid->addWidget( group, i / 2, i % 2 );
	}
	
	for ( int i = 0; i < 2; i++ )
	{
		value[i] = new QSlider;
		value[i]->setRange( 0, int( valueMax[i] * 1000 ) );
		value[i]->setOrientation( Qt::Horizontal );
		connect( value[i], SIGNAL( sliderMoved( int ) ), this, SLOT( sltApply() ) );
		
		QGroupBox * grp = new QGroupBox( valueName[i] );
		QVBoxLayout * lay = new QVBoxLayout;
		grp->setLayout( lay );
		lay->addWidget( value[i] );
		vbox->addWidget( grp );
	}
	
	QPushButton * btAccept = new QPushButton( "Accept" );
	connect( btAccept, SIGNAL( clicked() ), this, SLOT( accept() ) );
	vbox->addWidget( btAccept );
	
	timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( sltReset() ) );
	timer->setInterval( 0 );
	timer->setSingleShot( true );
	timer->start();
}

void MaterialEdit::sltReset()
{
	if ( nif && iMaterial.isValid() )
	{
		setWindowTitle( "Material - " + nif->get<QString>( iMaterial, "Name" ) );
		
		matName->setText( nif->get<QString>( iMaterial, "Name" ) );
		
		for ( int i = 0; i < 4; i++ )
			color[i]->setColor( nif->get<Color3>( iMaterial, colorName[i] ).toQColor() );
		
		for ( int i = 0; i < 2; i++ )
			value[i]->setValue( int( nif->get<float>( iMaterial, valueName[i] ) * 1000 ) );
	}
	else
		reject();
}

void MaterialEdit::sltApply()
{
	if ( nif && iMaterial.isValid() )
	{
		QColor ctmp[4];
		for ( int i = 0; i < 4; i++ )
			ctmp[i] = color[i]->getColor();
		
		float v[2];
		for ( int i = 0; i < 2; i++ )
			v[i] = value[i]->value() / 1000.0;
		
		QModelIndex iName = nif->getIndex( iMaterial, "Name" );
		if ( iName.isValid() )
		{
			iName = iName.sibling( iName.row(), NifModel::ValueCol );
			nif->setData( iName, matName->text() );
		}
		
		for ( int i = 0; i < 4; i++ )
			nif->set<Color3>( iMaterial, colorName[i], Color3( ctmp[i] ) );
		
		for ( int i = 0; i < 2; i++ )
			nif->set<float>( iMaterial, valueName[i], v[i] );
	}
	else
		reject();
}

void MaterialEdit::nifDataChanged( const QModelIndex & i, const QModelIndex & j )
{
	if ( nif && iMaterial.isValid() )
	{
		if ( nif->getBlock( i ) == iMaterial || nif->getBlock( j ) == iMaterial )
		{
			if ( ! timer->isActive() )
				timer->start();
		}
	}
	else
		reject();
}

void MaterialEdit::nifDestroyed()
{
	nif = 0;
	reject();
}
