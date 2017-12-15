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

#ifndef NIFTREEVIEW_H
#define NIFTREEVIEW_H

#include <QTreeView> // Inherited
#include <data/nifvalue.h>

#include <memory>


//! Widget for showing a nif file as tree, list, or block details.
class NifTreeView final : public QTreeView
{
	Q_OBJECT

public:
	//! Constructor
	NifTreeView( QWidget * parent = 0, Qt::WindowFlags flags = 0 );
	//! Destructor
	~NifTreeView();

	//! Set the model used by the widget
	void setModel( QAbstractItemModel * model ) override final;
	//! Expand all branches
	void setAllExpanded( const QModelIndex & index, bool e );

	//! Accessor for EvalConditions
	bool evalConditions() const { return doRowHiding; }
	//! Is a row hidden?
	bool isRowHidden( int row, const QModelIndex & parent ) const;

	//! Minimum size
	QSize minimumSizeHint() const override final { return { 50, 50 }; }
	//! Default size
	QSize sizeHint() const override final { return { 400, 200 }; }

signals:
	//! Signal emmited when the current index changes; probably connected to NifSkope::select()
	void sigCurrentIndexChanged( const QModelIndex & );

public slots:
	//! Sets the root index
	void setRootIndex( const QModelIndex & index ) override final;
	//! Clear the root index; probably conncted to NifSkope::dList
	void clearRootIndex();

	//! Sets Hiding of non-applicable rows
	void setRowHiding( bool );

	//! Updates version conditions (connect to dataChanged)
	void updateConditions( const QModelIndex & topLeft, const QModelIndex & bottomRight );
protected slots:
	//! Recursively updates version conditions
	void updateConditionRecurse( const QModelIndex & index );
	//! Called when the current index changes
	void currentChanged( const QModelIndex & current, const QModelIndex & previous ) override final;

	//! Scroll to index; connected to expanded()
	void scrollExpand( const QModelIndex & index );

protected:
	void drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const override final;
	void keyPressEvent( QKeyEvent * e ) override final;

	QStyleOptionViewItem viewOptions() const override final;

	void autoExpand( const QModelIndex & index );

	bool doRowHiding = true;
	bool autoExpanded = false;

	class BaseModel * nif = nullptr;

	//! Row Copy
	void copy();
	//! Row Paste
	void paste();
	void pasteTo( QModelIndex idx, const NifValue & srcValue );

	//! Array/Compound Paste
	void pasteArray();

	//! Get a list of only the value column fields from lists of rows
	QModelIndexList valueIndexList( const QModelIndexList & rows ) const;
};


//! A global clipboard for NifTreeView to store a NifValue for all windows
class NifValueClipboard
{

public:
	NifValue getValue() { return value; }
	void setValue( const NifValue & val ) { value = val; }

	const std::vector<NifValue>& getValues() const { return values; }
	void setValues( const std::vector<NifValue>& vals ) { values = vals; }

	void clear()
	{
		value.clear();
	}

private:
	//! The value stored from a single row copy
	NifValue value = NifValue();
	//! The values stored from a single array copy
	std::vector<NifValue> values;
};

// The global NifTreeView clipboard pointer
static auto valueClipboard = std::make_unique<NifValueClipboard>();

#endif
