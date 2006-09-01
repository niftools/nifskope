#ifndef SPELL_SKELETON_H
#define SPELL_SKELETON_H

#include <QDialog>

class QSpinBox;

class SkinPartitionDialog : public QDialog
{
	Q_OBJECT
public:
	SkinPartitionDialog( int maxInfluences );
	
	int maxBonesPerPartition();
	int maxBonesPerVertex();
	
protected slots:
	void changed();
	
protected:
	QSpinBox * spnPart;
	QSpinBox * spnVert;
	
	int maxInfluences;
};

#endif
