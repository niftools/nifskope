#include "blocks.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QRegularExpression>
#include <QSettings>

#include <algorithm> // std::stable_sort

// Brief description is deliberately not autolinked to class Spell
/*! \file blocks.cpp
 * \brief Block manipulation spells
 *
 * All classes here inherit from the Spell class.
 *
 * spRemoveBranch is declared in \link spells/blocks.h \endlink so that it is accessible to spCombiTris.
 */

const char * B_ERR = QT_TR_NOOP( "%1 failed with errors." );

 // Use Unicode symbols for separators
 // to lessen chance of splitting incorrectly.
 // (Previous separators were `|` and `/`)
const char * NAME_SEP = "˃"; // This is Unicode U+02C3
const char * MIME_SEP = "˂"; // This is Unicode U+02C2
const char * STR_BR = "nifskope˂nibranch˂%1";
const char * STR_BL = "nifskope˂niblock˂%1˂%2";


// Since nifxml doesn't track any of this data...

//! The valid controller types for each block
QMultiMap<QString, QString> ctlrMapping = {
	{ "NiObjectNET", "NiExtraDataController" },
	{ "NiAVObject", "NiControllerManager" },
	{ "NiAVObject", "NiVisController" },
	{ "NiAVObject", "NiTransformController" },
	{ "NiAVObject", "NiMultiTargetTransformController" },
	{ "NiParticles", "NiPSysModifierCtlr" },
	{ "NiParticles", "NiPSysUpdateCtlr" },
	{ "NiParticles", "NiPSysResetOnLoopCtlr" },
	{ "NiGeometry", "NiGeomMorpherController" },
	{ "NiLight", "NiLightColorController" },
	{ "NiLight", "NiLightDimmerController" },
	{ "NiMaterialProperty", "NiAlphaController" },
	{ "NiMaterialProperty", "NiMaterialColorController" },
	{ "NiTexturingProperty", "NiFlipController" },
	{ "NiTexturingProperty", "NiTextureTransformController" },
	// New Particles
	{ "NiPSParticleSystem", "NiPSEmitterCtlr" },
	{ "NiPSParticleSystem", "NiPSForceCtlr" },
	{ "NiPSParticleSystem", "NiPSResetOnLoopCtlr" },
	// New Geometry
	{ "NiMesh", "NiMorphWeightsController" },
};

//! The valid controller types for each block, Bethesda-only
QMultiMap<QString, QString> ctlrMappingBS = {
	// OB+
	{ "NiAVObject", "BSProceduralLightningController" },
	{ "NiAlphaProperty", "BSNiAlphaPropertyTestRefController" },
	{ "NiCamera", "BSFrustumFOVController" },
	{ "NiNode", "bhkBlendController" },
	{ "NiNode", "NiBSBoneLODController" },
	// FO3
	{ "BSShaderPPLightingProperty", "BSRefractionFirePeriodController" },
	{ "BSShaderPPLightingProperty", "BSRefractionStrengthController" },
	{ "NiMaterialProperty", "BSMaterialEmittanceMultController" },
	// SK+
	{ "NiNode", "BSLagBoneController" },
	{ "BSEffectShaderProperty", "BSEffectShaderPropertyColorController" },
	{ "BSEffectShaderProperty", "BSEffectShaderPropertyFloatController" },
	{ "BSLightingShaderProperty", "BSLightingShaderPropertyColorController" },
	{ "BSLightingShaderProperty", "BSLightingShaderPropertyFloatController" },
	// FO4
	{ "NiLight", "NiLightRadiusController" }, 
};

//! Blocks that are never used beyond 10.1.0.0
QStringList legacyOnlyBlocks = {
	"NiBone",
	"NiImage",
	"NiRawImageData",
	"NiParticleModifier",
	"NiParticleSystemController",
	"NiTriShapeSkinController",
	"NiEnvMappedTriShape",
	"NiEnvMappedTriShapeData",
	"NiTextureProperty",
	"NiMultiTextureProperty",
	"NiTransparentProperty",
	// Morrowind
	"AvoidNode",
	"RootCollisionNode",
	"NiBSAnimationNode",
	"NiBSParticleNode"
};

//! Blocks that store data for NiTimeControllers
QStringList animationData = {
	"NiPosData",
	"NiRotData",
	"NiBoolData",
	"NiFloatData",
	"NiColorData",
	"NiTransformData",
	"NiKeyframeData",
	"NiMorphData",
	"NiUVData",
	"NiVisData",
	"NiBSplineData",
	"NiBSplineBasisData",
	"NiDefaultAVObjectPalette"
};

//! The interpolators that return true for NiInterpolator::IsBoolValueSupported()
QStringList boolValue = {
	"NiBoolInterpolator",
	"NiBlendBoolInterpolator"
};
//! The interpolators that return true for NiInterpolator::IsFloatValueSupported()
QStringList floatValue = { 
	"NiFloatInterpolator",
	"NiBlendFloatInterpolator",
	"NiBSplineFloatInterpolator"
};
//! The interpolators that return true for NiInterpolator::IsPoint3ValueSupported()
QStringList point3Value = { 
	"NiPoint3Interpolator",
	"NiBlendPoint3Interpolator",
	"NiBSplinePoint3Interpolator"
};
//! The interpolators that return true for NiInterpolator::IsTransformValueSupported()
QStringList transformValue = {
	"NiTransformInterpolator",
	"NiBlendTransformInterpolator",
	"NiBlendAccumTransformInterpolator",
	"NiBSplineTransformInterpolator",
	"NiPathInterpolator",
	"NiLookAtInterpolator"
};

//! The kind of interpolator values supported on each controller
QMultiMap<QString, QStringList> interpMapping = 
{
	{ "NiBoolInterpController", boolValue },
	{ "NiFloatInterpController", floatValue },
	{ "NiPoint3InterpController", point3Value },
	{ "NiFloatExtraDataController", floatValue },
	{ "NiFloatsExtraDataController", floatValue },
	{ "NiFloatsExtraDataPoint3Controller", point3Value },
	{ "NiTransformController", transformValue },
	{ "NiMultiTargetTransformController", transformValue },
	{ "NiPSysEmitterCtlr", floatValue },      // Interpolator
	{ "NiPSysEmitterCtlr", boolValue },       // Visibility Interpolator
	{ "NiPSysModifierBoolCtlr", boolValue },
	{ "NiPSEmitterFloatCtlr", floatValue },
	{ "NiPSForceFloatCtlr", floatValue },
	{ "NiPSForceBoolCtlr", boolValue },
	{ "NiMorphWeightsController", floatValue },
	{ "NiGeomMorpherController", floatValue },
};

//! The string names which can appear in the block root
QStringList rootStringList =
{
	"Name",
	"Modifier Name",   // NiPSysModifierCtlr
	"File Name",       // NiSourceTexture
	"String Data",     // NiStringExtraData
	"Extra Data Name", // NiExtraDataController
	"Accum Root Name", // NiSequence
	"Look At Name",    // NiLookAtInterpolator
	"Driven Name",     // NiLookAtEvaluator
	"Emitter Name",    // NiPSEmitterCtlr
	"Force Name",      // NiPSForceCtlr
	"Mesh Name",       // NiPhysXMeshDesc
	"Shape Name",      // NiPhysXShapeDesc
	"Actor Name",      // NiPhysXActorDesc
	"Joint Name",      // NiPhysXJointDesc
	"Wet Material",    // BSLightingShaderProperty FO4+
	"Behaviour Graph File", // BSBehaviorGraphExtraData
};

