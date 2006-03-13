#include "message.h"

inline void space( QString & s )
{
	if ( ! s.isEmpty() )
		s += " ";
}

template <> Message & Message::operator<<( const char * x )
{
	s += x;
	return *this;
}

template <> Message & Message::operator<<( QString x )
{
	space( s );
	s += "\"" + x + "\"";
	return *this;
}

template <> Message & Message::operator<<( QByteArray x )
{
	space( s );
	s += "\"" + x + "\"";
	return *this;
}

template <> Message & Message::operator<<( int x )
{
	space( s );
	s += QString::number( x );
	return *this;
}

template <> Message & Message::operator<<( unsigned int x )
{
	space( s );
	s += QString::number( x );
	return *this;
}

template <> Message & Message::operator<<( double x )
{
	space( s );
	s += QString::number( x );
	return *this;
}

