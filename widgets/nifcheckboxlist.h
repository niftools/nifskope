/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
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

#include "../nifvalue.h"

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QItemDelegate>
#include <QPointer>
#include <QApplication>
#include <QStandardItem>

#include "../nifmodel.h"

// Original Implementation by:  da-crystal
//   http://da-crystal.net/GCMS/blog/checkboxlist-in-qt/

class CheckBoxListDelegate : public QItemDelegate
{
public:
    CheckBoxListDelegate(QObject *parent);
	 ~CheckBoxListDelegate();
 
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
				  const QModelIndex &index) const;
 
    QWidget *createEditor(QWidget *parent,
		 const QStyleOptionViewItem & option ,
		 const QModelIndex & index ) const;
 
	 void setEditorData(QWidget *editor,
										 const QModelIndex &index) const;
 
	 void setModelData(QWidget *editor, QAbstractItemModel *model,
										const QModelIndex &index) const;
 
	 void updateEditorGeometry(QWidget *editor,
		 const QStyleOptionViewItem &option, const QModelIndex &index ) const;
 };
 

class CheckBoxList: public QComboBox
{
	Q_OBJECT;
 
public:
	CheckBoxList(QWidget *widget = 0);
	virtual ~CheckBoxList();
	bool eventFilter(QObject *object, QEvent *event);
	virtual void paintEvent(QPaintEvent *);
	
	virtual void updateText() {}
};    

//////////////////////////////////////////////////////////////////////////

class NifCheckListBoxEditor : public QLineEdit
{
	Q_OBJECT
public:
	explicit NifCheckListBoxEditor(QWidget* parent);

	bool hasFocus() const;

protected:
	virtual void focusInEvent(QFocusEvent * e);
	virtual void focusOutEvent(QFocusEvent * e);

private:
	Q_DISABLE_COPY(NifCheckListBoxEditor);
	bool inFocus;
};

//////////////////////////////////////////////////////////////////////////

class NifCheckBoxListValidator : public QValidator
{
	Q_OBJECT
public:
	explicit NifCheckBoxListValidator(NifCheckListBoxEditor* edit);

	virtual State validate(QString &, int &) const;
	virtual void fixup(QString &) const;

private:
	Q_DISABLE_COPY(NifCheckBoxListValidator);
	NifCheckListBoxEditor* edit;
};

//////////////////////////////////////////////////////////////////////////

class NifCheckBoxList : public CheckBoxList
{
	Q_OBJECT;
public:
	NifCheckBoxList(QWidget *widget = 0);
	virtual ~NifCheckBoxList();

protected:
	void updateText();
	void parseText(const QString& text);

private slots:
	void sltDataChanged( const QModelIndex &, const QModelIndex & );
	void sltEditingFinished ();

private:
	QPointer<NifCheckBoxListValidator> validator;
};


#endif
