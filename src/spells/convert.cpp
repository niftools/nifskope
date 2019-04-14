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
#include <QDirIterator>

#include <algorithm> // std::stable_sort

class Converter
{
    NifModel * nifSrc;
    NifModel * nifDst;
    bool * handledBlocks;

    QString textureRootSrc = "textures\\";
    QString textureRootDst = "textures\\new_vegas\\";



public:
    Converter(NifModel * newNifSrc, NifModel * newNifDst, uint blockCount) {
        nifSrc = newNifSrc;
        nifDst = newNifDst;
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
        auto iArr = nifSrc->getIndex( parentSrc, arr );
        auto iArrDst = nifDst->getIndex( parentDst, arr );

        if ( name.isEmpty() ) {
            for ( int i = 0; i < nifSrc->rowCount( iArr ); i++ ) {
                nifDst->set<QString>( iArrDst.child( i, 0 ), nifSrc->string( iArr.child( i, 0 ) ) );
            }
        } else {
            for ( int i = 0; i < nifSrc->rowCount( iArr ); i++ )
                nifDst->set<QString>( iArrDst.child( i, 0 ), name, nifSrc->string( iArr.child( i, 0 ), name, false ) );
        }
    }

    //! Set "Name" et al. for NiObjectNET, NiExtraData, NiPSysModifier, etc.
    void setNiObjectRootStrings( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc)
    {
        for ( int i = 0; i < nifDst->rowCount( iBlockDst ); i++ ) {
            auto iStringSrc = iBlockSrc.child( i, 0 );
            auto iStringDst = iBlockDst.child( i, 0 );
            if ( rootStringList.contains( nifSrc->itemName( iStringSrc ) ) ) {
                nifDst->set<QString>( iStringDst,  nifSrc->string( iStringSrc ) );
            }
        }
    }

    //! Set strings for NiMesh
    void setStringsNiMesh( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc )
    {
        QStringList strings;
        auto iDataSrc = nifSrc->getIndex( iBlockSrc, "Datastreams" );
        auto iDataDst = nifDst->getIndex( iBlockDst, "Datastreams" );
        for ( int i = 0; i < nifSrc->rowCount( iDataSrc ); i++ )
            setStringsArray( iDataDst.child( i, 0 ), iDataSrc.child( i, 0 ), "Component Semantics", "Name" );
    }