//! Get strings array
QStringList getStringsArray( NifModel * nif, const QModelIndex & parent,
							 const QString & arr, const QString & name = {} )
{
	QStringList strings;
	auto iArr = nif->getIndex( parent, arr );
	if ( !iArr.isValid() )
		return {};

	if ( name.isEmpty() ) {
		for ( int i = 0; i < nif->rowCount( iArr ); i++ )
			strings << nif->string( iArr.child( i, 0 ) );
	} else {
		for ( int i = 0; i < nif->rowCount( iArr ); i++ )
			strings << nif->string( iArr.child( i, 0 ), name, false );
	}

	return strings;
}
//! Set strings array
void setStringsArray( NifModel * nif, const QModelIndex & parent, QStringList & strings,
					  const QString & arr, const QString & name = {} )
{
	auto iArr = nif->getIndex( parent, arr );
	if ( !iArr.isValid() )
		return;

	if ( name.isEmpty() ) {
		for ( int i = 0; i < nif->rowCount( iArr ); i++ )
			nif->set<QString>( iArr.child( i, 0 ), strings.takeFirst() );
	} else {
		for ( int i = 0; i < nif->rowCount( iArr ); i++ )
			nif->set<QString>( iArr.child( i, 0 ), name, strings.takeFirst() );
	}
}
//! Get "Name" et al. for NiObjectNET, NiExtraData, NiPSysModifier, etc.
QStringList getNiObjectRootStrings( NifModel * nif, const QModelIndex & iBlock )
{
	QStringList strings;
	for ( int i = 0; i < nif->rowCount( iBlock ); i++ ) {
		auto iString = iBlock.child( i, 0 );
		if ( rootStringList.contains( nif->itemName( iString ) ) )
			strings << nif->string( iString );
	}

	return strings;
}
//! Set "Name" et al. for NiObjectNET, NiExtraData, NiPSysModifier, etc.
void setNiObjectRootStrings( NifModel * nif, const QModelIndex & iBlock, QStringList & strings )
{
	for ( int i = 0; i < nif->rowCount( iBlock ); i++ ) {
		auto iString = iBlock.child( i, 0 );
		if ( rootStringList.contains( nif->itemName( iString ) ) )
			nif->set<QString>( iString, strings.takeFirst() );
	}
}
//! Get strings for NiMesh
QStringList getStringsNiMesh( NifModel * nif, const QModelIndex & iBlock )
{
	// "Datastreams/Component Semantics/Name" * "Num Datastreams"
	QStringList strings;
	auto iData = nif->getIndex( iBlock, "Datastreams" );
	if ( !iData.isValid() )
		return {};

	for ( int i = 0; i < nif->rowCount( iData ); i++ )
		strings << getStringsArray( nif, iData.child( i, 0 ), "Component Semantics", "Name" );

	return strings;
}
//! Set strings for NiMesh
void setStringsNiMesh( NifModel * nif, const QModelIndex & iBlock, QStringList & strings )
{
	auto iData = nif->getIndex( iBlock, "Datastreams" );
	if ( !iData.isValid() )
		return;

	for ( int i = 0; i < nif->rowCount( iData ); i++ )
		setStringsArray( nif, iData.child( i, 0 ), strings, "Component Semantics", "Name" );
}
//! Get strings for NiSequence
QStringList getStringsNiSequence( NifModel * nif, const QModelIndex & iBlock )
{
	QStringList strings;
	auto iControlledBlocks = nif->getIndex( iBlock, "Controlled Blocks" );
	if ( !iControlledBlocks.isValid() )
		return {};

	for ( int i = 0; i < nif->rowCount( iControlledBlocks ); i++ ) {
		auto iChild = iControlledBlocks.child( i, 0 );
		strings << nif->string( iChild, "Target Name", false )
				<< nif->string( iChild, "Node Name", false )
				<< nif->string( iChild, "Property Type", false )
				<< nif->string( iChild, "Controller Type", false )
				<< nif->string( iChild, "Controller ID", false )
				<< nif->string( iChild, "Interpolator ID", false );
	}

	return strings;
}
//! Set strings for NiSequence
void setStringsNiSequence( NifModel * nif, const QModelIndex & iBlock, QStringList & strings )
{
	auto iControlledBlocks = nif->getIndex( iBlock, "Controlled Blocks" );
	if ( !iControlledBlocks.isValid() )
		return;

	for ( int i = 0; i < nif->rowCount( iControlledBlocks ); i++ ) {
		auto iChild = iControlledBlocks.child( i, 0 );
		nif->set<QString>( iChild, "Target Name", strings.takeFirst() );
		nif->set<QString>( iChild, "Node Name", strings.takeFirst() );
		nif->set<QString>( iChild, "Property Type", strings.takeFirst() );
		nif->set<QString>( iChild, "Controller Type", strings.takeFirst() );
		nif->set<QString>( iChild, "Controller ID", strings.takeFirst() );
		nif->set<QString>( iChild, "Interpolator ID", strings.takeFirst() );
	}
}

//! Builds string list for datastream
QStringList serializeStrings( NifModel * nif, const QModelIndex & iBlock, const QString & type )
{
	auto strings = getNiObjectRootStrings( nif, iBlock );
	if ( nif->inherits( type, "NiSequence" ) )
		strings << getStringsNiSequence( nif, iBlock );
	else if ( type == "NiTextKeyExtraData" )
		strings << getStringsArray( nif, iBlock, "Text Keys", "Value" );
	else if ( type == "NiMesh" )
		strings << getStringsNiMesh( nif, iBlock );
	else if ( type == "NiStringsExtraData" )
		strings << getStringsArray( nif, iBlock, "Data" );
	else if ( type == "NiMorphWeightsController" )
		strings << getStringsArray( nif, iBlock, "Target Names" );
	
	if ( type == "NiMesh" || nif->inherits( type, "NiGeometry" ) )
		strings << getStringsArray( nif, nif->getIndex( iBlock, "Material Data" ), "Material Name" );;

	return strings;
}

//! Consumes string list from datastream
void deserializeStrings( NifModel * nif, const QModelIndex & iBlock, const QString & type, QStringList & strings )
{
	setNiObjectRootStrings( nif, iBlock, strings );
	if ( nif->inherits( type, "NiSequence" ) )
		setStringsNiSequence( nif, iBlock, strings );
	else if ( type == "NiTextKeyExtraData" )
		setStringsArray( nif, iBlock, strings, "Text Keys", "Value" );
	else if ( type == "NiMesh" )
		setStringsNiMesh( nif, iBlock, strings );
	else if ( type == "NiStringsExtraData" )
		setStringsArray( nif, iBlock, strings, "Data" );
	else if ( type == "NiMorphWeightsController" )
		setStringsArray( nif, iBlock, strings, "Target Names" );

	if ( type == "NiMesh" || nif->inherits( type, "NiGeometry" ) )
		setStringsArray( nif, nif->getIndex( iBlock, "Material Data" ), strings, "Material Name" );
}

//! Add a link to the specified block to a link array
/*!
 * @param nif The model
 * @param iParent The block containing the link array
 * @param array The name of the link array
 * @param link A reference to the block to insert into the link array
 */
bool addLink( NifModel * nif, const QModelIndex & iParent, const QString & array, int link )
{
	QModelIndex iSize  = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
	QModelIndex iArray = nif->getIndex( iParent, array );

	if ( iSize.isValid() && (iSize.flags() & Qt::ItemIsEnabled) ) {
		// size is valid: dynamically sized array?
		if ( iArray.isValid() && ( iArray.flags() & Qt::ItemIsEnabled ) ) {
			int numlinks = nif->get<int>( iSize );
			nif->set<int>( iSize, numlinks + 1 );
			nif->updateArray( iArray );
			nif->setLink( iArray.child( numlinks, 0 ), link );
			return true;
		}

	} else if ( iArray.isValid() && (iArray.flags() & Qt::ItemIsEnabled) ) {
		// static array, find a empty entry and insert link there
		NifItem * item = static_cast<NifItem *>( iArray.internalPointer() );

		if ( nif->isArray( iArray ) && item ) {
			for ( int c = 0; c < item->childCount(); c++ ) {
				if ( item->child( c )->value().toLink() == -1 ) {
					nif->setLink( iArray.child( c, 0 ), link );
					return true;
				}
			}
		}
	}

	return false;
}

//! Remove a link to a block from the specified link array
/*!
 * @param nif The model
 * @param iParent The block containing the link array
 * @param array The name of the link array
 * @param link A reference to the block to remove from the link array
 */
