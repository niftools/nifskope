#include "spellbook.h"

#include "lib/nvtristripwrapper.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>


// Brief description is deliberately not autolinked to class Spell
/*! \file normals.cpp
 * \brief Vertex normal spells
 *
 * All classes here inherit from the Spell class.
 */

//! Recalculates and faces the normals of a mesh
class spFaceNormals final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Face Normals" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	static QModelIndex getShapeData( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = nif->getBlock( index );

		if ( nif->isNiBlock( index, { "NiTriShape", "BSLODTriShape", "NiTriStrips" } ) )
			iData = nif->getBlock( nif->getLink( index, "Data" ) );

		if ( nif->isNiBlock( iData, { "NiTriShapeData", "NiTriStripsData" } ) )
			return iData;

		if ( nif->isNiBlock( index, { "BSTriShape", "BSMeshLODTriShape", "BSSubIndexTriShape" } ) ) {
			auto vf = nif->get<BSVertexDesc>( index, "Vertex Desc" );
			if ( (vf & VertexFlags::VF_SKINNED) && nif->getUserVersion2() == 100 ) {
				// Skinned SSE
				auto skinID = nif->getLink( nif->getIndex( index, "Skin" ) );
				auto partID = nif->getLink( nif->getBlock( skinID, "NiSkinInstance" ), "Skin Partition" );
				auto iPartBlock = nif->getBlock( partID, "NiSkinPartition" );
				if ( iPartBlock.isValid() )
					return nif->getIndex( iPartBlock, "Vertex Data" );
			}

			return nif->getIndex( index, "Vertex Data" );
		}

		return QModelIndex();
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return getShapeData( nif, index ).isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = getShapeData( nif, index );

		auto faceNormals = []( const QVector<Vector3> & verts, const QVector<Triangle> & triangles, QVector<Vector3> & norms ) {
			for ( const Triangle & tri : triangles ) {
				Vector3 a = verts[tri[0]];
				Vector3 b = verts[tri[1]];
				Vector3 c = verts[tri[2]];

				Vector3 fn = Vector3::crossproduct( b - a, c - a );
				norms[tri[0]] += fn;
				norms[tri[1]] += fn;
				norms[tri[2]] += fn;
			}

			for ( int n = 0; n < norms.count(); n++ ) {
				norms[n].normalize();
			}
		};

		if ( nif->getUserVersion2() < 100 ) {
			QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
			QVector<Triangle> triangles;
			QModelIndex iPoints = nif->getIndex( iData, "Points" );

			if ( iPoints.isValid() ) {
				QVector<QVector<quint16> > strips;

				for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
					strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );

				triangles = triangulate( strips );
			} else {
				triangles = nif->getArray<Triangle>( iData, "Triangles" );
			}


			QVector<Vector3> norms( verts.count() );
			
			faceNormals( verts, triangles, norms );

			nif->set<int>( iData, "Has Normals", 1 );
			nif->updateArray( iData, "Normals" );
			nif->setArray<Vector3>( iData, "Normals", norms );
		} else {
			QVector<Triangle> triangles;
			int numVerts;
			auto vf = nif->get<BSVertexDesc>( index, "Vertex Desc" );
			if ( !((vf & VertexFlags::VF_SKINNED) && nif->getUserVersion2() == 100) ) {
				numVerts = nif->get<int>( index, "Num Vertices" );
				triangles = nif->getArray<Triangle>( index, "Triangles" );
			} else {
				// Skinned SSE
				auto iPart = iData.parent();
				numVerts = nif->get<uint>( iPart, "Data Size" ) / nif->get<uint>( iPart, "Vertex Size" );

				// Get triangles from all partitions
				auto numParts = nif->get<int>( iPart, "Num Skin Partition Blocks" );
				auto iParts = nif->getIndex( iPart, "Partition" );
				for ( int i = 0; i < numParts; i++ )
					triangles << nif->getArray<Triangle>( iParts.child( i, 0 ), "Triangles" );
			}

			QVector<Vector3> verts;
			verts.reserve( numVerts );
			QVector<Vector3> norms( numVerts );

			for ( int i = 0; i < numVerts; i++ ) {
				auto idx = nif->index( i, 0, iData );

				verts += nif->get<Vector3>( idx, "Vertex" );
			}

			faceNormals( verts, triangles, norms );

			// Pause updates between model/view
			nif->setState( BaseModel::Processing );
			for ( int i = 0; i < numVerts; i++ ) {
				nif->set<ByteVector3>( nif->index( i, 0, iData ), "Normal", norms[i] );
			}
			nif->resetState();
		}

		return index;
	}
};

REGISTER_SPELL( spFaceNormals )

//! Flip normals of a mesh, without recalculating them.
class spFlipNormals final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Flip Normals" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = spFaceNormals::getShapeData( nif, index );
		return ( iData.isValid() && nif->get<bool>( iData, "Has Normals" ) );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = spFaceNormals::getShapeData( nif, index );

		QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );

		for ( int n = 0; n < norms.count(); n++ )
			norms[n] = -norms[n];

		nif->setArray<Vector3>( iData, "Normals", norms );

		return index;
	}
};