    void  setStringsNiSequence( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc ) {
        auto iControlledBlocksSrc = nifSrc->getIndex( iBlockSrc, "Controlled Blocks" );
        auto iControlledBlocksDst = nifDst->getIndex( iBlockDst, "Controlled Blocks" );

        for ( int i = 0; i < nifDst->rowCount( iControlledBlocksDst ); i++ ) {
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

        auto bType = nifSrc->createRTTIName( iSrc );

        const QString& form = QString( STR_BL ).arg( nifSrc->getVersion(), bType );

        if ( nifSrc->saveIndex( buffer, iSrc ) ) {
            auto result = acceptFormat( form, nifDst );
            auto version = result.first;

            if ( buffer.open( QIODevice::ReadWrite ) ) {
                QModelIndex block = nifDst->insertNiBlock( bType, nifDst->getBlockCount() );
                nifDst->loadIndex( buffer, block );
                blockLink( nifDst, iDst, block );

                // Post-Load corrections

                // NiDataStream RTTI arg values
                if ( nifDst->checkVersion( 0x14050000, 0 ) && bType == QLatin1String( "NiDataStream" ) ) {
                    NiMesh::DataStreamMetadata metadata = {};
                    nifDst->extractRTTIArgs( result.second, metadata );
                    nifDst->set<quint32>( block, "Usage", metadata.usage );
                    nifDst->set<quint32>( block, "Access", metadata.access );
                }

                // Set strings
                if ( nifDst->checkVersion( 0x14010001, 0 ) ) {
                    setNiObjectRootStrings( block, iSrc );
                    if ( nifSrc->inherits( bType, "NiSequence" ) ) {
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

                    if ( bType == "NiMesh" || nifDst->inherits( bType, "NiGeometry" ) )
                        setStringsArray( nifDst->getIndex( block, "Material Data" ), nifSrc->getIndex( iSrc, "Material Data" ), "Material Name" );
                }

                return block;
            }
        }

        return QModelIndex();
    }

    template <typename T> void copyValue( const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name ) {
        nifDst->set<T>( iDst, name, nifSrc->get<T>( iSrc, name ) );
    }

    template <typename T> void copyValue( const QModelIndex & iDst, const QModelIndex & iSrc ) {
        nifDst->set<T>( iDst, nifSrc->get<T>( iSrc ) );
    }

    class Copy
    {
        QModelIndex iDst;
        QModelIndex iSrc;
        NifModel * nifDst;
        NifModel * nifSrc;
    public:
        Copy(QModelIndex newIDst, QModelIndex newISrc, NifModel * newNifDst, NifModel * newNifSrc) {
            iDst = newIDst;
            iSrc = newISrc;
            nifDst = newNifDst;
            nifSrc = newNifSrc;
        }

        template<typename TDst, typename TSrc> void copyValue() {
            nifDst->set<TDst>( iDst, nifSrc->get<TSrc>( iSrc ) );
        }

        template<typename TDst, typename TSrc> void copyValue(const QString & nameDst, const QString & nameSrc) {
            nifDst->set<TDst>( iDst, nameDst, nifSrc->get<TSrc>( iSrc, nameSrc ) );
        }

        template<typename TDst, typename TSrc> void copyValue(const QString & name) {
            copyValue<TDst, TSrc>(name, name);
        }

        template<typename T> void copyValue() {
            copyValue<T, T>();
        }

        template<typename T> void copyValue(const QString & nameDst, const QString & nameSrc) {
            copyValue<T, T>(nameDst, nameSrc);
        }

        template<typename T> void copyValue(const QString & name) {
            copyValue<T, T>(name, name);
        }
    };

    QModelIndex insertNiBlock(const QString & name) { return nifDst->insertNiBlock(name); }

    void bhkRigidBody(QModelIndex iDst, QModelIndex iSrc) {
        Copy * c = new Copy(iDst, iSrc, nifDst, nifSrc);

        nifDst->set<float>(iDst, "Time Factor", 1.0);
        c->copyValue<float>("Friction");
        nifDst->set<float>(iDst, "Rolling Friction Multiplier", 0.0);
        c->copyValue<float>("Restitution");
        c->copyValue<float>("Max Linear Velocity");
        c->copyValue<float>("Max Angular Velocity");
        c->copyValue<float>("Penetration Depth");
        c->copyValue<int>("Motion System");
        // Deactivator Type
        c->copyValue<int>("Solver Deactivation");
        c->copyValue<int>("Quality Type");
        c->copyValue<float>("Num Constraints");
        // Constraints
        // Body Flags
    }

    void scaleVector4(QModelIndex iVector4, float scale) {
        Vector4 v = nifDst->get<Vector4>(iVector4);
        v *= scale;
        nifDst->set<Vector4>(iVector4, v);
    }

    // bhkConvexVerticesShape
    void collapseScale(QModelIndex iNode, float scale) {
        QModelIndex iVertices = nifDst->getIndex(iNode, "Vertices");
        for (int i = 0; i < nifDst->rowCount(iVertices); i++) {
            QModelIndex iVertex = iVertices.child(i, 0);
            scaleVector4(iVertex, scale);
        }

        // Don't have to scale normals right?
    }

    void collapseScaleRigidBody(QModelIndex iNode, float scale) {
        scaleVector4(nifDst->getIndex(iNode, "Translation"), scale);
        scaleVector4(nifDst->getIndex(iNode, "Center"), scale);
    }

    void collisionObject( QModelIndex parent, QModelIndex iNode ) {
        QModelIndex iRigidBodySrc = nifSrc->getBlock(nifSrc->getLink(iNode, "Body"));

        QModelIndex shape = copyBlock(QModelIndex(), nifSrc->getBlock(nifSrc->getLink(iRigidBodySrc, "Shape")));
        // NOTE: Copy of rigidBody is only correct up to and including Angular Damping
        QModelIndex iRigidBodyDst = copyBlock(QModelIndex(), iRigidBodySrc);
        QModelIndex colNode = insertNiBlock("bhkCollisionObject");

        // Collision Object
        copyValue<ushort>(colNode, iNode, "Flags");
        handledBlocks[nifSrc->getBlockNumber(iNode)] = false;
        nifDst->setLink(colNode, "Body", nifDst->getBlockNumber(iRigidBodyDst));

        // TODO: Skyrim layers
        bhkRigidBody(iRigidBodyDst, iRigidBodySrc);
        handledBlocks[nifSrc->getBlockNumber(iRigidBodySrc)] = false;

        // Shape
        // NOTE: Radius not rendered? Seems to be at 10 times always
        nifDst->setLink(iRigidBodyDst, "Shape", nifDst->getBlockNumber(shape));
        handledBlocks[nifSrc->getLink(iRigidBodySrc, "Shape")] = false;
        // TODO: Material

        // Scale the collision object.
        // NOTE: scaleNode currently breaks collision for the object.
        bool bScaleNode = false;
        QModelIndex scaleNode;
        if (bScaleNode) {
            scaleNode = insertNiBlock("NiNode");
            uint numChildren = nifDst->get<uint>(parent, "Num Children");
            nifDst->set<uint>(parent, "Num Children", numChildren + 1);
            nifDst->updateArray(parent, "Children");
            nifDst->setLink(nifDst->getIndex(parent, "Children").child(int(numChildren), 0), nifDst->getBlockNumber(scaleNode));
            nifDst->set<float>(scaleNode, "Scale", nifDst->get<float>(shape, "Radius"));

            parent = scaleNode;
        } else {
            collapseScale(shape, nifDst->get<float>(shape, "Radius"));
            collapseScaleRigidBody(iRigidBodyDst, nifDst->get<float>(shape, "Radius"));
        }

        // Link to parent
        nifDst->setLink(parent, "Collision Object", nifDst->getBlockNumber(colNode));
        nifDst->setLink(colNode, "Target", nifDst->getBlockNumber(parent));
    }

    void bsFadeNode( QModelIndex iNode ) {
        printf("BSFadeNode...\n");

        handledBlocks[nifSrc->getBlockNumber(iNode)] = false;

        QModelIndex linkNode;

        QModelIndex fadeNode = insertNiBlock("BSFadeNode");

        copyValue<QString>(fadeNode, iNode, "Name");
//        copyValue<uint>(fadeNode, iNode, "Num Extra Data List");
//        newNif->updateArray(newNif->getIndex(fadeNode, "Extra Data List"));
        QVector<qint32> links = nifSrc->getLinkArray(iNode, "Extra Data List");
        for (int link:links) {
            linkNode = nifSrc->getBlock(link);

            if (nifSrc->getBlockName(linkNode) == "BSXFlags") {
                copyBlock(fadeNode, linkNode);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;
            }

            if (nifSrc->getBlockName(linkNode) == "NiStringExtraData") {
                copyBlock(fadeNode, linkNode);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;
            }
        }

        // TODO: Controller
        copyValue<uint>(fadeNode, iNode, "Flags");
        copyValue<Vector3>(fadeNode, iNode, "Translation");
        copyValue<Matrix>(fadeNode, iNode, "Rotation");
        copyValue<float>(fadeNode, iNode, "Scale");
        copyValue<uint>(fadeNode, iNode, "Num Children");

        // Collision object
        // TODO: Collision object tree must be numbered in reverse e.g:
        // 8 bhkCollsionObject
        // 7 bhkRigidBody
        // 6 bhkConvexVerticesShape
        // Ideally change insertion order to accomplish
        //
        // bhkRigidBody causes all other issues.
        // MO_SYS_SPHERE_STABILIZED works
        // MO_SYS_BOX_STABILIZED works???
        //
        // Scale collision with NiNode
        linkNode = nifSrc->getBlock( nifSrc->getLink(iNode, "Collision Object") );
        if (linkNode.isValid()) {
            if (nifSrc->getBlockName(linkNode) == "bhkCollisionObject") {
                collisionObject(fadeNode, linkNode);
            }
        }

        nifDst->updateArray(fadeNode, "Children");
        links = nifSrc->getLinkArray(iNode, "Children");
        for (int i = 0; i < links.count(); i++) {
            linkNode = nifSrc->getBlock(links[i]);

            if (nifSrc->getBlockName(linkNode) == "NiTriStrips") {
                QModelIndex triShape = niTriStrips(linkNode);
                nifDst->setLink(nifDst->getIndex(fadeNode, "Children").child(i, 0), nifDst->getBlockNumber(triShape));
            }
        }
        // TODO: Children

        // TODO: Num Effects
        // TODO: Effects

    }

    /**
     * Insert prefix after 'textures\'.
     * @brief bsShaderTextureSet
     * @param iDst
     */
    void bsShaderTextureSet(QModelIndex iDst) {
        QModelIndex iTextures = nifDst->getIndex(iDst, "Textures");
        for (int i = 0; i < nifDst->get<int>(iDst, "Num Textures"); i++) {
            QString str = nifDst->string(iTextures.child(i, 0));
            if (str.length() > 0) {
                nifDst->set<QString>(iTextures.child(i, 0), str.insert(int(strlen("textures\\")), "new_vegas\\"));
            }
        }
    }

    uint getFlagsBSShaderFlags1(QModelIndex iNiTriStripsData, QModelIndex iBSShaderPPLightingProperty) {
        if (!iBSShaderPPLightingProperty.isValid()) {
            return 0;
        }

        uint mask = ~(
                uint(1 << 2 ) +
                uint(1 << 4 ) +
                uint(1 << 5 ) +
                uint(1 << 6 ) +
                uint(1 << 7 ) +
                uint(1 << 8 ) +
                uint(1 << 9 ) +
                uint(1 << 10) + // Maybe?
                uint(1 << 11) +
                uint(1 << 12) +
                uint(1 << 14) +
                uint(1 << 19) +
                uint(1 << 21) +
                uint(1 << 22) +
                uint(1 << 23) +
                uint(1 << 25) +
                uint(1 << 28) +
                uint(1 << 30));
        uint flags = nifSrc->get<uint>(iBSShaderPPLightingProperty, "Shader Flags") & mask;

        if (iNiTriStripsData.isValid()) {
            // Set Model_Space_Normals in case there are no tangents
            if (!(nifSrc->get<int>(iNiTriStripsData, "BS Vector Flags") & (1 << 12))) {
                flags = flags | (1 << 12);
            }
        }

        return flags;
    }

    void bSShaderPPLightingProperty(QModelIndex iDst, QModelIndex iSrc) {
        Copy c = Copy(iDst, iSrc, nifDst, nifSrc);

        printf("Link: %d\n", nifSrc->getLink(iSrc, "Texture Set"));
        QModelIndex textureSet = copyBlock(iDst, nifSrc->getBlock(nifSrc->getLink(iSrc, "Texture Set")));
//                QModelIndex iTextures = nifDst->getIndex(textureSet, "Textures");
        bsShaderTextureSet(textureSet);

        handledBlocks[nifSrc->getLink(iSrc, "Texture Set")] = false;

        nifDst->set<int>(textureSet, "Num Textures", 10);
        nifDst->updateArray(nifDst->getIndex(textureSet, "Textures"));
        // TODO: Texture Names
        nifDst->setLink(iDst, "Texture Set", nifDst->getBlockNumber(textureSet));
        copyValue<uint>(iDst, iSrc, "Texture Clamp Mode");
        copyValue<float>(iDst, iSrc, "Refraction Strength");


//                c.copyValue<uint>("Shader Flags 1", "Shader Flags");
        c.copyValue<uint>("Shader Flags 2");

//                copyValue<uint>()

        // TODO: Parallax
        // TODO: BSShaderFlags

        // TODO: BSShaderFlags2
        handledBlocks[nifSrc->getBlockNumber(iSrc)] = false;
    }

    QModelIndex niTriStrips( QModelIndex iNode) {
        printf("NiTriStrips...\n");

        handledBlocks[nifSrc->getBlockNumber(iNode)] = false;

        const QModelIndex triShape = nifDst->insertNiBlock( "BSTriShape" );
        QModelIndex shaderProperty = nifDst->insertNiBlock( "BSLightingShaderProperty" );

        nifDst->setLink( triShape, "Shader Property", nifDst->getBlockNumber( shaderProperty ) );

        copyValue<Vector3>(triShape, iNode, "Translation");
        copyValue<Matrix>(triShape, iNode, "Rotation");
        copyValue<uint>(triShape, iNode, "Flags");
        copyValue<float>(triShape, iNode, "Scale");

        QModelIndex iNiTriStripsData;
        QModelIndex iBSShaderPPLightingProperty;

        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iNode));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            if (nifSrc->getBlockName(linkNode) == "NiTriStripsData") {
                niTriStripsData(linkNode, triShape);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;

                iNiTriStripsData = linkNode;
            } else if (nifSrc->getBlockName(linkNode) == "NiAlphaProperty") {
                copyBlock(triShape, linkNode);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;
            } else if (nifSrc->getBlockName(linkNode) == "BSShaderPPLightingProperty") {
                bSShaderPPLightingProperty(shaderProperty, linkNode);
                iBSShaderPPLightingProperty = linkNode;
            } else if (nifSrc->getBlockName(linkNode) == "NiMaterialProperty") {
                // TODO: Glossiness
                copyValue<float>(shaderProperty, linkNode, "Alpha");
                copyValue<Color3>(shaderProperty, linkNode, "Specular Color");
                copyValue<Color3>(shaderProperty, linkNode, "Emissive Color");
                nifDst->set<float>(shaderProperty, "Emissive Multiple", nifSrc->get<float>(linkNode, "Emissive Mult"));
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;
            }
        }

