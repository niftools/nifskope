#include "spellbook.h"

#include "nvtristripwrapper.h"

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

		if ( nif->isNiBlock( index, { "BSTriShape", "BSMeshLODTriShape", "BSSubIndexTriShape" } ) )
			return nif->getIndex( index, "Vertex Data" );

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

		if ( nif->getUserVersion2() < 130 ) {
			QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
			QVector<Triangle> triangles;
			QModelIndex iPoints = nif->getIndex( iData, "Points" );

			if ( iPoints.isValid() ) {
				QList<QVector<quint16> > strips;

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
			int numVerts = nif->get<int>( index, "Num Vertices" );
			QVector<Vector3> verts;
			QVector<Vector3> norms( numVerts );
			QVector<Triangle> triangles = nif->getArray<Triangle>( index, "Triangles" );

			for ( int i = 0; i < numVerts; i++ ) {
				auto idx = nif->index( i, 0, iData );

				verts += nif->get<HalfVector3>( idx, "Vertex" );
			}

			faceNormals( verts, triangles, norms );

			// Pause updates between model/view
			nif->setEmitChanges( false );
			for ( int i = 0; i < numVerts; i++ ) {
				// Unpause updates if last
				if ( i == numVerts - 1 )
					nif->setEmitChanges( true );

				auto idx = nif->index( i, 0, iData );

				nif->set<ByteVector3>( idx, "Normal", *static_cast<ByteVector3 *>(&norms[i]) );
			}
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

		if ( nif->getUserVersion2() < 130 ) {
			verts = nif->getArray<Vector3>( iData, "Vertices" );
			norms = nif->getArray<Vector3>( iData, "Normals" );
		} else {
			int numVerts = nif->get<int>( index, "Num Vertices" );
			for ( int i = 0; i < numVerts; i++ ) {
				auto idx = nif->index( i, 0, iData );

				verts += nif->get<HalfVector3>( idx, "Vertex" );
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

		if ( nif->getUserVersion2() < 130 ) {
			nif->setArray<Vector3>( iData, "Normals", snorms );
		} else {
			int numVerts = nif->get<int>( index, "Num Vertices" );
			// Pause updates between model/view
			nif->setEmitChanges( false );
			for ( int i = 0; i < numVerts; i++ ) {
				// Unpause updates if last
				if ( i == numVerts - 1 )
					nif->setEmitChanges( true );

				auto idx = nif->index( i, 0, iData );

				nif->set<ByteVector3>( idx, "Normal", *static_cast<ByteVector3 *>(&snorms[i]) );
			}
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

