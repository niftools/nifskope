/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "nifmodel.h"

#include "xml/xmlconfig.h"
#include "message.h"
#include "spellbook.h"
#include "data/niftypes.h"
#include "io/nifstream.h"

#include <QByteArray>
#include <QColor>
#include <QDebug>
#include <QFile>
#include <QSettings>

QHash<QString, QString> arrayPseudonyms;
QHash<QString, QString> multiArrayPseudonyms1;
QHash<QString, QString> multiArrayPseudonyms2;

void setupArrayPseudonyms()
{
	if ( !arrayPseudonyms.isEmpty() )
		return;

	#define registerPseudonym(plural, singular)		arrayPseudonyms.insert(QStringLiteral(plural), QStringLiteral(singular))

	registerPseudonym("Active Keys", "Active Key");
	registerPseudonym("Actors", "Actor");
	registerPseudonym("Affected Node Pointers", "Affected Node Pointer");
	registerPseudonym("Affected Nodes", "Affected Node");
	registerPseudonym("Anim Note Arrays", "Anim Note Array");
	registerPseudonym("Anim Notes", "Anim Note");
	registerPseudonym("Attachments", "Attachment");
	registerPseudonym("Big Tris", "Big Tri");
	registerPseudonym("Big Verts", "Big Vert");
	registerPseudonym("Bitangents", "Bitangent");
	registerPseudonym("Block Infos", "Block Info");
	registerPseudonym("Block Offsets", "Block Offset");
	registerPseudonym("Block Type Hashes", "Block Type Hash");
	registerPseudonym("Block Types", "Block Type");
	registerPseudonym("Blocks", "Block");
	registerPseudonym("Bone Bounds", "Bone Bound");
	registerPseudonym("Bone Indices", "Bone Index");
	registerPseudonym("Bone List", "Bone");
	registerPseudonym("Bone Transforms", "Bone Transform");
	registerPseudonym("Bone Weights", "Bone Weight");
	registerPseudonym("Bones", "Bone");
	registerPseudonym("Bounding Volumes", "Bounding Volume");
	registerPseudonym("Buttons", "Button");
	registerPseudonym("Chained Entities", "Chained Entity");
	registerPseudonym("Channel Types", "Channel Type");
	registerPseudonym("Channels", "Channel");
	registerPseudonym("Children", "Child");
	registerPseudonym("Chunk Materials", "Chunk Material");
	registerPseudonym("Chunk Transforms", "Chunk Transform");
	registerPseudonym("Chunks", "Chunk");
	registerPseudonym("Clothes", "Cloth");
	registerPseudonym("Colliders", "Collider");
	registerPseudonym("Color Keys", "Color Key");
	registerPseudonym("Colors", "Color");
	registerPseudonym("Compact Control Points", "Compact Control Point");
	registerPseudonym("Compartments", "Compartment");
	registerPseudonym("Complete Points", "Complete Point");
	registerPseudonym("Component Formats", "Component Format");
	registerPseudonym("Compressed Vertices", "Compressed Vertex");
	registerPseudonym("Connect Points", "Connect Point");
	registerPseudonym("Constraints", "Constraint");
	registerPseudonym("Control Points", "Control Point");
	registerPseudonym("Controlled Blocks", "Controlled Block");
	registerPseudonym("Controller Seq List", "Controller Seq");
	registerPseudonym("Controller Sequences", "Controller Sequence");
	registerPseudonym("Corners", "Corner");
	registerPseudonym("Cut Offsets", "Cut Offset");
	registerPseudonym("Data Sizes", "Data Size");
	registerPseudonym("Datastreams", "Datastream");
	registerPseudonym("Dests", "Dest");
	registerPseudonym("DIV2 Floats", "DIV2 Float");
	registerPseudonym("DIV2 Ints", "DIV2 Int");
	registerPseudonym("Effects", "Effect");
	registerPseudonym("Elements", "Element");
	registerPseudonym("Emitter Meshes", "Emitter Mesh");
	registerPseudonym("Emitters", "Emitter");
	registerPseudonym("Evaluators", "Evaluator");
	registerPseudonym("Extra Data List", "Extra Data");
	registerPseudonym("Extra Targets", "Extra Target");
	registerPseudonym("Filter Constants", "Filter Constant");
	registerPseudonym("Filter Ops", "Filter Op");
	registerPseudonym("Filters", "Filter");
	registerPseudonym("Fixtures", "Fixture");
	registerPseudonym("Float Control Points", "Float Control Point");
	registerPseudonym("Forces", "Force");
	registerPseudonym("Generations", "Generation");
	registerPseudonym("Group Collision Flags", "Group Collision Flag");
	registerPseudonym("Groups", "Group");
	registerPseudonym("Images", "Image");
	registerPseudonym("In Portals", "In Portal");
	registerPseudonym("Indices", "Index");
	registerPseudonym("Instance Nodes", "Instance Node");
	registerPseudonym("Instances", "Instance");
	registerPseudonym("Interp Array Items", "Interp Item");
	registerPseudonym("Interpolator Weights", "Interpolator Weight");
	registerPseudonym("Interpolators", "Interpolator");
	registerPseudonym("Items", "Item");
	registerPseudonym("Joints", "Joint");
	registerPseudonym("Keys", "Key");
	registerPseudonym("Knots", "Knot");
	registerPseudonym("Limits", "Limit");
	registerPseudonym("Lines", "Line");
	registerPseudonym("LOD Distances", "LOD Distance");
	registerPseudonym("LOD Entries", "LOD Entry");
	registerPseudonym("LOD Levels", "LOD Level");
	registerPseudonym("LODs", "LOD");
	registerPseudonym("Mapped Primitives", "Mapped Primitive");
	registerPseudonym("Master Particles", "Master Particle");
	registerPseudonym("Match Groups", "Match Group");
	registerPseudonym("Material Descs", "Material Desc");
	registerPseudonym("Materials", "Material");
	registerPseudonym("Mesh Emitters", "Mesh Emitter");
	registerPseudonym("Meshes", "Mesh");
	registerPseudonym("Mipmaps", "Mipmap");
	registerPseudonym("Modified Meshes", "Modified Mesh");
	registerPseudonym("Modifiers", "Modifier");
	registerPseudonym("Morphs", "Morph");
	registerPseudonym("Node Groups", "Node Group");
	registerPseudonym("Nodes", "Node");
	registerPseudonym("Normals", "Normal");
	registerPseudonym("Objs", "Obj");
	registerPseudonym("Out Portals", "Out Portal");
	registerPseudonym("Particle Meshes", "Particle Mesh");
	registerPseudonym("Particle Normals", "Particle Normal");
	registerPseudonym("Particle Systems", "Particle System");
	registerPseudonym("Particle Triangles", "Particle Triangle");
	registerPseudonym("Particle Vertices", "Particle Vertex");
	registerPseudonym("Particles", "Particle");
	registerPseudonym("Partitions", "Partition");
	registerPseudonym("Pivots", "Pivot");
	registerPseudonym("Points", "Point");
	registerPseudonym("Polygon Indices", "Polygon Index");
	registerPseudonym("Polygons", "Polygon");
	registerPseudonym("Poses", "Pose");
	registerPseudonym("Positions", "Position");
	registerPseudonym("Properties", "Property");
	registerPseudonym("Proportion Levels", "Proportion Level");
	registerPseudonym("Props", "Prop");
	registerPseudonym("Quaternion Keys", "Quaternion Key");
	registerPseudonym("Radii", "Radius");
	registerPseudonym("Refs", "Ref");
	registerPseudonym("Regions", "Region");
	registerPseudonym("Rooms", "Room");
	registerPseudonym("Roots", "Root");
	registerPseudonym("Rotation Angles", "Rotation Angle");
	registerPseudonym("Rotation Axes", "Rotation Axis");
	registerPseudonym("Rotation Keys", "Rotation Key");
	registerPseudonym("Rotation Speeds", "Rotation Speed");
	registerPseudonym("Rotations", "Rotation");
	registerPseudonym("Scales", "Scale");
	registerPseudonym("Segment Starts", "Segment Start");
	registerPseudonym("Shader Textures", "Shader Texture");
	registerPseudonym("Shadow Casters", "Shadow Caster");
	registerPseudonym("Shadow Receivers", "Shadow Receiver");
	registerPseudonym("Shape Descriptions", "Shape Description");
	registerPseudonym("Shape Properties", "Shape Property");
	registerPseudonym("Simulation Steps", "Simulation Step");
	registerPseudonym("Size Keys", "Size Key");
	registerPseudonym("Sizes", "Size");
	registerPseudonym("Skin Indices", "Skin Index");
	registerPseudonym("Skins", "Skin");
	registerPseudonym("Sources", "Source");
	registerPseudonym("Spawn Rate Keys", "Spawn Rate Key");
	registerPseudonym("Spawners", "Spawner");
	registerPseudonym("Spheres", "Sphere");
	registerPseudonym("States", "State");
	registerPseudonym("Strings", "String");
	registerPseudonym("Strip Lengths", "Strip Length");
	registerPseudonym("Strips", "Strip");
	registerPseudonym("Sub Shapes", "Sub Shape");
	registerPseudonym("SubEntry List", "SubEntry");
	registerPseudonym("Submesh To Region Map", "Submesh To Region");
	registerPseudonym("Submit Points", "Submit Point");
	registerPseudonym("Subtexture Offsets", "Subtexture Offset");
	registerPseudonym("Systems", "System");
	registerPseudonym("Tangents", "Tangent");
	registerPseudonym("Target Names", "Target Name");
	registerPseudonym("Tear Indices", "Tear Index");
	registerPseudonym("Tear Split Planes", "Tear Split Plane");
	registerPseudonym("Text Keys", "Text Key");
	registerPseudonym("Texture Array", "Texture");
	registerPseudonym("Texture Arrays", "Texture Array");
	registerPseudonym("Textures", "Texture");
	registerPseudonym("Transforms", "Transform");
	registerPseudonym("Translations", "Translation");
	registerPseudonym("Tread Transforms", "Tread Transform");
	registerPseudonym("Triangles Copy", "Triangle Copy");
	registerPseudonym("Triangles", "Triangle");
	registerPseudonym("UV Groups", "UV Group");
	registerPseudonym("Vector Blocks", "Vector Block");
	registerPseudonym("Vectors", "Vector");
	registerPseudonym("Vels", "Vel");
	registerPseudonym("Vertex Colors", "Vertex Color");
	registerPseudonym("Vertex Counts", "Vertex Count");
	registerPseudonym("Vertex Data", "Vertex");
	registerPseudonym("Vertex Indices", "Vertex Index");
	registerPseudonym("Vertex Positions", "Vertex Position");
	registerPseudonym("Vertex Weights", "Vertex Weight");
	registerPseudonym("Vertices", "Vertex");
	registerPseudonym("Wall Planes", "Wall Plane");
	registerPseudonym("Walls", "Wall");
	registerPseudonym("Weight Indices", "Weight Index");
	registerPseudonym("Weights", "Weight");
	registerPseudonym("XYZ Rotations", "XYZ Rotation");

	#define registerMultiPseudonym(plural, singular1, singular2) \
		multiArrayPseudonyms1.insert(QStringLiteral(plural), QStringLiteral(singular1)); \
		multiArrayPseudonyms2.insert(QStringLiteral(plural), QStringLiteral(singular2))

	registerMultiPseudonym("Bone Data", "Bone", "Weight");
	registerMultiPseudonym("Bone Indices", "Vertex", "Bone Index");
	registerMultiPseudonym("Points", "Strip", "Point");
	registerMultiPseudonym("RGB Image Data", "RGB X", "RGB Y");
	registerMultiPseudonym("RGBA Image Data", "RGBA X", "RGBA Y");
	registerMultiPseudonym("Strips", "Strip", "Point");
	registerMultiPseudonym("UV Sets", "UV Set", "UV");
	registerMultiPseudonym("Vertex Weights", "Vertex", "Weight");
}

