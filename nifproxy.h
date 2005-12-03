/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifndef NIFPROXYMODEL_H
#define NIFPROXYMODEL_H

#include <QAbstractItemModel>

#include <QHash>
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
	
	void xRowsInserted( const QModelIndex &, int, int );
	void xRowsRemoved( const QModelIndex &, int, int );

protected:
	void updateRoot( bool fast );
	void updateItem( NifProxyItem * item, bool fast );
	
	void updateLinks( int block );
	void updateLinks( int block, const QModelIndex & index );
	
	QHash< int, QList<int> > childLinks;
	QHash< int, QList<int> > parentLinks;
	
	NifModel * nif;
	
	NifProxyItem * root;
};

#endif
