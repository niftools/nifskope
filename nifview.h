#ifndef NIFTREEVIEW_H
#define NIFTREEVIEW_H

#include <QTreeView>

class NifModel;

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
	
public slots:	
	void setEvalConditions( bool );

protected:
    void drawBranches( QPainter * painter, const QRect & rect, const QModelIndex & index ) const;

	QStyleOptionViewItem viewOptions() const;
	
	bool ForceExpanded;
	bool EvalConditions;
	
	NifModel * nif;
};


#endif
