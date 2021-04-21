#ifndef BSSHAPE_H
#define BSSHAPE_H

#include "gl/glmesh.h"
#include "gl/gltools.h"


class NifModel;
class NodeList;

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

	// Shape

	void drawVerts() const override;
	QModelIndex vertexAt( int ) const override;

protected:

	QPersistentModelIndex iVertData;
	QPersistentModelIndex iTriData;

	QString skinDataName;
	QString skinInstName;

	int numVerts = 0;
	int numTris = 0;

	Vector3 bsphereCenter;
	float bsphereRadius = 0.0;
};

#endif // BSSHAPE_H
