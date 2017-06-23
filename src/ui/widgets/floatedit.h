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

#ifndef FLOATEDIT_H
#define FLOATEDIT_H

#include <QDoubleValidator> // Inherited
#include <QLineEdit>        // Inherited


class FloatValidator final : public QDoubleValidator
{
	Q_OBJECT

public:
	FloatValidator( QObject * parent = nullptr )
		: QDoubleValidator( parent )
	{}
	FloatValidator( double bottom, double top, int decimals, QObject * parent = nullptr )
		: QDoubleValidator( bottom, top, decimals, parent )
	{}

	QValidator::State validate( QString & input, int & pos ) const override final;

	bool canMin() const;
	bool canMax() const;
};

class FloatEdit final : public QLineEdit
{
	Q_OBJECT

public:
	FloatEdit( QWidget * = nullptr );

	float value() const;

	void setRange( float minimum, float maximum );

	bool isMin() const;
	bool isMax() const;

protected:
	void contextMenuEvent( class QContextMenuEvent * event ) override final;

signals:
	void sigEdited( float );
	void sigEdited( const QString & );
	void valueChanged( float );
	void valueChanged( const QString & );

public slots:
	void setValue( float val );
	void set( float val, float n, float x );
	void setMin();
	void setMax();

protected slots:
	void edited();

protected:
	float val;
	FloatValidator * validator;
	class QAction * actMin;
	class QAction * actMax;
};

#endif
