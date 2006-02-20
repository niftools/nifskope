#include "../spellbook.h"

#include <QDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSpinBox>

/* XPM */
static char * flag42_xpm[] = {
"64 64 43 1",
" 	c None",
".	c #000100","+	c #040702","@	c #090C08","#	c #0E100C","$	c #121311",
"%	c #161815","&	c #1B1C1A","*	c #222422","=	c #2C2D2B","-	c #2F312E",
";	c #343633",">	c #3A3C3A",",	c #3F413E","'	c #434542",")	c #4F514E",
"!	c #545553","~	c #5A5C59","{	c #60625F","]	c #646563","^	c #686A67",
"/	c #6E706D","(	c #747673","_	c #777976",":	c #7F817E","<	c #80827F",
"[	c #868885","}	c #8F918E","|	c #90928F","1	c #979996","2	c #A5A7A4",
"3	c #ACAEAB","4	c #B3B5B2","5	c #B5B7B4","6	c #BDBFBC","7	c #C2C4C1",
"8	c #CDCFCC","9	c #D3D5D2","0	c #DADCD9","a	c #E0E2DF","b	c #E4E6E3",
"c	c #EFF1EE","d	c #FEFFFC",
"                                                                ",
"                                                                ",
"                                                                ",
"                    4/|a                                        ",
"                   5....#-)|a                                   ",
"                   '.++.....++#-)|a                             ",
"                   $..+++.++++.....#-)|a                        ",
"                   @..5a})-#+++.++.....+#-)|a                   ",
"                  a.+.adddddda})-#++.+++.....@$-)|a             ",
"                  |+.$dddddddddddda})-#++.++......+#-)|a        ",
"                  !++=ddddddddddddddddda})-#+++.+++....&a       ",
"                  ,+.>dddddddddddddddddddddda5:)-#++.+..8       ",
"                  =+.!d7~~38addddddddddddddddddddda}=+++a       ",
"                  #..}d]....#[dddddddddddddddddddddd)+.=        ",
"                  .++a4$.+...$b80ddddddddddddddddddd=++!        ",
"                 a++.d:...$..!_$,adddddddddddddddddd#+.}        ",
"                 |++$d:++.391=..$8ddddddddddddddddda.++a        ",
"                 )+.=d&+..&<>..*(bddddddddddddddddd|.+$         ",
"                 ,+.,a..++.$...4ddddddddddddddddddd>++>         ",
"                 -+.!!++++.#..$dddddddddddddddddddd#+.}         ",
"                 $+.}.++{2!.+.[ddda316cddddddddddda.++a         ",
"                a++.a^+]dd]..&ddd[$...1ddddddddddd|++$          ",
"                }++#dddddd!+.=ddd..++.]dd4)-(bdddd>++>          ",
"                )+.-dddddd!..!ddd$+...3d5$...|dddd#+.}          ",
"                ,++>dddddd5;>5ddd_++*5db..++.$,>%+.++a          ",
"                -..)dddddddddddddda9add|.++......+++$           ",
"                $+.<dddddddddddddddddd4&..++.+++++++=           ",
"                +..+#>}adddddddddddddd>.+++++#,,,#+.)           ",
"               a++.+....#=!}addddddddd;++++++!dd0$+.}           ",
"               |+++++++.....@-!}addddd4>%$%$*4dd}...b           ",
"               !++=a}>#+++++......>|addddddddddd!++=            ",
"               ,+.>    a})-@+++......#-)|adddddd=..)            ",
"               =+.!        a})=$...++.....=)|add$+.|            ",
"               #..}             a}>#+++........=+..a            ",
"              a.++a                 a})=@..+++..++%             ",
"              5++.                      a}!=..+++.+             ",
"              :++$                           b}>#+^             ",
"              !..-                                              ",
"              =+.!                                              ",
"              $+.:                                              ",
"              .+.4                                              ",
"             b++.a                                              ",
"             }.+$                                               ",
"             )+.-                                               ",
"             ,++>                                               ",
"             -..)                                               ",
"             $+.|                                               ",
"             +..a                                               ",
"            a+++                                                ",
"            |++#                                                ",
"            !++=                                                ",
"            ,..,                                                ",
"            =+.!                                                ",
"            #+.}                                                ",
"           a.++a                                                ",
"           |++$                                                 ",
"           !++=                                                 ",
"           ,+.,                                                 ",
"           =+.!                                                 ",
"           @+.[                                                 ",
"           .++0                                                 ",
"           ]+]                                                  ",
"                                                                ",
"                                                                "};

