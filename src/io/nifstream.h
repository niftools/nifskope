/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef NIFSTREAM_H
#define NIFSTREAM_H

#include <QCoreApplication>

#include <memory>


//! @file nifstream.h NifIStream, NifOStream, NifSStream

class NifValue;
class BaseModel;
class QDataStream;
class QIODevice;


//! An input stream that reads a file into a model.
class NifIStream final
{
	Q_DECLARE_TR_FUNCTIONS( NifIStream )

public:
	NifIStream( BaseModel * m, QIODevice * d ) : model( m ), device( d )
	{
		init();
	}

	//! Reads a NifValue from the underlying device. Returns true if successful.
	bool read( NifValue & );

private:
	//! The model that data is being read into.
	BaseModel * model;
	//! The underlying device that data is being read from.
	QIODevice * device;
	//! The data stream that is wrapped around the device (simplifies endian conversion)
	std::unique_ptr<QDataStream> dataStream;

	//! Initialises the stream.
	void init();

	//! Whether a boolean is 32-bit.
	bool bool32bit = false;
	//! Whether link adjustment is required.
	bool linkAdjust = false;
	//! Whether string adjustment is required.
	bool stringAdjust = false;
	//! Whether the model is big-endian
	bool bigEndian = false;

	//! The maximum length of a string that can be read.
	int maxLength = 0x8000;
};


//! An output stream that writes a model to a file.
class NifOStream final
{
	Q_DECLARE_TR_FUNCTIONS( NifOStream )

public:
	NifOStream( const BaseModel * n, QIODevice * d ) : model( n ), device( d ) { init(); }

	//! Writes a NifValue to the underlying device. Returns true if successful.
	bool write( const NifValue & );

private:
	//! The model that data is being read from.
	const BaseModel * model;
	//! The underlying device that data is being written to.
	QIODevice * device;

	//! Initialises the stream.
	void init();

	//! Whether a boolean is 32-bit.
	bool bool32bit = false;
	//! Whether link adjustment is required.
	bool linkAdjust = false;
	//! Whether string adjustment is required.
	bool stringAdjust = false;
	//! Whether the model is big-endian
	bool bigEndian = false;
};


//! A stream that determines the size of values in a model.
class NifSStream final
{
public:
	NifSStream( const BaseModel * n ) : model( n ) { init(); }

	//! Determine the size of a given NifValue.
	int size( const NifValue & );

private:
	//! The model that values are being sized for.
	const BaseModel * model;

	//! Initialises the stream.
	void init();

	//! Whether booleans are 32-bit or not.
	bool bool32bit;
	//! Whether string adjustment is required.
	bool stringAdjust;
};

#endif