void delLink( NifModel * nif, const QModelIndex & iParent, QString array, int link )
{
	QModelIndex iSize   = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
	QModelIndex iArray  = nif->getIndex( iParent, array );
	QList<qint32> links = nif->getLinkArray( iArray ).toList();

	if ( iSize.isValid() && iArray.isValid() && links.contains( link ) ) {
		links.removeAll( link );
		nif->set<int>( iSize, links.count() );
		nif->updateArray( iArray );
		nif->setLinkArray( iArray, links.toVector() );
	}
}


//! Link one block to another
/*!
* @param nif The model
* @param index The block to link to (becomes parent)
* @param iBlock The block to link (becomes child)
*/
void blockLink( NifModel * nif, const QModelIndex & index, const QModelIndex & iBlock )
{
	if ( nif->isLink( index ) && nif->inherits( iBlock, nif->itemTmplt( index ) ) ) {
		nif->setLink( index, nif->getBlockNumber( iBlock ) );
	}

	if ( nif->inherits( index, "NiNode" ) && nif->inherits( iBlock, "NiAVObject" ) ) {
		addLink( nif, index, "Children", nif->getBlockNumber( iBlock ) );

		if ( nif->inherits( iBlock, "NiDynamicEffect" ) ) {
			addLink( nif, index, "Effects", nif->getBlockNumber( iBlock ) );
		}
	} else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiProperty" ) ) {
		if ( !addLink( nif, index, "Properties", nif->getBlockNumber( iBlock ) ) ) {
			// Absent in Bethesda 20.2.0.7 stream version > 34
			if ( nif->inherits( nif->getBlockName( iBlock ), "BSShaderProperty" ) ) {
				nif->setLink( index, "Shader Property", nif->getBlockNumber( iBlock ) );
			} else if ( nif->getBlockName( iBlock ) == "NiAlphaProperty" ) {
				nif->setLink( index, "Alpha Property", nif->getBlockNumber( iBlock ) );
			}
		}
	} else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiExtraData" ) ) {
		addLink( nif, index, "Extra Data List", nif->getBlockNumber( iBlock ) );
	} else if ( nif->inherits( index, "NiObjectNET" ) && nif->inherits( iBlock, "NiTimeController" ) ) {
		if ( nif->getLink( index, "Controller" ) > 0 ) {
			blockLink( nif, nif->getBlock( nif->getLink( index, "Controller" ) ), iBlock );
		} else {
			nif->setLink( index, "Controller", nif->getBlockNumber( iBlock ) );
			nif->setLink( iBlock, "Target", nif->getBlockNumber( index ) );
		}
	} else if ( nif->inherits( index, "NiTimeController" ) && nif->inherits( iBlock, "NiTimeController" ) ) {
		if ( nif->getLink( index, "Next Controller" ) > 0 ) {
			blockLink( nif, nif->getBlock( nif->getLink( index, "Next Controller" ) ), iBlock );
		} else {
			nif->setLink( index, "Next Controller", nif->getBlockNumber( iBlock ) );
			nif->setLink( iBlock, "Target", nif->getLink( index, "Target" ) );
		}
	} else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiCollisionObject" ) ) {
		nif->setLink( index, "Collision Object", nif->getBlockNumber( iBlock ) );
	}
}

//! Helper function for branch paste
static qint32 getBlockByName( NifModel * nif, const QString & tn )
{
	QStringList ls = tn.split( NAME_SEP );
	QString type = ls.value( 0 );
	QString name = ls.value( 1 );

	if ( type.isEmpty() || name.isEmpty() )
		return -1;

	for ( int b = 0; b < nif->getBlockCount(); b++ ) {
		QModelIndex iBlock = nif->getBlock( b );

		if ( nif->itemName( iBlock ) == type && nif->get<QString>( iBlock, "Name" ) == name )
			return b;
	}

	return -1;
}

//! Helper function for branch copy
static void populateBlocks( QList<qint32> & blocks, NifModel * nif, qint32 block )
{
	if ( !blocks.contains( block ) )
		blocks.append( block );

	for ( const auto link : nif->getChildLinks( block ) ) {
		populateBlocks( blocks, nif, link );
	}
}

//! Remove the children from the specified block
static void removeChildren( NifModel * nif, const QPersistentModelIndex & iBlock )
{
	// Build list of child links
	QVector<QPersistentModelIndex> iChildren;
	for ( const auto link : nif->getChildLinks( nif->getBlockNumber( iBlock ) ) ) {
		iChildren.append( nif->getBlock( link ) );
	}

	// Remove children of child links
	for ( const QPersistentModelIndex& iChild : iChildren ) {
		if ( iChild.isValid() && nif->getBlockNumber( iBlock ) == nif->getParent( nif->getBlockNumber( iChild ) ) ) {
			removeChildren( nif, iChild );
		}
	}

	// Remove children
	for ( const QPersistentModelIndex& iChild : iChildren ) {
		if ( iChild.isValid() && nif->getBlockNumber( iBlock ) == nif->getParent( nif->getBlockNumber( iChild ) ) ) {
			nif->removeNiBlock( nif->getBlockNumber( iChild ) );
		}
	}
}

//! Set values in blocks that cannot be handled in nif.xml such as inherited values
void blockDefaults( NifModel * nif, const QString & type, const QModelIndex & index )
{
	// Set Bethesda NiExtraData names to their required strings
	static QMap<QString, QString> nameMap = {
		{"BSBehaviorGraphExtraData", "BGED"},
		{"BSBoneLODExtraData", "BSBoneLOD"},
		{"BSBound", "BBX"},
		{"BSClothExtraData", "CED"},
		{"BSConnectPoint::Children", "CPT"},
		{"BSConnectPoint::Parents", "CPA"},
		{"BSDecalPlacementVectorExtraData", "DVPG"},
		{"BSDistantObjectLargeRefExtraData", "DOLRED"},
		{"BSEyeCenterExtraData", "ECED"},
		{"BSFurnitureMarker", "FRN"},
		{"BSFurnitureMarkerNode", "FRN"},
		{"BSInvMarker", "INV"},
		{"BSPositionData", "BSPosData"},
		{"BSWArray", "BSW"},
		{"BSXFlags", "BSX"},
	};

	auto iterName = nameMap.find( type );
	if ( iterName != nameMap.end() )
		nif->set<QString>( nif->getIndex( index, "Name" ), iterName.value() );
}

//! Filters a list of blocks based on version (since nif.xml lacks this data)
void blockFilter( NifModel * nif, std::list<QString>& blocks, const QString & type = {} )
{
	blocks.erase( std::remove_if( blocks.begin(), blocks.end(),
		[nif, type] ( const QString& s ) { return !nif->inherits( s, type )
			// Obsolete/Undecoded
			|| s.startsWith( "NiClod" ) || s.startsWith( "NiArk" ) || s.startsWith( "NiBez" )
			|| s.startsWith( "Ni3ds" ) || s.startsWith( "NiBinaryVox" )
			// Legacy
			|| ( ( (nif->inherits( s, "NiParticles" ) && !nif->inherits( s, "NiParticleSystem" ))
				   || (nif->inherits( s, "NiParticlesData" ) && !s.startsWith( "NiP" )) // NiRotating, NiAutoNormal, etc.
				   || nif->inherits( s, legacyOnlyBlocks ) )
				 && nif->getVersionNumber() > 0x0a010000 )
			// Bethesda
			|| ( (s.startsWith( "bhk" ) || s.startsWith( "hk" ) || s.startsWith( "BS" )
				 || s.endsWith( "ShaderProperty" )) && nif->getUserVersion2() == 0 )
			// Introduced in 20.2.0.8
			|| (( s.startsWith( "NiPhysX" ) && nif->getVersionNumber() < 0x14020008 ))
			// Introduced in 20.5
			|| ( ((s.startsWith( "NiPS" ) && !s.contains( "PSys" )) || s.startsWith( "NiMesh" )
				   || s.contains( "Evaluator" )
				   ) && nif->getVersionNumber() < 0x14050000 )
			// Deprecated in 20.5
			|| ( (s.startsWith( "NiParticle" ) || s.contains( "PSys" ) || s.startsWith( "NiTri" )
				   || s.contains( "Interpolator" )
				   ) && nif->getVersionNumber() >= 0x14050000 );
		} ),
		blocks.end()
	);
}

