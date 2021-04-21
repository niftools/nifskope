#ifndef SP_MESH_H
#define SP_MESH_H

#include "spellbook.h"


//! \file mesh.h Mesh spell headers

//! Update center and radius of a mesh
class spUpdateCenterRadius final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update Center/Radius" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

//! Update Triangles on Data from Skin
class spUpdateTrianglesFromSkin final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Update Triangles From Skin" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

#endif
