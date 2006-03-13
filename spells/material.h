#ifndef SPELL_MATERIAL_H

#include <QDialog>

#include "../nifmodel.h"

class ColorWheel;
class QLineEdit;
class QSlider;
class QTimer;

class MaterialEdit : public QDialog
{
	Q_OBJECT
public:
	MaterialEdit( NifModel * n, const QModelIndex & i );

protected slots:
	void sltApply();
	void sltReset();
	
	void nifDataChanged( const QModelIndex &, const QModelIndex & );
	void nifDestroyed();

protected:
	NifModel * nif;
	QPersistentModelIndex iMaterial;
	
	QLineEdit * matName;
	
	ColorWheel * color[4];
	QSlider * value[2];
	
	QTimer * timer;
};

#endif
