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
    std::map<int, int> indexMap = std::map<int, int>();
    QVector<std::tuple<int, QModelIndex>> linkList = QVector<std::tuple<int, QModelIndex>>();

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

                setHandled(block, iSrc);

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

            if (!iDst.isValid()) {
                qDebug() << "Invalid Destination";
            }

            if (!iSrc.isValid()) {
                qDebug() << "Invalid Source";
            }

            nifDst = newNifDst;
            nifSrc = newNifSrc;
        }

        template<typename TDst, typename TSrc> void copyValue(const QModelIndex iTarget, const QModelIndex iSource) {
            nifDst->set<TDst>( iTarget, nifSrc->get<TSrc>( iSource ) );
        }

        template<typename TDst, typename TSrc> void copyValue() {
            copyValue<TDst, TSrc>(iDst, iSrc);
        }

        template<typename TDst, typename TSrc> void copyValue(const QString & nameDst, const QString & nameSrc) {
            QModelIndex iDstNew = nifDst->getIndex(iDst, nameDst);
            QModelIndex iSrcNew = nifSrc->getIndex(iSrc, nameSrc);

            if (!iDstNew.isValid()) {
                qDebug() << "Dst: Invalid value name" << nameDst << "in block type" << nifDst->getBlockName(iDst);
            }

            if (!iSrcNew.isValid()) {
                qDebug() << "Src: Invalid value name" << nameSrc << "in block type" << nifSrc->getBlockName(iSrc);
            }

            copyValue<TDst, TSrc>(iDstNew, iSrcNew);
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

    void setHandled(const QModelIndex iDst,  const QModelIndex iSrc) {
        handledBlocks[nifSrc->getBlockNumber(iSrc)] = false;
        indexMap[nifSrc->getBlockNumber(iSrc)] = nifDst->getBlockNumber(iDst);
    }

    void niPSysData(const QModelIndex iDst, const QModelIndex iSrc) {
        Copy c = Copy(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<int>("Group ID");
        c.copyValue<ushort>("BS Max Vertices");
        c.copyValue<int>("Keep Flags");
        c.copyValue<int>("Compress Flags");
        c.copyValue<bool>("Has Vertices");
        // TODO: Vertices
        c.copyValue<ushort>("BS Vector Flags");
        c.copyValue<bool>("Has Normals");
        c.copyValue<Vector3>("Center");
        c.copyValue<float>("Radius");
        c.copyValue<bool>("Has Vertex Colors");
        // TODO: Vertex Colors
        // TODO: UV Sets
        c.copyValue<ushort>("Consistency Flags");
        c.copyValue<int>("Group ID");
        // TODO: Additional Data
        c.copyValue<bool>("Has Radii");
        c.copyValue<ushort>("Num Active");
        c.copyValue<bool>("Has Sizes");
        c.copyValue<bool>("Has Rotations");
        c.copyValue<bool>("Has Rotation Angles");
        c.copyValue<bool>("Has Rotation Axes");
        c.copyValue<bool>("Has Texture Indices");
        c.copyValue<int>("Num Subtexture Offsets");
        // TODO: Subtexture Offsets
        c.copyValue<bool>("Has Rotation Speeds");
    }

    std::tuple<QModelIndex, QModelIndex> copyLink(QModelIndex iDst, QModelIndex iSrc, const QString & name = "") {
        int lSrc;

        if (name.length() > 0) {
            lSrc = nifSrc->getLink(iSrc, name);
        } else {
            lSrc = nifSrc->getLink(iSrc);
        }

        if (lSrc != -1) {
            QModelIndex iLinkDst;
            const QModelIndex iLinkSrc = nifSrc->getBlock(lSrc);

            const QString type = nifSrc->getBlockName(iLinkSrc);

            if (type == "NiPSysData") {
                iLinkDst = nifDst->insertNiBlock(type);
                niPSysData(iLinkDst, iLinkSrc);
            } else {
                iLinkDst = copyBlock(QModelIndex(), iLinkSrc);
            }

            if (name.length() > 0) {
                if (!nifDst->setLink(iDst, name, nifDst->getBlockNumber(iLinkDst))) {
                    qDebug() << "Failed to set link";
                }
            } else {
                if (!nifDst->setLink(iDst, nifDst->getBlockNumber(iLinkDst))) {
                    qDebug() << "Failed to set link for" << nifDst->getBlockName(iLinkDst);
                }
            }

            setHandled(iLinkDst, iLinkSrc);

            return {iLinkDst, iLinkSrc};
        }

        return {QModelIndex(), QModelIndex()};
    }

    void reLinkExec() {
        qDebug() << "Relinking...";

        QModelIndex iDst;
        int lSrc;

        for (int i = 0; i < linkList.count(); i++) {
            std::tie(lSrc, iDst) = linkList[i];

            if (indexMap.count(lSrc) > 0) {
                int lDst = indexMap[lSrc];
//                nifDst->setLink(iDst, name, lDst);
                nifDst->setLink(iDst, lDst);
            } else {
                qDebug() << "Link" << lSrc << "not found";
                nifDst->setLink(iDst, -1);
            }
        }
    }

    void reLink(QModelIndex iDst, QModelIndex iSrc) {
        int lSrc = nifSrc->getLink(iSrc);
        if (lSrc != -1) {
            linkList.append({lSrc, iDst});
        }
    }

    void reLink(QModelIndex iDst, QModelIndex iSrc, const QString & name) {
        reLink(nifDst->getIndex(iDst, name), nifSrc->getIndex(iSrc, name));
    }

    void reLinkArray(QModelIndex iArrayDst, QModelIndex iArraySrc, const QString arrayName = "", const QString & name = "") {
        if (arrayName.length() != 0) {
            iArrayDst = nifDst->getIndex(iArrayDst, arrayName);
            iArraySrc = nifSrc->getIndex(iArraySrc, arrayName);
        }

        for (int i = 0; i < nifSrc->rowCount(iArrayDst); i++) {
            QModelIndex iLinkDst = iArrayDst.child(i, 0);
            QModelIndex iLinkSrc = iArraySrc.child(i, 0);

            if (name.length() == 0) {
                reLink(iLinkDst, iLinkSrc);
            } else {
                reLink(iLinkDst, iLinkSrc, name);
            }
        }
    }

    void reLinkRec(const QModelIndex iDst) {
        for (int i = 0; i < nifDst->rowCount(iDst); i++) {
            QModelIndex iLink = iDst.child(i, 0);

            int lSrc = nifDst->getLink(iLink);
            if (lSrc != -1) {
                linkList.append({lSrc, iLink});
            } else if (nifDst->rowCount(iLink) > 0) {
                reLinkRec(iLink);
            }
        }
    }

    void niAVDefaultObjectPalette(QModelIndex iDst, QModelIndex iSrc) {
        reLink(iDst, iSrc, "Scene");
        reLinkArray(iDst, iSrc, "Objs", "AV Object");
    }

    void niInterpolator(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Interpolator") {
        QModelIndex iInterpolatorDst;
        QModelIndex iInterpolatorSrc;
        std::tie(iInterpolatorDst, iInterpolatorSrc) = copyLink(iDst, iSrc, name);
        if (iInterpolatorSrc.isValid()) {
            copyLink(iInterpolatorDst, iInterpolatorSrc, "Data");
        }
    }

    void niControllerSequence(QModelIndex iDst, QModelIndex iSrc) {
        // Controlled Blocks
        QModelIndex iControlledBlocksDst = nifDst->getIndex(iDst, "Controlled Blocks");
        QModelIndex iControlledBlocksSrc = nifSrc->getIndex(iSrc, "Controlled Blocks");
        if (iControlledBlocksSrc.isValid()) {
            for (int i = 0; i < nifDst->rowCount(iControlledBlocksDst); i++) {
                QModelIndex iBlockDst = iControlledBlocksDst.child(i, 0);
                QModelIndex iBlockSrc = iControlledBlocksSrc.child(i, 0);

                // Interpolator
                niInterpolator(iBlockDst, iBlockSrc);

                // Controller
                niControllerCopy(iBlockDst, iBlockSrc);
            }
        }

        // Text keys
        copyLink(iDst, iSrc, "Text Keys");

        // Manager
        reLink(iDst, iSrc, "Manager");

        // TODO: Anim Note Arrays
    }

    void niControllerSequences(QModelIndex iDst, QModelIndex iSrc) {
        for (int i = 0; i < nifDst->rowCount(iDst); i++) {
            QModelIndex iSeqSrc = nifSrc->getBlock(nifSrc->getLink(iSrc.child(i, 0)));
            QModelIndex iSeqDst = copyBlock(QModelIndex(), iSeqSrc);

            niControllerSequence(iSeqDst, iSeqSrc);

            nifDst->setLink(iDst.child(i, 0), nifDst->getBlockNumber(iSeqDst));
        }
    }

    void niControllerCopy(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Controller") {
        int numController = nifSrc->getLink(iSrc, name);
        if (numController != -1) {
            QModelIndex iControllerSrc = nifSrc->getBlock(numController);

            QString type = nifSrc->getBlockName(iControllerSrc);

            if (type == "NiMaterialColorController") {
                QModelIndex iControllerDst = nifDst->insertNiBlock("BSEffectShaderPropertyFloatController");

                nifDst->setLink(iDst, name, nifDst->getBlockNumber(iControllerDst));

                Copy c = Copy(iControllerDst, iControllerSrc, nifDst, nifSrc);

                c.copyValue<int>("Flags");
                c.copyValue<float>("Frequency");
                c.copyValue<float>("Phase");
                c.copyValue<float>("Start Time");
                c.copyValue<float>("Stop Time");
                nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", 0);

                niController(iControllerDst, iControllerSrc);
            } else if (type == "BSMaterialEmittanceMultController") {
                QModelIndex iControllerDst = nifDst->insertNiBlock("BSEffectShaderPropertyColorController");

                nifDst->setLink(iDst, name, nifDst->getBlockNumber(iControllerDst));

                Copy c = Copy(iControllerDst, iControllerSrc, nifDst, nifSrc);

                c.copyValue<int>("Flags");
                c.copyValue<float>("Frequency");
                c.copyValue<float>("Phase");
                c.copyValue<float>("Start Time");
                c.copyValue<float>("Stop Time");
                nifDst->set<uint>(iControllerDst, "Type of Controlled Color", 0);

                niController(iControllerDst, iControllerSrc);
            } else {
                QModelIndex iControllerDst = copyBlock(QModelIndex(), nifSrc->getBlock(numController));
                nifDst->setLink(iDst, name, nifDst->getBlockNumber(iControllerDst));
                niController(iControllerDst, iControllerSrc);
            }
        }
    }

    void collisionObjectCopy(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Collision Object") {
        int numCollision = nifSrc->getLink(iSrc, name);
        if (numCollision != -1) {
            QModelIndex iCollisionSrc = nifSrc->getBlock(numCollision);
            if (nifSrc->getBlockName(iCollisionSrc) == "bhkCollisionObject") {
                collisionObject(iDst, iCollisionSrc);
            }
        }
    }

    void niController(QModelIndex iDst, QModelIndex iSrc) {
        setHandled(iDst, iSrc);

        // Target
        reLink(iDst, iSrc, "Target");

        // Next Controller
        niControllerCopy(iDst, iSrc, "Next Controller");

        // Interpolator
        niInterpolator(iDst, iSrc);

        // Visibility Interpolator
        niInterpolator(iDst, iSrc, "Visibility Interpolator");

        // Object Palette
        QModelIndex iObjectPaletteDst;
        QModelIndex iObjectPaletteSrc;
        std::tie(iObjectPaletteDst, iObjectPaletteSrc) = copyLink(iDst, iSrc, "Object Palette");
        if (iObjectPaletteSrc.isValid()) {
            niAVDefaultObjectPalette(iObjectPaletteDst, iObjectPaletteSrc);
        }

        // Extra Targets
        if (nifSrc->getIndex(iSrc, "Extra Targets").isValid()) {
            reLinkArray(iDst, iSrc, "Extra Targets");
        }

        // Controller Sequences
        QModelIndex iControllerSequencesSrc = nifSrc->getIndex(iSrc, "Controller Sequences");
        if (iControllerSequencesSrc.isValid()) {
            niControllerSequences(nifDst->getIndex(iDst, "Controller Sequences"), iControllerSequencesSrc);
        }
    }

    QModelIndex insertNiBlock(const QString & name) { return nifDst->insertNiBlock(name); }

    // NOTE: Props collision not rendered correctly in nifSkope but should work in-game.
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

    void niTexturingProperty(QModelIndex iDst, QModelIndex iSrc) {
        setHandled(iDst, iSrc);

//        Copy c = Copy(iDst, iSrc, nifDst, nifSrc);

        if (nifDst->getBlockName(iDst) == "BSEffectShaderProperty") {
            // TODO: Extra Data
            // TODO: Controller

            // TODO: Base Texture
            if (nifSrc->get<bool>(iSrc, "Has Base Texture")) {
                QModelIndex iBaseTextureSrc = nifSrc->getIndex(iSrc, "Base Texture");
                QModelIndex iNiSourceTexture = nifSrc->getBlock(nifSrc->getLink(iBaseTextureSrc, "Source"));
                Copy c = Copy(iDst, iNiSourceTexture, nifDst, nifSrc);

                setHandled(iDst, iNiSourceTexture);

                QString path = nifSrc->string(iNiSourceTexture, QString("File Name"));
                nifDst->set<QString>(iDst, "Source Texture", updateTexturePath(path));
            }

            // TODO: Dark Texture
            // TODO: Detail Texture
            // TODO: Gloss Texture
            // TODO: Glow Texture
            // TODO: Bump Map Texture
            // TODO: Normal Texture
            // TODO: Parallax Texture
            // TODO: Decal 0 Texture
            // TODO: Shader Textures
        }
    }

    void niMaterialProperty(QModelIndex iDst, QModelIndex iSrc) {
        Copy c = Copy(iDst, iSrc, nifDst, nifSrc);

        // TODO: Extra Data
        niControllerCopy(iDst, iSrc);
        // TODO: Specular Color
        nifDst->set<Color4>(iDst, "Emissive Color", Color4(nifSrc->get<Color3>(iSrc, "Emissive Color")));
        // TODO: Glossiness
        // TODO: Alpha
        c.copyValue<float>("Emissive Multiple", "Emissive Mult");

        setHandled(iDst, iSrc);
    }

    void niParticleSystem(QModelIndex iDst, QModelIndex iSrc) {
        setHandled(iDst, iSrc);

        Copy c = Copy(iDst, iSrc, nifDst, nifSrc);
        c.copyValue<QString>("Name");
        // TODO: Extra data
        niControllerCopy(iDst, iSrc);
        c.copyValue<int>("Flags");
        c.copyValue<Vector3>("Translation");
        c.copyValue<Matrix>("Rotation");
        c.copyValue<float>("Scale");
        // TODO: Properties
        QModelIndex iPropertiesSrc = nifSrc->getIndex(iSrc, "Properties");
        if (nifSrc->rowCount(iPropertiesSrc) > 0) {
            QModelIndex iShaderProperty = nifDst->insertNiBlock("BSEffectShaderProperty");

            nifDst->setLink(iDst, "Shader Property", nifDst->getBlockNumber(iShaderProperty));

            for (int i = 0; i < nifSrc->rowCount(iPropertiesSrc); i++) {
                QModelIndex iPropertySrc = nifSrc->getBlock(nifSrc->getLink(iPropertiesSrc.child(i, 0)));
                QString type = nifSrc->getBlockName(iPropertySrc);

                if (type == "BSShaderNoLightingProperty") {
                    bSShaderLightingProperty(iShaderProperty, iPropertySrc);
                } else if (type == "NiTexturingProperty") {
                    niTexturingProperty(iShaderProperty, iPropertySrc);
                } else if (type == "NiMaterialProperty") {
                    niMaterialProperty(iShaderProperty, iPropertySrc);
                }
            }
        }

        collisionObjectCopy(iDst, iSrc);
        copyLink(iDst, iSrc, "Data");
        // TODO: Skin Instance
        // TODO: Material Data
        c.copyValue<int>("World Space");
        c.copyValue<uint>("Num Modifiers");
        // TODO: Modifiers

        QModelIndex iModifiersDst = nifDst->getIndex(iDst, "Modifiers");
        QModelIndex iModifiersSrc = nifSrc->getIndex(iSrc, "Modifiers");
        nifDst->updateArray(iModifiersDst);
        for (int i = 0; i < nifSrc->rowCount(iModifiersSrc); i++) {
            QModelIndex iModifierDst  = std::get<0>(copyLink(iModifiersDst.child(i, 0), iModifiersSrc.child(i, 0)));
            reLinkRec(iModifierDst);
        }
    }

    QModelIndex bsFadeNode( QModelIndex iNode ) {
        QModelIndex linkNode;
        QModelIndex fadeNode = insertNiBlock("BSFadeNode");

        setHandled(fadeNode, iNode);

//        indexMap.insert(nifSrc->getBlockNumber(iNode), nifDst->getBlockNumber(fadeNode));
        indexMap[nifSrc->getBlockNumber(iNode)] = nifDst->getBlockNumber(fadeNode);

        copyValue<QString>(fadeNode, iNode, "Name");
//        copyValue<uint>(fadeNode, iNode, "Num Extra Data List");
//        newNif->updateArray(newNif->getIndex(fadeNode, "Extra Data List"));
        QVector<qint32> links = nifSrc->getLinkArray(iNode, "Extra Data List");
        for (int link:links) {
            linkNode = nifSrc->getBlock(link);

            QString type = nifSrc->getBlockName(linkNode);
            if (type == "BSXFlags") {
                copyBlock(fadeNode, linkNode);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;
            } else if (type == "NiStringExtraData") {
                copyBlock(fadeNode, linkNode);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;
            }
        }

        niControllerCopy(fadeNode, iNode);


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
            QModelIndex iChildDst = nifDst->getIndex(fadeNode, "Children").child(i, 0);

            QString type = nifSrc->getBlockName(linkNode);
            if (nifSrc->getBlockName(linkNode) == "NiTriStrips") {
                QModelIndex triShape = niTriStrips(linkNode);
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(triShape));
            } else if (type == "BSFadeNode" || type == "NiNode") {
                QModelIndex iFadeNodeChild = bsFadeNode(linkNode);
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(iFadeNodeChild));
            } else if (type == "NiParticleSystem") {
                QModelIndex iNiParticleSystemDst = nifDst->insertNiBlock("NiParticleSystem");
                niParticleSystem(iNiParticleSystemDst, linkNode);
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(iNiParticleSystemDst));
//                copyBlock(QModelIndex(), linkNode);
            }
        }

        // TODO: Num Effects
        // TODO: Effects

        return fadeNode;
    }

    QString updateTexturePath(QString fname) {
        return fname.insert(int(strlen("textures\\")), "new_vegas\\");
    }

    /**
     * Insert prefix after 'textures\'.
     * @brief bsShaderTextureSet
     * @param iDst
     */
    void bsShaderTextureSet(QModelIndex iDst, QModelIndex iSrc) {
        QModelIndex iTextures = nifDst->getIndex(iDst, "Textures");
        for (int i = 0; i < nifDst->get<int>(iDst, "Num Textures"); i++) {
            QString str = nifDst->string(iTextures.child(i, 0));
            if (str.length() > 0) {
                nifDst->set<QString>(iTextures.child(i, 0), updateTexturePath(str));
            }
        }

        nifDst->set<int>(iDst, "Num Textures", 10);
        nifDst->updateArray(nifDst->getIndex(iDst, "Textures"));

        setHandled(iDst, iSrc);
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

    void bSShaderLightingProperty(QModelIndex iDst, QModelIndex iSrc) {
        Copy c = Copy(iDst, iSrc, nifDst, nifSrc);

        // TODO: Texture Names

        copyValue<uint>(iDst, iSrc, "Texture Clamp Mode");

//                c.copyValue<uint>("Shader Flags 1", "Shader Flags");
        c.copyValue<uint>("Shader Flags 2");
//                copyValue<uint>()
        // TODO: BSShaderFlags
        // TODO: BSShaderFlags2

        if (nifDst->getBlockName(iDst) != "BSEffectShaderProperty") {
            if (nifSrc->getBlockName(iSrc) == "BSShaderPPLightingProperty") {
                QModelIndex iTextureSetSrc = nifSrc->getBlock(nifSrc->getLink(iSrc, "Texture Set"));
                QModelIndex iTextureSet = copyBlock(iDst, iTextureSetSrc);

                bsShaderTextureSet(iTextureSet, iTextureSetSrc);
                nifDst->setLink(iDst, "Texture Set", nifDst->getBlockNumber(iTextureSet));
                copyValue<float>(iDst, iSrc, "Refraction Strength");
                // TODO: Parallax
            } else {
                QString fileNameSrc = nifSrc->string(iSrc, QString("File Name"));

                if (fileNameSrc.length() > 0) {
                    QModelIndex iTextureSetDst = nifDst->insertNiBlock("BSShaderTextureSet");

                    nifDst->set<int>(iTextureSetDst, "Num Textures", 10);
                    nifDst->updateArray(nifDst->getIndex(iTextureSetDst, "Textures"));
                    nifDst->set<QString>(nifDst->getIndex(iTextureSetDst, "Textures").child(0, 0), updateTexturePath(fileNameSrc));
                    nifDst->setLink(iDst, "Texture Set", nifDst->getBlockNumber(iTextureSetDst));
                }

                // TODO: Falloff
            }
        }

        setHandled(iDst, iSrc);
    }

    QModelIndex niTriStrips( QModelIndex iNode) {
        const QModelIndex triShape = nifDst->insertNiBlock( "BSTriShape" );
        setHandled(triShape, iNode);

        copyValue<QString>(triShape, iNode, "Name");
        if (nifDst->string(triShape, QString("Name")).length() == 0) {
            qDebug() << "Important Warning: triShape has no name!";
        }

        QModelIndex shaderProperty = nifDst->insertNiBlock( "BSLightingShaderProperty" );

        nifDst->setLink( triShape, "Shader Property", nifDst->getBlockNumber( shaderProperty ) );

        copyValue<Vector3>(triShape, iNode, "Translation");
        copyValue<Matrix>(triShape, iNode, "Rotation");
        copyValue<uint>(triShape, iNode, "Flags");
        copyValue<float>(triShape, iNode, "Scale");

        QModelIndex iNiTriStripsData;
        QModelIndex iBSShaderLightingProperty;

        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iNode));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            QString type = nifSrc->getBlockName(linkNode);

            if (type == "NiTriStripsData") {
                niTriStripsData(linkNode, triShape);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;

                iNiTriStripsData = linkNode;
            } else if (type == "NiAlphaProperty") {
                QModelIndex iNiAlphaProperty = copyBlock(triShape, linkNode);

                // Disable the enable blending flag which turns at least effects\ambient\fxvulturesnv.nif invisible.
                nifDst->set<int>(iNiAlphaProperty, "Flags", nifDst->get<int>(iNiAlphaProperty, "Flags") & ~1);
            } else if (type == "BSShaderPPLightingProperty" || type == "BSShaderNoLightingProperty") {
                bSShaderLightingProperty(shaderProperty, linkNode);
                iBSShaderLightingProperty = linkNode;
            } else if (type == "NiMaterialProperty") {
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
        nifDst->set<uint>(shaderProperty, "Shader Flags 1", getFlagsBSShaderFlags1(iNiTriStripsData, iBSShaderLightingProperty));

        return triShape;
    }

    void niTriStripsData( QModelIndex iNode, QModelIndex triShape ) {
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

void convert(const QString & fname, const QString & root = "") {
    qDebug() << "Processing: " + fname;

    // Load

    NifModel nif = NifModel();
    if (!nif.loadFromFile(fname)) {
        fprintf(stderr, "Failed to load nif\n");
    }

    NifModel newNif = NifModel();
    if (!newNif.loadFromFile("D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\template.nif")) {
        fprintf(stderr, "Failed to load template\n");
    }

    // Convert

    Converter c = Converter(&nif, &newNif, uint(nif.getBlockCount()));

    QString type = nif.getBlockName(nif.getBlock(0));
    if (type == "BSFadeNode" || type == "NiNode") {
       c.bsFadeNode(nif.getBlock(0));
    }

    c.reLinkExec();
    c.unhandledBlocks();

    // Save

    QString fnameDst = QString(fname);
    if (root.length() > 0) {
        fnameDst.remove(0, root.length() + (root.endsWith('/') ? 0 : 1));
    } else {
        fnameDst = QFileInfo(fnameDst).fileName();
    }

    fnameDst = "D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\" + fnameDst;
    qDebug() << "Destination: " + fnameDst;

    if (!newNif.saveToFile(fnameDst)) {
        fprintf(stderr, "Failed to save nif\n");
    }
}

void convertNif(QString fname) {
    if (fname.length() > 0) {
        if (QFileInfo(fname).isFile()) {
            convert(fname);

            return;
        } else if (QDir(fname).exists()) {
            QDirIterator it(fname, QStringList() << "*.nif", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                convert(it.next(), fname);
            }

            return;
        }
    }

    qDebug() << "Path not found";
}
