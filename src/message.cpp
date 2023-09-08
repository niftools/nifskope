#include "message.h"

#include <QApplication>
#include <QAbstractButton>
#include <QMap>
#include <QCloseEvent>
#include <QScreen>


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
QMessageBox* Message::message( QWidget * parent, const QString & str, QMessageBox::Icon icon )
{
	auto msgBox = new QMessageBox( parent );
	msgBox->setWindowFlags( msgBox->windowFlags() | Qt::Tool );
	msgBox->setAttribute( Qt::WA_DeleteOnClose );
	msgBox->setWindowModality( Qt::NonModal );

	msgBox->setText( str );
	msgBox->setIcon( icon );

	msgBox->show();

	msgBox->activateWindow();

	return msgBox;
}

//! Static helper for message box with detail text
QMessageBox* Message::message( QWidget * parent, const QString & str, const QString & err, QMessageBox::Icon icon )
{
	if ( !parent )
		parent = qApp->activeWindow();

	auto msgBox = new QMessageBox( parent );
	msgBox->setAttribute( Qt::WA_DeleteOnClose );
	msgBox->setWindowModality( Qt::NonModal );
	msgBox->setWindowFlags( msgBox->windowFlags() | Qt::Tool );

	msgBox->setText( str );
	msgBox->setIcon( icon );
	msgBox->setDetailedText( err );

	msgBox->show();

	msgBox->activateWindow();

	return msgBox;
}

//! Static helper for installed message handler
void Message::message( QWidget * parent, const QString & str, const QMessageLogContext * context, QMessageBox::Icon icon )
{

#ifdef QT_NO_DEBUG
	if ( !QString( context->category ).startsWith( "nifskope", Qt::CaseInsensitive ) ) {
		
		// TODO: Log these into a log table widget like Qt Creator's issues tab

		return;
	}
#endif

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

class DetailsMessageBox : public QMessageBox
{
public:
	explicit DetailsMessageBox( QWidget * parent, const QString & txt )
		: QMessageBox( parent ), m_key( txt ) { }

	~DetailsMessageBox();

	const QString & key() const { return m_key; }

	int detailsCount() const { return m_nDetails; }
	void updateDetailsCount() { m_nDetails++; }

protected:
	void closeEvent( QCloseEvent * event ) override;

	QString m_key;
	int m_nDetails = 0;
};

static QVector<DetailsMessageBox *> messageBoxes;

void unregisterMessageBox( DetailsMessageBox * msgBox ) 
{
	messageBoxes.removeOne( msgBox );
}

void Message::append( QWidget * parent, const QString & str, const QString & err, QMessageBox::Icon icon )
{
	if ( !parent )
		parent = qApp->activeWindow();

	// Create one box per parent widget and error string, accumulate messages
	DetailsMessageBox * msgBox = nullptr;
	for ( auto box : messageBoxes ) {
		if ( box->parentWidget() == parent && box->key() == str ) {
			msgBox = box;
			break;
		}
	}

	if ( msgBox ) {
		// Limit the number of detail lines to MAX_DETAILS_COUNTER
		// because when errors are spammed at hundreds or thousands in a row, this makes NifSkope unresponsive.
		const int MAX_DETAILS_COUNTER = 50;

		if ( !err.isEmpty() && msgBox->detailsCount() <= MAX_DETAILS_COUNTER ) {
			// Append strings to existing message box's Detailed Text
			// Show box if it has been closed before
			msgBox->show();
			QString newLine = ( msgBox->detailsCount() < MAX_DETAILS_COUNTER ) ? (err + "\n") : QString("...\n");
			msgBox->setDetailedText( msgBox->detailedText().append( newLine ) );
			msgBox->updateDetailsCount();
		}

	} else {
		// Create new message box
		msgBox = new DetailsMessageBox( parent, str );
		messageBoxes.append( msgBox );

		msgBox->setAttribute( Qt::WA_DeleteOnClose );
		msgBox->setWindowModality( Qt::NonModal );
		msgBox->setWindowFlags( msgBox->windowFlags() | Qt::Tool );

		// Set the min. width of the label containing str to a quarter of the screen resolution.
		// This makes the detailed text more readable even when str is short.
		auto screen = QGuiApplication::primaryScreen();
		if ( screen ) {
			msgBox->setStyleSheet( 
				QString(" QLabel[objectName^=\"qt_msgbox_label\"]{min-width: %1px;}" )
				.arg ( screen->size().width() / 4 )
			);
		}

		msgBox->setText( str );
		msgBox->setIcon( icon );

		if ( !err.isEmpty() ) {
			msgBox->setDetailedText( err + "\n" );
			msgBox->updateDetailsCount();

			// Auto-show detailed text on first show.
			// https://stackoverflow.com/questions/36083551/qmessagebox-show-details
			for ( auto btn : msgBox->buttons() ) {
				if ( msgBox->buttonRole( btn ) == QMessageBox::ActionRole ) {
					btn->click(); // "Click" it to expand the detailed text
					break;
				}
			}
		}

		msgBox->show();
		msgBox->activateWindow();

		// Clear Detailed Text with each confirmation
		connect( msgBox, &QMessageBox::buttonClicked, [msgBox]( QAbstractButton * button ) { 
			Q_UNUSED( button );
			unregisterMessageBox( msgBox );
		} );
	}
}

void Message::append( const QString & str, const QString & err, QMessageBox::Icon icon )
{
	append( nullptr, str, err, icon );
}

DetailsMessageBox::~DetailsMessageBox()
{
	unregisterMessageBox( this ); // Just in case if buttonClicked or closeEvent fail
}

void DetailsMessageBox::closeEvent( QCloseEvent * event )
{	
	QMessageBox::closeEvent( event );
	if ( event->isAccepted() )
		unregisterMessageBox( this );
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
	s += x; //"\"" + x + "\"";
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