        // Multinode dependant values

        // Shader Flags
        nifDst->set<uint>(shaderProperty, "Shader Flags 1", getFlagsBSShaderFlags1(iNiTriStripsData, iBSShaderPPLightingProperty));

        return triShape;
    }

    void niTriStripsData( QModelIndex iNode, QModelIndex triShape ) {
        printf("NiTriStripsData...\n");

        handledBlocks[nifSrc->getBlockNumber(iNode)] = false;

         // TODO: Vertex Colors
         // TODO: Consistency flags

         QModelIndex boundingSphere = nifDst->getIndex(triShape, "Bounding Sphere");
         nifDst->set<Vector3>(boundingSphere, "Center", nifSrc->get<Vector3>( iNode, "Center"));
         nifDst->set<float>(boundingSphere, "Radius", nifSrc->get<float>( iNode, "Radius"));

         auto vf = nifSrc->get<int>(iNode, "BS Vector Flags");
         auto newVf = nifDst->get<BSVertexDesc>( triShape, "Vertex Desc" );

         if (vf & 1) {
            newVf.SetFlag(VertexFlags::VF_UV);
         }

         if (vf & 4096) {
            newVf.SetFlag(VertexFlags::VF_TANGENT);
         }

         if (nifSrc->get<bool>( iNode, "Has Vertices")) {
             newVf.SetFlag(VertexFlags::VF_VERTEX);
         }

         if (nifSrc->get<bool>( iNode, "Has Normals")) {
             newVf.SetFlag(VertexFlags::VF_NORMAL);
         }

         if (nifSrc->get<bool>( iNode, "Has Vertex Colors")) {
             newVf.SetFlag(VertexFlags::VF_COLORS);
         }

         newVf.ResetAttributeOffsets(nifDst->getUserVersion2());

         nifDst->set<BSVertexDesc>(triShape, "Vertex Desc", newVf);

         QModelIndex points = nifSrc->getIndex(iNode, "Points");
         uint numTriangles = 0;
         QVector<Triangle> arr = QVector<Triangle>();
         for (int i = 0; i < nifSrc->rowCount(points); i++) {
            QVector<ushort> point = nifSrc->getArray<ushort>(points.child(i, 0));
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

         uint numVertices = nifSrc->get<uint>( iNode, "Num Vertices");

         nifDst->set<uint>(triShape, "Num Vertices", numVertices);
         nifDst->set<uint>(triShape, "Num Triangles", numTriangles);
         nifDst->set<uint>(triShape, "Data Size", newVf.GetVertexSize() * numVertices + numTriangles * 6);
         nifDst->updateArray( nifDst->getIndex( triShape, "Vertex Data" ) );
         nifDst->updateArray( nifDst->getIndex( triShape, "Triangles" ) );

         QModelIndex data = nifDst->getIndex(triShape, "Vertex Data");
         QModelIndex triangles = nifDst->getIndex(triShape, "Triangles");

         nifDst->setArray<Triangle>(triangles, arr);


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

         QVector<Vector3> verts = nifSrc->getArray<Vector3>( iNode, "Vertices" );
         QVector<Vector3> norms = nifSrc->getArray<Vector3>( iNode, "Normals" );
         QVector<Vector3> tangents = nifSrc->getArray<Vector3>( iNode, "Tangents" );
         QVector<Vector3> bitangents = nifSrc->getArray<Vector3>( iNode, "Bitangents" );

         // TODO: Process for no tangents

         // TODO: Fix for multiple arrays
         QVector<Vector2> uvSets = nifSrc->getArray<Vector2>( nifSrc->getIndex( iNode, "UV Sets"), "UV Sets");
    //             QVector<ushort> points = nif->getArray<ushort>( nif->getIndex( iNode, "Points"), "Points" );
    //             QVector<QModelIndex> points = nif->getArray<QModelIndex>(triShape, "Points");

         QVector<HalfVector3> newVerts = QVector<HalfVector3>(verts.count());

         // Create vertex data
         for ( int i = 0; i < verts.count(); i++ ) {
             nifDst->set<HalfVector3>( data.child( i, 0 ).child(0,0), HalfVector3(verts[i]));
             verts.count() == norms.count()    && nifDst->set<ByteVector3>( data.child( i, 0 ).child(7,0), ByteVector3(norms[i]));
             verts.count() == tangents.count() && nifDst->set<ByteVector3>( data.child( i, 0 ).child(9,0), ByteVector3(tangents[i]));
             verts.count() == uvSets.count()   && nifDst->set<HalfVector2>( data.child( i, 0 ).child(6,0), HalfVector2(uvSets[i]));
             if (verts.count() == bitangents.count()) {
                 nifDst->set<float>( data.child( i, 0 ).child(1,0),  bitangents[i][0]);

                 // TODO: Set Bitangent Y and Z
             }
         }
     }

    void unhandledBlocks() {
        printf("Unhandled blocks:\n");
        for ( int i = 0; i < nifSrc->getBlockCount(); i++ ) {
            if (handledBlocks[i]) {
                printf("%d:\t%s\n", i, nifSrc->getBlockName(nifSrc->getBlock( i )).toUtf8().constData());
            }
        }
    }
};

