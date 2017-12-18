#include "stringpalette.h"
#include "spellbook.h"

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QStringListModel>


// Brief description is deliberately not autolinked to class Spell
/*! \file stringpalette.cpp
 * \brief String palette editing spells
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

//! Edit a single offset into a string palette.
class spEditStringOffset final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit String Offset" ); }
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
		return nif->getValue( index ).type() == NifValue::tStringOffset && getStringPalette( nif, index ).isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iPalette = getStringPalette( nif, index );

		QMap<QString, int> strings = readStringPalette( nif, iPalette );
		QString string = getString( strings, nif->get<int>( index ) );

		QDialog dlg;

		QLabel * lb = new QLabel( &dlg );
		lb->setText( Spell::tr( "Select a string or enter a new one" ) );

		QListWidget * lw = new QListWidget( &dlg );
		lw->addItems( strings.keys() );

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

		nif->set<int>( index, addString( nif, iPalette, le->text() ) );

		return index;
	}

	//! Gets the string palette referred to by this string offset
	static QModelIndex getStringPalette( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iPalette = nif->getBlock( nif->getLink( index.parent(), "String Palette" ) );

		if ( iPalette.isValid() )
			return iPalette;

		return QModelIndex();
	}

	//! Reads a string palette and returns a map of strings to offsets
	static QMap<QString, int> readStringPalette( const NifModel * nif, const QModelIndex & iPalette )
	{
		QByteArray bytes = nif->get<QByteArray>( iPalette, "Palette" );
		QMap<QString, int> strings;
		int x = 0;

		while ( x < bytes.count() ) {
			QString s( &bytes.data()[x] );
			strings.insert( s, x );
			x += s.length() + 1;
		}

		return strings;
	}

	//! Gets a particular string from a string palette
	static QString getString( const QMap<QString, int> & strings, int ofs )
	{
		if ( ofs >= 0 ) {
			QMapIterator<QString, int> it( strings );

			while ( it.hasNext() ) {
				it.next();

				if ( ofs == it.value() )
					return it.key();
			}
		}

		return QString();
	}

	//! Add a string to a string palette or find the index of it if it exists
	/**
	 * \param nif The model the string palette is in
	 * \param iPalette The index of the string palette
	 * \param string The string to add or find
	 * \return The index of the string in the palette
	 */
	static int addString( NifModel * nif, const QModelIndex & iPalette, const QString & string )
	{
		if ( string.isEmpty() )
			return 0xffffffff;

		QMap<QString, int> strings = readStringPalette( nif, iPalette );

		if ( strings.contains( string ) )
			return strings[ string ];

		QByteArray bytes = nif->get<QByteArray>( iPalette, "Palette" );
		int ofs = bytes.count();
		bytes += string.toLatin1();
		bytes.append( '\0' );
		nif->set<QByteArray>( iPalette, "Palette", bytes );
		return ofs;
	}
};

REGISTER_SPELL( spEditStringOffset )

// documented in stringpalette.h
StringPaletteRegexDialog::StringPaletteRegexDialog( NifModel * nif, QPersistentModelIndex & index, QWidget * parent ) : QDialog( parent )
{
	this->nif = nif;
	iPalette  = index;

	listview  = new QListView;
	listmodel = new QStringListModel;
	listview->setModel( listmodel );

	grid = new QGridLayout;
	setLayout( grid );

	search  = new QLineEdit( this );
	replace = new QLineEdit( this );

	QLabel * title = new QLabel( this );
	title->setText( Spell::tr( "Entries in the string palette" ) );
	QLabel * subTitle = new QLabel( this );
	subTitle->setText( Spell::tr( "Enter a pair of regular expressions to search and replace." ) );
	QLabel * refText = new QLabel( this );
	refText->setText(
		Spell::tr( "See <a href='%1'>%2</a> for syntax." ).arg( "http://doc.trolltech.com/latest/qregexp.html", "http://doc.trolltech.com/latest/qregexp.html" )
	);
	QLabel * searchText = new QLabel( this );
	searchText->setText( Spell::tr( "Search:" ) );
	QLabel * replaceText = new QLabel( this );
	replaceText->setText( Spell::tr( "Replace:" ) );

	QPushButton * ok = new QPushButton( Spell::tr( "Ok" ), this );
	QPushButton * cancel  = new QPushButton( Spell::tr( "Cancel" ), this );
	QPushButton * preview = new QPushButton( Spell::tr( "Preview" ), this );

	QObject::connect( ok, &QPushButton::clicked, this, &StringPaletteRegexDialog::accept );
	QObject::connect( cancel, &QPushButton::clicked, this, &StringPaletteRegexDialog::reject );
	QObject::connect( preview, &QPushButton::clicked, this, &StringPaletteRegexDialog::stringlistRegex );

	int currentRow = 0;
	grid->addWidget( title, currentRow, 0, 1, 3 );
	currentRow++;
	grid->addWidget( listview, currentRow, 0, 1, 3 );
	currentRow++;
	grid->addWidget( subTitle, currentRow, 0, 1, 3 );
	currentRow++;
	grid->addWidget( refText, currentRow, 0, 1, 3 );
	currentRow++;
	grid->addWidget( searchText, currentRow, 0, 1, 1 );
	grid->addWidget( search, currentRow, 1, 1, 2 );
	currentRow++;
	grid->addWidget( replaceText, currentRow, 0, 1, 1 );
	grid->addWidget( replace, currentRow, 1, 1, 2 );
	currentRow++;
	grid->addWidget( ok, currentRow, 0, 1, 1 );
	grid->addWidget( cancel, currentRow, 1, 1, 1 );
	grid->addWidget( preview, currentRow, 2, 1, 1 );
}