QIcon * flag42_xpm_icon = 0;

class spEditFlags : public Spell
{
public:
	QString name() const { return "Flags"; }
	QString page() const { return ""; }
	bool instant() const { return true; }
	QIcon icon() const
	{
		if ( ! flag42_xpm_icon ) flag42_xpm_icon = new QIcon( flag42_xpm );
		return *flag42_xpm_icon;
	}
	
	enum FlagType
	{
		Alpha, Controller, Node, Shape, ZBuffer, None
	};
	
	QModelIndex getFlagIndex( const NifModel * nif, const QModelIndex & index ) const
	{
		if ( nif->getValue( index ).type() == NifValue::tFlags && nif->itemType( index.parent() ) == "NiBlock" )
			return index;
		if ( nif->itemType( index ) == "NiBlock" )
			return nif->getIndex( index, "Flags" );
		return QModelIndex();
	}
	
	FlagType queryType( const NifModel * nif, const QModelIndex & index ) const
	{
		if ( nif->getValue( index ).type() == NifValue::tFlags )
		{
			QString name = nif->itemName( index.parent() );
			if ( name == "NiAlphaProperty" )
				return Alpha;
			else if ( nif->inherits( name, "AController" ) )
				return Controller;
			else if ( name == "NiNode" )
				return Node;
			else if ( name == "NiTriShape" || name == "NiTriStrips" )
				return Shape;
			else if ( name == "NiZBufferProperty" )
				return ZBuffer;
		}
		return None;
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return queryType( nif, getFlagIndex( nif, index ) ) != None;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iFlags = getFlagIndex( nif, index );
		
		switch ( queryType( nif, iFlags ) )
		{
			case Alpha:
				alphaFlags( nif, iFlags );
				break;
			case Controller:
				controllerFlags( nif, iFlags );
				break;
			case Node:
				nodeFlags( nif, iFlags );
				break;
			case Shape:
				shapeFlags( nif, iFlags );
				break;
			case ZBuffer:
				zbufferFlags( nif, iFlags );
				break;
			default:
				break;
		}
		return index;
	}
	
