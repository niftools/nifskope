/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2008, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
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

#include <QTreeView>

//! Widget for showing a nif file as tree, list, or block details.
class NifTreeView : public QTreeView
{
	Q_OBJECT
public:
	NifTreeView();
	~NifTreeView();
	
	void setModel( QAbstractItemModel * model );
	void setAllExpanded( const QModelIndex & index, bool e );
	
	bool evalConditions() const { return EvalConditions; }
    bool isRowHidden(int row, const QModelIndex &parent) const;
	
	QSize minimumSizeHint() const { return QSize( 50, 50 ); }
	QSize sizeHint() const { return QSize( 400, 400 ); }
	
signals:
	void sigCurrentIndexChanged( const QModelIndex & );

public slots:
	void setRootIndex( const QModelIndex & index );
	void clearRootIndex();

	void setEvalConditions( bool );
	
protected slots:
	void updateConditions();
	void currentChanged( const QModelIndex & current, const QModelIndex & previous );

protected:
    void drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const;
	void keyPressEvent( QKeyEvent * e );

	QStyleOptionViewItem viewOptions() const;
	
	bool EvalConditions;
	
	class BaseModel * nif;
};


#endif
