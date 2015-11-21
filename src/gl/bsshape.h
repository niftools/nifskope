#ifndef BSSHAPE_H
#define BSSHAPE_H

#include "glmesh.h"
#include "glnode.h"
#include "gltools.h"

class BSShape : public Shape
{

public:
	BSShape( Scene * s, const QModelIndex & b );
	~BSShape() { clear(); }

	// IControllable

	void clear() override;
	void update( const NifModel * nif, const QModelIndex & ) override;
	void transform() override;

	// end IControllable

	// Node

	void transformShapes() override;

	void drawShapes( NodeList * secondPass = nullptr, bool presort = false ) override;
	void drawSelection() const override;

	BoundSphere bounds() const override;

	bool isHidden() const override;
	//QString textStats() const override;

	// end Node
protected:

	QPersistentModelIndex iVertData;
	QPersistentModelIndex iTriData;

	int numVerts;
	int numTris;

	QVector<Color4> test1;
	QVector<Color4> test2;
	QVector<Color4> test3;
};

#endif // BSSHAPE_H