	void alphaFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );
		if ( ! flags ) flags = 0x18ed;
		
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );
		
		static const char * blendModes[11] = {
			"One", "Zero", "Src Color", "Inv Src Color", "Dst Color", "Inv Dst Color", "Src Alpha", "Inv Src Alpha",
			"Dst Alpha", "Inv Dst Alpha", "Src Alpha Saturate"
		};
		static const char * testModes[8] = {
			"Always", "Less", "Equal", "Less or Equal", "Greater", "Not Equal", "Greater or Equal", "Never"
		};
		
		QCheckBox * chkBlend = dlgCheck( vbox, "Enable Blending" );
		chkBlend->setChecked( flags & 1 );
		
		QComboBox * cmbSrc = dlgCombo( vbox, "Source Blend Mode", blendModes, 11, chkBlend );
		cmbSrc->setCurrentIndex( flags >> 1 & 0x0f );
		
		QComboBox * cmbDst = dlgCombo( vbox, "Destination Blend Mode", blendModes, 11, chkBlend );
		cmbDst->setCurrentIndex( flags >> 5 & 0x0f );
		
		QCheckBox * chkTest = dlgCheck( vbox, "Enable Testing" );
		chkTest->setChecked( flags & ( 1 << 9 ) );
		
		QComboBox * cmbTest = dlgCombo( vbox, "Alpha Test Function", testModes, 8, chkTest );
		cmbTest->setCurrentIndex( flags >> 10 & 0x07 );
		
		QSpinBox * spnTest = dlgSpin( vbox, "Alpha Test Threshold", 0x00, 0xff, chkTest );
		spnTest->setValue( nif->get<int>( nif->getBlock( index ), "Threshold" ) );
		
		QCheckBox * chkSort = dlgCheck( vbox, "Sort Triangles" );
		chkSort->setChecked( ( flags & 0x2000 ) == 0 );
		
		dlgButtons( &dlg, vbox );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			flags = flags & 0xfffe;
			if ( chkBlend->isChecked() )
			{
				flags |= 1;
				flags = flags & 0xffe1 | cmbSrc->currentIndex() << 1;
				flags = flags & 0xfe1f | cmbDst->currentIndex() << 5;
			}
			
			flags = flags & 0xfdff;
			if ( chkTest->isChecked() )
			{
				flags |= 0x0200;
				flags = flags & 0xe3ff | ( cmbTest->currentIndex() << 10 );
				nif->set<int>( nif->getBlock( index ), "Threshold", spnTest->value() );
			}
			
			flags = flags & 0xdfff | ( chkSort->isChecked() ? 0 : 0x2000 );
			
			nif->set<int>( index, flags );
		}
	}
	
	void nodeFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );
		if ( ! flags ) flags = 8;
		
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );
		
		QCheckBox * chkHidden = dlgCheck( vbox, "Hidden" );
		chkHidden->setChecked( flags & 1 );
		
		static const char * collisionModes[4] = { "None", "Triangles", "Bounding Box", "Continue" };
		
		QComboBox * cmbCollision = dlgCombo( vbox, "Collision Detection", collisionModes, 4 );
		cmbCollision->setCurrentIndex( flags >> 1 & 3 );
		
		QCheckBox * chkSkin = dlgCheck( vbox, "Skin Influence" );
		chkSkin->setChecked( ! ( flags & 8 ) );
		
		dlgButtons( &dlg, vbox );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			flags = flags & 0xfffe | ( chkHidden->isChecked() ? 1 : 0 );
			flags = flags & 0xfff9 | ( cmbCollision->currentIndex() << 1 );
			flags = flags & 0xfff7 | ( chkSkin->isChecked() ? 0 : 8 );
			nif->set<int>( index, flags );
		}
	}
	
	void controllerFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );
		if ( ! flags ) flags = 8;
		
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );
		
		QCheckBox * chkActive = dlgCheck( vbox, "Active" );
		chkActive->setChecked( flags & 8 );
		
		static const char * loopModes[3] = { "Cycle", "Reverse", "Clamp" };
		
		QComboBox * cmbLoop = dlgCombo( vbox, "Loop Mode", loopModes, 3 );
		cmbLoop->setCurrentIndex( flags >> 1 & 3 );
		
		dlgButtons( &dlg, vbox );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			flags = flags & 0xfff7 | ( chkActive->isChecked() ? 8 : 0 );
			flags = flags & 0xfff9 | cmbLoop->currentIndex() << 1;
			nif->set<int>( index, flags );
		}
	}
	
	void shapeFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );
		
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );
		
		QCheckBox * chkHidden = dlgCheck( vbox, "Hidden" );
		chkHidden->setChecked( flags & 0x01 );
		
		static const char * collisionModes[4] = { "None", "Triangles", "Bounding Box", "Continue" };
		
		QComboBox * cmbCollision = dlgCombo( vbox, "Collision Detection", collisionModes, 4 );
		cmbCollision->setCurrentIndex( flags >> 1 & 3 );
		
		QCheckBox * chkShadow = 0;
		if ( nif->checkVersion( 0x04000020, 0x04000020 ) )
		{
			dlgCheck( vbox, "Shadow" );
			chkShadow->setChecked( flags & 0x40 );
		}
		
		dlgButtons( &dlg, vbox );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			flags = flags & 0xfffe | ( chkHidden->isChecked() ? 0x01 : 0 );
			flags = flags & 0xfff9 | ( cmbCollision->currentIndex() << 1 );
			if ( chkShadow )
				flags = flags & 0xffbf | ( chkShadow->isChecked() ? 0x40 : 0 );
			nif->set<int>( index, flags );
		}
	}
	
	void zbufferFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );
		if ( ! flags ) flags = 8;
		
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );
		
		QCheckBox * chkEnable = dlgCheck( vbox, "Enable Z Buffer" );
		chkEnable->setChecked( flags & 1 );
		
		QCheckBox * chkROnly = dlgCheck( vbox, "Z Buffer Read Only" );
		chkROnly->setChecked( ( flags & 2 ) == 0 );
		
		static const char * depthModes[8] = {
			"Always", "Less", "Equal", "Less or Equal", "Greater", "Not Equal", "Greater or Equal", "Never"
		};
		
		QComboBox * cmbFunc = 0;
		if ( nif->checkVersion( 0x20000004, 0 ) )
		{
			cmbFunc = dlgCombo( vbox, "Z Buffer Test Function", depthModes, 8, chkEnable );
			cmbFunc->setCurrentIndex( ( flags >> 2 ) & 0x07 );
		}
		
		dlgButtons( &dlg, vbox );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			flags = flags & 0xfffe | ( chkEnable->isChecked() ? 1 : 0 );
			flags = flags & 0xfffd | ( chkROnly->isChecked() ? 0 : 2 );
			if ( cmbFunc )
				flags = flags & 0xffe3 | ( cmbFunc->currentIndex() << 2 );
			nif->set<int>( index, flags );
		}
	}
	
	QCheckBox * dlgCheck( QVBoxLayout * vbox, const QString & name, QCheckBox * chk = 0 )
	{
		QCheckBox * box = new QCheckBox( name );
		vbox->addWidget( box );
		if ( chk )
		{
			QObject::connect( chk, SIGNAL( toggled( bool ) ), box, SLOT( setEnabled( bool ) ) );
			box->setEnabled( chk->isChecked() );
		}
		return box;
	}
	
	QComboBox * dlgCombo( QVBoxLayout * vbox, const QString & name, const char * items[], int numitems, QCheckBox * chk = 0 )
	{
		vbox->addWidget( new QLabel( name ) );
		QComboBox * cmb = new QComboBox;
		vbox->addWidget( cmb );
		for ( int c = 0; c < numitems; c++ )
			cmb->addItem( items[c] );
		if ( chk )
		{
			QObject::connect( chk, SIGNAL( toggled( bool ) ), cmb, SLOT( setEnabled( bool ) ) );
			cmb->setEnabled( chk->isChecked() );
		}
		return cmb;
	}
	
	QSpinBox * dlgSpin( QVBoxLayout * vbox, const QString & name, int min, int max, QCheckBox * chk = 0 )
	{
		vbox->addWidget( new QLabel( name ) );
		QSpinBox * spn = new QSpinBox;
		vbox->addWidget( spn );
		spn->setRange( min, max );
		if ( chk )
		{
			QObject::connect( chk, SIGNAL( toggled( bool ) ), spn, SLOT( setEnabled( bool ) ) );
			spn->setEnabled( chk->isChecked() );
		}
		return spn;
	}
	
	void dlgButtons( QDialog * dlg, QVBoxLayout * vbox )
	{
		QHBoxLayout * hbox = new QHBoxLayout;
		vbox->addLayout( hbox );
		
		QPushButton * btAccept = new QPushButton( "Accept" );
		hbox->addWidget( btAccept );
		QObject::connect( btAccept, SIGNAL( clicked() ), dlg, SLOT( accept() ) );
		
		QPushButton * btReject = new QPushButton( "Cancel" );
		hbox->addWidget( btReject );
		QObject::connect( btReject, SIGNAL( clicked() ), dlg, SLOT( reject() ) );
	}
};

REGISTER_SPELL( spEditFlags )
