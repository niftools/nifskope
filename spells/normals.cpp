#include "../spellbook.h"

#include "../NvTriStrip/qtwrapper.h"

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
class spFaceNormals : public Spell
{
public:
	QString name() const { return Spell::tr("Face Normals"); }
	QString page() const { return Spell::tr("Mesh"); }
	
	static QModelIndex getShapeData( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = nif->getBlock( index );
		if ( nif->isNiBlock( index, "NiTriShape" ) || nif->isNiBlock( index, "NiTriStrips" ) )
			iData = nif->getBlock( nif->getLink( index, "Data" ) );
		if ( nif->isNiBlock( iData, "NiTriShapeData" ) || nif->isNiBlock( iData, "NiTriStripsData" ) )
			return iData;
		else return QModelIndex();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return getShapeData( nif, index ).isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = getShapeData( nif, index );
		
		QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
		QVector<Triangle> triangles;
		QModelIndex iPoints = nif->getIndex( iData, "Points" );
		if ( iPoints.isValid() )
		{
			QList< QVector< quint16 > > strips;
			for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
				strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );
			triangles = triangulate( strips );
		}
		else
		{
			triangles = nif->getArray<Triangle>( iData, "Triangles" );
		}
		
		
		QVector<Vector3> norms( verts.count() );
		foreach ( Triangle tri, triangles )
		{
			Vector3 a = verts[ tri[0] ];
			Vector3 b = verts[ tri[1] ];
			Vector3 c = verts[ tri[2] ];
			
			Vector3 fn = Vector3::crossproduct( b - a, c - a );
			norms[ tri[0] ] += fn;
			norms[ tri[1] ] += fn;
			norms[ tri[2] ] += fn;
		}
		
		for ( int n = 0; n < norms.count(); n++ )
		{
			norms[ n ].normalize();
		}
		
		nif->set<int>( iData, "Has Normals", 1 );
		nif->updateArray( iData, "Normals" );
		nif->setArray<Vector3>( iData, "Normals", norms );
		
		return index;
	}
};

REGISTER_SPELL( spFaceNormals )

//! Flip normals of a mesh, without recalculating them.
class spFlipNormals : public Spell
{
public:
	QString name() const { return Spell::tr("Flip Normals"); }
	QString page() const { return Spell::tr("Mesh"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = spFaceNormals::getShapeData( nif, index );
		return ( iData.isValid() && nif->get<bool>( iData, "Has Normals" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
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
class spSmoothNormals : public Spell
{
public:
	QString name() const { return Spell::tr("Smooth Normals"); }
	QString page() const { return Spell::tr("Mesh"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return spFaceNormals::getShapeData( nif, index ).isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = spFaceNormals::getShapeData( nif, index );
		
		QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
		QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );
		
		if ( verts.isEmpty() || verts.count() != norms.count() )
			return index;
		
		QDialog dlg;
		dlg.setWindowTitle( Spell::tr("Smooth Normals") );
		
		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );
		
		QDoubleSpinBox * angle = new QDoubleSpinBox;
		angle->setRange( 0, 180 );
		angle->setValue( 60 );
		angle->setSingleStep( 5 );
		
		grid->addWidget( new QLabel( Spell::tr("Max Smooth Angle") ), 0, 0 );
		grid->addWidget( angle, 0, 1 );
		
		QDoubleSpinBox * dist = new QDoubleSpinBox;
		dist->setRange( 0, 1 );
		dist->setDecimals( 4 );
		dist->setSingleStep( 0.01 );
		dist->setValue( 0.001 );
		
		grid->addWidget( new QLabel( Spell::tr("Max Vertex Distance") ), 1, 0 );
		grid->addWidget( dist, 1, 1 );
		
		QPushButton * btOk = new QPushButton;
		btOk->setText( Spell::tr("Smooth") );
		QObject::connect( btOk, SIGNAL( clicked() ), & dlg, SLOT( accept() ) );
		
		QPushButton * btCancel = new QPushButton;
		btCancel->setText( Spell::tr("Cancel") );
		QObject::connect( btCancel, SIGNAL( clicked() ), & dlg, SLOT( reject() ) );
		
		grid->addWidget( btOk, 2, 0 );
		grid->addWidget( btCancel, 2, 1 );
		
		if ( dlg.exec() != QDialog::Accepted )
			return index;
		
		
		float maxa = angle->value() / 180 * PI;
		float maxd = dist->value();
		
		QVector<Vector3> snorms( norms );
		
		for ( int i = 0; i < verts.count(); i++ )
		{
			const Vector3 & a = verts[i];
			Vector3 an = norms[i];
			for ( int j = i+1; j < verts.count(); j++ )
			{
				const Vector3 & b = verts[j];
				if ( ( a - b ).squaredLength() < maxd )
				{
					Vector3 bn = norms[j];
					if ( Vector3::angle( an, bn ) < maxa )
					{
						snorms[i] += bn;
						snorms[j] += an;
					}
				}
			}
		}
		
		for ( int i = 0; i < verts.count(); i++ )
			snorms[i].normalize();
		
		nif->setArray<Vector3>( iData, "Normals", snorms );
		
		return index;
	}
};

REGISTER_SPELL( spSmoothNormals )

//! Normalises any single Vector3 or array.
/**
 * Most used on Normals, Bitangents and Tangents.
 */
class spNormalize : public Spell
{
public:
	QString name() const { return Spell::tr("Normalize"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getValue( index ).type() == NifValue::tVector3 )
			|| ( nif->isArray( index ) && nif->getValue( index.child( 0, 0 ) ).type() == NifValue::tVector3 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		if ( nif->isArray( index ) )
		{
			QVector<Vector3> norms = nif->getArray<Vector3>( index );
			for ( int n = 0; n < norms.count(); n++ )
				norms[n].normalize();
			nif->setArray<Vector3>( index, norms );
		}
		else
		{
			Vector3 n = nif->get<Vector3>( index );
			n.normalize();
			nif->set<Vector3>( index, n );
		}
		return index;
	}
};

REGISTER_SPELL( spNormalize )