// documented in stringpalette.h
void StringPaletteRegexDialog::setStringList( QStringList & list )
{
	originalList = new QStringList( list );
	listmodel->setStringList( list );
}

// documented in stringpalette.h
QStringList StringPaletteRegexDialog::getStringList()
{
	stringlistRegex();
	return listmodel->stringList();
}

// documented in stringpalette.h
void StringPaletteRegexDialog::stringlistRegex()
{
	QRegularExpression replacer( search->text() );
	listmodel->setStringList( *originalList );
	listmodel->setStringList( listmodel->stringList().replaceInStrings( replacer, replace->text() ) );
}

//! Edit a string palette entry and update all references
class spEditStringEntries final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Replace Entries" ); }
	QString page() const override final { return Spell::tr( "String Palette" ); }

	bool instant() const override final { return false; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->inherits( index, "NiSequence" )
		       && nif->getBlock( nif->getLink( index, "String Palette" ) ).isValid()
		       && nif->checkVersion( 0x0A020000, 0x14000005 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		// string offset is used in ControllerLink which exists in NiSequence
		// a single palette could be share by multiple NiSequences

		QPersistentModelIndex iPalette = nif->getBlock( nif->getLink( index, "String Palette" ) );
		qDebug() << "This block uses " << iPalette;

		if ( !iPalette.isValid() ) {
			iPalette = nif->getBlock( nif->getLink( index.parent(), "String Palette" ) );

			if ( !iPalette.isValid() ) {
				qCWarning( nsSpell ) << Spell::tr( "Cannot find string palette" );
				return QModelIndex();
			}
		}

		// display entries in current string palette, in order they appear
		StringPaletteRegexDialog * sprd = new StringPaletteRegexDialog( nif, iPalette );

		QByteArray bytes = nif->get<QByteArray>( iPalette, "Palette" );

		// map of old offsets to strings
		// QMap is always sorted by key, in this case the offsets
		QMap<int, QString> oldPalette;
		int x = 0;

		while ( x < bytes.count() ) {
			QString s( &bytes.data()[x] );
			oldPalette.insert( x, s );
			x += s.length() + 1;
		}

		QList<int> oldOffsets = oldPalette.keys();

		QStringList oldEntries = oldPalette.values();

		sprd->setStringList( oldEntries );

		// display dialog
		if ( sprd->exec() != QDialog::Accepted ) {
			return index;
		}

		// get replaced entries
		QStringList newEntries = sprd->getStringList();

		//qDebug() << newEntries;

		// rebuild palette
		bytes.clear();
		x = 0;

		QMap<int, int> offsetMap;

		for ( int i = 0; i < newEntries.size(); i++ ) {
			QString s = newEntries.at( i );

			if ( s.length() == 0 ) {
				// set references to empty
				offsetMap.insert( oldOffsets[i], -1 );
			} else {
				offsetMap.insert( oldOffsets[i], x );
				bytes += s;
				bytes.append( '\0' );
				x += ( s.length() + 1 );
			}
		}

		// find all NiSequence blocks in the current model
		QList<QPersistentModelIndex> sequenceList;

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QPersistentModelIndex current = nif->getBlock( i, "NiSequence" );

			if ( current.isValid() ) {
				sequenceList.append( current );
			}
		}

		qDebug() << "Found sequences " << sequenceList;

		// find their string palettes
		QList<QPersistentModelIndex> sequenceUpdateList;
		QListIterator<QPersistentModelIndex> sequenceListIterator( sequenceList );

		while ( sequenceListIterator.hasNext() ) {
			QPersistentModelIndex temp = sequenceListIterator.next();
			QPersistentModelIndex tempPalette = nif->getBlock( nif->getLink( temp, "String Palette" ) );

			//qDebug() << "Sequence " << temp << " uses " << tempPalette;
			if ( iPalette == tempPalette ) {
				//qDebug() << "Identical to this sequence palette!";
				sequenceUpdateList.append( temp );
			}
		}

		// update all references to that palette
		QListIterator<QPersistentModelIndex> sequenceUpdateIterator( sequenceUpdateList );
		int numRefsUpdated = 0;

		while ( sequenceUpdateIterator.hasNext() ) {
			QPersistentModelIndex nextBlock = sequenceUpdateIterator.next();
			//qDebug() << "Need to update " << nextBlock;

			QPersistentModelIndex blocks = nif->getIndex( nextBlock, "Controlled Blocks" );

			for ( int i = 0; i < nif->rowCount( blocks ); i++ ) {
				QPersistentModelIndex thisBlock = blocks.child( i, 0 );

				for ( int j = 0; j < nif->rowCount( thisBlock ); j++ ) {
					if ( nif->getValue( thisBlock.child( j, 0 ) ).type() == NifValue::tStringOffset ) {
						// we shouldn't ever exceed the limit of an int, even though the type
						// is properly a uint
						int oldValue = nif->get<int>( thisBlock.child( j, 0 ) );
						qDebug() << "Index " << thisBlock.child( j, 0 )
						           << " is a string offset with name "
						           << nif->itemName( thisBlock.child( j, 0 ) )
						           << " and value "
						           << nif->get<int>( thisBlock.child( j, 0 ) );


						if ( oldValue != -1 ) {
							int newValue = offsetMap.value( oldValue );
							nif->set<int>( thisBlock.child( j, 0 ), newValue );
							numRefsUpdated++;
						}
					}
				}
			}
		}

		// update the palette itself
		nif->set<QByteArray>( iPalette, "Palette", bytes );

		Message::info( nullptr, Spell::tr( "Updated %1 offsets in %2 sequences" ).arg( numRefsUpdated ).arg( sequenceUpdateList.size() ) );

		return index;
	}
};