inline QString resolveArrayPseudonym( const QHash<QString, QString> & pseudonymMap, const NifItem * item )
{
	return pseudonymMap.value( item->name(), item->name() ) + " " + QString::number( item->row() );
}

//! @file nifmodel.cpp The NIF data model.

NifModel::NifModel( QObject * parent ) : BaseModel( parent )
{
	setupArrayPseudonyms();
	updateSettings();

	clear();
}

void NifModel::updateSettings()
{
	QSettings settings;

	settings.beginGroup( "Settings/NIF/Startup Defaults" );

	cfg.startupVersion = settings.value( "Version", "20.0.0.5" ).toString();
	cfg.userVersion = settings.value( "User Version", "11" ).toInt();
	cfg.userVersion2 = settings.value( "User Version 2", "11" ).toInt();

	settings.endGroup();
}

QString NifModel::version2string( quint32 v )
{
	if ( v == 0 )
		return QString();

	QString s;

	if ( v < 0x0303000D ) {
		//This is an old-style 2-number version with one period
		s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
		    + QString::number( ( v >> 16 ) & 0xff, 10 );

		quint32 sub_num1 = ( (v >> 8) & 0xff );
		quint32 sub_num2 = (v & 0xff);

		if ( sub_num1 > 0 || sub_num2 > 0 ) {
			s = s + QString::number( sub_num1, 10 );
		}

		if ( sub_num2 > 0 ) {
			s = s + QString::number( sub_num2, 10 );
		}
	} else {
		//This is a new-style 4-number version with 3 periods
		s = QString::number( ( v >> 24 ) & 0xff, 10 ) + "."
		    + QString::number( ( v >> 16 ) & 0xff, 10 ) + "."
		    + QString::number( ( v >> 8 ) & 0xff, 10 ) + "."
		    + QString::number( v & 0xff, 10 );
	}

	return s;
}

quint32 NifModel::version2number( const QString & s )
{
	if ( s.isEmpty() )
		return 0;

	if ( s.contains( "." ) ) {
		QStringList l = s.split( "." );

		quint32 v = 0;

		if ( l.count() > 4 ) {
			// Should probaby post a warning here or something.  Version # has more than 3 dots in it.
			return 0;
		} else if ( l.count() == 2 ) {
			// This is an old style version number.  Take each digit following the first one at a time.
			// The first one is the major version
			v += l[0].toInt() << (3 * 8);

			if ( l[1].size() >= 1 ) {
				v += l[1].mid( 0, 1 ).toInt() << (2 * 8);
			}

			if ( l[1].size() >= 2 ) {
				v += l[1].mid( 1, 1 ).toInt() << (1 * 8);
			}

			if ( l[1].size() >= 3 ) {
				v += l[1].mid( 2, -1 ).toInt();
			}

			return v;
		}

		// This is a new style version number with dots separating the digits
		for ( int i = 0; i < 4 && i < l.count(); i++ ) {
			v += l[i].toInt( 0, 10 ) << ( (3 - i) * 8 );
		}

		return v;
	}

	bool ok;
	quint32 i = s.toUInt( &ok );
	return ( i == 0xffffffff ? 0 : i );
}

void NifModel::clear()
{
	beginResetModel();
	fileinfo = QFileInfo();
	filename = QString();
	folder = QString();
	bsVersion = 0;
	root->killChildren();

	NifData headerData = NifData( "NiHeader", "Header" );
	NifData footerData = NifData( "NiFooter", "Footer" );
	headerData.setIsCompound( true );
	headerData.setIsConditionless( true );
	footerData.setIsCompound( true );
	footerData.setIsConditionless( true );

	insertType( root, headerData );
	insertType( root, footerData );
	version = version2number( cfg.startupVersion );

	if ( !supportedVersions.isEmpty() && !isVersionSupported( version ) ) {
		Message::warning( nullptr, tr( "Unsupported 'Startup Version' %1 specified, reverting to 20.0.0.5" ).arg( cfg.startupVersion ) );
		version = 0x14000005;
	}
	endResetModel();

	NifItem * header = getHeaderItem();

	auto headerVer = getItem( header, "Version" );
	if ( headerVer )
		headerVer->setFileVersionValue( version );

	QString header_string( ( version <= 0x0A000100 ) ? "NetImmerse File Format, Version " : "Gamebryo File Format, Version " );
	header_string += version2string( version );
	set<QString>( header, "Header String", header_string );

	set<int>( getItem( header, "User Version", false ), cfg.userVersion );
	set<int>( getItem( header, "BS Header\\BS Version", false ), cfg.userVersion2 );
	invalidateItemConditions( header );

	//set<int>( header, "Unknown Int 3", 11 );

	if ( version < 0x0303000D ) {
		QVector<QString> copyright( 3 );
		copyright[0] = "Numerical Design Limited, Chapel Hill, NC 27514";
		copyright[1] = "Copyright (c) 1996-2000";
		copyright[2] = "All Rights Reserved";

		setArray<QString>( header, "Copyright", copyright );
	}

	cacheBSVersion( header );

	lockUpdates = false;
	needUpdates = utNone;
}


/*
 *  header functions
 */

QString NifModel::extractRTTIArgs( const QString & RTTIName, NiMesh::DataStreamMetadata & metadata ) const
{
	QStringList nameAndArgs = RTTIName.split( "\x01" );
	Q_ASSERT( nameAndArgs.size() >= 1 );

	if ( nameAndArgs[0] == QLatin1String( "NiDataStream" ) ) {
		Q_ASSERT( nameAndArgs.size() == 3 );
		metadata.usage = NiMesh::DataStreamUsage( nameAndArgs[1].toInt() );
		metadata.access = NiMesh::DataStreamAccess( nameAndArgs[2].toInt() );
	}

	return nameAndArgs[0];
}

QString NifModel::createRTTIName( const NifItem * block ) const
{
	if ( !block )
		return {};

	if ( block->hasName("NiDataStream") ) {
		return QString( "NiDataStream\x01%1\x01%2" )
			.arg( NifItem::get<quint32>( getItem( block, "Usage", true ) ) )
			.arg( NifItem::get<quint32>( getItem( block, "Access", true ) ) );
	}

	return block->name();
}

const NifItem * NifModel::getHeaderItem() const
{
	return root->child( 0 );
}

void NifModel::updateHeader()
{
	emit beginUpdateHeader();

	if ( lockUpdates ) {
		needUpdates = UpdateType( needUpdates | utHeader );
		return;
	}

	NifItem * header = getHeaderItem();

	set<int>( header, "Num Blocks", getBlockCount() );

	NifItem * itemBlockTypes       = getItem( header, "Block Types" );
	NifItem * itemBlockTypeIndices = getItem( header, "Block Type Index" );
	NifItem * itemBlockSizes       = ( version >= 0x14020000 ) ? getItem( header, "Block Size" ) : nullptr;
	// 20.3.1.2 Custom Version
	NifItem * itemBlockTypeHashes  = ( version == 0x14030102 ) ? getItem( header, "Block Type Hashes" ) : nullptr;

	// Update Block Types, Block Type Index, and Block Size
	if ( (itemBlockTypes || itemBlockTypeHashes) && itemBlockTypeIndices ) {
		QVector<QString> blockTypes;
		QVector<int> blockTypeIndices;
		QVector<int> blockSizes;

		for ( int r = firstBlockRow(); r <= lastBlockRow(); r++ ) {
			NifItem * itemBlock = root->child( r );
			if ( !itemBlock ) // Just in case...
				continue;

			QString blockName = createRTTIName( itemBlock );

			int iBlockType = blockTypes.indexOf( blockName );
			if ( iBlockType < 0 ) {
				blockTypes.append( blockName );
				iBlockType = blockTypes.count() - 1;
			}			
			blockTypeIndices.append( iBlockType );

			if ( itemBlockSizes ) {
				updateChildArraySizes( itemBlock );
				blockSizes.append( blockSize( itemBlock ) );
			}
		}

		set<int>( header, "Num Block Types", blockTypes.count() );

		setState( Processing );
		updateArraySize(itemBlockTypeIndices);
		itemBlockTypeIndices->setArray<int>( blockTypeIndices );
		if ( itemBlockTypes ) {
			updateArraySize(itemBlockTypes);
			itemBlockTypes->setArray<QString>( blockTypes );
		}
		if ( itemBlockSizes ) {
			updateArraySize(itemBlockSizes);
			itemBlockSizes->setArray<int>( blockSizes );
		}
		// 20.3.1.2 Custom Version
		if ( itemBlockTypeHashes ) {
			updateArraySize( itemBlockTypeHashes );

			QVector<quint32> hashes;
			hashes.reserve( blockTypes.count() );
			for ( const auto & t : blockTypes )
				hashes << DJB1Hash( t.toStdString().c_str() );
			setArray( itemBlockTypeHashes, hashes );
		}

		restoreState();

		// For 20.1 and above strings are saved in the header.  Max String Length must be updated.
		if ( version >= 0x14010003 ) {
			int nMaxLen = 0;
			auto headerStrings = getItem( header, "Strings" );
			if ( headerStrings ) {
				for ( auto c : headerStrings->childIter() ) {
					int len = c->get<QString>().length();
					if ( len > nMaxLen )
						nMaxLen = len;
				}
			}
			set<uint>( header, "Max String Length", nMaxLen );
		}
	}
}



/*
 *  footer functions
 */

const NifItem * NifModel::getFooterItem() const
{
	int nRootChildren = root->childCount();
	return ( nRootChildren >= 2 ) ? root->child( nRootChildren - 1 ) : nullptr;
}

void NifModel::updateFooter()
{
	if ( lockUpdates ) {
		needUpdates = UpdateType( needUpdates | utFooter );
		return;
	}

	NifItem * footer = getFooterItem();
	NifItem * itemRoots = getItem( footer, "Roots" );
	if ( itemRoots ) {
		set<int>( footer, "Num Roots", rootLinks.count() );
		updateArraySize( itemRoots );
		for ( int r = 0; r < itemRoots->childCount(); r++ ) {
			auto child = itemRoots->child( r );
			if ( child )
				child->setLinkValue( rootLinks.value( r ) );
		}
	}
}

/*
 *  array functions
 */