//! Creates a menu structure for a list of blocks
QMap<QString, QMenu *> blockMenu( NifModel * nif, const std::list<QString> & blocks, bool categorize = false, bool filter = false )
{
	QMap<QString, QMenu *> map;
	auto ids = blocks;
	ids.sort();
	if ( filter )
		blockFilter( nif, ids );

	bool firstCat = false;
	for ( const QString& id : ids ) {
		QString alph( "Other" );
		QString beth = (nif->getUserVersion2() == 0) ? alph : "Bethesda";
		QString hk = (nif->getUserVersion2() == 0) ? alph : "Havok";

		bool alphabetized = false;
		// Group Old Particles
		if ( id.contains( "PSys" ) )
			alph = QString( "Ni&P(Sys)..." );
		// Group New Particles
		else if ( id.startsWith( "NiPS" ) )
			alph = QString( "Ni&P(S)..." );
		// Group Havok
		else if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
			alph = hk;
		// Group PhysX
		else if ( id.startsWith( "NiPhysX" ) )
			alph = "PhysX";
		// Group Bethesda
		else if ( id.startsWith( "BS" ) || id.endsWith( "ShaderProperty" ) )
			alph = beth;
		// Group Custom
		else if ( !id.startsWith( "Ni" )
				  || (id.startsWith( "NiBS" ) && !id.startsWith( "NiBSp" )) // Bethesda but not NiBSpline
				  || id.startsWith( "NiDeferred" ) )
			alph = "Other";
		// Alphabetize Everything else Ni
		else if ( id.startsWith( "Ni" ) ) {
			alph = QString( "Ni&" ) + id.mid( 2, 1 ) + "...";
			alphabetized = true;
		}


		// Categories
		QString cat;
		if ( !alphabetized )
			cat = ""; // Already grouped well above
		else if ( nif->inherits( id, "NiInterpolator" ) || nif->inherits( id, "NiEvaluator" )
				  || nif->inherits( id, "NiTimeController" )
				  || id.contains( "Sequence" )
				  || animationData.contains( id ) )
			cat = "NiAnimation...";
		else if ( nif->inherits( id, "NiNode" ) )
			cat = "NiNode...";
		else if ( nif->inherits( id, "NiGeometry" ) || nif->inherits( id, "NiGeometryData" )
				  || id.contains( "Skin" ) )
			cat = "NiGeometry...";
		else if ( nif->inherits( id, "NiAVObject" ) )
			cat = "NiAVObject...";
		else if ( nif->inherits( id, "NiExtraData" ) )
			cat = "NiExtraData...";
		else if ( nif->inherits( id, "NiProperty" ) )
			cat = "NiProperty...";
		else if ( nif->inherits( id, "NiObject" ) )
			cat = "NiObject...";

		if ( !map.contains( alph ) )
			map[alph] = new QMenu( alph );

		map[alph]->addAction( id );

		if ( categorize && !cat.isEmpty() ) {
			if ( !firstCat ) {
				// Use NiAAA to place it alphabetically between NiZBu and NiA[a-z]
				// which will split the alphabetization and categorization.
				map["NiAAA"] = new QMenu( "" );
				firstCat = true;
			}

			if ( !map.contains( cat ) )
				map[cat] = new QMenu( cat );

			map[cat]->addAction( id );
		}
	}

	return map;
}

//! Insert an unattached block
class spInsertBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Insert" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return ( !index.isValid() || !index.parent().isValid() );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QMenu menu;
		menu.addSection( tr( "Alphabetical" ) );
		for ( QMenu * m : blockMenu( nif, NifModel::allNiBlocks().toStdList(), true, true ) ) {
			if ( m->title().isEmpty() )
				menu.addSection( tr( "Categories" ) );
			else if ( m->actions().size() == 1 )
				menu.addAction( m->actions().at( 0 ) );
			else
				menu.addMenu( m );
		}

		QAction * act = menu.exec( QCursor::pos() );

		if ( act ) {
			// insert block
			QModelIndex newindex = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );

			// Set values that can't be handled by defaults in nif.xml
			blockDefaults( nif, act->text(), newindex );

			// return index to new block
			return newindex;
		}

		return index;
	}
};

REGISTER_SPELL( spInsertBlock )

//! Attach a Property to a block
class spAttachProperty final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Attach Property" ); }
	QString page() const override final { return Spell::tr( "Node" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		if ( nif->itemType( index ) != "NiBlock" )
			return false;

		if ( nif->getUserVersion() < 12 )
			return nif->inherits( index, "NiAVObject" ); // Not Skyrim

		// Skyrim and later
		return nif->inherits( index, "NiGeometry" ) || nif->inherits( index, "BSTriShape" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		for ( const QString& id : ids ) {
			if ( (nif->inherits( index, "NiGeometry" ) || nif->inherits( index, "BSTriShape" ))
				 && nif->getUserVersion2() > 34 ) {
				if ( !(id == "BSLightingShaderProperty" || id == "BSEffectShaderProperty" || id == "NiAlphaProperty") )
					continue;
			}

			if ( nif->inherits( id, "NiProperty" ) )
				menu.addAction( id );
		}

		if ( menu.actions().isEmpty() )
			return index;

		QAction * act = menu.exec( QCursor::pos() );

		if ( act ) {
			QPersistentModelIndex iParent = index;
			QModelIndex iProperty = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );

			if ( !addLink( nif, iParent, "Properties", nif->getBlockNumber( iProperty ) ) ) {
				// Skyrim and later
				auto name = nif->getBlockName( iProperty );
				if ( name == "BSLightingShaderProperty" || name == "BSEffectShaderProperty" ) {
					if ( !nif->setLink( iParent, "Shader Property", nif->getBlockNumber( iProperty ) ) ) {
						qCWarning( nsSpell ) << Spell::tr( "Failed to attach property." );
					}
				} else if ( name == "NiAlphaProperty" ) {
					if ( !nif->setLink( iParent, "Alpha Property", nif->getBlockNumber( iProperty ) ) ) {
						qCWarning( nsSpell ) << Spell::tr( "Failed to attach property." );
					}
				}
			}

			return iProperty;
		}

		return index;
	}
};

REGISTER_SPELL( spAttachProperty )

//! Attach a Node to a block
class spAttachNode final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Attach Node" ); }
	QString page() const override final { return Spell::tr( "Node" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiNode" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		for ( const QString& id : ids ) {
			if ( nif->inherits( id, "NiAVObject" ) && !nif->inherits( id, "NiDynamicEffect" ) )
				menu.addAction( id );
		}

		QAction * act = menu.exec( QCursor::pos() );

		if ( act ) {
			QPersistentModelIndex iParent = index;
			QModelIndex iNode = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iNode ) );
			return iNode;
		}

		return index;
	}
};

REGISTER_SPELL( spAttachNode )


