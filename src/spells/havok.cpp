#include "spellbook.h"

#include "gl/gltools.h"
#include "spells/blocks.h"

#include "lib/nvtristripwrapper.h"
#include "lib/qhull.h"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>

#include <algorithm> // std::sort


// Brief description is deliberately not autolinked to class Spell
/*! \file havok.cpp
 * \brief Havok spells
 *
 * All classes here inherit from the Spell class.
 */

//! For Havok coordinate transforms
static const float havokConst = 7.0;

//! Creates a convex hull using Qhull
class spCreateCVS final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Create Convex Shape" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		if ( !(nif->inherits( index, "NiTriBasedGeom" ) || nif->inherits( index, "BSTriShape" ))
			 || !nif->getUserVersion2() )
			return false;

		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		if ( !iData.isValid() && nif->getIndex( index, "Vertex Data" ).isValid() )
			iData = index;

		return iData.isValid() && nif->get<int>( iData, "Num Vertices" ) > 0;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iData = nif->getBlock( nif->getLink( index, "Data" ) );
		if ( !iData.isValid() )
			iData = nif->getIndex( index, "Vertex Data" );

		if ( !iData.isValid() )
			return index;

		float havokScale = (nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion() >= 12) ? 10.0f : 1.0f;

		havokScale *= havokConst;

		/* those will be filled with the CVS data */
		QVector<Vector4> convex_verts, convex_norms;

		/* get the verts of our mesh */
		QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
		QVector<Vector3> vertsTrans;

		if ( nif->getUserVersion2() < 100 ) {
			verts = nif->getArray<Vector3>( iData, "Vertices" );
		} else {
			int numVerts = nif->get<int>( index, "Num Vertices" );
			verts.reserve( numVerts );
			for ( int i = 0; i < numVerts; i++ )
				verts += nif->get<Vector3>( nif->index( i, 0, iData ), "Vertex" );
		}

		// Offset by translation of NiTriShape
		Vector3 trans = nif->get<Vector3>( index, "Translation" );
		Matrix rot = nif->get<Matrix>( index, "Rotation" );
		float scale = nif->get<float>( index, "Scale" );
		Transform transform;
		transform.translation = trans;
		transform.rotation = rot;
		transform.scale = scale;

		for ( auto v : verts ) {
			vertsTrans.append( transform * v );
		}

		// to store results
		QVector<Vector4> hullVerts, hullNorms;

		// ask for precision
		QDialog dlg;
		QVBoxLayout * vbox = new QVBoxLayout;
		dlg.setLayout( vbox );

		vbox->addWidget( new QLabel( Spell::tr( "Enter the maximum roundoff error to use" ) ) );
		vbox->addWidget( new QLabel( Spell::tr( "Larger values will give a less precise but better performing hull" ) ) );

		QDoubleSpinBox * precSpin = new QDoubleSpinBox;
		precSpin->setRange( 0, 5 );
		precSpin->setDecimals( 3 );
		precSpin->setSingleStep( 0.01 );
		precSpin->setValue( 0.25 );
		vbox->addWidget( precSpin );

		vbox->addWidget( new QLabel( Spell::tr( "Collision Radius" ) ) );

		QDoubleSpinBox * spnRadius = new QDoubleSpinBox;
		spnRadius->setRange( 0, 0.5 );
		spnRadius->setDecimals( 4 );
		spnRadius->setSingleStep( 0.001 );
		spnRadius->setValue( 0.05 );
		vbox->addWidget( spnRadius );

		QHBoxLayout * hbox = new QHBoxLayout;
		vbox->addLayout( hbox );

		QPushButton * ok = new QPushButton;
		ok->setText( Spell::tr( "Ok" ) );
		hbox->addWidget( ok );

		QPushButton * cancel = new QPushButton;
		cancel->setText( Spell::tr( "Cancel" ) );
		hbox->addWidget( cancel );

		QObject::connect( ok, &QPushButton::clicked, &dlg, &QDialog::accept );
		QObject::connect( cancel, &QPushButton::clicked, &dlg, &QDialog::reject );

		if ( dlg.exec() != QDialog::Accepted ) {
			return index;
		}

		/* make a convex hull from it */
		compute_convex_hull( vertsTrans, hullVerts, hullNorms, (float)precSpin->value() );

		// sort and remove duplicate vertices
		QList<Vector4> sortedVerts;
		for ( Vector4 vert : hullVerts ) {
			vert /= havokScale;

			if ( !sortedVerts.contains( vert ) ) {
				sortedVerts.append( vert );
			}
		}
		std::sort( sortedVerts.begin(), sortedVerts.end(), Vector4::lexLessThan );
		QListIterator<Vector4> vertIter( sortedVerts );

		while ( vertIter.hasNext() ) {
			Vector4 sorted = vertIter.next();
			convex_verts.append( sorted );
		}

		// sort and remove duplicate normals
		QList<Vector4> sortedNorms;
		for ( Vector4 norm : hullNorms ) {
			norm = Vector4( Vector3( norm ), norm[3] / havokScale );

			if ( !sortedNorms.contains( norm ) ) {
				sortedNorms.append( norm );
			}
		}
		std::sort( sortedNorms.begin(), sortedNorms.end(), Vector4::lexLessThan );
		QListIterator<Vector4> normIter( sortedNorms );

		while ( normIter.hasNext() ) {
			Vector4 sorted = normIter.next();
			convex_norms.append( sorted );
		}

		/* create the CVS block */
		QModelIndex iCVS = nif->insertNiBlock( "bhkConvexVerticesShape" );

		/* set CVS verts */
		nif->set<uint>( iCVS, "Num Vertices", convex_verts.count() );
		nif->updateArray( iCVS, "Vertices" );
		nif->setArray<Vector4>( iCVS, "Vertices", convex_verts );

		/* set CVS norms */
		nif->set<uint>( iCVS, "Num Normals", convex_norms.count() );
		nif->updateArray( iCVS, "Normals" );
		nif->setArray<Vector4>( iCVS, "Normals", convex_norms );

		// radius is always 0.1?
		// TODO: Figure out if radius is not arbitrarily set in vanilla NIFs
		nif->set<float>( iCVS, "Radius", spnRadius->value() );

		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ) );
		QModelIndex collisionLink = nif->getIndex( iParent, "Collision Object" );
		QModelIndex collisionObject = nif->getBlock( nif->getLink( collisionLink ) );

		// create bhkCollisionObject
		if ( !collisionObject.isValid() ) {
			collisionObject = nif->insertNiBlock( "bhkCollisionObject" );

			nif->setLink( collisionLink, nif->getBlockNumber( collisionObject ) );
			nif->setLink( collisionObject, "Target", nif->getBlockNumber( iParent ) );
		}

		QModelIndex rigidBodyLink = nif->getIndex( collisionObject, "Body" );
		QModelIndex rigidBody = nif->getBlock( nif->getLink( rigidBodyLink ) );

		// create bhkRigidBody
		if ( !rigidBody.isValid() ) {
			rigidBody = nif->insertNiBlock( "bhkRigidBody" );

			nif->setLink( rigidBodyLink, nif->getBlockNumber( rigidBody ) );
		}

		QPersistentModelIndex shapeLink = nif->getIndex( rigidBody, "Shape" );
		QPersistentModelIndex shape = nif->getBlock( nif->getLink( shapeLink ) );

		QVector<qint32> shapeLinks;
		bool replace = true;
		if ( shape.isValid() ) {
			shapeLinks = { nif->getBlockNumber( shape ) };

			QString questionTitle = tr( "Create List Shape" );
			QString questionBody = tr( "This collision object already has a shape. Combine into a list shape? 'No' will replace the shape." );

			bool isListShape = false;
			if ( nif->inherits( shape, "bhkListShape" ) ) {
				isListShape = true;
				questionTitle = tr( "Add to List Shape" );
				questionBody = tr( "This collision object already has a list shape. Add to list shape? 'No' will replace the list shape." );
				shapeLinks = nif->getLinkArray( shape, "Sub Shapes" );
			}

			int response = QMessageBox::question( nullptr, questionTitle, questionBody,	QMessageBox::Yes, QMessageBox::No );
			if ( response == QMessageBox::Yes ) {
				QModelIndex iListShape = shape;
				if ( !isListShape ) {
					iListShape = nif->insertNiBlock( "bhkListShape" );
					nif->setLink( shapeLink, nif->getBlockNumber( iListShape ) );
				}

				shapeLinks << nif->getBlockNumber( iCVS );
				nif->set<uint>( iListShape, "Num Sub Shapes", shapeLinks.size() );
				nif->updateArray( iListShape, "Sub Shapes" );
				nif->setLinkArray( iListShape, "Sub Shapes", shapeLinks );
				nif->set<uint>( iListShape, "Num Unknown Ints", shapeLinks.size() );
				nif->updateArray( iListShape, "Unknown Ints" );
				replace = false;
			}
		} 
		
		if ( replace ) {
			// Replace link
			nif->setLink( shapeLink, nif->getBlockNumber( iCVS ) );
			// Remove all old shapes
			spRemoveBranch rm;
			rm.castIfApplicable( nif, shape );
		}

		Message::info( nullptr,
					   Spell::tr( "Created hull with %1 vertices, %2 normals" )
						.arg( convex_verts.count() )
						.arg( convex_norms.count() ) 
		);

		return (iCVS.isValid()) ? iCVS : rigidBody;
	}
};