bool NifModel::updateArraySizeImpl( NifItem * array )
{
	if ( !isArrayEx( array ) ) {
		if ( array )
			reportError( array, __func__, "The input item is not an array." );
		return false;
	}

	// Binary array handling
	if ( array->isBinary() ) {
		return updateByteArraySize( array );
	}

	// Get new array size
	int nNewSize = evalArraySize( array );

	if ( nNewSize > 1024 * 1024 * 8 ) {
		reportError( array, __func__, tr( "Array size %1 is much too large." ).arg( nNewSize ) );
		return false;
	} else if ( nNewSize < 0 ) {
		reportError( array, __func__, tr( "Array size %1 is invalid." ).arg( nNewSize ) );
		return false;
	}

	int nOldSize = array->childCount();

	if ( nNewSize > nOldSize ) { // Add missing items
		NifData data( array->name(),
					  array->strType(),
					  array->templ(),
					  NifValue( NifValue::type( array->strType() ) ),
					  addConditionParentPrefix( array->arg() ),
					  addConditionParentPrefix( array->arr2() ) // arr1 in children is parent arr2
		);

		// Fill data flags
		data.setIsConditionless( true );
		data.setIsCompound( array->isCompound() );
		data.setIsArray( array->isMultiArray() );

		beginInsertRows( itemToIndex(array), nOldSize, nNewSize - 1 );
		array->prepareInsert( nNewSize - nOldSize );
		for ( int c = nOldSize; c < nNewSize; c++ )
			insertType( array, data );
		endInsertRows();
	}

	if ( nNewSize < nOldSize ) { // Remove excess items
		beginRemoveRows( itemToIndex(array), nNewSize, nOldSize - 1 );
		array->removeChildren( nNewSize, nOldSize - nNewSize );
		endRemoveRows();
	}

	if ( nNewSize != nOldSize
		&& state != Loading
		&& ( isCompound(array->strType()) || NifValue::isLink(NifValue::type(array->strType())) )
		&& getTopItem( array ) != getFooterItem()
	) {
		updateLinks();
		updateFooter();
		emit linksChanged();
	}

	return true;
}

bool NifModel::updateByteArraySize( NifItem * array )
{
	// TODO (Gavrant): I don't understand what's going on here, rewrite the function

	int nNewSize = evalArraySize( array );
	int nOldRows = array->childCount();
	if ( nNewSize == 0 )
		return false;

	// Create byte array for holding blob data
	QByteArray bytes;
	bytes.resize( nNewSize );

	// Previous row count

	// Grab data from existing rows if appropriate and then purge
	if ( nOldRows > 1 ) {
		for ( int i = 0; i < nOldRows; i++ ) {
			if ( NifItem * child = array->child( 0 ) ) {
				bytes[i] = get<quint8>( child );
			}
		}

		beginRemoveRows( itemToIndex(array), 0, nOldRows - 1 );
		array->removeChildren( 0, nOldRows );
		endRemoveRows();

		nOldRows = 0;
	}

	// Create the dummy row for holding the byte array
	if ( nOldRows == 0 ) {
		NifData data( array->name(), array->strType(), array->templ(), NifValue( NifValue::tBlob ), addConditionParentPrefix( array->arg() ) );
		data.setBinary( true );

		beginInsertRows( itemToIndex(array), 0, 1 );
		array->prepareInsert( 1 );
		insertType( array, data );
		endInsertRows();
	}

	// Update the byte array
	if ( NifItem * child = array->child( 0 ) ) {
		QByteArray * bm = child->isBinary() ? get<QByteArray *>( child ) : nullptr;
		if ( !bm ) {
			set<QByteArray>( child, bytes );
		} else if ( bm->size() == 0 ) {
			*bm = bytes;
		} else {
			bm->resize( nNewSize );
		}
	}

	return true;
}

bool NifModel::updateChildArraySizes( NifItem * parent )
{
	if ( !parent )
		return false;

	for ( auto child : parent->childIter() ) {
		if ( evalCondition( child ) ) {
			if ( child->isArrayEx() ) {
				if ( !updateArraySize(child) )
					return false;
			}
			if ( child->childCount() > 0 ) {
				if ( !updateChildArraySizes(child) )
					return false;
			}
		}
	}

	return true;
}

/*
 *  block functions
 */

QModelIndex NifModel::insertNiBlock( const QString & identifier, int at )
{
	NifBlockPtr block = blocks.value( identifier );

	if ( block ) {
		if ( at < 0 || at > getBlockCount() )
			at = -1;

		if ( at >= 0 )
			adjustLinks( root, at, 1 );

		if ( at >= 0 )
			at++;
		else
			at = getBlockCount() + 1;

		beginInsertRows( QModelIndex(), at, at );

		NifData d = NifData( identifier, "NiBlock", block->text );
		d.setIsConditionless( true );
		NifItem * branch = insertBranch( root, d, at );
		endInsertRows();

		if ( !block->ancestor.isEmpty() )
			insertAncestor( branch, block->ancestor );

		branch->prepareInsert( block->types.count() );

		if ( getBSVersion() >= 151 && identifier.startsWith( "BSLighting" ) ) {
			// TODO: This appears to be incomplete
			for ( const NifData& data : block->types ) {
				insertType( branch, data );
			}
		} else {
			for ( const NifData& data : block->types ) {
				insertType( branch, data );
			}
		}

		if ( state != Loading ) {
			updateHeader();
			updateLinks();
			updateFooter();
			emit linksChanged();
		}

		return createIndex( branch->row(), 0, branch );
	}

	logMessage(tr("Could not insert NiBlock."), tr("Unknown block %1").arg(identifier), QMessageBox::Critical);
	
	return QModelIndex();
}

void NifModel::removeNiBlock( int blocknum )
{
	if ( !isValidBlockNumber( blocknum ) )
		return;

	adjustLinks( root, blocknum, 0 );
	adjustLinks( root, blocknum, -1 );
	beginRemoveRows( QModelIndex(), blocknum + 1, blocknum + 1 );
	root->removeChild( blocknum + 1 );
	endRemoveRows();
	updateLinks();
	updateFooter();
	emit linksChanged();
}

void NifModel::moveNiBlock( int src, int dst )
{
	if ( !isValidBlockNumber( src ) )
		return;

	beginRemoveRows( QModelIndex(), src + 1, src + 1 );
	NifItem * block = root->takeChild( src + 1 );
	endRemoveRows();

	if ( dst >= 0 )
		dst++;

	beginInsertRows( QModelIndex(), dst, dst );
	dst = root->insertChild( block, dst ) - 1;
	endInsertRows();

	QMap<qint32, qint32> map;

	if ( src < dst ) {
		for ( int l = src; l <= dst; l++ )
			map.insert( l, l - 1 );
	} else {
		for ( int l = dst; l <= src; l++ )
			map.insert( l, l + 1 );
	}


	map.insert( src, dst );

	mapLinks( root, map );

	updateLinks();
	updateHeader();
	updateFooter();
	emit linksChanged();
}

void NifModel::updateStrings( NifModel * src, NifModel * tgt, NifItem * item )
{
	if ( !item )
		return;

	if ( item->hasValueType(NifValue::tStringIndex) || item->hasValueType(NifValue::tSizedString) || item->hasStrType("string") ) {
		QString str = src->resolveString( item );
		tgt->assignString( tgt->createIndex( 0, 0, item ), str, false );
	}

	for ( auto child : item->children() ) {
		updateStrings( src, tgt, child );
	}
}

QMap<qint32, qint32> NifModel::moveAllNiBlocks( NifModel * targetnif, bool update )
{
	int bcnt = getBlockCount();

	bool doStringUpdate = ( this->getVersionNumber() >= 0x14010003 || targetnif->getVersionNumber() >= 0x14010003 );

	QMap<qint32, qint32> map;

	beginRemoveRows( QModelIndex(), 1, bcnt );
	targetnif->beginInsertRows( QModelIndex(), targetnif->getBlockCount(), targetnif->getBlockCount() + bcnt - 1 );

	for ( int i = 0; i < bcnt; i++ ) {
		map.insert( i, targetnif->root->insertChild( root->takeChild( 1 ), targetnif->root->childCount() - 1 ) - 1 );
	}

	endRemoveRows();
	targetnif->endInsertRows();

	for ( int i = 0; i < bcnt; i++ ) {
		NifItem * item = targetnif->root->child( targetnif->getBlockCount() - i );
		targetnif->mapLinks( item, map );

		if ( doStringUpdate )
			updateStrings( this, targetnif, item );
	}

	if ( update ) {
		updateLinks();
		updateHeader();
		updateFooter();
		emit linksChanged();

		targetnif->updateLinks();
		targetnif->updateHeader();
		targetnif->updateFooter();
		emit targetnif->linksChanged();
	}

	return map;
}

void NifModel::reorderBlocks( const QVector<qint32> & order )
{
	if ( getBlockCount() <= 1 )
		return;

	QString err = tr( "NifModel::reorderBlocks() - invalid argument" );

	if ( order.count() != getBlockCount() ) {
		logMessage(tr("Reorder Blocks error"), err, QMessageBox::Critical);
		return;
	}

	QMap<qint32, qint32> linkMap;
	QMap<qint32, qint32> blockMap;

	for ( qint32 n = 0; n < order.count(); n++ ) {
		if ( blockMap.contains( order[n] ) || !isValidBlockNumber( order[n] ) ) {
			logMessage(tr("Reorder Blocks error"), err, QMessageBox::Critical);
			return;
		}

		blockMap.insert( order[n], n );

		if ( order[n] != n )
			linkMap.insert( n, order[n] );
	}

	if ( linkMap.isEmpty() )
		return;

	// take all the blocks
	beginRemoveRows( QModelIndex(), 1, root->childCount() - 2 );
	QList<NifItem *> temp;

	for ( qint32 n = 0; n < order.count(); n++ )
		temp.append( root->takeChild( 1 ) );

	endRemoveRows();

	// then insert them again in the new order
	beginInsertRows( QModelIndex(), 1, temp.count() );
	for ( const auto n : blockMap ) {
		root->insertChild( temp[ n ], root->childCount() - 1 );
	}
	endInsertRows();

	mapLinks( root, linkMap );
	updateLinks();
	emit linksChanged();

	updateHeader();
	updateFooter();
}

void NifModel::mapLinks( const QMap<qint32, qint32> & map )
{
	mapLinks( root, map );
	updateLinks();
	emit linksChanged();

	updateHeader();
	updateFooter();
}

int NifModel::getBlockNumber( const NifItem * item ) const
{
	const NifItem * block = getTopItem( item );
	if ( block ) {
		int iRow = block->row();
		if ( isBlockRow( iRow ) )
			return iRow - firstBlockRow();
	}

	return -1;
}

const NifItem * NifModel::_getBlockItem( const NifItem * block, const QString & ancestor ) const
{
	if ( block && inherits( block->name(), ancestor ) )
		return block;

	return nullptr;
}

const NifItem * NifModel::_getBlockItem( const NifItem * block, const QLatin1String & ancestor ) const
{
	if ( block && inherits( block->name(), ancestor ) )
		return block;

	return nullptr;
}

const NifItem * NifModel::_getBlockItem( const NifItem * block, const std::initializer_list<const char *> & ancestors ) const
{
	if ( block && inherits( block->name(), ancestors ) )
		return block;

	return nullptr;
}

const NifItem * NifModel::_getBlockItem( const NifItem * block, const QStringList & ancestors ) const
{
	if ( block && inherits( block->name(), ancestors ) )
		return block;

	return nullptr;
}

const NifItem * NifModel::getBlockItem( qint32 link ) const
{
	if ( isValidBlockNumber( link ) )
		return root->child( link + firstBlockRow() );

	return nullptr;
}

const NifItem * NifModel::getBlockItem( const NifItem * item ) const
{
	const NifItem * block = getTopItem( item );
	return isNiBlock(block) ? block : nullptr;
}

bool NifModel::isNiBlock( const NifItem * item, const std::initializer_list<const char *> & testTypes ) const
{
	if ( isNiBlock(item) ) {
		for ( auto name : testTypes ) {
			if ( item->hasName(name) )
				return true;
		}
	}
	return false;
}

bool NifModel::isNiBlock( const NifItem * item, const QStringList & testTypes ) const
{
	if ( isNiBlock( item ) ) {
		for ( const QString & name : testTypes ) {
			if ( item->hasName(name) )
				return true;
		}
	}

	return false;
}


/*
 *  ancestor functions
 */

