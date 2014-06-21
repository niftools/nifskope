#ifndef SPELL_SKELETON_H
#define SPELL_SKELETON_H

#include <QDialog> // Inherited


//! \file skeleton.h SkinPartitionDialog

class QCheckBox;
class QSpinBox;

//! Dialog box for skin partitions
class SkinPartitionDialog final : public QDialog
{
	Q_OBJECT

public:
	//! Constructor
	SkinPartitionDialog( int maxInfluences );

	//! Returns the value of spnPart
	int maxBonesPerPartition();
	//! Returns the value of spnVert
	int maxBonesPerVertex();
	//! Returns the value of ckTStrip
	bool makeStrips();
	//! Returns the value of ckPad
	bool padPartitions();

protected slots:
	//! Sets the minimum value of spnPart to the value of spnVert
	void changed();

protected:
	//! The number of bones per partition
	QSpinBox * spnPart;
	//! The number of bones per vertex
	QSpinBox * spnVert;
	//! Whether strips should be made
	QCheckBox * ckTStrip;
	//! Whether padding should be used
	QCheckBox * ckPad;

	//! The maximum number of influences; unused?
	int maxInfluences;
};

#endif
