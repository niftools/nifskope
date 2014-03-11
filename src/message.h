#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMetaType>
#include <QString>


class Message
{
public:
	Message( QtMsgType t = QtWarningMsg ) : typ( t ) {}
	
	template <typename T> Message & operator<<( T );
	
	operator QString () const { return s; }
	
	QtMsgType type() const { return typ; }

protected:
	QString s;
	QtMsgType typ;
};

#define DbgMsg() Message( QtDebugMsg )

#endif