REGISTER_SPELL( spCreateCVS )

//! Transforms Havok constraints
class spConstraintHelper final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "A -> B" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && 
			nif->isNiBlock( nif->getBlock( index ),
				{ "bhkMalleableConstraint",
				  "bhkBreakableConstraint",
				  "bhkRagdollConstraint",
				  "bhkLimitedHingeConstraint",
				  "bhkHingeConstraint",
				  "bhkPrismaticConstraint" }
			);
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iConstraint = nif->getBlock( index );
		QString name = nif->itemName( iConstraint );

		if ( name == "bhkMalleableConstraint" || name == "bhkBreakableConstraint" ) {
			if ( nif->getIndex( iConstraint, "Ragdoll" ).isValid() ) {
				name = "bhkRagdollConstraint";
			} else if ( nif->getIndex( iConstraint, "Limited Hinge" ).isValid() ) {
				name = "bhkLimitedHingeConstraint";
			} else if ( nif->getIndex( iConstraint, "Hinge" ).isValid() ) {
				name = "bhkHingeConstraint";
			}
		}

		QModelIndex iBodyA = nif->getBlock( nif->getLink( bhkGetEntity( nif, iConstraint, "Entity A" ) ), "bhkRigidBody" );
		QModelIndex iBodyB = nif->getBlock( nif->getLink( bhkGetEntity( nif, iConstraint, "Entity B" ) ), "bhkRigidBody" );

		if ( !iBodyA.isValid() || !iBodyB.isValid() ) {
			Message::warning( nullptr, Spell::tr( "Couldn't find the bodies for this constraint." ) );
			return index;
		}

		Transform transA = bhkBodyTrans( nif, iBodyA );
		Transform transB = bhkBodyTrans( nif, iBodyB );

		QModelIndex iConstraintData;
		if ( name == "bhkLimitedHingeConstraint" ) {
			iConstraintData = nif->getIndex( iConstraint, "Limited Hinge" );
			if ( !iConstraintData.isValid() )
				iConstraintData = iConstraint;
		} else if ( name == "bhkRagdollConstraint" ) {
			iConstraintData = nif->getIndex( iConstraint, "Ragdoll" );
			if ( !iConstraintData.isValid() )
				iConstraintData = iConstraint;
		} else if ( name == "bhkHingeConstraint" ) {
			iConstraintData = nif->getIndex( iConstraint, "Hinge" );
			if ( !iConstraintData.isValid() )
				iConstraintData = iConstraint;
		}

		if ( !iConstraintData.isValid() )
			return index;

		Vector3 pivot = Vector3( nif->get<Vector4>( iConstraintData, "Pivot A" ) );
		pivot = transA * pivot;
		pivot = transB.rotation.inverted() * ( pivot - transB.translation ) / transB.scale;
		nif->set<Vector4>( iConstraintData, "Pivot B", { pivot[0], pivot[1], pivot[2], 0 } );

		QString axisA, axisB, twistA, twistB, twistA2, twistB2;
		if ( name.endsWith( "HingeConstraint" ) ) {
			axisA = "Axis A";
			axisB = "Axis B";
			twistA = "Perp Axis In A1";
			twistB = "Perp Axis In B1";
			twistA2 = "Perp Axis In A2";
			twistB2 = "Perp Axis In B2";
		} else if ( name == "bhkRagdollConstraint" ) {
			axisA = "Plane A";
			axisB = "Plane B";
			twistA = "Twist A";
			twistB = "Twist B";
		}

		if ( axisA.isEmpty() || axisB.isEmpty() || twistA.isEmpty() || twistB.isEmpty() )
			return index;

		Vector3 axis = Vector3( nif->get<Vector4>( iConstraintData, axisA ) );
		axis = transA.rotation * axis;
		axis = transB.rotation.inverted() * axis;
		nif->set<Vector4>( iConstraintData, axisB, { axis[0], axis[1], axis[2], 0 } );

		axis = Vector3( nif->get<Vector4>( iConstraintData, twistA ) );
		axis = transA.rotation * axis;
		axis = transB.rotation.inverted() * axis;
		nif->set<Vector4>( iConstraintData, twistB, { axis[0], axis[1], axis[2], 0 } );

		if ( !twistA2.isEmpty() && !twistB2.isEmpty() ) {
			axis = Vector3( nif->get<Vector4>( iConstraintData, twistA2 ) );
			axis = transA.rotation * axis;
			axis = transB.rotation.inverted() * axis;
			nif->set<Vector4>( iConstraintData, twistB2, { axis[0], axis[1], axis[2], 0 } );
		}

		return index;
	}
};

