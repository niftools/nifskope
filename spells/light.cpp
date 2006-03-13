#include "../spellbook.h"

#include "light.h"
#include "color.h"

#include "../nifdelegate.h"

#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSlider>
#include <QTimer>

/* XPM */
static char * light42_xpm[] = {
"24 24 43 1",
" 	c None",
".	c #000100","+	c #0E0D02","@	c #111401","#	c #151500","$	c #191903",
"%	c #1E1D02","&	c #201E00","*	c #2A2C01","=	c #2D2D00","-	c #2E2F00",
";	c #2F3000",">	c #3B3A00",",	c #3D3C00","'	c #3E3D00",")	c #454300",
"!	c #464800","~	c #494B00","{	c #525200","]	c #565700","^	c #6B6900",
"/	c #6B6D00","(	c #797A00","_	c #7E7F02",":	c #848300","<	c #9D9E03",
"[	c #A6A600","}	c #B3B202","|	c #B8B500","1	c #CFD000","2	c #D7D600",
"3	c #DDDB00","4	c #E4E200","5	c #E9E600","6	c #E8EB00","7	c #ECEF00",
"8	c #F1F300","9	c #F3F504","0	c #F6F800","a	c #F9FA00","b	c #FBFC00",
"c	c #FEFE00","d	c #FFFF01",
"         -,'~'*         ",
"       $[8bbdb5_        ",
"       }bddddddb[       ",
"      :bddddddddb<      ",
"     -2dddddddddda;     ",
"     'addddddddddb{     ",
"     ~bddddddddddd^     ",
"     ]dddddddddddd/     ",
"     ~bddddddddddd{     ",
"     ,addddddddddb;     ",
"      3dddddddddd7      ",
"      :adddddddd9:      ",
"       (8bddddb8)       ",
"        ,}4741|]        ",
"         +@##%&         ",
"         ......         ",
"         ......         ",
"         ......         ",
"         ......         ",
"         ......         ",
"         ......         ",
"         ......         ",
"         ......         ",
"          ....          "};

QIcon * light42_xpm_icon = 0;

class spLightEdit : public Spell
{
public:
	QString name() const { return "Light"; }
	QString page() const { return ""; }
	bool instant() const { return true; }
	QIcon icon() const
	{
		if ( ! light42_xpm_icon ) light42_xpm_icon = new QIcon( light42_xpm );
		return *light42_xpm_icon;
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iBlock = nif->getBlock( index );
		QModelIndex sibling = index.sibling( index.row(), 0 );
		return index.isValid() && nif->inherits( iBlock, "ALight" ) && ( iBlock == sibling || nif->getIndex( iBlock, "Name" ) == sibling );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		LightEdit * le = new LightEdit( nif, nif->getBlock( index ) );
		le->setAttribute( Qt::WA_DeleteOnClose );
		le->show();
		return index;
	}
};

REGISTER_SPELL( spLightEdit )

static const char * colorName[3] = { "Ambient Color", "Diffuse Color", "Specular Color" };
static const char * valueName[1] = { "Dimmer" };
static const float valueMax[1] = { 1.0 };
static const char * attenuationName[3] = { "Constant", "Linear", "Quadratic" };
static const char * spotName[2] = { "Cutoff Angle", "Exponent" };
static const float spotRange[2][2] = { { 0.0, 90.0 }, { 0.0, 128.0 } };

LightEdit::LightEdit( NifModel * n, const QModelIndex & i )
	: QDialog(), nif( n ), iLight( i )
{
	connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
		this, SLOT( nifDataChanged( const QModelIndex &, const QModelIndex & ) ) );
	connect( nif, SIGNAL( modelReset() ), this, SLOT( sltReset() ) );
	connect( nif, SIGNAL( destroyed() ), this, SLOT( nifDestroyed() ) );
	
	QVBoxLayout * vbox = new QVBoxLayout;
	setLayout( vbox );
	
	{
		lightName = new QLineEdit;
		connect( lightName, SIGNAL( textEdited( const QString & ) ), this, SLOT( sltApply() ) );
		
		QGroupBox * group = new QGroupBox( "Name" );
		QVBoxLayout * lay = new QVBoxLayout;
		group->setLayout( lay );
		lay->addWidget( lightName );
		
		vbox->addWidget( group );
	}
	
	{
		QHBoxLayout * hbox = new QHBoxLayout;
		vbox->addLayout( hbox );
		
		position = new VectorEdit;
		connect( position, SIGNAL( sigEdited() ), this, SLOT( sltApply() ) );
		
		direction = new RotationEdit;
		connect( direction, SIGNAL( sigEdited() ), this, SLOT( sltApply() ) );
		
		QGroupBox * grp = new QGroupBox( "Position" );
		QVBoxLayout * lay = new QVBoxLayout;
		grp->setLayout( lay );
		lay->addWidget( position );
		hbox->addWidget( grp );
		
		grp = new QGroupBox( "Rotation" );
		lay = new QVBoxLayout;
		grp->setLayout( lay );
		lay->addWidget( direction );
		hbox->addWidget( grp );
	}
	
	QGridLayout * grid = new QGridLayout;
	vbox->addLayout( grid );
	
	for ( int i = 0; i < 3; i++ )
	{
		color[i] = new ColorWheel;
		color[i]->setSizeHint( QSize( 140, 140 ) );
		connect( color[i], SIGNAL( sigColorEdited( const QColor & ) ), this, SLOT( sltApply() ) );
		
		QGroupBox * group = new QGroupBox( colorName[i] );
		QVBoxLayout * lay = new QVBoxLayout;
		group->setLayout( lay );
		lay->addWidget( color[i] );
		
		grid->addWidget( group, 0, i );
	}
	
	for ( int i = 0; i < 1; i++ )
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
	
	QHBoxLayout * hbox = new QHBoxLayout;
	hbox->setMargin( 0 );
	vbox->addLayout( hbox );
	
	isPoint = nif->inherits( iLight, "APointLight" );
	if ( isPoint )
	{
		QGroupBox * grp = new QGroupBox( "Attenuation" );
		QGridLayout * grid = new QGridLayout;
		grp->setLayout( grid );
		hbox->addWidget( grp );
		
		for ( int i = 0; i < 3; i++ )
		{
			grid->addWidget( new QLabel( attenuationName[i] ), i, 0 );
			
			attenuation[i] = new QDoubleSpinBox;
			attenuation[i]->setRange( 0.0, 1.0 );
			connect( attenuation[i], SIGNAL( valueChanged( double ) ), this, SLOT( sltApply() ) );
			grid->addWidget( attenuation[i], i, 1 );
		}
	}
	
	isSpot = nif->itemName( iLight ) == "NiSpotLight";
	if ( isSpot )
	{
		QGroupBox * grp = new QGroupBox( "Spot" );
		QGridLayout * grid = new QGridLayout;
		grp->setLayout( grid );
		hbox->addWidget( grp );
		
		for ( int i = 0; i < 2; i++ )
		{
			grid->addWidget( new QLabel( spotName[i] ), i, 0 );
			
			spot[i] = new QDoubleSpinBox;
			spot[i]->setRange( spotRange[i][0], spotRange[i][1] );
			connect( spot[i], SIGNAL( valueChanged( double ) ), this, SLOT( sltApply() ) );
			grid->addWidget( spot[i], i, 1 );
		}
	}
	
	QPushButton * btAccept = new QPushButton( "Accept" );
	connect( btAccept, SIGNAL( clicked() ), this, SLOT( accept() ) );
	vbox->addWidget( btAccept );
	
	timer = new QTimer( this );
	connect( timer, SIGNAL( timeout() ), this, SLOT( sltReset() ) );
	timer->setInterval( 0 );
	timer->setSingleShot( true );
	timer->start();
	
	setting = false;
}

