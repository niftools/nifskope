#ifndef BSSHAPE_H
#define BSSHAPE_H

#include "gl/glmesh.h"
#include "gl/gltools.h"


class NifModel;
class NodeList;

class BSShape : public Shape
{

public:
	BSShape( Scene * s, const QModelIndex & b ) : Shape( s, b ) { }

	// Node

	void transformShapes() override;

	void drawShapes( NodeList * secondPass = nullptr, bool presort = false ) override;
	void drawSelection() const override;

	BoundSphere bounds() const override;

	// end Node

	// Shape

	void drawVerts() const override;
	QModelIndex vertexAt( int ) const override;

protected:
	BoundSphere dataBound;

	bool isDynamic = false;

	void updateImpl( const NifModel * nif, const QModelIndex & index ) override;
	void updateData( const NifModel * nif ) override;
};

#endif // BSSHAPE_H