//! Attach a new block to an empty Ref link
class spAddNewRef final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Attach" ); }
	bool instant() const { return true; }
	QIcon icon() const { return QIcon( ":img/add" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		auto val = nif->getValue( index );
		if ( val.type() == NifValue::tLink )
			return nif->isLink( index ) && nif->getLink( index ) == -1;
		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		NifItem * item = static_cast<NifItem *>(index.internalPointer());
		auto type = item->temp();

		std::list<QString> allIds = nif->allNiBlocks().toStdList();
		blockFilter( nif, allIds, type );

		auto iBlock = nif->getBlock( index );

		std::list<QString> ids;
		auto ctlrFilter = [nif, &ids, &allIds, &iBlock] ( QMultiMap<QString, QString> m ) {
			auto i = m.begin();
			while ( i != m.end() ) {
				if ( nif->inherits( nif->getBlockName( iBlock ), i.key() ) )
					for ( const auto & id : allIds )
						if ( nif->inherits( id, i.value() ) )
							ids.push_back( id );
				++i;
			}
		};

		auto interpFilter = [nif, &ids, &allIds, &iBlock]( QMultiMap<QString, QStringList> m ) {
			auto i = m.begin();
			while ( i != m.end() ) {
				if ( nif->inherits( nif->getBlockName( iBlock ), i.key() ) )
					for ( const auto & id : allIds )
						for ( const auto & s : i.value() )
							if ( nif->inherits( id, s ) )
								ids.push_back( id );
				++i;
			}
		};

		if ( nif->inherits( type, "NiTimeController" ) ) {
			// Show only applicable types for controller links for the given block
			if ( nif->inherits( iBlock, "NiTimeController" ) && item->name() == "Next Controller" )
				iBlock = nif->getBlock( nif->getLink( index.parent(), "Target" ) );

			if ( nif->getVersionNumber() > 0x14050000 ) {
				ctlrMapping.insertMulti( "NiNode", "NiSkinningLODController" );
			}
			// Block-to-Controller Mapping
			ctlrFilter( ctlrMapping );
			// Bethesda Controllers
			if ( nif->getUserVersion2() > 0 )
				ctlrFilter( ctlrMappingBS );

		} else if ( nif->inherits( iBlock, "NiTimeController" ) 
					&& nif->inherits( type, "NiInterpolator" ) ) {
			// Show only applicable types for interpolator links for the given block
			interpFilter( interpMapping );
		} else {
			ids = allIds;
		}
		
		ids.sort();
		ids.unique();

		QMenu menu;
		if ( ids.size() < 8 ) {
			for ( const QString& id : ids )
				menu.addAction( id );
		} else {
			for ( QMenu * m : blockMenu( nif, ids ) ) {
				if ( m->actions().size() == 1 )
					menu.addAction( m->actions().at(0) );
				else
					menu.addMenu( m );
			}
		}

		QAction * act;
		if ( ids.size() == 1 )
			act = menu.actions().at(0);
		else
			act = menu.exec( QCursor::pos() );

		if ( act ) {
			// insert block
			QModelIndex newindex = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );

			if ( !nif->setLink( index, nif->getBlockNumber( newindex ) ) ) {
				qCWarning( nsSpell ) << tr( "Failed to attach link." );
			}

			if ( nif->inherits( nif->getBlockName( newindex ), "NiTimeController" ) ) {
				auto blk = nif->getBlockNumber( iBlock );
				nif->setLink( newindex, "Target", blk );
			}

			// Set values that can't be handled by defaults in nif.xml
			blockDefaults( nif, act->text(), newindex );

			// return index to new block
			return newindex;
		}

		return index;
	}
};

REGISTER_SPELL( spAddNewRef )


//! Attach a dynamic effect (4/5 are lights) to a block
class spAttachLight final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Attach Effect" ); }
	QString page() const override final { return Spell::tr( "Node" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiNode" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		for ( const QString& id : ids ) {
			if ( nif->inherits( id, "NiDynamicEffect" ) )
				menu.addAction( id );
		}


		QAction * act = menu.exec( QCursor::pos() );

		if ( act ) {
			QPersistentModelIndex iParent = index;
			QModelIndex iLight = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iLight ) );
			addLink( nif, iParent, "Effects", nif->getBlockNumber( iLight ) );

			if ( nif->checkVersion( 0, 0x04000002 ) ) {
				nif->set<int>( iLight, "Num Affected Nodes", 1 );
				nif->updateArray( iLight, "Affected Nodes" );
				nif->updateArray( iLight, "Affected Node Pointers" );
			}

			if ( act->text() == "NiTextureEffect" ) {
				nif->set<int>( iLight, "Flags", 4 );
				QModelIndex iSrcTex = nif->insertNiBlock( "NiSourceTexture", nif->getBlockNumber( iLight ) + 1 );
				nif->setLink( iLight, "Source Texture", nif->getBlockNumber( iSrcTex ) );
			}

			return iLight;
		}

		return index;
	}
};

REGISTER_SPELL( spAttachLight )

//! Attach extra data to a block
class spAttachExtraData final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Attach Extra Data" ); }
	QString page() const override final { return Spell::tr( "Node" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiObjectNET" ) && nif->checkVersion( 0x0a000100, 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		for ( const QString& id : ids ) {
			if ( nif->inherits( id, "NiExtraData" ) )
				menu.addAction( id );
		}

		QAction * act = menu.exec( QCursor::pos() );

		if ( act ) {
			QPersistentModelIndex iParent = index;
			QModelIndex iExtra = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );

			// Set values that can't be handled by defaults in nif.xml
			blockDefaults( nif,  act->text(), iExtra );

			addLink( nif, iParent, "Extra Data List", nif->getBlockNumber( iExtra ) );
			return iExtra;
		} 

		return index;
	}
};

REGISTER_SPELL( spAttachExtraData )

//! Remove a block
class spRemoveBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Remove" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) >= 0;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		nif->removeNiBlock( nif->getBlockNumber( index ) );
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlock )

//! Copy a block to the clipboard
class spCopyBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Copy" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return{ Qt::CTRL + Qt::SHIFT + Qt::Key_C }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QByteArray data;
		QBuffer buffer( &data );
		if ( !buffer.open( QIODevice::WriteOnly ) )
			return {};

		QDataStream ds( &buffer );

		auto bType = nif->createRTTIName( index );

		if ( nif->checkVersion( 0x14010001, 0 ) )
			ds << serializeStrings( nif, index, bType );

		if ( nif->saveIndex( buffer, index ) ) {
			QMimeData * mime = new QMimeData;
			mime->setData( QString( STR_BL ).arg( nif->getVersion(), bType ), data );
			QApplication::clipboard()->setMimeData( mime );
		}

		return index;
	}
};

REGISTER_SPELL( spCopyBlock )

