#ifndef SPELL_CONVERT_H
#define SPELL_CONVERT_H

#include "spellbook.h"
#include "stripify.h"
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

//! \file convert.h spToFO42

//! Convert to Fallout 4
class spToFO42 final : public Spell
{
public:
    QString name() const override final { return Spell::tr( "Convert to Fallout 4" ); }
    QString page() const override final { return Spell::tr( "Batch" ); }

    bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
    QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

class Converter
{
    NifModel * nif;
    NifModel * newNif;
    bool * handledBlocks;



public:
//    class NiTriStrips
//    {
//        //
//    };
    Converter(NifModel * nif1, NifModel * nif2, uint blockCount) {
        nif = nif1;
        newNif = nif2;
        handledBlocks = new bool[blockCount];
    }

    // From blocks.cpp
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

    // From blocks.cpp
    const char * MIME_SEP = "˂"; // This is Unicode U+02C2
    const char * STR_BL = "nifskope˂niblock˂%1˂%2";

    //! Set strings array
    void setStringsArray( const QModelIndex & parentDst, const QModelIndex & parentSrc, const QString & arr, const QString & name = {} )
    {
        auto iArr = nif->getIndex( parentSrc, arr );
        auto iArrDst = newNif->getIndex( parentDst, arr );

        if ( name.isEmpty() ) {
            for ( int i = 0; i < nif->rowCount( iArr ); i++ ) {
                newNif->set<QString>( iArrDst.child( i, 0 ), nif->string( iArr.child( i, 0 ) ) );
            }
        } else {
            for ( int i = 0; i < nif->rowCount( iArr ); i++ )
                newNif->set<QString>( iArrDst.child( i, 0 ), name, nif->string( iArr.child( i, 0 ), name, false ) );
        }
    }

    //! Set "Name" et al. for NiObjectNET, NiExtraData, NiPSysModifier, etc.
    void setNiObjectRootStrings( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc)
    {
        for ( int i = 0; i < newNif->rowCount( iBlockDst ); i++ ) {
            auto iStringSrc = iBlockSrc.child( i, 0 );
            auto iStringDst = iBlockDst.child( i, 0 );
            if ( rootStringList.contains( nif->itemName( iStringSrc ) ) ) {
                newNif->set<QString>( iStringDst,  nif->string( iStringSrc ) );
            }
        }
    }

    //! Set strings for NiMesh
    void setStringsNiMesh( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc )
    {
        QStringList strings;
        auto iDataSrc = nif->getIndex( iBlockSrc, "Datastreams" );
        auto iDataDst = newNif->getIndex( iBlockDst, "Datastreams" );
        for ( int i = 0; i < nif->rowCount( iDataSrc ); i++ )
            setStringsArray( iDataDst.child( i, 0 ), iDataSrc.child( i, 0 ), "Component Semantics", "Name" );
    }

    void  setStringsNiSequence( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc ) {
        auto iControlledBlocksSrc = nif->getIndex( iBlockSrc, "Controlled Blocks" );
        auto iControlledBlocksDst = newNif->getIndex( iBlockDst, "Controlled Blocks" );

        for ( int i = 0; i < newNif->rowCount( iControlledBlocksDst ); i++ ) {
            auto iChildDst = iControlledBlocksDst.child( i, 0 );
            auto iChildSrc = iControlledBlocksSrc.child( i, 0 );

            copyValue<QString>(iChildDst, iChildSrc, "Target Name");
            copyValue<QString>(iChildDst, iChildSrc, "Node Name");
            copyValue<QString>(iChildDst, iChildSrc, "Property Type");
            copyValue<QString>(iChildDst, iChildSrc, "Controller Type");
            copyValue<QString>(iChildDst, iChildSrc, "Controller ID");
            copyValue<QString>(iChildDst, iChildSrc, "Interpolator ID");
        }
    }

    // From blocks.cpp
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

