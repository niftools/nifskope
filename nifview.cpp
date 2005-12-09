#include "nifview.h"

#include "nifmodel.h"

NifTreeView::NifTreeView() : QTreeView()
{
	nif = 0;
	EvalConditions = false;
	
	setUniformRowHeights( true );
	setAlternatingRowColors( true );
	setContextMenuPolicy( Qt::CustomContextMenu );
}

NifTreeView::~NifTreeView()
{
}

void NifTreeView::setModel( QAbstractItemModel * model )
{
	if ( nif && EvalConditions )
		disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );

	nif = qobject_cast<NifModel*>( model );
	
	QTreeView::setModel( model );
	
	if ( nif && EvalConditions )
		connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );
}

void NifTreeView::setCurrentIndexExpanded( const QModelIndex & index )
{
	if ( ! index.isValid() )
		setCurrentIndex( index );
	
	if ( index.model() != model() )	return;
	
	QModelIndex idx = index;
	while ( idx.isValid() )
	{
		expand( idx );
		idx = idx.parent();
	}
	
	setCurrentIndex( index );
}

void NifTreeView::setEvalConditions( bool c )
{
	if ( EvalConditions == c )
		return;
	
	EvalConditions = c;
	
	if ( nif )
	{
		if ( EvalConditions )
			connect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );
		else
			disconnect( nif, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( updateConditions() ) );
	}
	
	doItemsLayout();
}

bool NifTreeView::isRowHidden( int r, const QModelIndex & index ) const
{
	return ( EvalConditions && index.isValid() && nif && index.model() == nif && !nif->evalCondition( index.child( r, 0 ) ) );
}

void NifTreeView::setAllExpanded( const QModelIndex & index, bool e )
{
	if ( ! ( model() && index.model() == model() ) )	return;
	for ( int r = 0; r < model()->rowCount( index ); r++ )
	{
		QModelIndex child = model()->index( r, 0, index );
		if ( model()->hasChildren( child ) )
		{
			setExpanded( child, e );
			setAllExpanded( child, e );
		}
	}
}

QStyleOptionViewItem NifTreeView::viewOptions() const
{
	QStyleOptionViewItem opt = QTreeView::viewOptions();
	opt.showDecorationSelected = true;
	return opt;
}

void NifTreeView::drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const
{
	if ( rootIsDecorated() )
		QTreeView::drawBranches( painter, rect, index );
}

void NifTreeView::updateConditions()
{
	if ( EvalConditions )
		doItemsLayout();
}

