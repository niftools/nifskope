#ifndef MESSAGE_H
#define MESSAGE_H

#include <QLoggingCategory>
#include <QMessageBox>
#include <QMetaType>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY( ns )
Q_DECLARE_LOGGING_CATEGORY( nsGl )
Q_DECLARE_LOGGING_CATEGORY( nsIo )
Q_DECLARE_LOGGING_CATEGORY( nsNif )
Q_DECLARE_LOGGING_CATEGORY( nsSpell )


class QMessageLogContext;

class Message : QObject
{
	Q_OBJECT
	Message();
	~Message();

public:
	static void message( QWidget *, const QString &, QMessageBox::Icon );
	static void message( QWidget *, const QString &, const QString &, QMessageBox::Icon );
	static void message( QWidget *, const QString &, const QMessageLogContext *, QMessageBox::Icon );

	static void append( const QString &, const QString &, QMessageBox::Icon = QMessageBox::Warning );
	static void append( QWidget *, const QString &, const QString &, QMessageBox::Icon = QMessageBox::Warning );

	static void critical( QWidget *, const QString & );
	static void critical( QWidget *, const QString &, const QString & );

	static void warning( QWidget *, const QString & );
	static void warning( QWidget *, const QString &, const QString & );

	static void info( QWidget *, const QString & );
	static void info( QWidget *, const QString &, const QString & );
};

class TestMessage
{
public:
	TestMessage( QtMsgType t = QtWarningMsg ) : typ( t ) {}

	template <typename T> TestMessage & operator<<(T);

	operator QString() const { return s; }

	QtMsgType type() const { return typ; }

protected:
	QString s;
	QtMsgType typ;

};

#endif