//! Paste a block from the clipboard
class spPasteBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Paste" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	QPair<QString, QString> acceptFormat( const QString & format, const NifModel * nif )
	{
		QStringList split = format.split( MIME_SEP );

		NiMesh::DataStreamMetadata metadata = {};
		auto bType = nif->extractRTTIArgs( split.value( MIME_IDX_TYPE ), metadata );
		if ( !NifModel::isNiBlock( bType ) )
			return {};

		if ( split.value( MIME_IDX_APP ) == "nifskope" && split.value( MIME_IDX_STREAM ) == "niblock" )
			return {split.value( MIME_IDX_VER ), bType};

		return {};
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( index );
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( mime ) {
			for ( const QString& form : mime->formats() ) {
				if ( !acceptFormat( form, nif ).first.isEmpty() )
					return true;
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( mime ) {
			for ( const QString& form : mime->formats() ) {
				auto result = acceptFormat( form, nif );
				auto version = result.first;

				NiMesh::DataStreamMetadata metadata = {};
				auto bType = nif->extractRTTIArgs( result.second, metadata );

				if ( !version.isEmpty()
					 && (version == nif->getVersion() || QMessageBox::question( nullptr,
							tr( "Paste Block" ),
							tr( "Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." )
								.arg( nif->getVersion() )
								.arg( version ),
							tr( "Continue" ),
							tr( "Cancel" ) ) == 0)
				) {
					QByteArray data = mime->data( form );
					QBuffer buffer( &data );

					if ( buffer.open( QIODevice::ReadOnly ) ) {
						QDataStream ds( &buffer );
						QStringList strings;
						if ( nif->checkVersion( 0x14010001, 0 ) )
							ds >> strings;

						QModelIndex block = nif->insertNiBlock( bType, nif->getBlockCount() );
						nif->loadIndex( buffer, block );
						blockLink( nif, index, block );

						// Post-Load corrections

						// NiDataStream RTTI arg values
						if ( nif->checkVersion( 0x14050000, 0 ) && bType == QLatin1String( "NiDataStream" ) ) {
							nif->set<quint32>( block, "Usage", metadata.usage );
							nif->set<quint32>( block, "Access", metadata.access );
						}

						// Set strings
						if ( nif->checkVersion( 0x14010001, 0 ) )
							deserializeStrings( nif, block, bType, strings );

						return block;
					}
				}
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBlock )

//! Paste a block from the clipboard over another
class spPasteOverBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Paste Over" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return{ Qt::CTRL + Qt::SHIFT + Qt::Key_V }; }

	QPair<QString, QString> acceptFormat( const QString & format, const NifModel * nif, const QModelIndex & iBlock )
	{
		QStringList split = format.split( MIME_SEP );

		NiMesh::DataStreamMetadata metadata = {};
		auto bType = nif->extractRTTIArgs( split.value( MIME_IDX_TYPE ), metadata );
		if ( !nif->isNiBlock( iBlock, bType ) )
			return {};

		if ( split.value( MIME_IDX_APP ) == "nifskope"
			 && split.value( MIME_IDX_STREAM ) == "niblock" )
			return {split.value( MIME_IDX_VER ), bType};

		return {};
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( mime ) {
			for ( const QString& form : mime->formats() ) {
				if ( !acceptFormat( form, nif, index ).first.isEmpty() )
					return true;
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( mime ) {
			for ( const QString& form : mime->formats() ) {
				auto result = acceptFormat( form, nif, index );
				auto version = result.first;

				NiMesh::DataStreamMetadata metadata = {};
				auto bType = nif->extractRTTIArgs( result.second, metadata );

				if ( !version.isEmpty()
					 && (version == nif->getVersion() || QMessageBox::question( nullptr,
							tr( "Paste Over" ),
							tr( "Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." )
								.arg( nif->getVersion() )
							    .arg( version ),
							tr( "Continue" ),
							tr( "Cancel" ) ) == 0) )
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( &data );
					if ( buffer.open( QIODevice::ReadOnly ) ) {
						QDataStream ds( &buffer );

						QStringList strings;
						if ( nif->checkVersion( 0x14010001, 0 ) )
							ds >> strings;

						nif->loadIndex( buffer, index );

						// NiDataStream RTTI arg values
						if ( nif->checkVersion( 0x14050000, 0 ) && bType == QLatin1String( "NiDataStream" ) ) {
							nif->set<quint32>( index, "Usage", metadata.usage );
							nif->set<quint32>( index, "Access", metadata.access );
						}

						// Set strings
						if ( nif->checkVersion( 0x14010001, 0 ) )
							deserializeStrings( nif, index, bType, strings );

						return index;
					}
				}
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteOverBlock )

//! Copy a branch (a block and its descendents) to the clipboard

bool spCopyBranch::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	return nif->isNiBlock( index );
}

QModelIndex spCopyBranch::cast( NifModel * nif, const QModelIndex & index )
{
	QList<qint32> blocks;
	populateBlocks( blocks, nif, nif->getBlockNumber( index ) );

	QMap<qint32, qint32> blockMap;

	for ( int b = 0; b < blocks.count(); b++ )
		blockMap.insert( blocks[b], b );

	QMap<qint32, QString> parentMap;
	for ( const auto block : blocks )
	{
		for ( const auto link : nif->getParentLinks( block ) ) {
			if ( !blocks.contains( link ) && !parentMap.contains( link ) ) {
				QString failMessage = Spell::tr( "parent link invalid" );
				QModelIndex iParent = nif->getBlock( link );

				if ( iParent.isValid() ) {
					failMessage = Spell::tr( "parent unnamed" );
					QString name = nif->get<QString>( iParent, "Name" );

					if ( !name.isEmpty() ) {
						parentMap.insert( link, nif->itemName( iParent ) + NAME_SEP + name );
						continue;
					}
				}

				Message::append( tr( B_ERR ).arg( name() ),
								 tr( "failed to map parent link %1 %2 for block %3 %4; %5." )
									.arg( link )
									.arg( nif->itemName( nif->getBlock( link ) ) )
									.arg( block )
									.arg( nif->itemName( nif->getBlock( block ) ) )
									.arg( failMessage ),
								 QMessageBox::Critical
				);
				return index;
			}
		}
	}

	QByteArray data;
	QBuffer buffer( &data );

	if ( buffer.open( QIODevice::WriteOnly ) ) {
		QDataStream ds( &buffer );
		ds << blocks.count();
		ds << blockMap;
		ds << parentMap;

		for ( const auto block : blocks ) {
			auto iBlock = nif->getBlock( block );
			auto bType = nif->createRTTIName( iBlock );

			ds << bType;

			if ( nif->checkVersion( 0x14010001, 0 ) )
				ds << serializeStrings( nif, iBlock, bType );

			if ( !nif->saveIndex( buffer, iBlock ) ) {
				Message::append( tr( B_ERR ).arg( name() ),
								 tr( "failed to save block %1 %2." ).arg( block ).arg( bType ),
								 QMessageBox::Critical
				);
				return index;
			}
		}

		QMimeData * mime = new QMimeData;
		mime->setData( QString( STR_BR ).arg( nif->getVersion() ), data );
		QApplication::clipboard()->setMimeData( mime );
	}

	return index;
}


REGISTER_SPELL( spCopyBranch )

//! Paste a branch from the clipboard

QString spPasteBranch::acceptFormat( const QString & format, const NifModel * nif )
{
	Q_UNUSED( nif );
	QStringList split = format.split( MIME_SEP );

	if ( split.value( MIME_IDX_APP ) == "nifskope" && split.value( MIME_IDX_STREAM ) == "nibranch" )
		return split.value( MIME_IDX_VER );

	return QString();
}

bool spPasteBranch::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	if ( index.isValid() && !nif->isNiBlock( index ) && !nif->isLink( index ) )
		return false;

	const QMimeData * mime = QApplication::clipboard()->mimeData();

	if ( index.isValid() && mime ) {
		for ( const QString& form : mime->formats() ) {
			if ( nif->isVersionSupported( nif->version2number( acceptFormat( form, nif ) ) ) )
				return true;
		}
	}

	return false;
}

QModelIndex spPasteBranch::cast( NifModel * nif, const QModelIndex & index )
{
	const QMimeData * mime = QApplication::clipboard()->mimeData();

	if ( mime ) {
		for ( const QString& form : mime->formats() ) {
			QString v = acceptFormat( form, nif );

			if ( !v.isEmpty()
				&& ( v == nif->getVersion()
					|| QMessageBox::question( 0, tr( "Paste Branch" ),
					        tr( "Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." )
					            .arg( nif->getVersion() ).arg( v ), tr( "Continue" ),
					        tr( "Cancel" )
						) == 0 
				    )
				)
			{
				QByteArray data = mime->data( form );
				QBuffer buffer( &data );

				if ( buffer.open( QIODevice::ReadOnly ) ) {
					QDataStream ds( &buffer );

					int count;
					ds >> count;

					QMap<qint32, qint32> blockMap;
					ds >> blockMap;
					QMutableMapIterator<qint32, qint32> ibm( blockMap );

					auto origBlockCount = nif->getBlockCount();
					while ( ibm.hasNext() ) {
						ibm.next();
						ibm.value() += origBlockCount;
					}

					QMap<qint32, QString> parentMap;
					ds >> parentMap;

					QMapIterator<qint32, QString> ipm( parentMap );

					while ( ipm.hasNext() ) {
						ipm.next();
						qint32 block = getBlockByName( nif, ipm.value() );

						if ( ipm.key() == 0 ) {
							// Ignore Root
							blockMap.insert( ipm.key(), 0 );
						} else if ( block > 0 ) {
							blockMap.insert( ipm.key(), block );
						} else {
							Message::append( tr( B_ERR ).arg( name() ),
											 tr( "failed to map parent link %1" ).arg( ipm.value() ),
											 QMessageBox::Critical
							);
							return index;
						}
					}

					QModelIndex iRoot;

					nif->holdUpdates( true );
					for ( int c = 0; c < count; c++ ) {
						QString bType;
						QStringList strings;
						ds >> bType;
						if ( nif->checkVersion( 0x14010001, 0 ) )
							ds >> strings;

						NiMesh::DataStreamMetadata metadata = {};
						bType = nif->extractRTTIArgs( bType, metadata );

						QModelIndex block = nif->insertNiBlock( bType, -1 );
						if ( !nif->loadAndMapLinks( buffer, block, blockMap ) )
							return index;

						// NiDataStream RTTI arg values
						if ( nif->checkVersion( 0x14050000, 0 ) && bType == QLatin1String( "NiDataStream" ) ) {
							nif->set<quint32>( block, "Usage", metadata.usage );
							nif->set<quint32>( block, "Access", metadata.access );
						}

						// Set strings
						if ( nif->checkVersion( 0x14010001, 0 ) )
							deserializeStrings( nif, block, bType, strings );


						if ( c == 0 )
							iRoot = block;
					}
					nif->holdUpdates( false );

					blockLink( nif, index, iRoot );

					return iRoot;
				}
			}
		}
	}

	return QModelIndex();
}


REGISTER_SPELL( spPasteBranch )

//! Paste branch without parenting; see spPasteBranch
/*!
 * This was originally a dodgy hack involving duplicating the contents of
 * spPasteBranch and neglecting to link the blocks; now it calls
 * spPasteBranch with a bogus index.
 */
class spPasteBranch2 final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Paste At End" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	// hotkey() won't work here, probably because the context menu is not available

	QString acceptFormat( const QString & format, const NifModel * nif )
	{
		Q_UNUSED( nif );
		QStringList split = format.split( MIME_SEP );

		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "nibranch" )
			return split.value( 2 );

		return QString();
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		//if ( index.isValid() && ! nif->isNiBlock( index ) && ! nif->isLink( index ) )
		//	return false;
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( mime && !index.isValid() ) {
			for ( const QString& form : mime->formats() ) {
				if ( nif->isVersionSupported( nif->version2number( acceptFormat( form, nif ) ) ) )
					return true;
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( index );
		spPasteBranch paster;
		paster.cast( nif, QModelIndex() );
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBranch2 )

// definitions for spRemoveBranch moved to blocks.h
bool spRemoveBranch::isApplicable( const NifModel * nif, const QModelIndex & iBlock )
{
	int ix = nif->getBlockNumber( iBlock );
	return ( nif->isNiBlock( iBlock ) && ix >= 0 && ( nif->getRootLinks().contains( ix ) || nif->getParent( ix ) >= 0 ) );
}

QModelIndex spRemoveBranch::cast( NifModel * nif, const QModelIndex & index )
{
	QPersistentModelIndex iBlock = index;
	removeChildren( nif, iBlock );
	nif->removeNiBlock( nif->getBlockNumber( iBlock ) );
	return QModelIndex();
}

REGISTER_SPELL( spRemoveBranch )

//! Convert descendents to siblings?
class spFlattenBranch final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Flatten Branch" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ), "NiNode" );
		return nif->inherits( index, "NiNode" ) && iParent.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iNode ) override final
	{
		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( iNode ) ), "NiNode" );
		doNode( nif, iNode, iParent, Transform() );
		return iNode;
	}

	void doNode( NifModel * nif, const QModelIndex & iNode, const QModelIndex & iParent, const Transform & tp )
	{
		if ( !nif->inherits( iNode, "NiNode" ) )
			return;

		Transform t = tp * Transform( nif, iNode );

		QList<qint32> links;

		for ( const auto l : nif->getLinkArray( iNode, "Children" ) ) {
			QModelIndex iChild = nif->getBlock( l );

			if ( nif->getParent( nif->getBlockNumber( iChild ) ) == nif->getBlockNumber( iNode ) ) {
				Transform tc = t * Transform( nif, iChild );
				tc.writeBack( nif, iChild );
				addLink( nif, iParent, "Children", l );
				delLink( nif, iNode, "Children", l );
				links.append( l );
			}
		}

		for ( const auto l : links ) {
			doNode( nif, nif->getBlock( l, "NiNode" ), iParent, tp );
		}
	}
};

REGISTER_SPELL( spFlattenBranch )

//! Move a block up in the NIF
class spMoveBlockUp final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Move Up" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return { Qt::CTRL + Qt::Key_Up }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) > 0;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		int ix = nif->getBlockNumber( iBlock );
		nif->moveNiBlock( ix, ix - 1 );
		return nif->getBlock( ix - 1 );
	}
};

