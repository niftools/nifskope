#include "nifview.h"

#include "nifmodel.h"

NifTreeView::NifTreeView() : QTreeView()
{
	nif = 0;
	ForceExpanded = false;
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
	nif = qobject_cast<NifModel*>( model );
	QTreeView::setModel( model );
}

void NifTreeView::setEvalConditions( bool c )
{
	EvalConditions = c;
	doItemsLayout();
}

bool NifTreeView::isRowHidden( int r, const QModelIndex & index ) const
{
	return ( EvalConditions && index.isValid() && nif && index.model() == nif && !nif->evalCondition( index.child( r, 0 ) ) );
}

void NifTreeView::setAllExpanded( const QModelIndex & index, bool e )
{
	if ( ! model() )	return;
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
