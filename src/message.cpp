#include "message.h"

#include <QAbstractButton>
#include <QMap>


Q_LOGGING_CATEGORY( ns, "nifskope" )
Q_LOGGING_CATEGORY( nsGl, "nifskope.gl" )
Q_LOGGING_CATEGORY( nsIo, "nifskope.io" )
Q_LOGGING_CATEGORY( nsNif, "nifskope.nif" )
Q_LOGGING_CATEGORY( nsSpell, "nifskope.spell" )


Message::Message() : QObject( nullptr )
{

}

Message::~Message()
{

}

//! Static helper for message box without detail text
void Message::message( QWidget * parent, const QString & str, QMessageBox::Icon icon )
{
	auto msgBox = new QMessageBox( parent );

	msgBox->setText( str );
	msgBox->setIcon( icon );
	msgBox->setWindowFlags( msgBox->windowFlags() | Qt::WindowStaysOnTopHint );

	msgBox->open();
}

//! Static helper for message box with detail text
void Message::message( QWidget * parent, const QString & str, const QString & err, QMessageBox::Icon icon )
{
	auto msgBox = new QMessageBox( parent );

	msgBox->setText( str );
	msgBox->setIcon( icon );
	msgBox->setWindowFlags( msgBox->windowFlags() | Qt::WindowStaysOnTopHint );
	msgBox->setDetailedText( err );

	msgBox->open();
}

//! Static helper for installed message handler
void Message::message( QWidget * parent, const QString & str, const QMessageLogContext * context, QMessageBox::Icon icon )
{
	QString d;
	d.append( QString( "%1: %2\n" ).arg( "File" ).arg( context->file ) );
	d.append( QString( "%1: %2\n" ).arg( "Function" ).arg( context->function ) );
	d.append( QString( "%1: %2\n" ).arg( "Line" ).arg( context->line ) );
	d.append( QString( "%1: %2\n" ).arg( "Category" ).arg( context->category ) );
	d.append( QString( "%1:\n\n%2" ).arg( "Message" ).arg( str ) );

	message( parent, str, d, icon );
}

/*
* Critical message boxes
*
*/

void Message::critical( QWidget * parent, const QString & str )
{
	message( parent, str, QMessageBox::Critical );
}

void Message::critical( QWidget * parent, const QString & str, const QString & err )
{
	message( parent, str, err, QMessageBox::Critical );
}

/*
* Warning message boxes
*
*/

void Message::warning( QWidget * parent, const QString & str )
{
	message( parent, str, QMessageBox::Warning );
}

void Message::warning( QWidget * parent, const QString & str, const QString & err )
{
	message( parent, str, err, QMessageBox::Warning );
}

/*
* Info message boxes
*
*/

void Message::info( QWidget * parent, const QString & str )
{
	message( parent, str, QMessageBox::Information );
}

void Message::info( QWidget * parent, const QString & str, const QString & err )
{
	message( parent, str, err, QMessageBox::Information );
}

/*
* Accumulate messages in detailed text area
*
*/

static QMap<QString, QMessageBox *> messageBoxes;

void Message::append( const QString & str, const QString & err, QMessageBox::Icon icon )
{
	// Create one box per error string, accumulate messages
	auto box = messageBoxes[str];
	if ( box ) {
		// Append strings to existing message box's Detailed Text
		// Show box if it has been closed before
		box->show();
		box->setDetailedText( box->detailedText().append( err + "\n" ) );
	} else {
		// Create new message box
		auto msgBox = new QMessageBox( nullptr );

		msgBox->setText( str );
		msgBox->setIcon( icon );
		msgBox->setDetailedText( err + "\n" );
		msgBox->setWindowFlags( msgBox->windowFlags() | Qt::WindowStaysOnTopHint );
		msgBox->open();

		messageBoxes[str] = msgBox;

		// Clear Detailed Text with each confirmation
		connect( msgBox, &QMessageBox::buttonClicked, [msgBox]( QAbstractButton * button ) { 
			msgBox->setDetailedText( "" );
		} );
	}
}



/*
 Old message class
*/

inline void space( QString & s )
{
	if ( !s.isEmpty() )
		s += " ";
}

template <> TestMessage & TestMessage::operator<<(const char * x)
{
	space( s );
	s += x;
	return *this;
}

template <> TestMessage & TestMessage::operator<<(QString x)
{
	space( s );
	s += "\"" + x + "\"";
	return *this;
}

template <> TestMessage & TestMessage::operator<<(QByteArray x)
{
	space( s );
	s += "\"" + x + "\"";
	return *this;
}

template <> TestMessage & TestMessage::operator<<(int x)
{
	space( s );
	s += QString::number( x );
	return *this;
}

template <> TestMessage & TestMessage::operator<<(unsigned int x)
{
	space( s );
	s += QString::number( x );
	return *this;
}

template <> TestMessage & TestMessage::operator<<(double x)
{
	space( s );
	s += QString::number( x );
	return *this;
}

template <> TestMessage & TestMessage::operator<<(float x)
{
	space( s );
	s += QString::number( x );
	return *this;
}