void NifModel::insertAncestor( NifItem * parent, const QString & identifier, int at )
{
	setState( Inserting );

	Q_UNUSED( at );
	NifBlockPtr ancestor = blocks.value( identifier );

	if ( ancestor ) {
		if ( !ancestor->ancestor.isEmpty() )
			insertAncestor( parent, ancestor->ancestor );

		//parent->insertChild( NifData( identifier, "Abstract" ) );
		parent->prepareInsert( ancestor->types.count() );
		for ( const NifData& data : ancestor->types ) {
			insertType( parent, data );
		}
	} else {
		logMessage(tr("Cannot insert parent."), tr("Unknown parent %1").arg(identifier));
	}

	restoreState();
}

bool NifModel::inherits( const QString & blockName, const QString & ancestor ) const
{
	if ( blockName == ancestor )
		return true;

	NifBlockPtr type = blocks.value( blockName );
	return type && inherits( type->ancestor, ancestor );
}

bool NifModel::inherits( const QString & blockName, const QLatin1String & ancestor ) const
{
	if ( blockName == ancestor )
		return true;

	NifBlockPtr type = blocks.value( blockName );
	return type && inherits( type->ancestor, ancestor );
}

bool NifModel::inherits( const QString & blockName, const std::initializer_list<const char *> & ancestors ) const
{
	for ( auto a : ancestors ) {
		if ( inherits( blockName, a ) )
			return true;
	}

	return false;
}

bool NifModel::inherits( const QString & blockName, const QStringList & ancestors ) const
{
	for ( const QString & a : ancestors ) {
		if ( inherits( blockName, a ) )
			return true;
	}

	return false;
}

bool NifModel::blockInherits( const NifItem * item, const QString & ancestor ) const
{
	const NifItem * block = getTopItem( item );
	return isNiBlock(block) ? inherits( block->name(), ancestor ) : false;
}

bool NifModel::blockInherits( const NifItem * item, const QLatin1String & ancestor ) const
{
	const NifItem * block = getTopItem( item );
	return isNiBlock(block) ? inherits( block->name(), ancestor ) : false;
}

bool NifModel::blockInherits(const NifItem * item, const std::initializer_list<const char *> & ancestors ) const
{
	const NifItem * block = getTopItem( item );
	return isNiBlock(block) ? inherits( block->name(), ancestors ) : false;
}

bool NifModel::blockInherits(const NifItem * item, const QStringList & ancestors ) const
{
	const NifItem * block = getTopItem( item );
	return isNiBlock(block) ? inherits( block->name(), ancestors ) : false;
}


/*
 *  basic and compound type functions
 */

void NifModel::insertType( NifItem * parent, const NifData & data, int at )
{
	setState( Inserting );

	if ( data.isArray() ) {
		insertBranch( parent, data, at );
	} else if ( data.isCompound() ) {
		NifBlockPtr compound = compounds.value( data.type() );
		if ( !compound )
			return;
		NifItem * branch = insertBranch( parent, data, at );
		branch->prepareInsert( compound->types.count() );
		for ( const NifData & d : compound->types ) {
			insertType( branch, d );
		}
	} else if ( data.isMixin() ) {
		NifBlockPtr compound = compounds.value( data.type() );
		if ( !compound )
			return;
		parent->prepareInsert( compound->types.count() );
		for ( const NifData & d : compound->types ) {
			insertType( parent, d );
		}
	} else if ( data.isTemplated() ) {
		QString tmp = parent->templ();
		NifItem * tItem = parent;

		while ( tmp == XMLTMPL && tItem->parent() ) {
			tItem = tItem->parent();
			tmp = tItem->templ();
		}

		NifData d( data );

		if ( d.type() == XMLTMPL ) {
			d.value.changeType( NifValue::type( tmp ) );
			d.setType( tmp );
			// The templates are now filled
			d.setTemplated( false );
		}

		if ( d.templ() == XMLTMPL )
			d.setTempl( tmp );

		insertType( parent, d, at );
	} else {
		if ( data.valueType() == NifValue::tString || data.valueType() == NifValue::tFilePath )
			// Kludge for string conversion.
			//  Ensure that the string type is correct for the nif version
			parent->insertChild( data, version < 0x14010003 ? NifValue::tSizedString : NifValue::tStringIndex, at);
		else
			parent->insertChild( data, at );
	}

	restoreState();
}


/*
 *  QAbstractModel interface
 */

QVariant NifModel::data( const QModelIndex & index, int role ) const
{
	QModelIndex _buddy = buddy( index );
	if ( _buddy != index )
		return data( _buddy, role );

	const NifItem * item = getItem( index );
	if ( !item )
		return QVariant();

	int column = index.column();

	bool ndr = ( role == NifSkopeDisplayRole );
	if ( ndr )
		role = Qt::DisplayRole;

	switch ( role ) {
	case Qt::DisplayRole:
		{
			switch ( column ) {
			case NameCol:
				{
					const QString & iname = item->name();

					if ( ndr )
						return iname;

					if ( isNiBlock(item) )
						return QString::number( getBlockNumber(item) ) + " " + iname;

					auto p = item->parent();
					if ( p && p->isArray() && !p->isBinary() ) {
						// Is it a 2nd level array of a multi-array?
						if ( p->isMultiArray() )
							return resolveArrayPseudonym( multiArrayPseudonyms1, item );

						// Is it an item (3rd level) of a multi-array?
						auto pp = p->parent();
						if ( pp && pp->isMultiArray() )
							return resolveArrayPseudonym( multiArrayPseudonyms2, item );

						// It's an item of a non-binary array.
						return resolveArrayPseudonym( arrayPseudonyms, item );
					}

					// Gavrant: not sure why the previous code prepended the item's name with a whitespace.
					// Let's leave that whitespace for first level items, but omit it for their subitems to save a bit of screen space.
					if ( !p || !p->parent() || p->parent() == root )
						return " " + iname;

					return iname;
				}
				break;
			case TypeCol:
				{
					if ( !item->templ().isEmpty() ) {
						const NifItem * tempItem = item;
						while ( tempItem && tempItem->templ() == XMLTMPL )
							tempItem = tempItem->parent();

						return QString( "%1<%2>" ).arg( item->strType(), tempItem ? tempItem->templ() : QString() );
					}

					return item->strType();
				}
				break;
			case ValueCol:
				{
					auto vt = item->valueType();			

					if ( vt == NifValue::tString || vt == NifValue::tFilePath ) {
						return QString( resolveString( item ) ).replace( "\n", " " ).replace( "\r", " " );
					
					} else if ( vt == NifValue::tStringOffset ) {
						int offset = item->get<int>();
						if ( offset < 0 || offset == 0x0000FFFF )
							return tr( "<empty>" );

						auto itemPalette = getItemX( item, "String Palette" );
						if ( !itemPalette )
							return tr( "<palette link not found>" );
						auto paletteLink = getLink( itemPalette );
						if ( paletteLink < 0 )
							return tr( "<empty palette link>" );
						itemPalette = getBlockItem( paletteLink );
						if ( !itemPalette )
							return tr( "<invalid palette link>" );
						itemPalette = getItem( itemPalette, "Palette", false );
						if ( !itemPalette )
							return tr( "<palette not found>" );

						QByteArray *pBytes = itemPalette->get<QByteArray *>();
						if ( !pBytes || offset >= pBytes->count() )
							return tr( "<invalid offset>" );

						return QString( &pBytes->data()[offset] );
					
					} else if ( vt == NifValue::tStringIndex ) {
						int iStrIndex = item->get<int>();
						if ( iStrIndex == -1 )
							return QString();
						if ( iStrIndex < 0 )
							return tr( "%1 - <invalid string index>" ).arg( iStrIndex );

						auto headerStrings = getItem( getHeaderItem(), "Strings" );
						if ( !headerStrings )
							return tr( "%1 - <header strings not found>" ).arg( iStrIndex );
						if ( iStrIndex >= headerStrings->childCount() )
							return tr( "%1 - <invalid string index>" ).arg( iStrIndex );
						auto itemString = getItem( headerStrings, iStrIndex );
						QString s = itemString ? itemString->get<QString>() : QString("<?>");
						return QString( "%2 [%1]" ).arg( iStrIndex ).arg( s );
					
					} else if ( vt == NifValue::tBlockTypeIndex ) {
						int iBlock = item->get<int>();
						auto itemBlockTypes = getItemX( item, "Block Types" );
						if ( !itemBlockTypes )
							return tr( "%1 - <block types not found>" ).arg( iBlock );

						auto itemBlockEntry = itemBlockTypes->child( iBlock & 0x7FFF );
						if ( !itemBlockEntry )
							return tr( "%1 - <index invalid>" ).arg( iBlock );

						return QString( "%2 [%1]" ).arg( iBlock ).arg( itemBlockEntry->get<QString>() );
					
					} else if ( item->isLink() ) {
						int link = item->getLinkValue();
						if ( link < 0 )
							return tr( "None" );
						if ( link >= getBlockCount() )
							return tr( "%1 <invalid block index>" ).arg( link );

						auto block = root->child( link + firstBlockRow() );
						if ( !block )
							return tr( "%1 <???>" ).arg( link );

						QString blockName = get<QString>( block, "Name" );
						if ( !blockName.isEmpty() )
							return tr( "%1 (%2)" ).arg( link ).arg( blockName );

						return tr( "%1 [%2]" ).arg( link ).arg( block->name() );
					
					} else if ( item->isCount() ) {
						if ( item->hasStrType("BSVertexDesc") )
							return item->get<BSVertexDesc>().toString();

						QString optId = NifValue::enumOptionName( item->strType(), item->getCountValue() );
						if ( optId.isEmpty() )
							return item->getValueAsString();

						return optId;
					}

					return item->getValueAsString().replace( "\n", " " ).replace( "\r", " " );
				}
				break;
			case ArgCol:
				return item->arg();
			case Arr1Col:
				return item->arr1();
			case Arr2Col:
				return item->arr2();
			case CondCol:
				return item->cond();
			case Ver1Col:
				return version2string( item->ver1() );
			case Ver2Col:
				return version2string( item->ver2() );
			case VerCondCol:
				return item->vercond();
			default:
				return QVariant();
			}
		}
	case Qt::DecorationRole:
		{
			switch ( column ) {
			case NameCol:
				// (QColor, QIcon or QPixmap) as stated in the docs
				/*if ( itemStrType( index ) == "NiBlock" )
				    return QString::number( getBlockNumber( index ) );*/
				return QVariant();
			default:
				return QVariant();
			}
		}
	case Qt::EditRole:
		{
			switch ( column ) {
			case NameCol:
				return item->name();
			case TypeCol:
				return item->strType();
			case ValueCol:
				{
					if ( item->hasValueType(NifValue::tString) || item->hasValueType(NifValue::tFilePath) )
						return resolveString( item );
					return item->getValueAsVariant();
				}
			case ArgCol:
				return item->arg();
			case Arr1Col:
				return item->arr1();
			case Arr2Col:
				return item->arr2();
			case CondCol:
				return item->cond();
			case Ver1Col:
				return version2string( item->ver1() );
			case Ver2Col:
				return version2string( item->ver2() );
			case VerCondCol:
				return item->vercond();
			default:
				return QVariant();
			}
		}
	case Qt::ToolTipRole:
		{
			switch ( column ) {
			case NameCol:
				{
					if ( isArrayEx( item->parent() ) ) {
						return QString();
					} else {
						QString tip = QString( "<p><b>%1</b></p><p>%2</p>" )
						              .arg( item->name() )
						              .arg( QString( item->text() ).replace( "<", "&lt;" ).replace( "\n", "<br/>" ) );

						if ( NifBlockPtr blk = blocks.value( item->name() ) ) {
							tip += "<p>Ancestors:<ul>";

							while ( blocks.contains( blk->ancestor ) ) {
								tip += QString( "<li>%1</li>" ).arg( blk->ancestor );
								blk  = blocks.value( blk->ancestor );
							}

							tip += "</ul></p>";
						}

						return tip;
					}
				}
				break;
			case TypeCol:
				return NifValue::typeDescription( item->strType() );
			case ValueCol:
				{
					switch ( item->valueType() ) {
					case NifValue::tByte:
					case NifValue::tWord:
					case NifValue::tShort:
					case NifValue::tBool:
					case NifValue::tInt:
					case NifValue::tUInt:
					case NifValue::tULittle32:
						{
							return tr( "dec: %1\nhex: 0x%2" )
							       .arg( item->getValueAsString() )
							       .arg( item->getCountValue(), 8, 16, QChar( '0' ) );
						}
					case NifValue::tFloat:
					case NifValue::tHfloat:
					case NifValue::tNormbyte:
						{
							return tr( "float: %1\nhex: 0x%2" )
							       .arg( NumOrMinMax( item->getFloatValue(), 'g', 8 ) )
							       .arg( item->getCountValue(), 8, 16, QChar( '0' ) );
						}
					case NifValue::tFlags:
						{
							quint16 f = item->getCountValue();
							return tr( "dec: %1\nhex: 0x%2\nbin: 0b%3" )
							       .arg( f )
							       .arg( f, 4, 16, QChar( '0' ) )
							       .arg( f, 16, 2, QChar( '0' ) );
						}
					case NifValue::tStringIndex:
						return QString( "0x%1" )
						       .arg( item->getCountValue(), 8, 16, QChar( '0' ) );
					case NifValue::tStringOffset:
						return QString( "0x%1" )
						       .arg( item->getCountValue(), 8, 16, QChar( '0' ) );
					case NifValue::tVector3:
						return item->get<Vector3>().toHtml();
					case NifValue::tHalfVector3:
						return item->get<HalfVector3>().toHtml();
					case NifValue::tUshortVector3:
						return item->get<UshortVector3>().toHtml();
					case NifValue::tByteVector3:
						return item->get<ByteVector3>().toHtml();
					case NifValue::tMatrix:
						return item->get<Matrix>().toHtml();
					case NifValue::tMatrix4:
						return item->get<Matrix4>().toHtml();
					case NifValue::tQuat:
					case NifValue::tQuatXYZW:
						return item->get<Quat>().toHtml();
					case NifValue::tColor3:
						{
							Color3 c = item->get<Color3>();
							return QString( "R %1\nG %2\nB %3" )
							       .arg( c[0] )
							       .arg( c[1] )
							       .arg( c[2] );
						}
					case NifValue::tByteColor4:
						{
							Color4 c = item->get<ByteColor4>();
							return QString( "R %1\nG %2\nB %3\nA %4" )
								.arg( c[0] )
								.arg( c[1] )
								.arg( c[2] )
								.arg( c[3] );
						}
					case NifValue::tColor4:
						{
							Color4 c = item->get<Color4>();
							return QString( "R %1\nG %2\nB %3\nA %4" )
							       .arg( c[0] )
							       .arg( c[1] )
							       .arg( c[2] )
							       .arg( c[3] );
						}
					default:
						break;
					}
				}
				break;
			default:
				break;
			}
		}
		return QVariant();
	case Qt::BackgroundColorRole:
		{
			// "notify" about an invalid index in "Triangles"
			// TODO: checkbox, "show invalid only"
			if ( column == ValueCol && item->isTriangle() ) {
				const NifItem * nv = findItemX( item, "Num Vertices" );

				if ( !nv ) {
					qDebug() << "Num Vertices is null";
					return QVariant();
				}

				quint32 nvc = nv->getCountValue();
				Triangle t  = item->get<Triangle>();

				if ( t[0] >= nvc || t[1] >= nvc || t[2] >= nvc )
					return QColor::fromRgb( 240, 210, 210 );
			} else if ( column == ValueCol && item->isColor() ) {
				return item->getColorValue();
			}
		}
		return QVariant();
	case Qt::UserRole:
		{
			if ( column == ValueCol ) {
				SpellPtr spell = SpellBook::instant( this, index );
				if ( spell )
					return spell->page() + "/" + spell->name();
			}
		}
		return QVariant();
	default:
		return QVariant();
	}
}