REGISTER_SPELL( spMoveBlockUp )

//! Move a block down in the NIF
class spMoveBlockDown final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Move Down" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return { Qt::CTRL + Qt::Key_Down }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) < nif->getBlockCount() - 1;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock ) override final
	{
		int ix = nif->getBlockNumber( iBlock );
		nif->moveNiBlock( ix, ix + 1 );
		return nif->getBlock( ix + 1 );
	}
};

REGISTER_SPELL( spMoveBlockDown )

//! Remove blocks by regex
class spRemoveBlocksById final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Remove By Id" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QSettings settings;
		QString key = QString( "%1/%2/%3/Match Expression" ).arg( "Spells", page(), name() );

		bool ok = true;
		QString match = QInputDialog::getText( 0, Spell::tr( "Remove Blocks by Id" ), Spell::tr( "Enter a regular expression:" ), QLineEdit::Normal,
			settings.value( key, "^BS|^NiBS|^bhk|^hk" ).toString(), &ok );

		if ( !ok )
			return QModelIndex();

		settings.setValue( key, match );

		QRegularExpression exp( match );

		int n = 0;

		while ( n < nif->getBlockCount() ) {
			QModelIndex iBlock = nif->getBlock( n );

			if ( nif->itemName( iBlock ).indexOf( exp ) >= 0 )
				nif->removeNiBlock( n );
			else
				n++;
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlocksById )

//! Remove all blocks except a given branch
class spCropToBranch final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Crop To Branch" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index );
	}

	// construct list of block numbers of all blocks that are in the link's branch (including link itself)
	QList<quint32> getBranch( NifModel * nif, quint32 link )
	{
		QList<quint32> branch;
		// add the link itself
		branch << link;
		// add all its children, grandchildren, ...
		for ( const auto child : nif->getChildLinks( link ) ) {
			// check that child is not in branch to avoid infinite recursion
			if ( !branch.contains( child ) )
				// it's not in there yet so add the child and grandchildren etc...
				branch << getBranch( nif, child );
		}

		// done, so return result
		return branch;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		// construct list of block numbers of all blocks in this branch of index
		QList<quint32> branch = getBranch( nif, nif->getBlockNumber( index ) );
		//qDebug() << branch;
		// remove non-branch blocks
		int n = 0; // tracks the current block number in the new system (after some blocks have been removed already)
		int m = 0; // tracks the block number in the old system i.e.  as they are numbered in the branch list

		while ( n < nif->getBlockCount() ) {
			if ( !branch.contains( m ) )
				nif->removeNiBlock( n );
			else
				n++;

			m++;
		}

		// done
		return QModelIndex();
	}
};

REGISTER_SPELL( spCropToBranch )

//! Convert block types
class spConvertBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Convert" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QStringList ids = nif->allNiBlocks();
		ids.sort();

		QString btype = nif->getBlockName( index );

		QMap<QString, QMenu *> map;
		for ( const QString& id : ids ) {
			QString x( "Other" );

			// Exclude siblings not in inheritance chain
			if ( btype == id || ( !nif->inherits( btype, id ) && !nif->inherits( id, btype ) ) )
				continue;

			if ( id.startsWith( "Ni" ) )
				x = QString( "Ni&" ) + id.mid( 2, 1 ) + "...";

			if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
				x = "Havok";

			if ( id.startsWith( "BS" ) || id == "AvoidNode" || id == "RootCollisionNode" )
				x = "Bethesda";

			if ( id.startsWith( "Fx" ) )
				x = "Firaxis";

			if ( !map.contains( x ) )
				map[ x ] = new QMenu( x );

			map[ x ]->addAction( id );
		}

		QMenu menu;
		for ( QMenu * m : map ) {
			menu.addMenu( m );
		}

		QAction * act = menu.exec( QCursor::pos() );

		if ( act ) {
			nif->convertNiBlock( act->text(), index );
		}

		return index;
	}
};

