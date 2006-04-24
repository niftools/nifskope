#include "../spellbook.h"

#include "../NvTriStrip/qtwrapper.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>

class spFaceNormals : public Spell
{
public:
	QString name() const { return "Make Normals"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		return ( nif->isNiBlock( index, "NiTriShape" ) && nif->isNiBlock( iData, "NiTriShapeData" ) )
			|| ( nif->isNiBlock( index, "NiTriStrips" ) && nif->isNiBlock( iData, "NiTriStripsData" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iShape )
	{
		QModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
		
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
		
		return iShape;
	}
};

REGISTER_SPELL( spFaceNormals )

class spSmoothNormals : public Spell
{
public:
	QString name() const { return "Smooth Normals"; }
	QString page() const { return "Mesh"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		return nif->get<int>( iData, "Has Normals" ) && ( ( nif->isNiBlock( index, "NiTriShape" ) && nif->isNiBlock( iData, "NiTriShapeData" ) )
			|| ( nif->isNiBlock( index, "NiTriStrips" ) && nif->isNiBlock( iData, "NiTriStripsData" ) ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iShape )
	{
		QModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
		
		QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
		QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );
		
		if ( verts.isEmpty() || verts.count() != norms.count() )
			return iShape;
		
		QDialog dlg;
		dlg.setWindowTitle( "Smooth Normals" );
		
		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );
		
		QDoubleSpinBox * angle = new QDoubleSpinBox;
		angle->setRange( 0, 180 );
		angle->setValue( 60 );
		angle->setSingleStep( 5 );
		
		grid->addWidget( new QLabel( "Max Smooth Angle" ), 0, 0 );
		grid->addWidget( angle, 0, 1 );
		
		QDoubleSpinBox * dist = new QDoubleSpinBox;
		dist->setRange( 0, 1 );
		dist->setDecimals( 4 );
		dist->setSingleStep( 0.01 );
		dist->setValue( 0.001 );
		
		grid->addWidget( new QLabel( "Max Vertex Distance" ), 1, 0 );
		grid->addWidget( dist, 1, 1 );
		
		QPushButton * btOk = new QPushButton;
		btOk->setText( "Smooth" );
		QObject::connect( btOk, SIGNAL( clicked() ), & dlg, SLOT( accept() ) );
		
		QPushButton * btCancel = new QPushButton;
		btCancel->setText( "Cancel" );
		QObject::connect( btCancel, SIGNAL( clicked() ), & dlg, SLOT( reject() ) );
		
		grid->addWidget( btOk, 2, 0 );
		grid->addWidget( btCancel, 2, 1 );
		
		if ( dlg.exec() != QDialog::Accepted )
			return iShape;
		
		
		float maxa = angle->value() / 180 * PI;
		float maxd = dist->value();
		
		for ( int i = 0; i < verts.count(); i++ )
		{
			const Vector3 & a = verts[i];
			Vector3 & an = norms[i];
			for ( int j = i+1; j < verts.count(); j++ )
			{
				const Vector3 & b = verts[j];
				if ( ( a - b ).squaredLength() < maxd )
				{
					Vector3 & bn = norms[j];
					if ( Vector3::angle( an, bn ) < maxa )
					{
						Vector3 n = an + bn;
						n.normalize();
						an = bn = n;
					}
				}
			}
		}
		
		nif->setArray<Vector3>( iData, "Normals", norms );
		
		return iShape;
	}
};

REGISTER_SPELL( spSmoothNormals );
