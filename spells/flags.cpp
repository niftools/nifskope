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
	QString name() const { return "Edit"; }
	QString page() const { return "Flags"; }
	QIcon icon() const
	{
		if ( ! flag42_xpm_icon )
			flag42_xpm_icon = new QIcon( flag42_xpm );
		return *flag42_xpm_icon;
	}
	bool instant() const { return true; }
	
	enum FlagType
	{
		Alpha, Node, Controller, None
	};
	
	FlagType queryType( const NifModel * nif, const QModelIndex & index ) const
	{
		if ( nif->itemValue( index ).type() == NifValue::tFlags )
		{
			QString name = nif->itemName( index.parent() );
			if ( name == "NiAlphaProperty" )
				return Alpha;
		}
		return None;
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return queryType( nif, index ) != None;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		switch ( queryType( nif, index ) )
		{
			case Alpha:
				alphaFlags( nif, index );
				break;
			default:
				break;
		}
		return index;
	}
	
	void alphaFlags( NifModel * nif, const QModelIndex & index )
	{
		quint16 flags = nif->get<int>( index );
		if ( ! flags ) flags = 237;
		
		static const char * blendModes[11] = {
			"One", "Zero", "Src Color", "Inv Src Color", "Dst Color", "Inv Dst Color", "Src Alpha", "Inv Src Alpha",
			"Dst Alpha", "Inv Dst Alpha", "Src Alpha Saturate"
		};
		static const char * testModes[8] = {
			"Always", "Less", "Equal", "Less or Equal", "Greater", "Not Equal", "Less or Equal", "Never"
		};
		
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );
		
		QCheckBox * chkBlend = new QCheckBox( "Enable Blending" );
		vbox->addWidget( chkBlend );
		chkBlend->setChecked( flags & 1 );
		
		vbox->addWidget( new QLabel( "Source Blend Mode" ) );
		
		QComboBox * cmbSrc = new QComboBox;
		vbox->addWidget( cmbSrc );
		for ( int c = 0; c < 11; c++ )
			cmbSrc->addItem( blendModes[c] );
		cmbSrc->setCurrentIndex( flags >> 1 & 0x0f );
		QObject::connect( chkBlend, SIGNAL( toggled( bool ) ), cmbSrc, SLOT( setEnabled( bool ) ) );
		cmbSrc->setEnabled( chkBlend->isChecked() );
		
		vbox->addWidget( new QLabel( "Destination Blend Mode" ) );
		
		QComboBox * cmbDst = new QComboBox;
		vbox->addWidget( cmbDst );
		for ( int c = 0; c < 11; c++ )
			cmbDst->addItem( blendModes[c] );
		cmbDst->setCurrentIndex( flags >> 5 & 0x0f );
		QObject::connect( chkBlend, SIGNAL( toggled( bool ) ), cmbDst, SLOT( setEnabled( bool ) ) );
		cmbDst->setEnabled( chkBlend->isChecked() );
		
		QCheckBox * chkTest = new QCheckBox( "Enable Testing" );
		vbox->addWidget( chkTest );
		chkTest->setChecked( flags & ( 1 << 9 ) );
		
		vbox->addWidget( new QLabel( "Alpha Test Function" ) );
		
		QComboBox * cmbTest = new QComboBox;
		vbox->addWidget( cmbTest );
		for ( int c = 0; c < 8; c++ )
			cmbTest->addItem( testModes[c] );
		cmbTest->setCurrentIndex( flags >> 10 & 0x07 );
		QObject::connect( chkTest, SIGNAL( toggled( bool ) ), cmbTest, SLOT( setEnabled( bool ) ) );
		cmbTest->setEnabled( chkTest->isChecked() );
		
		vbox->addWidget( new QLabel( "Alpha Test Threshold" ) );
		
		QSpinBox * spnTest = new QSpinBox;
		vbox->addWidget( spnTest );
		spnTest->setRange( 0, 0xff );
		spnTest->setValue( nif->get<int>( nif->getBlock( index ), "Threshold" ) );
		QObject::connect( chkTest, SIGNAL( toggled( bool ) ), spnTest, SLOT( setEnabled( bool ) ) );
		spnTest->setEnabled( chkTest->isChecked() );
		
		dlgButtons( &dlg, vbox );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			flags = 0;
			if ( chkBlend->isChecked() )
			{
				flags |= 1;
				flags |= cmbSrc->currentIndex() << 1;
				flags |= cmbDst->currentIndex() << 5;
			}
			if ( chkTest->isChecked() )
			{
				flags |= 1 << 9;
				flags |= cmbTest->currentIndex() << 10;
				nif->set<int>( nif->getBlock( index ), "Threshold", spnTest->value() );
			}
			nif->set<int>( index, flags );
		}
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