REGISTER_SPELL( spEditStringEntries )

//! Batch helper for spEditStringEntries
class spStringPaletteLister final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit String Palettes" ); }
	QString page() const override final { return Spell::tr( "Animation" ); }

	bool instant() const override final { return false; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( !index.isValid() && nif->checkVersion( 0x0A020000, 0x14000005 ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( index );
		QMap<QString, QModelIndex> sequenceMap;

		QList<QModelIndex> sequenceList;

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex current = nif->getBlock( i, "NiSequence" );

			if ( current.isValid() ) {
				sequenceList.append( current );
				QString key = QString( "%1 %2" ).arg( current.row(), 4, 10, QChar( '0' ) ).arg( nif->get<QString>( current, "Name" ) );
				sequenceMap.insert( key, current );
			}
		}

		// consider using QInputDialog::getItem() here, but this works
		QDialog dlg;

		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );
		int currentRow = 0;

		QLabel * title = new QLabel( &dlg );
		title->setText( Spell::tr( "Select an animation sequence to edit the string palette for" ) );
		grid->addWidget( title, currentRow, 0, 1, 2 );
		currentRow++;

		QListWidget * listWidget = new QListWidget( &dlg );
		listWidget->addItems( sequenceMap.keys() );
		QObject::connect( listWidget, &QListWidget::itemActivated, &dlg, &QDialog::accept );
		grid->addWidget( listWidget, currentRow, 0, 1, 2 );
		currentRow++;

		QPushButton * ok = new QPushButton( Spell::tr( "Ok" ), &dlg );
		QObject::connect( ok, &QPushButton::clicked, &dlg, &QDialog::accept );
		grid->addWidget( ok, currentRow, 0, 1, 1 );

		QPushButton * cancel = new QPushButton( Spell::tr( "Cancel" ), &dlg );
		QObject::connect( cancel, &QPushButton::clicked, &dlg, &QDialog::reject );
		grid->addWidget( cancel, currentRow, 1, 1, 1 );

		if ( dlg.exec() != QDialog::Accepted )
			return QModelIndex();

		spEditStringEntries * caster = new spEditStringEntries();

		caster->cast( nif, sequenceMap.value( listWidget->currentItem()->text() ) );

		return QModelIndex();
	}
};

REGISTER_SPELL( spStringPaletteLister )
