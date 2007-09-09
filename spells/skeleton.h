#ifndef SPELL_SKELETON_H
#define SPELL_SKELETON_H

#include <QDialog>
#include <QCheckBox>

class QSpinBox;

class SkinPartitionDialog : public QDialog
{
	Q_OBJECT
public:
	SkinPartitionDialog( int maxInfluences );
	
	int maxBonesPerPartition();
	int maxBonesPerVertex();
	bool makeStrips();
	
protected slots:
	void changed();
	
protected:
	QSpinBox * spnPart;
	QSpinBox * spnVert;
	QCheckBox * ckTStrip;
	
	int maxInfluences;
};

#endif