REGISTER_SPELL( spFlipNormals )

//! Smooths the normals of a mesh
class spSmoothNormals final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Smooth Normals" ); }
	QString page() const override final { return Spell::tr( "Mesh" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return spFaceNormals::getShapeData( nif, index ).isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = spFaceNormals::getShapeData( nif, index );

		QVector<Vector3> verts;
		QVector<Vector3> norms;

		int numVerts = 0;

		if ( nif->getUserVersion2() < 100 ) {
			verts = nif->getArray<Vector3>( iData, "Vertices" );
			norms = nif->getArray<Vector3>( iData, "Normals" );
		} else {
			auto vf = nif->get<BSVertexDesc>( index, "Vertex Desc" );
			if ( !((vf & VertexFlags::VF_SKINNED) && nif->getUserVersion2() == 100) ) {
				numVerts = nif->get<int>( index, "Num Vertices" );
			} else {
				// Skinned SSE
				// "Num Vertices" does not exist in the partition
				auto iPart = iData.parent();
				numVerts = nif->get<uint>( iPart, "Data Size" ) / nif->get<uint>( iPart, "Vertex Size" );
			}

			verts.reserve( numVerts );
			norms.reserve( numVerts );

			for ( int i = 0; i < numVerts; i++ ) {
				auto idx = nif->index( i, 0, iData );

				verts += nif->get<Vector3>( idx, "Vertex" );
				norms += nif->get<ByteVector3>( idx, "Normal" );
			}
		}

		if ( verts.isEmpty() || verts.count() != norms.count() )
			return index;

		QDialog dlg;
		dlg.setWindowTitle( Spell::tr( "Smooth Normals" ) );

		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );

		QDoubleSpinBox * angle = new QDoubleSpinBox;
		angle->setRange( 0, 180 );
		angle->setValue( 60 );
		angle->setSingleStep( 5 );

		grid->addWidget( new QLabel( Spell::tr( "Max Smooth Angle" ) ), 0, 0 );
		grid->addWidget( angle, 0, 1 );

		QDoubleSpinBox * dist = new QDoubleSpinBox;
		dist->setRange( 0, 1 );
		dist->setDecimals( 4 );
		dist->setSingleStep( 0.01 );
		dist->setValue( 0.001 );

		grid->addWidget( new QLabel( Spell::tr( "Max Vertex Distance" ) ), 1, 0 );
		grid->addWidget( dist, 1, 1 );

		QPushButton * btOk = new QPushButton;
		btOk->setText( Spell::tr( "Smooth" ) );
		QObject::connect( btOk, &QPushButton::clicked, &dlg, &QDialog::accept );

		QPushButton * btCancel = new QPushButton;
		btCancel->setText( Spell::tr( "Cancel" ) );
		QObject::connect( btCancel, &QPushButton::clicked, &dlg, &QDialog::reject );

		grid->addWidget( btOk, 2, 0 );
		grid->addWidget( btCancel, 2, 1 );

		if ( dlg.exec() != QDialog::Accepted )
			return index;


		float maxa = angle->value() / 180 * PI;
		float maxd = dist->value();

		QVector<Vector3> snorms( norms );

		for ( int i = 0; i < verts.count(); i++ ) {
			const Vector3 & a = verts[i];
			Vector3 an = norms[i];

			for ( int j = i + 1; j < verts.count(); j++ ) {
				const Vector3 & b = verts[j];

				if ( ( a - b ).squaredLength() < maxd ) {
					Vector3 bn = norms[j];

					if ( Vector3::angle( an, bn ) < maxa ) {
						snorms[i] += bn;
						snorms[j] += an;
					}
				}
			}
		}

		for ( int i = 0; i < verts.count(); i++ )
			snorms[i].normalize();

		if ( nif->getUserVersion2() < 100 ) {
			nif->setArray<Vector3>( iData, "Normals", snorms );
		} else {
			// Pause updates between model/view
			nif->setState( BaseModel::Processing );
			for ( int i = 0; i < numVerts; i++ )
				nif->set<ByteVector3>( nif->index( i, 0, iData ), "Normal", snorms[i] );
			nif->resetState();
		}
		

		return index;
	}
};

REGISTER_SPELL( spSmoothNormals )

//! Normalises any single Vector3 or array.
/**
 * Most used on Normals, Bitangents and Tangents.
 */
class spNormalize final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Normalize" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getValue( index ).type() == NifValue::tVector3 )
		       || ( nif->isArray( index ) && nif->getValue( index.child( 0, 0 ) ).type() == NifValue::tVector3 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		if ( nif->isArray( index ) ) {
			QVector<Vector3> norms = nif->getArray<Vector3>( index );

			for ( int n = 0; n < norms.count(); n++ )
				norms[n].normalize();

			nif->setArray<Vector3>( index, norms );
		} else {
			Vector3 n = nif->get<Vector3>( index );
			n.normalize();
			nif->set<Vector3>( index, n );
		}

		return index;
	}
};

REGISTER_SPELL( spNormalize )

