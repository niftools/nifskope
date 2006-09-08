#include "../spellbook.h"

#include <QDebug>

class spLimitedHingeHelper : public Spell
{
public:
	QString name() const { return "A -> B"; }
	QString page() const { return "Havok"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && nif->isNiBlock( nif->getBlock( index ), "bhkLimitedHingeConstraint" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iConstraint = nif->getBlock( index );
		
		QModelIndex iBodyA = nif->getBlock( nif->getLink( nif->getIndex( iConstraint, "Bodies" ).child( 0, 0 ) ), "bhkRigidBody" );
		QModelIndex iBodyB = nif->getBlock( nif->getLink( nif->getIndex( iConstraint, "Bodies" ).child( 1, 0 ) ), "bhkRigidBody" );
		
		if ( ! iBodyA.isValid() || ! iBodyB.isValid() )
		{
			qWarning() << "didn't find the bodies for this constraint";
			return index;
		}
		
		Transform transA = bodyTrans( nif, iBodyA );
		Transform transB = bodyTrans( nif, iBodyB );
		
		iConstraint = nif->getIndex( iConstraint, "Limited Hinge" );
		if ( ! iConstraint.isValid() )
			return index;
		
		Vector3 pivot = Vector3( nif->get<Vector4>( iConstraint, "Pivot A" ) ) * 7.0;
		pivot = transA * pivot;
		pivot = transB.rotation.inverted() * ( pivot - transB.translation ) / transB.scale / 7.0;
		nif->set<Vector4>( iConstraint, "Pivot B", Vector4( pivot[0], pivot[1], pivot[2], 0 ) );
		
		Vector3 axle = Vector3( nif->get<Vector4>( iConstraint, "Axle A" ) );
		axle = transA.rotation * axle;
		axle = transB.rotation.inverted() * axle;
		nif->set<Vector4>( iConstraint, "Axle B", Vector4( axle[0], axle[1], axle[2], 0 ) );
		
		axle = Vector3( nif->get<Vector4>( iConstraint, "Perp2AxleInA2" ) );
		axle = transA.rotation * axle;
		axle = transB.rotation.inverted() * axle;
		nif->set<Vector4>( iConstraint, "Perp2AxleInB2", Vector4( axle[0], axle[1], axle[2], 0 ) );
		
		return index;
	}
	
	static Transform bodyTrans( const NifModel * nif, const QModelIndex & index )
	{
		Transform t;
		if ( nif->isNiBlock( index, "bhkRigidBodyT" ) )
		{
			t.translation = nif->get<Vector3>( index, "Translation" ) * 7;
			t.rotation.fromQuat( nif->get<Quat>( index, "Rotation" ) );
		}
		
		qint32 l = nif->getBlockNumber( index );
		
		while ( ( l = nif->getParent( l ) ) >= 0 )
		{
			QModelIndex iAV = nif->getBlock( l, "NiAVObject" );
			if ( iAV.isValid() )
				t = Transform( nif, iAV ) * t;
		}
		
		return t;
	}
};

REGISTER_SPELL( spLimitedHingeHelper )
