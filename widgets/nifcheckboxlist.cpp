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

#include "nifcheckboxlist.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QAbstractItemView>
#include <QStylePainter>
#include <QLineEdit>
#include <QValidator>
#include <QDebug>

#include "../options.h"

CheckBoxList::CheckBoxList( QWidget *widget )
	: QComboBox( widget )
{
	// set delegate items view
	view()->setItemDelegate(new CheckBoxListDelegate(this));

	// Enable editing on items view
	view()->setEditTriggers(QAbstractItemView::CurrentChanged);

	// set "CheckBoxList::eventFilter" as event filter for items view
	view()->viewport()->installEventFilter(this);

	// it just cool to have it as default ;)
	view()->setAlternatingRowColors(true);
}


CheckBoxList::~CheckBoxList()
{
	;
}


bool CheckBoxList::eventFilter(QObject *object, QEvent *event)
{
	// don't close items view after we release the mouse button
	// by simple eating MouseButtonRelease in viewport of items view
	if(event->type() == QEvent::MouseButtonRelease && object==view()->viewport())
	{
		return true;
	}
	return QComboBox::eventFilter(object,event);
}


void CheckBoxList::paintEvent(QPaintEvent *)
{
	QStylePainter painter(this);
	painter.setPen(palette().color(QPalette::Text));

	// draw the combobox frame, focusrect and selected etc.
	QStyleOptionComboBox opt;
	initStyleOption(&opt);
	
	if (opt.currentText.isEmpty())
		updateText();
	
	painter.drawComplexControl(QStyle::CC_ComboBox, opt);

	// draw the icon and text
	painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

//////////////////////////////////////////////////////////////////////////

CheckBoxListDelegate::CheckBoxListDelegate( QObject *parent ) : QItemDelegate(parent)
{
	//view()->setItemDelegate(new MyDelegate(this));
	//view()->setEditTriggers(QAbstractItemView::CurrentChanged);
}
CheckBoxListDelegate::~CheckBoxListDelegate()
{

}

void CheckBoxListDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
	//Get item data
	bool value = index.data(Qt::UserRole).toBool();
	QString text = index.data(Qt::DisplayRole).toString();

	// fill style options with item data
	const QStyle *style = QApplication::style();
	QStyleOptionButton opt;
	opt.state |= value ? QStyle::State_On : QStyle::State_Off;
	opt.state |= QStyle::State_Enabled;
	opt.text = text;
	opt.rect = option.rect;

	// draw item data as CheckBox
	style->drawControl(QStyle::CE_CheckBox,&opt,painter);
}

QWidget * CheckBoxListDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem & option , const QModelIndex & index ) const
{
	Q_UNUSED(index);
	Q_UNUSED(option);
	// create check box as our editor
	QCheckBox *editor = new QCheckBox(parent);
	return editor;
}

void CheckBoxListDelegate::setEditorData( QWidget *editor, const QModelIndex &index ) const
{
	//set editor data
	QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
	myEditor->setText(index.data(Qt::DisplayRole).toString());
	myEditor->setChecked(index.data(Qt::UserRole).toBool());
}

void CheckBoxListDelegate::setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
{
	//get the value from the editor (CheckBox)
	QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
	bool value = myEditor->isChecked();

	//set model data
	QMap<int,QVariant> data;
	data.insert(Qt::DisplayRole,myEditor->text());
	data.insert(Qt::UserRole,value);
	model->setItemData(index,data);
}

void CheckBoxListDelegate::updateEditorGeometry( QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}

//////////////////////////////////////////////////////////////////////////

NifCheckBoxList::NifCheckBoxList( QWidget *widget /*= 0*/ )
	: CheckBoxList(widget)
{
	// Make editable
	NifCheckListBoxEditor *edit = new NifCheckListBoxEditor(this);
	validator = new NifCheckBoxListValidator(edit);
	edit->setValidator(validator);
	setLineEdit(edit);

	// set delegate items view
	view()->setItemDelegate(new CheckBoxListDelegate(this));

	// Enable editing on items view
	view()->setEditTriggers(QAbstractItemView::CurrentChanged);

	// set "CheckBoxList::eventFilter" as event filter for items view
	view()->viewport()->installEventFilter(this);

	// it just cool to have it as default ;)
	view()->setAlternatingRowColors(true);

	connect( model(), SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
		this, SLOT( sltDataChanged( const QModelIndex &, const QModelIndex & ) ) );

	connect( edit, SIGNAL( editingFinished() ), 
		this, SLOT( sltEditingFinished() ) );
}

NifCheckBoxList::~NifCheckBoxList()
{

}

void NifCheckBoxList::sltDataChanged( const QModelIndex &, const QModelIndex & )
{
	updateText();
}

void NifCheckBoxList::updateText()
{
	QString displayText;
	for (int i=0; i < count(); ++i) {
		QString txt = this->itemData(i, Qt::DisplayRole).toString();

		// dont bother updating if user entered number non-zero
		bool ok = false;
		quint32 val = txt.toULong(&ok, 0);
		if (ok && !val) return;

		bool checked = this->itemData(i, Qt::UserRole).toBool();
		if (checked) {
			if (!displayText.isEmpty()) displayText += " | ";
			displayText += txt;
		}
	}
	this->setEditText( displayText );
}


void NifCheckBoxList::parseText( const QString& text )
{
	// set flag to accept next editTextChanged request
	//  normally this gets called by setCurrentIndex which is undesirable.
	if (!text.isEmpty())
	{
		// Build RegEx for efficient search. Then set model to match
		QString str;
		QStringList list = text.split(QRegExp("\\s*\\|\\s*"), QString::SkipEmptyParts);
		QStringListIterator lit(list);
		while ( lit.hasNext() ) {
			if (!str.isEmpty()) str += "|";
			str += QRegExp::escape(lit.next());
		}
		str.insert(0, '(');
		str.append(')');

		QRegExp re(str);
		for (int i=0; i < count(); ++i) {
			QString txt = this->itemData(i, Qt::DisplayRole).toString();
			this->setItemData(i, re.exactMatch(txt), Qt::UserRole);
		}
		this->setEditText( text );
	}
}

void NifCheckBoxList::sltEditingFinished()
{
	QString text = this->lineEdit()->text();
	parseText(text);
}
//////////////////////////////////////////////////////////////////////////

NifCheckBoxListValidator::NifCheckBoxListValidator( NifCheckListBoxEditor * editor)
	: QValidator(NULL), edit(editor)
{
}

void NifCheckBoxListValidator::fixup( QString & text ) const
{
	Q_UNUSED(text);
}

QValidator::State NifCheckBoxListValidator::validate( QString & text, int & ) const
{
	Q_UNUSED(text);
	//qDebug() << "NifCheckBoxListValidator::validate: " << edit->hasFocus() << "," << text;
	return (edit->hasFocus()) ? QValidator::Acceptable : QValidator::Intermediate;
}

//////////////////////////////////////////////////////////////////////////

NifCheckListBoxEditor::NifCheckListBoxEditor( QWidget* parent)
	: QLineEdit(parent)
{
	inFocus = false;
}

void NifCheckListBoxEditor::focusInEvent( QFocusEvent * )
{
	inFocus = true;	
}

void NifCheckListBoxEditor::focusOutEvent( QFocusEvent * )
{
	inFocus = false;
}

bool NifCheckListBoxEditor::hasFocus() const
{
	return this->inFocus;
}