    // Based on spCopyBlock from blocks.cpp
    QModelIndex copyBlock(const QModelIndex & iDst, const QModelIndex & iSrc) {
        QByteArray data;
        QBuffer buffer( &data );
        if ( !buffer.open( QIODevice::ReadWrite ) )
            return {};

        auto bType = nif->createRTTIName( iSrc );

        const QString& form = QString( STR_BL ).arg( nif->getVersion(), bType );

        if ( nif->saveIndex( buffer, iSrc ) ) {
            auto result = acceptFormat( form, newNif );
            auto version = result.first;

            if ( buffer.open( QIODevice::ReadWrite ) ) {
                QModelIndex block = newNif->insertNiBlock( bType, newNif->getBlockCount() );
                newNif->loadIndex( buffer, block );
                blockLink( newNif, iDst, block );

                // Post-Load corrections

                // NiDataStream RTTI arg values
                if ( newNif->checkVersion( 0x14050000, 0 ) && bType == QLatin1String( "NiDataStream" ) ) {
                    NiMesh::DataStreamMetadata metadata = {};
                    newNif->extractRTTIArgs( result.second, metadata );
                    newNif->set<quint32>( block, "Usage", metadata.usage );
                    newNif->set<quint32>( block, "Access", metadata.access );
                }

                // Set strings
                if ( newNif->checkVersion( 0x14010001, 0 ) ) {
                    setNiObjectRootStrings( block, iSrc );
                    if ( nif->inherits( bType, "NiSequence" ) ) {
                        setStringsNiSequence( block, iSrc );
                    } else if ( bType == "NiTextKeyExtraData" ) {
                        setStringsArray( block, iSrc, "Text Keys", "Value" );
                    } else if ( bType == "NiMesh" ) {
                        setStringsNiMesh( block, iSrc );
                    } else if ( bType == "NiStringsExtraData" ) {
                        setStringsArray( block, iSrc, "Data" );
                    } else if ( bType == "NiMorphWeightsController" ) {
                        setStringsArray( block, iSrc, "Target Names" );
                    }

                    if ( bType == "NiMesh" || newNif->inherits( bType, "NiGeometry" ) )
                        setStringsArray( newNif->getIndex( block, "Material Data" ), nif->getIndex( iSrc, "Material Data" ), "Material Name" );
                }

                return block;
            }
        }

        return QModelIndex();
    }

    template <typename T> void copyValue( const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name ) {
        newNif->set<T>( iDst, name, nif->get<T>( iSrc, name ) );
    }

    template <typename T> void copyValue( const QModelIndex & iDst, const QModelIndex & iSrc ) {
        newNif->set<T>( iDst, nif->get<T>( iSrc ) );
    }

    QModelIndex insertNiBlock(const QString & name) { return newNif->insertNiBlock(name); }

    void bsFadeNode( QModelIndex iNode ) {
        printf("BSFadeNode...\n");

        handledBlocks[nif->getBlockNumber(iNode)] = false;

        QModelIndex linkNode;

        QModelIndex fadeNode = insertNiBlock("BSFadeNode");

        copyValue<QString>(fadeNode, iNode, "Name");
//        copyValue<uint>(fadeNode, iNode, "Num Extra Data List");
//        newNif->updateArray(newNif->getIndex(fadeNode, "Extra Data List"));
        QVector<qint32> links = nif->getLinkArray(iNode, "Extra Data List");
        for (int link:links) {
            linkNode = nif->getBlock(link);

            if (nif->getBlockName(linkNode) == "BSXFlags") {
                copyBlock(fadeNode, linkNode);
                handledBlocks[nif->getBlockNumber(linkNode)] = false;
            }

            if (nif->getBlockName(linkNode) == "NiStringExtraData") {
                copyBlock(fadeNode, linkNode);
                handledBlocks[nif->getBlockNumber(linkNode)] = false;
            }
        }

        // TODO: Controller
        copyValue<uint>(fadeNode, iNode, "Flags");
        copyValue<Vector3>(fadeNode, iNode, "Translation");
        copyValue<Matrix>(fadeNode, iNode, "Rotation");
        copyValue<float>(fadeNode, iNode, "Scale");

        // Collision object
        linkNode = nif->getBlock( nif->getLink(iNode, "Collision Object") );
        if (linkNode.isValid()) {
            if (nif->getBlockName(linkNode) == "bhkCollisionObject") {
                QModelIndex colNode = insertNiBlock("bhkCollisionObject");
                newNif->setLink(fadeNode, "Collision Object", newNif->getBlockNumber(colNode));
                copyValue<ushort>(colNode, linkNode, "Flags");
                newNif->setLink(colNode, "Target", newNif->getBlockNumber(fadeNode));
                handledBlocks[nif->getBlockNumber(linkNode)] = false;

//                QModelIndex rigidBody = insertNiBlock("bhkRigidBody");
                QModelIndex rigidBodySrc = nif->getBlock(nif->getLink(linkNode, "Body"));
                QModelIndex rigidBody = copyBlock(fadeNode.parent(), rigidBodySrc);
                // TODO: Skyrim layers
                newNif->setLink(colNode, "Body", newNif->getBlockNumber(rigidBody));
                handledBlocks[nif->getBlockNumber(rigidBodySrc)] = false;

                // NOTE: Radius not rendered? Seems to be at 10 times always
                QModelIndex shape = copyBlock(rigidBody, nif->getBlock(nif->getLink(rigidBodySrc, "Shape")));
                newNif->setLink(rigidBody, "Shape", newNif->getBlockNumber(shape));
                handledBlocks[nif->getLink(rigidBodySrc, "Shape")] = false;
                // TODO: Material
            }
        }

        copyValue<uint>(fadeNode, iNode, "Num Children");

        newNif->updateArray(fadeNode, "Children");
        links = nif->getLinkArray(iNode, "Children");
        for (int i = 0; i < links.count(); i++) {
            linkNode = nif->getBlock(links[i]);

            if (nif->getBlockName(linkNode) == "NiTriStrips") {
                QModelIndex triShape = niTriStrips(linkNode);
                newNif->setLink(newNif->getIndex(fadeNode, "Children").child(i, 0), newNif->getBlockNumber(triShape));
            }
        }
        // TODO: Children

        // TODO: Num Effects
        // TODO: Effects

    }