REGISTER_SPELL( spConstraintHelper )

//! Calculates Havok spring lengths
class spStiffSpringHelper final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Calculate Spring Length" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		return nif && nif->isNiBlock( nif->getBlock( idx ), "bhkStiffSpringConstraint" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & idx ) override final
	{
		QModelIndex iConstraint = nif->getBlock( idx );
		QModelIndex iSpring = nif->getIndex( iConstraint, "Stiff Spring" );
		if ( !iSpring.isValid() )
			iSpring = iConstraint;

		QModelIndex iBodyA = nif->getBlock( nif->getLink( bhkGetEntity( nif, iConstraint, "Entity A" ) ), "bhkRigidBody" );
		QModelIndex iBodyB = nif->getBlock( nif->getLink( bhkGetEntity( nif, iConstraint, "Entity B" ) ), "bhkRigidBody" );

		if ( !iBodyA.isValid() || !iBodyB.isValid() ) {
			Message::warning( nullptr, Spell::tr( "Couldn't find the bodies for this constraint" ) );
			return idx;
		}

		Transform transA = bhkBodyTrans( nif, iBodyA );
		Transform transB = bhkBodyTrans( nif, iBodyB );

		Vector3 pivotA( nif->get<Vector4>( iSpring, "Pivot A" ) );
		Vector3 pivotB( nif->get<Vector4>( iSpring, "Pivot B" ) );

		float length = ( transA * pivotA - transB * pivotB ).length();

		nif->set<float>( iSpring, "Length", length );

		return nif->getIndex( iSpring, "Length" );
	}
};