void convert(QString fname) {
    qDebug() << "Processing: " + fname;

    NifModel nif = NifModel();
    if (!nif.loadFromFile(fname)) {
        fprintf(stderr, "Failed to load nif\n");
    }

    NifModel newNif = NifModel();
    if (!newNif.loadFromFile("D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\template.nif")) {
        fprintf(stderr, "Failed to load template\n");
    }

    Converter c = Converter(&nif, &newNif, uint(nif.getBlockCount()));

    for ( int b = 0; b < nif.getBlockCount(); b++ ) {
        QModelIndex iNode = nif.getBlock( b );

        if (nif.getBlockName(iNode) == "BSFadeNode") {
           c.bsFadeNode(iNode);
        }
    }

    c.unhandledBlocks();

    QString fnameDst = "D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\" + QFileInfo(fname).fileName();
    qDebug() << "Destination: " + fnameDst;

    if (!newNif.saveToFile(fnameDst)) {
        fprintf(stderr, "Failed to save nif\n");
    }
}

QModelIndex convertNif(QString fname) {
    if (fname.length() > 0) {
        if (QFileInfo(fname).isFile()) {
            convert(fname);
        } else if (QDir(fname).exists()) {
            QDirIterator it(fname, QStringList() << "*.nif", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                convert(it.next());
            }
        }
    } else {
        qDebug() << "Path not found";
    }

    return QModelIndex();
}
