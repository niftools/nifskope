#include "../spellbook.h"

#include <QDebug>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

/* XPM */
static char * txt_xpm[] = {
"32 32 36 1",
" 	c None",
".	c #FFFFFF","+	c #000000","@	c #BDBDBD","#	c #717171","$	c #252525",
"%	c #4F4F4F","&	c #A9A9A9","*	c #A8A8A8","=	c #555555","-	c #EAEAEA",
";	c #151515",">	c #131313",",	c #D0D0D0","'	c #AAAAAA",")	c #080808",
"!	c #ABABAB","~	c #565656","{	c #D1D1D1","]	c #4D4D4D","^	c #4E4E4E",
"/	c #FDFDFD","(	c #A4A4A4","_	c #0A0A0A",":	c #A5A5A5","<	c #050505",
"[	c #C4C4C4","}	c #E9E9E9","|	c #D5D5D5","1	c #141414","2	c #3E3E3E",
"3	c #DDDDDD","4	c #424242","5	c #070707","6	c #040404","7	c #202020",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
" ...........          ....      ",
" .+++++++++.         .@#$.      ",
" .+++++++++.         .+++.      ",
" ....+++..............+++...    ",
"    .+++.   %++&.*++=++++++.    ",
"    .+++.  .-;+>,>+;-++++++.    ",
"    .+++.   .'++)++!..+++...    ",
"    .+++.    .=+++~. .+++.      ",
"    .+++.    .{+++{. .+++.      ",
"    .+++.    .]+++^. .+++/      ",
"    .+++.   .(++_++:..<++[..    ",
"    .+++.  .}>+;|;+1}.2++++.    ",
"    .+++.   ^++'.'++%.34567.    ",
"    .....  .................    ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                "};

QIcon * txt_xpm_icon = 0;

class spEditStringOffset : public Spell
{
public:
	QString name() const { return "Edit String Offset"; }
	QString page() const { return ""; }
	QIcon icon() const
	{
		if ( ! txt_xpm_icon )
			txt_xpm_icon = new QIcon( txt_xpm );
		return *txt_xpm_icon;
	}
	bool instant() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->getValue( index ).type() == NifValue::tStringOffset && getStringPalette( nif ).isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iPalette = getStringPalette( nif );
		
		QMap<QString, int> strings = readStringPalette( nif, iPalette );
		QString string = getString( strings, nif->get<int>( index ) );
		
		QDialog dlg;
		
		QLabel * lb = new QLabel( & dlg );
		lb->setText( "Select a string or enter a new one" );
		
		QListWidget * lw = new QListWidget( & dlg );
		lw->addItems( strings.keys() );
		
		QLineEdit * le = new QLineEdit( & dlg );
		le->setText( string );
		le->setFocus();
		
		QObject::connect( lw, SIGNAL( currentTextChanged( const QString & ) ), le, SLOT( setText( const QString & ) ) );
		QObject::connect( lw, SIGNAL( itemActivated( QListWidgetItem * ) ), & dlg, SLOT( accept() ) );
		QObject::connect( le, SIGNAL( returnPressed() ), & dlg, SLOT( accept() ) );
		
		QPushButton * bo = new QPushButton( "Ok", & dlg );
		QObject::connect( bo, SIGNAL( clicked() ), & dlg, SLOT( accept() ) );
		
		QPushButton * bc = new QPushButton( "Cancel", & dlg );
		QObject::connect( bc, SIGNAL( clicked() ), & dlg, SLOT( reject() ) );
		
		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );
		grid->addWidget( lb, 0, 0, 1, 2 );
		grid->addWidget( lw, 1, 0, 1, 2 );
		grid->addWidget( le, 2, 0, 1, 2 );
		grid->addWidget( bo, 3, 0, 1, 1 );
		grid->addWidget( bc, 3, 1, 1, 1 );
		
		if ( dlg.exec() != QDialog::Accepted )
			return index;
		
		nif->set<int>( index, addString( nif, iPalette, le->text() ) );
		
		return index;
	}
	
	static QModelIndex getStringPalette( const NifModel * nif )
	{
		QModelIndex iPalette;
		for ( int n = 0; n < nif->getBlockCount(); n++ )
		{
			QModelIndex idx = nif->getBlock( n, "NiStringPalette" );
			if ( idx.isValid() )
			{
				if ( ! iPalette.isValid() )
					iPalette = idx;
				else
					return QModelIndex();
			}
		}
		return iPalette;
	}
	
	static QMap<QString,int> readStringPalette( const NifModel * nif, const QModelIndex & iPalette )
	{
		QByteArray bytes = nif->get<QByteArray>( iPalette, "Palette" );
		QMap<QString,int> strings;
		int x = 0;
		while ( x < bytes.count() )
		{
			QString s( & bytes.data()[x] );
			strings.insert( s, x );
			x += s.length() + 1;
		}
		return strings;
	}
	
	static QString getString( const QMap<QString,int> strings, int ofs )
	{
		if ( ofs >= 0 )
		{
			QMapIterator<QString,int> it( strings );
			while ( it.hasNext() )
			{
				it.next();
				if ( ofs == it.value() )
					return it.key();
			}
		}
		return QString();
	}
	
	static int addString( NifModel * nif, const QModelIndex & iPalette, const QString & string )
	{
		if ( string.isEmpty() )
			return 0xffffffff;
		
		QMap<QString, int> strings = readStringPalette( nif, iPalette );
		if ( strings.contains( string ) )
			return strings[ string ];
		
		QByteArray bytes = nif->get<QByteArray>( iPalette, "Palette" );
		int ofs = bytes.count();
		bytes += string.toAscii();
		bytes.append( '\0' );
		nif->set<QByteArray>( iPalette, "Palette", bytes );
		return ofs;
	}
};

REGISTER_SPELL( spEditStringOffset )