REGISTER_SPELL( spStiffSpringHelper )

//! Packs Havok strips
class spPackHavokStrips final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Pack Strips" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		return nif->isNiBlock( idx, "bhkNiTriStripsShape" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		QPersistentModelIndex iShape( iBlock );

		QVector<Vector3> vertices;
		QVector<Triangle> triangles;
		QVector<Vector3> normals;

		for ( const auto lData : nif->getLinkArray( iShape, "Strips Data" ) ) {
			QModelIndex iData = nif->getBlock( lData, "NiTriStripsData" );

			if ( iData.isValid() ) {
				QVector<Vector3> vrts = nif->getArray<Vector3>( iData, "Vertices" );
				QVector<Triangle> tris;
				QVector<Vector3> nrms;

				QModelIndex iPoints = nif->getIndex( iData, "Points" );

				for ( int x = 0; x < nif->rowCount( iPoints ); x++ ) {
					tris += triangulate( nif->getArray<quint16>( iPoints.child( x, 0 ) ) );
				}

				QMutableVectorIterator<Triangle> it( tris );

				while ( it.hasNext() ) {
					Triangle & tri = it.next();

					Vector3 a = vrts.value( tri[0] );
					Vector3 b = vrts.value( tri[1] );
					Vector3 c = vrts.value( tri[2] );

					nrms << Vector3::crossproduct( b - a, c - a ).normalize();

					tri[0] += vertices.count();
					tri[1] += vertices.count();
					tri[2] += vertices.count();
				}

				for ( const Vector3& v : vrts ) {
					vertices += v / 7;
				}
				triangles += tris;
				normals += nrms;
			}
		}

		if ( vertices.isEmpty() || triangles.isEmpty() ) {
			Message::warning( nullptr, Spell::tr( "No mesh data was found." ) );
			return iShape;
		}

		QPersistentModelIndex iPackedShape = nif->insertNiBlock( "bhkPackedNiTriStripsShape", nif->getBlockNumber( iShape ) );

		nif->set<int>( iPackedShape, "Num Sub Shapes", 1 );
		QModelIndex iSubShapes = nif->getIndex( iPackedShape, "Sub Shapes" );
		nif->updateArray( iSubShapes );
		nif->set<int>( iSubShapes.child( 0, 0 ), "Layer", 1 );
		nif->set<int>( iSubShapes.child( 0, 0 ), "Num Vertices", vertices.count() );
		nif->set<int>( iSubShapes.child( 0, 0 ), "Material", nif->get<int>( iShape, "Material" ) );
		nif->setArray<float>( iPackedShape, "Unknown Floats", { 0.0f, 0.0f, 0.1f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.1f } );
		nif->set<float>( iPackedShape, "Scale", 1.0f );
		nif->setArray<float>( iPackedShape, "Unknown Floats 2", { 1.0f, 1.0f, 1.0f } );

		QModelIndex iPackedData = nif->insertNiBlock( "hkPackedNiTriStripsData", nif->getBlockNumber( iPackedShape ) );
		nif->setLink( iPackedShape, "Data", nif->getBlockNumber( iPackedData ) );

		nif->set<int>( iPackedData, "Num Triangles", triangles.count() );
		QModelIndex iTriangles = nif->getIndex( iPackedData, "Triangles" );
		nif->updateArray( iTriangles );

		for ( int t = 0; t < triangles.size(); t++ ) {
			nif->set<Triangle>( iTriangles.child( t, 0 ), "Triangle", triangles[ t ] );
			nif->set<Vector3>( iTriangles.child( t, 0 ), "Normal", normals.value( t ) );
		}

		nif->set<int>( iPackedData, "Num Vertices", vertices.count() );
		QModelIndex iVertices = nif->getIndex( iPackedData, "Vertices" );
		nif->updateArray( iVertices );
		nif->setArray<Vector3>( iVertices, vertices );

		QMap<qint32, qint32> lnkmap;
		lnkmap.insert( nif->getBlockNumber( iShape ), nif->getBlockNumber( iPackedShape ) );
		nif->mapLinks( lnkmap );

		// *** THIS SOMETIMES CRASHES NIFSKOPE        ***
		// *** UNCOMMENT WHEN BRANCH REMOVER IS FIXED ***
		// See issue #2508255
		spRemoveBranch BranchRemover;
		BranchRemover.castIfApplicable( nif, iShape );

		return iPackedShape;
	}
};