bool NifModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
	if ( role != Qt::EditRole )
		return false;

	NifItem * item = getItem( index );
	if ( !item )
		return false;

	// Set Buddy
	QModelIndex _buddy = buddy( index );
	if ( index != _buddy )
		return setData( _buddy, value, role );

	switch ( index.column() ) {
	case NifModel::NameCol:
		item->setName( value.toString() );

		if ( item->parent() && item->parent() == root )
			updateHeader();

		break;
	case NifModel::TypeCol:
		item->setStrType( value.toString() );
		break;
	case NifModel::ValueCol:
		{
			if ( item->hasValueType(NifValue::tString) || item->hasValueType(NifValue::tFilePath) ) {
				item->changeValueType( version < 0x14010003 ? NifValue::tSizedString : NifValue::tStringIndex );
				assignString( item, value.toString(), true );
			} else {
				item->setValueFromVariant( value );
			}
		}
		break;
	case NifModel::ArgCol:
		item->setArg( value.toString() );
		break;
	case NifModel::Arr1Col:
		item->setArr1( value.toString() );
		break;
	case NifModel::Arr2Col:
		item->setArr2( value.toString() );
		break;
	case NifModel::CondCol:
		item->setCond( value.toString() );
		break;
	case NifModel::Ver1Col:
		item->setVer1( NifModel::version2number( value.toString() ) );
		break;
	case NifModel::Ver2Col:
		item->setVer2( NifModel::version2number( value.toString() ) );
		break;
	case NifModel::VerCondCol:
		item->setVerCond( value.toString() );
	default:
		return false;
	}

	// reverse buddy lookup
	if ( index.column() == ValueCol ) {
		if ( item->hasName("File Name") ) {
			NifItem * parent = item->parent();
			if ( parent && ( parent->hasName("Texture Source") || parent->hasName("NiImage") ) ) {
				parent = parent->parent();
				if ( parent && parent->hasStrType("NiBlock") && parent->hasName("NiSourceTexture") ) {
					QModelIndex pidx = itemToIndex( parent, ValueCol );
					emit dataChanged( pidx, pidx );
				}
			}

		} else if ( item->hasName("Name") ) {
			NifItem * parent = item->parent();
			if ( parent && parent->hasStrType("NiBlock") ) {
				QModelIndex pidx = itemToIndex( parent, ValueCol );
				emit dataChanged( pidx, pidx );
			}

		}
	}

	if ( state == Default ) {
		onItemValueChange( item );
	}

	return true;
}

void NifModel::reset()
{
	beginResetModel();
	resetState();
	updateLinks();
	endResetModel();
}

bool NifModel::removeRows( int iStart, int count, const QModelIndex & parent )
{
	NifItem * item = getItem( parent );
	if ( !item )
		return false;

	int iEnd = iStart + count;
	if ( iStart >= 0 && iEnd <= item->childCount() && iEnd > iStart ) {
		bool hasLinks = false;

		for ( int r = iStart; r < iEnd; r++ ) {
			if ( isLink( item->child(r) ) ) {
				hasLinks = true;
				break;
			}
		}

		beginRemoveRows( parent, iStart, iEnd - 1 );
		item->removeChildren( iStart, count );
		endRemoveRows();

		if ( hasLinks ) {
			updateLinks();
			updateFooter();
			emit linksChanged();
		}

		return true;
	}

	return false;
}

QModelIndex NifModel::buddy( const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( !item )
		return QModelIndex();

	if ( index.column() == ValueCol && item->parent() == root && item->hasStrType("NiBlock") ) {
		QModelIndex buddy;

		if ( item->hasName("NiSourceTexture") || item->hasName("NiImage") ) {
			buddy = getIndex( index, "File Name" );
		} else if ( item->hasName("NiStringExtraData") ) {
			buddy = getIndex( index, "String Data" );
		} else {
			buddy = getIndex( index, "Name" );
		}

		if ( buddy.isValid() )
			buddy = buddy.sibling( buddy.row(), ValueCol );

		if ( buddy.isValid() )
			return buddy;
	} else if ( index.column() == ValueCol && item->parent() != root ) {

		if ( item->hasStrType("ControlledBlock") && item->hasName("Controlled Blocks") ) {
			QModelIndex buddy;

			if ( version >= 0x14010003 ) {
				buddy = getIndex( index, "Node Name" );
			} else if ( version <= 0x14000005 ) {
				buddy = getIndex( index, "Node Name Offset" );
			}

			if ( buddy.isValid() )
				buddy = buddy.sibling( buddy.row(), ValueCol );

			return buddy;
		}
	}

	return index;
}


/*
 *  load and save
 */

bool NifModel::setHeaderString( const QString & s, uint ver )
{
	if ( !( s.startsWith( "NetImmerse File Format" ) || s.startsWith( "Gamebryo" ) // official
	        || s.startsWith( "NDSNIF" )                                            // altantica
	        || s.startsWith( "NS" )                                                // neosteam
	        || s.startsWith( "Joymaster HS1 Object Format - (JMI)" )               // howling sword, uses .jmi extension
	     ) )
	{
		auto m = tr( "Could not open %1 because it is not a supported type." ).arg( fileinfo.fileName() );
		logMessage(m, m, QMessageBox::Critical);

		return false;
	}

	// Early Accept
	if ( isVersionSupported(ver) ) {
		version = ver;
		return true;
	}

	int p = s.indexOf( "Version", 0, Qt::CaseInsensitive );
	if ( p >= 0 ) {
		QString v = s;

		v.remove( 0, p + 8 );

		for ( int i = 0; i < v.length(); i++ ) {
			if ( v[i].isDigit() || v[i] == QChar( '.' ) ) {
				continue;
			} else {
				v = v.left( i );
			}
		}

		version = version2number( v );

		if ( !isVersionSupported( version ) ) {
			auto m = tr( "Version %1 (%2) is not supported." ).arg( version2string( version ), v );
			logMessage(m, m, QMessageBox::Critical);
			return false;
		}

		return true;
	}

	auto m = tr("Invalid header string");
	logMessage(m, m, QMessageBox::Critical);

	return false;
}

