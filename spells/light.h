#ifndef SPELL_LIGHT_H

#include <QDialog>

#include "../nifmodel.h"

class ColorWheel;
class RotationEdit;
class VectorEdit;

class QDoubleSpinBox;
class QLineEdit;
class QSlider;
class QTimer;

class LightEdit : public QDialog
{
	Q_OBJECT
public:
	LightEdit( NifModel * n, const QModelIndex & i );

protected slots:
	void sltApply();
	void sltReset();
	
	void nifDataChanged( const QModelIndex &, const QModelIndex & );
	void nifDestroyed();

protected:
	NifModel * nif;
	QPersistentModelIndex iLight;
	
	QLineEdit * lightName;
	
	VectorEdit * position;
	RotationEdit * direction;
	
	ColorWheel * color[3];
	QSlider * value[1];
	
	bool isPoint;
	QDoubleSpinBox * attenuation[3];
	
	bool isSpot;
	QDoubleSpinBox * spot[2];
	
	bool setting;
	
	QTimer * timer;
};

#endif