    QModelIndex niTriStrips( QModelIndex iNode) {
        printf("NiTriStrips...\n");

        handledBlocks[nif->getBlockNumber(iNode)] = false;

        const QModelIndex triShape = newNif->insertNiBlock( "BSTriShape" );
        QModelIndex shaderProperty = newNif->insertNiBlock( "BSLightingShaderProperty" );

        newNif->setLink( triShape, "Shader Property", newNif->getBlockNumber( shaderProperty ) );

        copyValue<Vector3>(triShape, iNode, "Translation");
        copyValue<Matrix>(triShape, iNode, "Rotation");
        copyValue<uint>(triShape, iNode, "Flags");
        copyValue<float>(triShape, iNode, "Scale");

        QList<int> links = nif->getChildLinks(nif->getBlockNumber(iNode));
        for (int i = 0; i < links.count(); i++) {
            iNode = nif->getBlock(links[i]);
            if (nif->getBlockName(iNode) == "NiTriStripsData") {
                niTriStripsData(iNode, triShape);
                handledBlocks[nif->getBlockNumber(iNode)] = false;
            } else if (nif->getBlockName(iNode) == "NiAlphaProperty") {
                copyBlock(triShape, iNode);
                handledBlocks[nif->getBlockNumber(iNode)] = false;
            } else if (nif->getBlockName(iNode) == "BSShaderPPLightingProperty") {
                printf("Link: %d\n", nif->getLink(iNode, "Texture Set"));
                QModelIndex textureSet = copyBlock(shaderProperty, nif->getBlock(nif->getLink(iNode, "Texture Set")));
                handledBlocks[nif->getLink(iNode, "Texture Set")] = false;

                newNif->set<int>(textureSet, "Num Textures", 10);
                newNif->updateArray(newNif->getIndex(textureSet, "Textures"));
                // TODO: Texture Names
                newNif->setLink(shaderProperty, "Texture Set", newNif->getBlockNumber(textureSet));
                copyValue<uint>(shaderProperty, iNode, "Texture Clamp Mode");
                copyValue<float>(shaderProperty, iNode, "Refraction Strength");
                // TODO: Parallax
                // TODO: BSShaderFlags
                // TODO: BSShaderFlags2
                handledBlocks[nif->getBlockNumber(iNode)] = false;
            } else if (nif->getBlockName(iNode) == "NiMaterialProperty") {
                // TODO: Glossiness
                copyValue<float>(shaderProperty, iNode, "Alpha");
                copyValue<Color3>(shaderProperty, iNode, "Specular Color");
                copyValue<Color3>(shaderProperty, iNode, "Emissive Color");
                newNif->set<float>(shaderProperty, "Emissive Multiple", nif->get<float>(iNode, "Emissive Mult"));
                handledBlocks[nif->getBlockNumber(iNode)] = false;
            }
        }

        return triShape;
    }

