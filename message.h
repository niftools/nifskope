#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>
#include <QMetaType>

class Message
{
public:
	Message( QtMsgType t = QtWarningMsg ) : typ( t ) {}
	
	template <typename T> Message & operator<<( T );
	
	operator QString () const { return s; }
	operator const char * () const { return s.toAscii().data(); }
	
	QtMsgType type() const { return typ; }

protected:
	QString s;
	QtMsgType typ;
};

#define DbgMsg() Message( QtDebugMsg )

#endif
