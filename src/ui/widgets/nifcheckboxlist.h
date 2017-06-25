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

#ifndef NIFCHECKBOXLIST_H
#define NIFCHECKBOXLIST_H

#include <QComboBox>     // Inherited
#include <QItemDelegate> // Inherited
#include <QLineEdit>     // Inherited
#include <QValidator>    // Inherited
#include <QPointer>

class QWidget;


// Original Implementation by:  da-crystal
//   http://da-crystal.net/GCMS/blog/checkboxlist-in-qt/

class CheckBoxListDelegate final : public QItemDelegate
{
public:
	CheckBoxListDelegate( QObject * parent );
	~CheckBoxListDelegate();

	void paint( QPainter * painter, const QStyleOptionViewItem & option,
	            const QModelIndex & index ) const override final;

	QWidget * createEditor( QWidget * parent,
	                        const QStyleOptionViewItem & option,
							const QModelIndex & index ) const override final;

	void setEditorData( QWidget * editor,
	                    const QModelIndex & index ) const override final;

	void setModelData( QWidget * editor, QAbstractItemModel * model,
	                   const QModelIndex & index ) const override final;

	void updateEditorGeometry( QWidget * editor,
	                           const QStyleOptionViewItem & option, const QModelIndex & index ) const override final;
};


class CheckBoxList : public QComboBox
{
	Q_OBJECT

public:
	CheckBoxList( QWidget * widget = nullptr );
	virtual ~CheckBoxList();
	bool eventFilter( QObject * object, QEvent * event ) override final;
	void paintEvent( QPaintEvent * ) override final;

	virtual void updateText() {}
};

//////////////////////////////////////////////////////////////////////////

class NifCheckListBoxEditor final : public QLineEdit
{
	Q_OBJECT

public:
	explicit NifCheckListBoxEditor( QWidget * parent );

	bool hasFocus() const;

protected:
	void focusInEvent( QFocusEvent * e ) override final;
	void focusOutEvent( QFocusEvent * e ) override final;

private:
	Q_DISABLE_COPY( NifCheckListBoxEditor )
	bool inFocus;
};

//////////////////////////////////////////////////////////////////////////

class NifCheckBoxListValidator final : public QValidator
{
	Q_OBJECT

public:
	explicit NifCheckBoxListValidator( NifCheckListBoxEditor * edit );

	State validate( QString &, int & ) const override final;
	void fixup( QString & ) const override final;

private:
	Q_DISABLE_COPY( NifCheckBoxListValidator )
	NifCheckListBoxEditor * edit;
};

//////////////////////////////////////////////////////////////////////////

class NifCheckBoxList : public CheckBoxList
{
	Q_OBJECT

public:
	NifCheckBoxList( QWidget * widget = nullptr );
	virtual ~NifCheckBoxList();

signals:
	void dataChanged();

protected:
	void updateText() override final;
	void parseText( const QString & text );

private slots:
	void sltDataChanged( const QModelIndex &, const QModelIndex & );
	void sltEditingFinished ();

private:
	QPointer<NifCheckBoxListValidator> validator;
};


#endif