    void niTriStripsData( QModelIndex iNode, QModelIndex triShape ) {
        printf("NiTriStripsData...\n");

        handledBlocks[nif->getBlockNumber(iNode)] = false;

         // TODO: Vertex Colors
         // TODO: Consistency flags

         QModelIndex boundingSphere = newNif->getIndex(triShape, "Bounding Sphere");
         newNif->set<Vector3>(boundingSphere, "Center", nif->get<Vector3>( iNode, "Center"));
         newNif->set<float>(boundingSphere, "Radius", nif->get<float>( iNode, "Radius"));

         auto vf = nif->get<int>(iNode, "BS Vector Flags");
         auto newVf = newNif->get<BSVertexDesc>( triShape, "Vertex Desc" );

         if (vf & 1) {
            newVf.SetFlag(VertexFlags::VF_UV);
         }

         if (vf & 4096) {
            newVf.SetFlag(VertexFlags::VF_TANGENT);
         }

         if (nif->get<bool>( iNode, "Has Vertices")) {
             newVf.SetFlag(VertexFlags::VF_VERTEX);
         }

         if (nif->get<bool>( iNode, "Has Normals")) {
             newVf.SetFlag(VertexFlags::VF_NORMAL);
         }

         if (nif->get<bool>( iNode, "Has Vertex Colors")) {
             newVf.SetFlag(VertexFlags::VF_COLORS);
         }

         newVf.ResetAttributeOffsets(newNif->getUserVersion2());

         newNif->set<BSVertexDesc>(triShape, "Vertex Desc", newVf);

         QModelIndex points = nif->getIndex(iNode, "Points");
         uint numTriangles = 0;
         QVector<Triangle> arr = QVector<Triangle>();
         for (int i = 0; i < nif->rowCount(points); i++) {
            QVector<ushort> point = nif->getArray<ushort>(points.child(i, 0));
            for (int j = 0; j < point.count() - 2; j++) {
                if (point[j + 1] == point[j + 2] || point[j] == point[j + 1] || point[j] == point[j + 2]) {
                    continue;
                }

                if (j & 1) {
                    arr.append(Triangle(point[j + 1], point[j], point[j + 2]));
                } else {
                    arr.append(Triangle(point[j], point[j + 1], point[j + 2]));
                }
                numTriangles++;
            }
         }

         uint numVertices = nif->get<uint>( iNode, "Num Vertices");

         newNif->set<uint>(triShape, "Num Vertices", numVertices);
         newNif->set<uint>(triShape, "Num Triangles", numTriangles);
         newNif->set<uint>(triShape, "Data Size", newVf.GetVertexSize() * numVertices + numTriangles * 6);
         newNif->updateArray( newNif->getIndex( triShape, "Vertex Data" ) );
         newNif->updateArray( newNif->getIndex( triShape, "Triangles" ) );

         QModelIndex data = newNif->getIndex(triShape, "Vertex Data");
         QModelIndex triangles = newNif->getIndex(triShape, "Triangles");

         newNif->setArray<Triangle>(triangles, arr);


         // 0  Vertex
         // 1  Bitangent X
         // 2  Unknown Short
         // 3  Vertex
         // 4  Bitangent X // Occurs twice??
         // 5  Unknown Int
         // 6  UV
         // 7  Normal
         // 8  Bitangent Y
         // 9  Tangent
         // 10 Bitangent Z
         // 11 Vertex Colors
         // 12 Bone Weights
         // 13 Bone Indices
         // 14 Eye Data

         QVector<Vector3> verts = nif->getArray<Vector3>( iNode, "Vertices" );
         QVector<Vector3> norms = nif->getArray<Vector3>( iNode, "Normals" );
         QVector<Vector3> tangents = nif->getArray<Vector3>( iNode, "Tangents" );
         QVector<Vector3> bitangents = nif->getArray<Vector3>( iNode, "Bitangents" );

         // TODO: Fix for multiple arrays
         QVector<Vector2> uvSets = nif->getArray<Vector2>( nif->getIndex( iNode, "UV Sets"), "UV Sets");
    //             QVector<ushort> points = nif->getArray<ushort>( nif->getIndex( iNode, "Points"), "Points" );
    //             QVector<QModelIndex> points = nif->getArray<QModelIndex>(triShape, "Points");

         QVector<HalfVector3> newVerts = QVector<HalfVector3>(verts.count());

         // Create vertex data
         for ( int i = 0; i < verts.count(); i++ ) {
             newNif->set<HalfVector3>( data.child( i, 0 ).child(0,0), HalfVector3(verts[i]));
             verts.count() == norms.count()    && newNif->set<ByteVector3>( data.child( i, 0 ).child(7,0), ByteVector3(norms[i]));
             verts.count() == tangents.count() && newNif->set<ByteVector3>( data.child( i, 0 ).child(9,0), ByteVector3(tangents[i]));
             verts.count() == uvSets.count()   && newNif->set<HalfVector2>( data.child( i, 0 ).child(6,0), HalfVector2(uvSets[i]));
             if (verts.count() == bitangents.count()) {
                 newNif->set<float>( data.child( i, 0 ).child(1,0),  bitangents[i][0]);

                 // TODO: Set Bitangent Y and Z
             }
         }
     }

    void unhandledBlocks() {
        printf("Unhandled blocks:\n");
        for ( int i = 0; i < nif->getBlockCount(); i++ ) {
            if (handledBlocks[i]) {
                printf("%d:\t%s\n", i, nif->getBlockName(nif->getBlock( i )).toUtf8().constData());
            }
        }
    }
};


#endif // SPELL_CONVERT_H
