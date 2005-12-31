/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
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

#ifndef NIFPROXYMODEL_H
#define NIFPROXYMODEL_H

#include <QAbstractItemModel>

#include <QList>

class NifModel;

class NifProxyItem;

class NifProxyModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	NifProxyModel( QObject * parent = 0 );
	~NifProxyModel();
	
    virtual void setModel(QAbstractItemModel *model);
    QAbstractItemModel *model() const;

	QModelIndex index( int row, int col, const QModelIndex & parent ) const;
	QModelIndex parent( const QModelIndex & index ) const;
	
	Qt::ItemFlags flags( const QModelIndex & index ) const;
	
	int columnCount( const QModelIndex & index ) const;
	int rowCount( const QModelIndex & index ) const;
	
	bool hasChildren( const QModelIndex & index ) const
	{ return rowCount( index ) > 0; }
	
	QVariant data( const QModelIndex & index, int role ) const;
	bool setData( const QModelIndex & index, const QVariant & v, int role );
	
	QVariant headerData( int section, Qt::Orientation o, int role ) const;

	QModelIndex mapTo( const QModelIndex & index ) const;
	QModelIndex mapFrom( const QModelIndex & index, const QModelIndex & ref ) const;
	QList<QModelIndex> mapFrom( const QModelIndex & index ) const;

public slots:
	void reset();
	
protected slots:
	void xDataChanged( const QModelIndex &, const QModelIndex & );
	
	void xLinksChanged();
	
protected:
	void updateRoot( bool fast );
	void updateItem( NifProxyItem * item, bool fast );
	
	NifModel * nif;
	
	NifProxyItem * root;
};

#endif