void LightEdit::sltReset()
{
	if ( nif && iLight.isValid() )
	{
		setting = true;
		
		setWindowTitle( "Light - " + nif->get<QString>( iLight, "Name" ) );
		
		lightName->setText( nif->get<QString>( iLight, "Name" ) );
		
		position->setVector3( nif->get<Vector3>( iLight, "Translation" ) );
		direction->setMatrix( nif->get<Matrix>( iLight, "Rotation" ) );
		
		for ( int i = 0; i < 3; i++ )
			color[i]->setColor( nif->get<Color3>( iLight, colorName[i] ).toQColor() );
		
		for ( int i = 0; i < 1; i++ )
			value[i]->setValue( int( nif->get<float>( iLight, valueName[i] ) * 1000 ) );
		
		if ( isPoint )
		{
			for ( int i = 0; i < 3; i++ )
				attenuation[i]->setValue( nif->get<float>( iLight, QString( "%1 Attenuation" ).arg( attenuationName[i] ) ) );
		}
		
		if ( isSpot )
		{
			for ( int i = 0; i < 2; i++ )
				spot[i]->setValue( nif->get<float>( iLight, spotName[i] ) );
		}
		
		setting = false;
	}
	else
		reject();
}

void LightEdit::sltApply()
{
	if ( setting ) return;
	
	if ( nif && iLight.isValid() )
	{
		QColor ctmp[3];
		for ( int i = 0; i < 3; i++ )
			ctmp[i] = color[i]->getColor();
		
		float v[1];
		for ( int i = 0; i < 1; i++ )
			v[i] = value[i]->value() / 1000.0;
		
		float a[3];
		if ( isPoint )
			for ( int i = 0; i < 3; i++ )
				a[i] = attenuation[i]->value();
		
		float s[2];
		if ( isSpot )
			for ( int i = 0; i < 2; i++ )
				s[i] = spot[i]->value();
		
		Vector3 pos = position->getVector3();
		Matrix mtx = direction->getMatrix();
		
		QModelIndex iName = nif->getIndex( iLight, "Name" );
		if ( iName.isValid() )
		{
			iName = iName.sibling( iName.row(), NifModel::ValueCol );
			nif->setData( iName, lightName->text() );
		}
		
		nif->set<Vector3>( iLight, "Translation", pos );
		nif->set<Matrix>( iLight, "Rotation", mtx );
		
		for ( int i = 0; i < 3; i++ )
			nif->set<Color3>( iLight, colorName[i], Color3( ctmp[i] ) );
		
		for ( int i = 0; i < 1; i++ )
			nif->set<float>( iLight, valueName[i], v[i] );
		
		if ( isPoint )
			for ( int i = 0; i < 3; i++ )
				nif->set<float>( iLight, QString( "%1 Attenuation" ).arg( attenuationName[i] ), a[i] );
		
		if ( isSpot )
			for ( int i = 0; i < 2; i++ )
				nif->set<float>( iLight, spotName[i], s[i] );
	}
	else
		reject();
}

void LightEdit::nifDataChanged( const QModelIndex & i, const QModelIndex & j )
{
	if ( nif && iLight.isValid() )
	{
		if ( nif->getBlock( i ) == iLight || nif->getBlock( j ) == iLight )
		{
			if ( ! timer->isActive() )
				timer->start();
		}
	}
	else
		reject();
}

void LightEdit::nifDestroyed()
{
	nif = 0;
	reject();
}