REGISTER_SPELL( spConvertBlock )

//! Duplicate a block in place
class spDuplicateBlock final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Duplicate" ); }
	QString page() const override final { return Spell::tr( "Block" ); }
	QKeySequence hotkey() const override final { return{ Qt::CTRL + Qt::SHIFT + Qt::Key_D }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		// from spCopyBlock
		QByteArray data;
		QBuffer buffer( &data );

		// Opening in ReadWrite doesn't work - race condition?
		if ( buffer.open( QIODevice::WriteOnly ) && nif->saveIndex( buffer, index ) ) {
			// from spPasteBlock
			if ( buffer.open( QIODevice::ReadOnly ) ) {
				QModelIndex block = nif->insertNiBlock( nif->getBlockName( index ), nif->getBlockCount() );
				nif->loadIndex( buffer, block );
				blockLink( nif, nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ) ), block );
				return block;
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spDuplicateBlock )

//! Duplicate a branch in place

bool spDuplicateBranch::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	return nif->isNiBlock( index );
}

QModelIndex spDuplicateBranch::cast( NifModel * nif, const QModelIndex & index )
{
	// from spCopyBranch
	QList<qint32> blocks;
	populateBlocks( blocks, nif, nif->getBlockNumber( index ) );

	QMap<qint32, qint32> blockMap;

	for ( int b = 0; b < blocks.count(); b++ )
		blockMap.insert( blocks[b], b );

	QMap<qint32, QString> parentMap;
	for ( const auto block : blocks )
	{
		for ( const auto link : nif->getParentLinks( block ) ) {
			if ( !blocks.contains( link ) && !parentMap.contains( link ) ) {
				QString failMessage = Spell::tr( "parent link invalid" );
				QModelIndex iParent = nif->getBlock( link );

				if ( iParent.isValid() ) {
					failMessage = Spell::tr( "parent unnamed" );
					QString name = nif->get<QString>( iParent, "Name" );

					if ( !name.isEmpty() ) {
						parentMap.insert( link, nif->itemName( iParent ) + NAME_SEP + name );
						continue;
					}
				}

				Message::append( tr( B_ERR ).arg( name() ),
								 tr( "failed to map parent link %1 %2 for block %3 %4; %5." )
									.arg( link )
									.arg( nif->itemName( nif->getBlock( link ) ) )
									.arg( block )
									.arg( nif->itemName( nif->getBlock( block ) ) )
									.arg( failMessage ),
								 QMessageBox::Critical
				);
				return index;
			}
		}
	}

	QByteArray data;
	QBuffer buffer( &data );

	if ( buffer.open( QIODevice::WriteOnly ) ) {
		QDataStream ds( &buffer );
		ds << blocks.count();
		ds << blockMap;
		ds << parentMap;
		for ( const auto block : blocks ) {
			ds << nif->itemName( nif->getBlock( block ) );

			if ( !nif->saveIndex( buffer, nif->getBlock( block ) ) ) {
				Message::append( tr( B_ERR ).arg( name() ),
								 tr( "failed to save block %1 %2." ).arg( block )
									.arg( nif->itemName( nif->getBlock( block ) ) ),
								 QMessageBox::Critical
				);
				return index;
			}
		}
	}

	// from spPasteBranch
	if ( buffer.open( QIODevice::ReadOnly ) ) {
		QDataStream ds( &buffer );

		int count;
		ds >> count;

		QMap<qint32, qint32> blockMap;
		ds >> blockMap;
		QMutableMapIterator<qint32, qint32> ibm( blockMap );

		while ( ibm.hasNext() ) {
			ibm.next();
			ibm.value() += nif->getBlockCount();
		}

		QMap<qint32, QString> parentMap;
		ds >> parentMap;

		QMapIterator<qint32, QString> ipm( parentMap );

		while ( ipm.hasNext() ) {
			ipm.next();
			qint32 block = getBlockByName( nif, ipm.value() );

			if ( block >= 0 ) {
				blockMap.insert( ipm.key(), block );
			} else {
				Message::append( tr( B_ERR ).arg( name() ),
								 tr( "failed to map parent link %1" ).arg( ipm.value() ),
								 QMessageBox::Critical
				);
				return index;
			}
		}

		QModelIndex iRoot;
		nif->holdUpdates( true );
		for ( int c = 0; c < count; c++ ) {
			QString type;
			ds >> type;

			QModelIndex block = nif->insertNiBlock( type, -1 );

			if ( !nif->loadAndMapLinks( buffer, block, blockMap ) )
				return index;

			if ( c == 0 )
				iRoot = block;
		}
		nif->holdUpdates( false );
		blockLink( nif, nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ) ), iRoot );

		return iRoot;
	}

	return QModelIndex();
}

REGISTER_SPELL( spDuplicateBranch )

//! Sort blocks by name
class spSortBlockNames final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Sort By Name" ); }
	QString page() const override final { return Spell::tr( "Block" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return ( !index.isValid() || !index.parent().isValid() );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex iBlock = nif->getBlock( n );

			if ( index.isValid() ) {
				iBlock = index;
				n = nif->getBlockCount();
			}

			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );

			// NiNode children are NIAVObjects and have a Name
			if ( iNumChildren.isValid() && iChildren.isValid() ) {
				QList<QPair<QString, qint32> > links;

				for ( int r = 0; r < nif->rowCount( iChildren ); r++ ) {
					qint32 l = nif->getLink( iChildren.child( r, 0 ) );

					if ( l >= 0 )
						links.append( QPair<QString, qint32>( nif->get<QString>( nif->getBlock( l ), "Name" ), l ) );
				}

				std::stable_sort( links.begin(), links.end() );

				for ( int r = 0; r < links.count(); r++ ) {
					if ( links[r].second != nif->getLink( iChildren.child( r, 0 ) ) )
						nif->setLink( iChildren.child( r, 0 ), links[r].second );

					nif->set<int>( iNumChildren, links.count() );
					nif->updateArray( iChildren );
				}
			}
		}

		if ( index.isValid() ) {
			return index;
		} 

		return QModelIndex();
	}
};

REGISTER_SPELL( spSortBlockNames )

//! Attach a Node as a parent of the current block
class spAttachParentNode final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Attach Parent Node" ); }
	QString page() const override final { return Spell::tr( "Node" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->isNiBlock( index );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		// find our current block number
		int thisBlockNumber = nif->getBlockNumber( index );
		// find our parent; most functions won't break if it doesn't exist,
		// so we don't care if it doesn't exist
		QModelIndex iParent = nif->getBlock( nif->getParent( thisBlockNumber ) );

		// find our index into the parent children array
		QVector<int> parentChildLinks = nif->getLinkArray( iParent, "Children" );
		int thisBlockIndex = parentChildLinks.indexOf( thisBlockNumber );

		// attach a new node
		// basically spAttachNode limited to NiNode and without the auto-attachment
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		for ( const QString& id : ids ) {
			if ( nif->inherits( id, "NiNode" ) )
				menu.addAction( id );
		}

		QModelIndex attachedNode;

		QAction * act = menu.exec( QCursor::pos() );

		if ( !act )
			return index;

		attachedNode = nif->insertNiBlock( act->text(), thisBlockNumber );

		// the attached node pushes this block down one row
		int attachedNodeNumber = thisBlockNumber++;

		// replace this block with the attached node
		nif->setLink( nif->getIndex( iParent, "Children" ).child( thisBlockIndex, 0 ), attachedNodeNumber );

		// attach ourselves to the attached node
		addLink( nif, attachedNode, "Children", thisBlockNumber );

		return attachedNode;
	}
};

REGISTER_SPELL( spAttachParentNode )

