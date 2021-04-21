#include "spellbook.h"

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>


// Brief description is deliberately not autolinked to class Spell
/*! \file headerstring.cpp
 * \brief Header string editing spells (spEditStringIndex)
 *
 * All classes here inherit from the Spell class.
 */

/* XPM */
static char const * txt_xpm[] = {
	"32 32 36 1",
	"   c None",
	".	c #FFFFFF", "+	c #000000", "@	c #BDBDBD", "#	c #717171", "$	c #252525",
	"%	c #4F4F4F", "&	c #A9A9A9", "*	c #A8A8A8", "=	c #555555", "-	c #EAEAEA",
	";	c #151515", ">	c #131313", ",	c #D0D0D0", "'	c #AAAAAA", ")	c #080808",
	"!	c #ABABAB", "~	c #565656", "{	c #D1D1D1", "]	c #4D4D4D", "^	c #4E4E4E",
	"/	c #FDFDFD", "(	c #A4A4A4", "_	c #0A0A0A", ":	c #A5A5A5", "<	c #050505",
	"[	c #C4C4C4", "}	c #E9E9E9", "|	c #D5D5D5", "1	c #141414", "2	c #3E3E3E",
	"3	c #DDDDDD", "4	c #424242", "5	c #070707", "6	c #040404", "7	c #202020",
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
	"                                "
};

static QIconPtr txt_xpm_icon = nullptr;

//! Edit the index of a header string
class spEditStringIndex final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit String Index" ); }
	QString page() const override final { return Spell::tr( "" ); }
	QIcon icon() const override final
	{
		if ( !txt_xpm_icon )
			txt_xpm_icon = QIconPtr( new QIcon(QPixmap( txt_xpm )) );

		return *txt_xpm_icon;
	}
	bool instant() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		NifValue::Type type = nif->getValue( index ).type();

		if ( type == NifValue::tStringIndex )
			return true;

		if ( (type == NifValue::tString || type == NifValue::tFilePath) && nif->checkVersion( 0x14010003, 0 ) )
			return true;

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		int offset = nif->get<int>( index );
		QStringList strings;
		QString string;

		if ( nif->getValue( index ).type() != NifValue::tStringIndex || !nif->checkVersion( 0x14010003, 0 ) )
			return index;

		QModelIndex header = nif->getHeader();
		QVector<QString> stringVector = nif->getArray<QString>( header, "Strings" );
		strings = stringVector.toList();

		if ( offset >= 0 && offset < stringVector.size() )
			string = stringVector.at( offset );

		QDialog dlg;

		QLabel * lb = new QLabel( &dlg );
		lb->setText( Spell::tr( "Select a string or enter a new one" ) );

		QListWidget * lw = new QListWidget( &dlg );
		lw->addItems( strings );

		QLineEdit * le = new QLineEdit( &dlg );
		le->setText( string );
		le->setFocus();

		QObject::connect( lw, &QListWidget::currentTextChanged, le, &QLineEdit::setText );
		QObject::connect( lw, &QListWidget::itemActivated, &dlg, &QDialog::accept );
		QObject::connect( le, &QLineEdit::returnPressed, &dlg, &QDialog::accept );

		QPushButton * bo = new QPushButton( Spell::tr( "Ok" ), &dlg );
		QObject::connect( bo, &QPushButton::clicked, &dlg, &QDialog::accept );

		QPushButton * bc = new QPushButton( Spell::tr( "Cancel" ), &dlg );
		QObject::connect( bc, &QPushButton::clicked, &dlg, &QDialog::reject );

		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );
		grid->addWidget( lb, 0, 0, 1, 2 );
		grid->addWidget( lw, 1, 0, 1, 2 );
		grid->addWidget( le, 2, 0, 1, 2 );
		grid->addWidget( bo, 3, 0, 1, 1 );
		grid->addWidget( bc, 3, 1, 1, 1 );

		if ( dlg.exec() != QDialog::Accepted )
			return index;

		nif->set<QString>( index, le->text() );

		return index;
	}
};

REGISTER_SPELL( spEditStringIndex )