REGISTER_SPELL( spPackHavokStrips )

//! Converts bhkListShape to bhkConvexListShape for FO3
class spConvertListShape final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Convert to bhkConvexListShape" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		return nif->isNiBlock( idx, "bhkListShape" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		QPersistentModelIndex iShape( iBlock );
		QPersistentModelIndex iRigidBody = nif->getBlock( nif->getParent( iShape ) );
		if ( !iRigidBody.isValid() )
			return {};

		auto iCLS = nif->insertNiBlock( "bhkConvexListShape" );

		nif->set<uint>( iCLS, "Num Sub Shapes", nif->get<uint>( iShape, "Num Sub Shapes" ) );
		nif->set<uint>( iCLS, "Material", nif->get<uint>( iShape, "Material" ) );
		nif->updateArray( iCLS, "Sub Shapes" );

		nif->setLinkArray( iCLS, "Sub Shapes", nif->getLinkArray( iShape, "Sub Shapes" ) );
		nif->setLinkArray( iShape, "Sub Shapes", {} );
		nif->removeNiBlock( nif->getBlockNumber( iShape ) );

		nif->setLink( iRigidBody, "Shape", nif->getBlockNumber( iCLS ) );

		return iCLS;
	}
};

REGISTER_SPELL( spConvertListShape )

//! Converts bhkConvexListShape to bhkListShape for FNV
class spConvertConvexListShape final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Convert to bhkListShape" ); }
	QString page() const override final { return Spell::tr( "Havok" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		return nif->isNiBlock( idx, "bhkConvexListShape" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		QPersistentModelIndex iShape( iBlock );
		QPersistentModelIndex iRigidBody = nif->getBlock( nif->getParent( iShape ) );
		if ( !iRigidBody.isValid() )
			return {};

		auto iLS = nif->insertNiBlock( "bhkListShape" );

		nif->set<uint>( iLS, "Num Sub Shapes", nif->get<uint>( iShape, "Num Sub Shapes" ) );
		nif->set<uint>( iLS, "Num Unknown Ints", nif->get<uint>( iShape, "Num Sub Shapes" ) );
		nif->set<uint>( iLS, "Material", nif->get<uint>( iShape, "Material" ) );
		nif->updateArray( iLS, "Sub Shapes" );
		nif->updateArray( iLS, "Unknown Ints" );

		nif->setLinkArray( iLS, "Sub Shapes", nif->getLinkArray( iShape, "Sub Shapes" ) );
		nif->setLinkArray( iShape, "Sub Shapes", {} );
		nif->removeNiBlock( nif->getBlockNumber( iShape ) );

		nif->setLink( iRigidBody, "Shape", nif->getBlockNumber( iLS ) );

		return iLS;
	}
};

REGISTER_SPELL( spConvertConvexListShape )