bool NifModel::load( QIODevice & device )
{
	QSettings settings;
	bool ignoreSize = settings.value( "Ignore Block Size", true ).toBool();

	clear();

	NifIStream stream( this, &device );

	if ( state != Loading )
		setState( Loading );

	// read header
	NifItem * header = getHeaderItem();
	if ( !loadHeader( header, stream ) ) {
		auto m = tr( "Failed to load file header (version %1, %2)" ).arg( version, 0, 16 ).arg( version2string( version ) );
		logMessage(tr(readFail), m, QMessageBox::Critical);

		resetState();
		return false;
	}

	int numblocks = 0;
	numblocks = get<int>( header, "Num Blocks" );
	//qDebug( "numblocks %i", numblocks );

	emit sigProgress( 0, numblocks );
	//QTime t = QTime::currentTime();

	qint64 curpos = 0;
	try
	{
		curpos = device.pos();

		if ( version >= 0x0303000d ) {
			// read in the NiBlocks
			QString prevblktyp;

			for ( int c = 0; c < numblocks; c++ ) {
				emit sigProgress( c + 1, numblocks );

				if ( device.atEnd() )
					throw tr( "unexpected EOF during load" );

				QString blktyp;
				quint32 size = UINT_MAX;
				try
				{
					if ( version >= 0x0a000000 ) {
						// block types are stored in the header for versions above 10.x.x.x
						//	the upper bit or the blocktypeindex seems to be related to PhysX
						int blktypidx = get<int>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Type Index" ) ) );
						blktyp = get<QString>( index( blktypidx & 0x7FFF, 0, getIndex( createIndex( header->row(), 0, header ), "Block Types" ) ) );
						
						// 20.3.1.2 Custom Version
						if ( version == 0x14030102 ) {
							auto hash = get<quint32>(
								index( blktypidx & 0x7FFF, 0, getIndex( createIndex( header->row(), 0, header ),
																	   "Block Type Hashes" ) )
							);

							if ( blockHashes.contains( hash ) )
								blktyp = blockHashes[hash]->id;
							else
								throw tr( "Block Hash not found." );
						}

						// note: some 10.0.1.0 version nifs from Oblivion in certain distributions seem to be missing
						//		 these four bytes on the havok blocks
						//		 (see for instance meshes/architecture/basementsections/ungrdltraphingedoor.nif)
						if ( (version < 0x0a020000) && ( !blktyp.startsWith( "bhk" ) ) ) {
							int dummy;
							device.read( (char *)&dummy, 4 );

							if ( dummy != 0 ) {
								logWarning(tr("Non-zero block separator (%1) preceding block %2").arg(dummy).arg(blktyp));
							}
						}

						// for version 20.2.0.? and above the block size is stored in the header
						if ( !ignoreSize && version >= 0x14020000 )
							size = get<quint32>( index( c, 0, getIndex( createIndex( header->row(), 0, header ), "Block Size" ) ) );
					} else {
						int len;
						device.read( (char *)&len, 4 );

						if ( len < 2 || len > 80 )
							throw tr( "next block (%1) does not start with a NiString" ).arg( c );

						blktyp = device.read( len );
					}

					// Hack for NiMesh data streams
					NiMesh::DataStreamMetadata metadata = {};

					if ( blktyp.startsWith( "NiDataStream\x01" ) )
						blktyp = extractRTTIArgs( blktyp, metadata );

					if ( isNiBlock( blktyp ) ) {
						//qDebug() << "loading block" << c << ":" << blktyp );
						QModelIndex newBlock = insertNiBlock( blktyp, -1 );

						if ( !loadItem( root->child( c + 1 ), stream ) ) {
							NifItem * child = root->child( c );
							throw tr( "failed to load block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( child ? child->name() : prevblktyp );
						}

						// NiMesh hack
						if ( blktyp == "NiDataStream" ) {
							set<quint32>( newBlock, "Usage", metadata.usage );
							set<quint32>( newBlock, "Access", metadata.access );
						}
					} else {
						logWarning(tr("Block %1 (%2) not inserted!").arg(c).arg(blktyp));

						throw tr( "Encountered unknown block (%1)" ).arg( blktyp );
					}
				}
				catch ( QString & err )
				{
					// version 20.3.0.3 can mostly recover from some failures because it store block sizes
					// XXX FIXME: if isNiBlock returned false, block numbering will be screwed up!!
					if ( size == UINT_MAX )
						throw err;
				}

				// Check device position and emit warning if location is not expected
				if ( size != UINT_MAX ) {
					qint64 pos = device.pos();

					if ( (curpos + size) != pos ) {
						// unable to seek to location... abort
						if ( device.seek( curpos + size ) ) {
							auto m = tr( "device position incorrect after block number %1 (%2) at 0x%3 ended at 0x%4 (expected 0x%5)" )
								.arg( c )
								.arg( blktyp )
								.arg( QString::number( curpos, 16 ) )
								.arg( QString::number( pos, 16 ) )
								.arg( QString::number( curpos + size, 16 )
							);

							logWarning(m);
						}
						else {
							throw tr( "failed to reposition device at block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( root->child( c )->name() );
						}
						curpos = device.pos();
					} else {
						curpos = pos;
					}
				}

				prevblktyp = blktyp;
			}

			// read in the footer
			// Disabling the throw because it hinders decoding when the XML is wrong,
			// and prevents any data whatsoever from loading.
			loadItem( getFooterItem(), stream );
			//if ( !loadItem( getFooterItem(), stream ) )
			//	throw tr( "failed to load file footer" );
		} else {
			// versions below 3.3.0.13
			QMap<qint32, qint32> linkMap;

			try {
				for ( qint32 c = 0; true; c++ ) {
					emit sigProgress( c + 1, 0 );

					if ( device.atEnd() )
						throw tr( "unexpected EOF during load" );

					int len;
					device.read( (char *)&len, 4 );

					if ( len < 0 || len > 80 )
						throw tr( "next block (%1) does not start with a NiString" ).arg( c );

					QString blktyp = device.read( len );

					if ( blktyp == "End Of File" ) {
						break;
					} else if ( blktyp == "Top Level Object" ) {
						device.read( (char *)&len, 4 );

						if ( len < 0 || len > 80 )
							throw tr( "next block (%1) does not start with a NiString" ).arg( c );

						blktyp = device.read( len );
					}

					qint32 p;
					device.read( (char *)&p, 4 );
					p -= 1;

					if ( p != c )
						linkMap.insert( p, c );

					if ( isNiBlock( blktyp ) ) {
						//qDebug() << "loading block" << c << ":" << blktyp );
						insertNiBlock( blktyp, -1 );

						if ( !loadItem( root->child( c + 1 ), stream ) )
							throw tr( "failed to load block number %1 (%2) previous block was %3" ).arg( c ).arg( blktyp ).arg( root->child( c )->name() );
					} else {
						throw tr( "encountered unknown block (%1)" ).arg( blktyp );
					}
				}
			}
			catch ( QString & err ) {
				//If this is an old file we should still map the links, even if it failed
				mapLinks( linkMap );

				//Re-throw exception so that the error is printed
				throw err;
			}

			//Also map links if nothing went wrong
			mapLinks( linkMap );
		}
	}
	catch ( QString & err )
	{
		logMessage(tr(readFail), QString("Pos %1: ").arg(device.pos()) + err, QMessageBox::Critical);
		reset();
		return false;
	}

	//qDebug() << t.msecsTo( QTime::currentTime() );
	reset(); // notify model views that a significant change to the data structure has occurded
	return true;
}

bool NifModel::save( QIODevice & device ) const
{
	NifOStream stream( this, &device );

	setState( Saving );

	// Force update header and footer prior to save
	if ( NifModel * mdl = const_cast<NifModel *>(this) ) {
		mdl->updateHeader();
		mdl->updateFooter();
	}

	emit sigProgress( 0, rowCount( QModelIndex() ) );

	for ( int c = 0; c < rowCount( QModelIndex() ); c++ ) {
		emit sigProgress( c + 1, rowCount( QModelIndex() ) );

		//qDebug() << "saving block " << c << ": " << itemName( index( c, 0 ) );

		if ( itemStrType( index( c, 0 ) ) == "NiBlock" ) {
			if ( version > 0x0a000000 ) {
				if ( version < 0x0a020000 ) {
					int null = 0;
					device.write( (char *)&null, 4 );
				}
			} else {
				if ( version < 0x0303000d ) {
					if ( rootLinks.contains( c - 1 ) ) {
						QString string = "Top Level Object";
						int len = string.length();
						device.write( (char *)&len, 4 );
						device.write( string.toLatin1().constData(), len );
					}
				}

				QString string = itemName( index( c, 0 ) );
				int len = string.length();
				device.write( (char *)&len, 4 );
				device.write( string.toLatin1().constData(), len );

				if ( version < 0x0303000d ) {
					device.write( (char *)&c, 4 );
				}
			}
		}

		if ( !saveItem( root->child( c ), stream ) ) {
			Message::critical( nullptr, tr( "Failed to write block %1 (%2)." ).arg( itemName( index( c, 0 ) ) ).arg( c - 1 ) );
			resetState();
			return false;
		}
	}

	if ( version < 0x0303000d ) {
		QString string = "End Of File";
		int len = string.length();
		device.write( (char *)&len, 4 );
		device.write( string.toLatin1().constData(), len );
	}

	resetState();
	return true;
}

bool NifModel::loadIndex( QIODevice & device, const QModelIndex & index )
{
	NifItem * item = getItem( index );
	if ( item ) {
		NifIStream stream( this, &device );
		bool ok = loadItem( item, stream );
		updateLinks();
		updateFooter();
		emit linksChanged();
		return ok;
	}

	reset();
	return false;
}

bool NifModel::loadAndMapLinks( QIODevice & device, const QModelIndex & index, const QMap<qint32, qint32> & map )
{
	NifItem * item = getItem( index );
	if ( item ) {
		NifIStream stream( this, &device );
		bool ok = loadItem( item, stream );
		mapLinks( item, map );
		updateLinks();
		updateFooter();
		emit linksChanged();
		return ok;
	}

	reset();
	return false;
}

bool NifModel::loadHeaderOnly( const QString & fname )
{
	clear();

	QFile f( fname );

	if ( !f.open( QIODevice::ReadOnly ) ) {
		Message::critical( nullptr, tr( "Failed to open %1" ).arg( fname ) );
		return false;
	}

	NifIStream stream( this, &f );

	// read header
	NifItem * header = getHeaderItem();

	if ( !loadHeader( header, stream ) ) {
		logMessage(tr(readFail), tr("Failed to load file header version %1").arg(version), QMessageBox::Critical);
		return false;
	}

	return true;
}

bool NifModel::earlyRejection( const QString & filepath, const QString & blockId, quint32 v )
{
	NifModel nif;

	if ( nif.loadHeaderOnly( filepath ) == false ) {
		//File failed to read entierly
		return false;
	}

	bool ver_match = false;

	if ( v == 0 ) {
		ver_match = true;
	} else if ( v != 0 && nif.getVersionNumber() == v ) {
		ver_match = true;
	}

	bool blk_match = false;

	if ( blockId.isEmpty() == true || v < 0x0A000100 ) {
		blk_match = true;
	} else {
		const auto & types = nif.getArray<QString>( nif.getHeaderItem(), "Block Types" );
		for ( const QString& s : types ) {
			if ( inherits( s, blockId ) ) {
				blk_match = true;
				break;
			}
		}
	}

	return (ver_match && blk_match);
}

bool NifModel::saveIndex( QIODevice & device, const QModelIndex & index ) const
{
	const NifItem * item = getItem( index );
	if ( item ) {
		NifOStream stream( this, &device );
		return saveItem( item, stream );
	}
	return false;
}

int NifModel::fileOffset( const QModelIndex & index ) const
{
	const NifItem * target = getItem( index );
	if ( target ) {
		NifSStream stream( this );
		int ofs = 0;

		for ( int c = 0; c < root->childCount(); c++ ) {
			const NifItem * block = root->child( c );

			if ( isBlockRow( c ) ) {
				if ( version > 0x0a000000 ) {
					if ( version < 0x0a020000 )
						ofs += 4;
				} else {
					if ( version < 0x0303000d ) {
						if ( rootLinks.contains( c - 1 ) )
							ofs += 4 + QLatin1String( "Top Level Object" ).size();
					}

					ofs += 4 + ( block ? block->name().length() : 0);

					if ( version < 0x0303000d )
						ofs += 4;
				}
			}

			if ( fileOffset( block, target, stream, ofs ) )
				return ofs;
		}
	}

	return -1;
}

int NifModel::blockSize( const NifItem * item ) const
{
	NifSStream stream( this );
	return blockSize( item, stream );
}

int NifModel::blockSize( const NifItem * item, NifSStream & stream ) const
{
	if ( !item )
		return 0;

	auto testSkip = testSkipIO(item);
	QString name;

	int size = 0;
	for ( auto child : item->childIter() ) {
		if ( child->isAbstract() ) {
			//qDebug() << "Not counting abstract item " << child->name();
			continue;
		}

		if ( evalCondition( child ) ) {
			if ( child->isArrayEx() || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( child->isArrayEx() && !child->isBinary() ) {
					int nRealSize = child->childCount();
					int nCalcSize = evalArraySize( child );
					if ( nRealSize != nCalcSize ) {
						reportError( 
							child,
							__func__,
							tr( "The array's size (%1) does not match its calculated size (%2)." ).arg( nRealSize ).arg( nCalcSize ) 
						);
					}
				}

				size += blockSize( child, stream );
			} else {
				size += stream.size( child->value() );
			}
		}

		// Get material path if current item is the Name field of a shader property
		if ( testSkip && child->hasName("Name") ) {
			auto iStr = child->get<int>();
			if ( iStr >= 0 )
				name = get<QString>(getItem(getHeaderItem(), "Strings"), iStr);
		}
		// Short circuit I/O after Controller if shader property Name is a material path
		if ( testSkip && child->hasName("Controller") && !name.isEmpty() )
			break;
	}

	return size;
}

bool NifModel::loadItem( NifItem * parent, NifIStream & stream )
{
	if ( !parent )
		return false;

	bool testSkip = testSkipIO(parent);
	QString name;

	for ( auto child : parent->childIter() ) {
		child->invalidateCondition();

		if ( child->isAbstract() ) {
			//qDebug() << "Not loading abstract item " << child->name();
			continue;
		}

		if ( evalCondition( child ) ) {
			if ( child->isArrayEx() ) {
				if ( !updateArraySize( child ) )
					return false;
				if ( !loadItem( child, stream ) )
					return false;
			} else if ( child->childCount() > 0 ) {
				if ( !loadItem( child, stream ) )
					return false;
			} else {
				if ( !stream.read( child->value() ) )
					return false;
			}
		}

		// Get material path if current item is the Name field of a shader property
		if ( testSkip && child->hasName("Name") ) {
			auto iStr = child->get<int>();
			if ( iStr >= 0 )
				name = get<QString>(getItem(getHeaderItem(), "Strings"), iStr);
		}
		// Short circuit I/O after Controller if shader property Name is a material path
		if ( testSkip && child->hasName("Controller") && !name.isEmpty() )
			break;
	}

	return true;
}

bool NifModel::loadHeader( NifItem * header, NifIStream & stream )
{
	// Load header separately and invalidate conditions before reading
	//	Compensates for < 20.0.0.5 User Version/User Version 2 program defaults issue
	if ( !header )
		return false;

	// Load Version String to set NifModel state
	NifValue verstr = NifValue(NifValue::tHeaderString);
	stream.read(verstr);
	// Reset Stream Device
	stream.reset();

	bsVersion = 0;

	// Reset User Version and BS Header\BS Version of the header without condition evaluation and
	// assuming that there could be muliple children with those names (who knows what will happen to nif.xml).
	bool bFoundUserVersion = false;
	bool bFoundBSVersion = false;
	for ( auto child : header->childIter() ) {
		if ( child->hasName("User Version") ) {
			child->set<int>( 0 );
			bFoundUserVersion = true;
		} else if ( child->hasName("BS Header") ) {
			for ( auto subChild : child->childIter() ) {
				if ( subChild->hasName("BS Version") ) {
					subChild->set<int>( 0 );
					bFoundBSVersion = true;
				}
			}
		}
	}

	if ( !bFoundUserVersion )
		reportError(header, "Could not find \"User Version\" subitem." );
	if ( !bFoundBSVersion )
		reportError(header, "Could not find \"BS Header\\BS Version\" subitem." );

	invalidateItemConditions( header );
	bool result = loadItem(header, stream);
	cacheBSVersion( header );
	return result;
}

bool NifModel::saveItem( const NifItem * parent, NifOStream & stream ) const
{
	if ( !parent )
		return false;

	auto testSkip = testSkipIO(parent);
	QString name;

	for ( auto child : parent->childIter() ) {
		if ( child->isAbstract() ) {
			qDebug() << "Not saving abstract item " << child->name();
			continue;
		}

		if ( evalCondition( child ) ) {
			if ( child->isArrayEx() || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( child->isArrayEx() && child->childCount() != evalArraySize( child ) ) {
					if ( child->isBinary() ) {
						// special byte
					} else {
						logWarning(tr("Block %1 %2 array size mismatch").arg(getBlockNumber(parent)).arg(child->name()));
					}
				}

				if ( !saveItem( child, stream ) )
					return false;
			} else {
				if ( !stream.write( child->value() ) )
					return false;
			}
		}
		
		// Get material path if current item is the Name field of a shader property
		if ( testSkip && child->hasName("Name") ) {
			auto iStr = child->get<int>();
			if ( iStr >= 0 )
				name = get<QString>(getItem(getHeaderItem(), "Strings"), iStr);
		}
		// Short circuit I/O after Controller if shader property Name is a material path
		if ( testSkip && child->hasName("Controller") && !name.isEmpty() )
			break;
	}

	return true;
}

bool NifModel::fileOffset( const NifItem * parent, const NifItem * target, NifSStream & stream, int & ofs ) const
{
	if ( !parent )
		return false;
	if ( parent == target )
		return true;

	for ( auto child : parent->childIter() ) {
		if ( child == target )
			return true;

		if ( evalCondition( child ) ) {
			if ( child->isArrayEx() || !child->arr2().isEmpty() || child->childCount() > 0 ) {
				if ( fileOffset( child, target, stream, ofs ) )
					return true;
			} else {
				ofs += stream.size( child->value() );
			}
		}
	}

	return false;
}

NifItem * NifModel::insertBranch( NifItem * parentItem, const NifData & data, int at )
{
	return parentItem->insertChild( data, NifValue::tNone, at );
}

const NifItem * NifModel::getConditionCacheItem( const NifItem * item ) const
{
	// For an array of BSVertexData/BSVertexDataSSE structures ("fixed compounds", see "Vertex Data" in BSTriShape) 
	// we use the first structure in the array as a cache of condition values for all the structure fields.
	const NifItem * compoundStruct = item->parent();
	if ( compoundStruct ) {
		const NifItem * compoundArray = compoundStruct->parent();
		if ( compoundArray->isArrayEx() && compoundStruct->row() > 0 && isFixedCompound( compoundStruct->strType() ) ) {
			const NifItem * refStruct = compoundArray->child( 0 );
			if ( !refStruct ) // Just in case...
				return nullptr;
			return refStruct->child( item->row() );
		}
	}

	return item;
}

bool NifModel::evalVersionImpl( const NifItem * item ) const
{
	// Early reject for ver1/ver2
	if ( !item->evalVersion(version) )
		return false;

	// If there is a vercond, evaluate it
	if ( !item->vercond().isEmpty() ) {
		const NifItem * refItem = getConditionCacheItem( item );
		if ( refItem != item )
			return evalVersion( refItem );
			
		NifModelEval functor( this, getHeaderItem() );
		if ( !item->verexpr().evaluateBool(functor) )
			return false;
	}

	return true;
}

bool NifModel::evalConditionImpl( const NifItem * item ) const
{
	if ( !item->cond().isEmpty() ) {
		const NifItem * refItem = getConditionCacheItem( item );
		if ( refItem != item )
			return evalCondition( refItem );
	}

	return BaseModel::evalConditionImpl( item );
}

void NifModel::invalidateDependentConditions( NifItem * item )
{
	if ( !item )
		return;

	NifItem * p = item->parent();
	if ( !p || p == root || p->isArrayEx() )
		return;

	const QString & name = item->name();
	for ( int i = item->row() + 1; i < p->childCount(); i++ ) {
		auto c = p->child( i );
		if ( !c ) // Just in case...
			continue;

		// String check for Name in cond or arg
		//	Note: May cause some false positives but this is OK
		if ( c->cond().contains(name) 
			|| c->arg().contains(name) 
			|| ( c->childCount() > 0 && !c->isArrayEx() ) // If it has children but is not an array, let's reset conditions just to be safe.
		) {
			c->invalidateCondition();
		}
	}
}

void NifModel::invalidateHeaderConditions()
{
	invalidateItemConditions( getHeaderItem() );
}

void NifModel::invalidateItemConditions( NifItem * item )
{
	if ( item ) {
		item->invalidateVersionCondition();
		item->invalidateCondition();
	}
}

/*
 *  link functions
 */

void NifModel::updateLinks( int block )
{
	if ( lockUpdates ) {
		needUpdates = UpdateType( needUpdates | utLinks );
		return;
	}

	if ( block >= 0 ) {
		childLinks[ block ].clear();
		parentLinks[ block ].clear();
		updateLinks( block, getBlockItem( block ) );
	} else {
		rootLinks.clear();
		childLinks.clear();
		parentLinks.clear();

		// Run updateLinks() for each block
		for ( int c = 0; c < getBlockCount(); c++ )
			updateLinks( c );

		// Run checkLinks() for each block
		for ( int c = 0; c < getBlockCount(); c++ ) {
			QStack<int> stack;
			checkLinks( c, stack );
		}


		int n = getBlockCount();
		QByteArray hasrefs( n, 0 );

		for ( int c = 0; c < n; c++ ) {
			for ( const auto d : childLinks.value( c ) ) {
				if ( d >= 0 && d < n )
					hasrefs[d] = 1;
			}
		}

		for ( int c = 0; c < n; c++ ) {
			if ( !hasrefs[c] )
				rootLinks.append( c );
		}
	}
}

void NifModel::updateLinks( int block, NifItem * parent )
{
	if ( !parent )
		return;

	auto links = parent->getLinkRows();
	for ( int l : links ) {
		NifItem * c = parent->child( l );
		if ( !c )
			continue;
	
		if ( c->childCount() > 0 ) {
			updateLinks( block, c );
			continue;
		}
	
		int i = c->getLinkValue();
		if ( i >= 0 ) {
			if ( c->valueType() == NifValue::tUpLink ) {
				if ( !parentLinks[block].contains( i ) )
					parentLinks[block].append( i );
			} else {
				if ( !childLinks[block].contains( i ) )
					childLinks[block].append( i );
			}
		}
	}
	
	auto linkparents = parent->getLinkAncestorRows();
	for ( int p : linkparents ) {
		NifItem * c = parent->child( p );
		if ( c && c->childCount() > 0 )
			updateLinks( block, c );
	}
}

void NifModel::checkLinks( int block, QStack<int> & parents )
{
	parents.push( block );
	foreach ( const auto child, childLinks.value( block ) ) {
		if ( parents.contains( child ) ) {
			logWarning(tr("Infinite recursive link detected (%1 -> %2 -> %1)").arg(block).arg(child));

			childLinks[block].removeAll( child );
		} else {
			checkLinks( child, parents );
		}
	}
	parents.pop();
}

void NifModel::adjustLinks( NifItem * parent, int block, int delta )
{
	if ( !parent )
		return;

	if ( parent->childCount() > 0 ) {
		for ( auto child : parent->children() )
			adjustLinks( child, block, delta );
	} else if ( parent->isLink() ) {
		int l = parent->getLinkValue();

		if ( l >= 0 && ( ( delta != 0 && l >= block ) || l == block ) ) {
			if ( delta == 0 )
				parent->setLinkValue( -1 );
			else
				parent->setLinkValue( l + delta );
		}
	}
}

void NifModel::mapLinks( NifItem * parent, const QMap<qint32, qint32> & map )
{
	if ( !parent )
		return;

	if ( parent->childCount() > 0 ) {
		for ( auto child : parent->children() )
			mapLinks( child, map );
	} else if ( parent->isLink() ) {
		int l = parent->getLinkValue();

		if ( l >= 0 ) {
			if ( map.contains( l ) )
				parent->setLinkValue( map[ l ] );
		}
	}
}

bool NifModel::setLink( NifItem * item, qint32 link )
{
	if ( item && item->setLinkValue(link) ) {
		onItemValueChange( item );
		return true;
	}

	return false;
}

QVector<qint32> NifModel::getLinkArray( const NifItem * arrayRootItem ) const
{
	QVector<qint32> links;

	if ( isArrayEx(arrayRootItem) ) {
		int nLinks = arrayRootItem->childCount();
		if ( nLinks > 0 ) {
			links.reserve( nLinks );
			for ( auto child : arrayRootItem->childIter() )
				links.append( child->getLinkValue() );
		}
	} else {
		if ( arrayRootItem )
			reportError( arrayRootItem, __func__, "The item is not an array." );
	}

	return links;
}

bool NifModel::setLinkArray( NifItem * arrayRootItem, const QVector<qint32> & links )
{
	if ( !isArrayEx(arrayRootItem) ) {
		if ( arrayRootItem )
			reportError( arrayRootItem, __func__, "The item is not an array." );
		return false;
	}

	int nLinks = arrayRootItem->childCount();
	if ( links.count() != nLinks ) {
		reportError( 
			arrayRootItem,
			__func__,
			tr( "The input QVector's size (%1) does not match the array's size (%2)." ).arg( links.count() ).arg( nLinks )
		);
		return false;
	}
	if ( nLinks == 0 )
		return true;

	for ( int i = 0; i < nLinks; i++ ) {
		auto child = arrayRootItem->child( i );
		if ( child )
			child->setLinkValue( links.at( i ) );
	}

	onArrayValuesChange( arrayRootItem );

	auto block = getTopItem( arrayRootItem );
	if ( block && block != getFooterItem() ) {
		updateLinks();
		updateFooter();
		emit linksChanged();
	}

	return true;
}

int NifModel::getParent( int block ) const
{
	int parent = -1;

	for ( int b = 0; b < getBlockCount(); b++ ) {
		if ( childLinks.value( b ).contains( block ) ) {
			parent = b;
			break;
		}
	}

	return parent;
}

int NifModel::getParent( const QModelIndex & index ) const
{
	return getParent( getBlockNumber( index ) );
}

QString NifModel::resolveString( const NifItem * item ) const
{
	if ( !item )
		return QString();

	if ( item->isString() )
		return item->get<QString>();

	if ( getVersionNumber() >= 0x14010003 ) {
		int iStrIndex = -1;
		auto vt = item->valueType();
		if ( vt == NifValue::tStringIndex )
			iStrIndex = item->get<int>();
		else if ( vt == NifValue::tNone ) {
			auto itemIndex = getItem( item, "Index" );
			if ( itemIndex )
				iStrIndex = itemIndex->get<int>();
			else
				reportError( item, __func__, "Could not find \"Index\" subitem." );
		} else {
			reportError( item, __func__, tr( "Unsupported value type (%1)." ).arg( int(vt) ) );
		}

		if ( iStrIndex < 0 )
			return QString();

		auto headerStrings = getItem( getHeaderItem(), "Strings" );
		if ( !headerStrings || iStrIndex >= headerStrings->childCount() )
			return QString();
		return BaseModel::get<QString>( headerStrings, iStrIndex );
	}

	if ( item->valueType() == NifValue::tNone ) {
		auto itemString = getItem( item, "String" );
		if ( itemString )
			return itemString->get<QString>();

		reportError( item, __func__, "Could not find \"String\" subitem." );
	} else {
		item->value().reportConvertToError( this, item, "a QString" );
	}

	return QString();
}

bool NifModel::assignString( NifItem * item, const QString & string, bool replace )
{
	if ( !item )
		return false;

	if ( getVersionNumber() >= 0x14010003 ) {
		NifItem * itemIndex = nullptr;
		int iOldStrIndex = -1;

		switch ( item->valueType() ) {
		case NifValue::tNone:
			itemIndex = getItem( item, "Index" );
			if ( !itemIndex ) {
				reportError( item, __func__, "Could not find \"Index\" subitem." );
				return false;
			}
			iOldStrIndex = itemIndex->get<int>();
			break;
		case NifValue::tStringIndex:
			itemIndex = item;
			iOldStrIndex = itemIndex->get<int>();
			break;
		case NifValue::tSizedString:
			if ( item->hasStrType("string") ) {
				itemIndex = item;
				iOldStrIndex = -1;
				break;
			}
			[[fallthrough]];
		default:
			return BaseModel::set<QString>( item, string );
		}

		NifItem * header = getHeaderItem();
		NifItem * headerStrings = getItem( header, "Strings", true );
		if ( !headerStrings )
			return false;
		int nHeaderStrings = headerStrings->childCount();

		if ( string.isEmpty() ) {
			if ( replace && iOldStrIndex >= 0 && iOldStrIndex < nHeaderStrings) {
				// TODO: Can we remove the string safely here?
			}

			itemIndex->changeValueType( NifValue::tStringIndex );
			return set<int>( itemIndex, 0xffffffff );
		}

		// Simply replace the string
		if ( replace && iOldStrIndex >= 0 && iOldStrIndex < nHeaderStrings ) {
			return BaseModel::set<QString>( headerStrings, iOldStrIndex, string );
		}

		int iNewStrIndex = -1;
		for ( int i = 0; i < nHeaderStrings; i++ ) {
			const NifItem * c = headerStrings->child( i );
			if ( c && c->get<QString>() == string ) {
				iNewStrIndex = i;
				break;
			}
		}
		if ( iNewStrIndex < 0 ) {
			// Append string to end of the header string list.
			iNewStrIndex = nHeaderStrings;
			set<uint>( header, "Num Strings", nHeaderStrings + 1 );
			updateArraySize( headerStrings );
			BaseModel::set<QString>( headerStrings, nHeaderStrings, string );
		}

		itemIndex->changeValueType( NifValue::tStringIndex );
		return set<int>( itemIndex, iNewStrIndex );
	} // endif getVersionNumber() >= 0x14010003

	// Handle the older simpler strings
	if ( item->valueType() == NifValue::tNone ) {
		NifItem * itemString = getItem( item, "String" );
		if ( !itemString ) {
			reportError( item, __func__, "Could not find \"String\" subitem." );
			return false;
		}
		return BaseModel::set<QString>( itemString, string );
	
	} else if ( item->valueType() == NifValue::tStringIndex ) {
		NifValue v( NifValue::tString );
		v.set<QString>( string, this, item );
		return setItemValue( item, v );
	}

	return BaseModel::set<QString>( item, string );
}


// convert a block from one type to another
void NifModel::convertNiBlock( const QString & identifier, const QModelIndex & index )
{
	NifItem * branch = getItem( index );
	if ( !branch )
		return;

	const QString & btype = branch->name();
	if ( btype == identifier )
		return;

	if ( !inherits( btype, identifier ) && !inherits( identifier, btype ) ) {
		logMessage(tr("Cannot convert NiBlock."), tr("Block type %1 and %2 are not related").arg(btype, identifier), QMessageBox::Critical);
		return;
	}

	NifBlockPtr srcBlock = blocks.value( btype );
	NifBlockPtr dstBlock = blocks.value( identifier );

	if ( srcBlock && dstBlock ) {
		branch->setName( identifier );

		if ( inherits( btype, identifier ) ) {
			// Remove any level between the two types
			for ( QString ancestor = btype; !ancestor.isNull() && ancestor != identifier; ) {
				NifBlockPtr block = blocks.value( ancestor );

				if ( !block )
					break;

				int n = block->types.count();

				if ( n > 0 )
					removeRows( branch->childCount() - n,  n, index );

				ancestor = block->ancestor;
			}
		} else if ( inherits( identifier, btype ) ) {
			// Add any level between the two types
			QStringList types;

			for ( QString ancestor = identifier; !ancestor.isNull() && ancestor != btype; ) {
				NifBlockPtr block = blocks.value( ancestor );

				if ( !block )
					break;

				types.insert( 0, ancestor );
				ancestor = block->ancestor;
			}

			for ( const QString& ancestor : types ) {
				NifBlockPtr block = blocks.value( ancestor );

				if ( !block )
					break;

				int cn = branch->childCount();
				int n  = block->types.count();

				if ( n > 0 ) {
					beginInsertRows( index, cn, cn + n - 1 );
					branch->prepareInsert( n );
					const auto & types = block->types;
					for ( const NifData& data : types ) {
						insertType( branch, data );
					}
					endInsertRows();
				}
			}
		}

		if ( state != Loading ) {
			updateHeader();
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}
}

bool NifModel::holdUpdates( bool value )
{
	bool retval = lockUpdates;

	if ( lockUpdates == value )
		return retval;

	lockUpdates = value;

	if ( !lockUpdates ) {
		updateModel( needUpdates );
		needUpdates = utNone;
	}

	return retval;
}

void NifModel::updateModel( UpdateType value )
{
	if ( value & utHeader )
		updateHeader();

	if ( value & utLinks )
		updateLinks();

	if ( value & utFooter )
		updateFooter();

	if ( value & utLinks )
		emit linksChanged();
}

bool NifModel::testSkipIO( const NifItem * item ) const
{
	bool testSkip = false;
	// Be advised, getBSVersion returns 0 if it's the file's header that is being loaded.
	// Though for shader properties loadItem happens after the header is fully processed, so the check below should work w/o issues.
	if ( getBSVersion() >= 151 && item->parent() == root ) {
		if ( item->hasName("BSLightingShaderProperty") || item->hasName("BSEffectShaderProperty") )
			testSkip = true;
	}
	return testSkip;
}

void NifModel::cacheBSVersion( const NifItem * headerItem )
{
	bsVersion = get<int>( headerItem, "BS Header\\BS Version" );
}

QString NifModel::topItemRepr( const NifItem * item ) const
{
	int iRow = item->row();
	if ( isBlockRow( iRow ) )
		return QString("%2 [%1]").arg( iRow - firstBlockRow() ).arg( item->name() );

	return item->name();
}

void NifModel::onItemValueChange( NifItem * item )
{
	invalidateDependentConditions( item );
	BaseModel::onItemValueChange( item );

	if ( item->isLink() ) {
		auto block = getTopItem( item );
		if ( block && block != getFooterItem() ) {
			updateLinks();
			updateFooter();
			emit linksChanged();
		}
	}
}


/*
 *  NifModelEval
 */

NifModelEval::NifModelEval( const NifModel * model, const NifItem * item )
{
	this->model = model;
	this->item = item;
}

QVariant NifModelEval::operator()( const QVariant & v ) const
{
	if ( v.type() == QVariant::String ) {
		QString left = v.toString();
		const NifItem * itemLeft = model->getItem( item, left, true );

		if ( itemLeft ) {
			if ( itemLeft->isCount() )
				return QVariant( itemLeft->getCountValue() );
			else if ( itemLeft->isFileVersion() )
				return QVariant( itemLeft->getFileVersionValue() );
		}

		return QVariant( 0 );
	}

	return v;
}
