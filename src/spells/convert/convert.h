#ifndef SPELL_CONVERT_H
#define SPELL_CONVERT_H

#include "spellbook.h"
#include "copier.h"
#include "progress.h"
#include "ui/conversiondialog.h"

#include <QThread>
#include <QMutex>

//! \file convert.h spToFO42

#define RUN_CONCURRENT true

/**
 * @brief The FileType enum describes a nif's in engine type.
 */
enum FileType
{
    Invalid,
    Standard,
    LODLandscape,
    LODObject,
    LODObjectHigh,
};

class HighLevelLOD
{
private:
    const QString fname;
    const QString worldspace;
    const int x;
    const int y;
public:
    HighLevelLOD(QString fname, QString worldspace, int x, int y);

//    QString getFileName(const QString & level, const QString & x, const QString & y) {
//        return pathDst + level + "." + x + "." + y + ".BTO";
//    }

    /**
     * Return true if both objects belong to the same grid coordinates for the given level.
     * @brief compare
     * @param level
     * @param other
     * @return
     */
    bool compare(int level, int otherX, int otherY);

    int getX() const;
    int getY() const;
    QString getFname() const;
    QString getWorldspace() const;
};

class Controller
{
private:
    // Original Controller blocknumber in destination nif.
    const int origin;
    const QString name;

    // TODO: These need to be included in any manager and sequences
    // TODO: Change QList to Controller List
    // TODO: Add an interpolator list which contains all interpolators which affect the block
    QList<int> clones;
    QList<QString> cloneBlockNames;

public:
    Controller(int origin, const QString & name);

    void add(int blockNumber, const QString & blockName);

    int getOrigin() const;
    QList<int> getClones() const;
    QList<QString> getCloneBlockNames() const;
};

class EnumMap : public QMap<QString, QString>
{
    const QString enumTypeSrc;
    const QString enumTypeDst;
    const NifModel * nifSrc;
public:
    EnumMap(QString enumTypeSrc, QString enumTypeDst, NifModel * nifSrc);

    void check();

    uint convert(NifValue option);

    uint convert(QModelIndex iSrc);
};

template <typename T>
class ListWrapper
{
public:
    ListWrapper(QList<T> & list) : list(list) {}
    void append(T s);
private:
    QList<T> & list;
    QMutex listMu;
};

class FileProperties
{
public:
    FileProperties(FileType fileType, QString fnameDst, QString fnameSrc, int lodLevel, int lodXCoord, int lodYCoord);

    FileProperties(FileType fileType, QString fnameDst, QString fnameSrc);

    FileProperties();

    FileType getFileType() const;
    int getLodLevel() const;
    int getLodXCoord() const;
    int getLodYCoord() const;
    QString getFnameSrc() const;

private:
    FileType fileType;
    QString fnameDst;
    QString fnameSrc;
    int lodLevel = 0;
    int lodXCoord = 0;
    int lodYCoord = 0;
};

class Converter
{
    // Texture root folder
    #define TEXTURE_ROOT "textures\\"
    // Texture folder inside root folder
    #define TEXTURE_FOLDER "new_vegas\\"

    NifModel * nifSrc;
    NifModel * nifDst;

    QVector<bool> handledBlocks;

    std::map<int, int> indexMap = std::map<int, int>();
    QVector<std::tuple<int, QModelIndex>> linkList = QVector<std::tuple<int, QModelIndex>>();

    EnumMap matMap = EnumMap("Fallout3HavokMaterial", "Fallout4HavokMaterial", nifSrc);
    EnumMap layerMap = EnumMap("Fallout3Layer", "Fallout4Layer", nifSrc);

    QList<QModelIndex> niControllerSequenceList;
    QList<Controller> controllerList;
    QList<QModelIndex> controlledBlockList;

    bool conversionResult = true;
    bool bLODLandscape = false;
    bool bLODBuilding = false;

    Progress progress;
    FileProperties fileProps;

public:
    Converter(NifModel * nifSrc, NifModel * nifDst, FileProperties fileProps, ProgressReceiver & receiver);

    void convert();

    bool getConversionResult() const;
    bool getBLODLandscape() const;
    bool getBLODBuilding() const;

private:
    /*******************************************************************************************************************
     * Block Conversion Functions
     ******************************************************************************************************************/

    QModelIndex bsFadeNode( QModelIndex iNode);

    QModelIndex niControllerSequence(QModelIndex iSrc);

    void niInterpolator(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Interpolator");

    /**
     * @brief niControllerSequences
     * Handle controller sequences from NiControllerManager blocks.
     * @param iDst
     * @param iSrc
     */
    void niControllerSequences(QModelIndex iDst, QModelIndex iSrc);

    QModelIndex niController(
            QModelIndex iDst,
            QModelIndex iSrc,
            Copier & c,
            const QString & name = "Controller",
            const QString & blockName = "",
            const int target = -1);

    QModelIndex niController(
            QModelIndex iDst,
            QModelIndex iSrc,
            const QString & name = "Controller",
            const QString & blockName = "",
            const int target = -1);

    /**
     * @brief bhkPackedNiTriStripsShapeAlt
     * @param iDst bhkCompressedMeshShape
     * @param iSrc
     * @param row
     * @return
     */
    QModelIndex bhkPackedNiTriStripsShapeAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row);

    // TODO:
    // Fix collision between chunks.
    // Handle scaling and out of bounds.
    // Chunk transforms.
    QModelIndex bhkPackedNiTriStripsShapeDataAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row);

    QModelIndex bhkPackedNiTriStripsShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkPackedNiTriStripsShapeDataSubShapeTriangles(
            QModelIndex iSubShapeSrc,
            QModelIndex iDataSrc,
            int firstVertIndex,
            int row);

    QModelIndex bhkPackedNiTriStripsShapeData(QModelIndex iSrc, QModelIndex iBhkNiTriStripsShapeDst, int row);

    QModelIndex bhkMoppBvTreeShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkNiTriStripsShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkNiTriStripsShapeData(QModelIndex iSrc, int row);

    QModelIndex bhkSphereShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkConvexTransformShape(
            QModelIndex iSrc,
            QModelIndex & parent,
            int row,
            bool & bScaleSet,
            float & radius);

    QModelIndex bhkTransformShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkBoxShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkCapsuleShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius);

    void collisionObject( QModelIndex parent, QModelIndex iSrc, QString type = "bhkCollisionObject" );

    // NOTE: Copy of rigidBody is only correct up to and including Angular Damping
    // NOTE: Some props have weird collision e.g: 9mmammo.nif.
    QModelIndex bhkRigidBody(QModelIndex iSrc, QModelIndex & parent, int row);

    // NOTE: Block number does not appear to matter
    QModelIndex bhkConstraint(QModelIndex iSrc);

    QModelIndex bhkConvexVerticesShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius);

    QModelIndex bhkListShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius);

    void niTexturingProperty(QModelIndex iDst, QModelIndex iSrc, const QString & sequenceBlockName);

    void niMaterialProperty(QModelIndex iDst, QModelIndex iSrc, const QString & sequenceBlockName);

    void particleSystemModifiers(QModelIndex iDst, QModelIndex iSrc, Copier & c);

    QModelIndex niParticleSystem(QModelIndex iSrc);

    void niPSys(QModelIndex iLinkDst, QModelIndex iLinkSrc);

    QModelIndex niPSysColliderManager(QModelIndex iDst, QModelIndex iSrc);

    QModelIndex niPSysCollider(QModelIndex iSrc);

    QModelIndex bsFurnitureMarker(QModelIndex iSrc);

    QModelIndex bsSegmentedTriShape(QModelIndex iSrc);

    QModelIndex bsMultiBound(QModelIndex iSrc);

    QModelIndex bsStripParticleSystem(QModelIndex iSrc);

    QModelIndex bsStripPSysData(QModelIndex iSrc);

    /**
     * @brief niCamera
     * Source block type: NiCamera
     * @param iSrc
     * @return
     */
    QModelIndex niCamera(QModelIndex iSrc);

    /**
     * @brief niTriShape converts 'NiTriShape' blocks.
     * Source block type: NiTriShape
     * @param iSrc
     * @return
     */
    QModelIndex niTriShape(QModelIndex iSrc);

    QModelIndex niTriShapeAlt(QModelIndex iSrc);

    // NOTE: Apply after vertices have been created for example by niTriShapeDataAlt()
    QModelIndex niSkinInstance(QModelIndex iBSTriShapeDst, QModelIndex iShaderPropertyDst, QModelIndex iSrc);

    /**
     * @brief niSkinData
     * Source block type: NiSkinData
     * @param iBSTriShapeDst
     * @param iSrc
     * @return
     */
    QModelIndex niSkinData(QModelIndex iBSTriShapeDst, QModelIndex iSrc);

    QModelIndex niTriShapeData(QModelIndex iSrc);

    QModelIndex niPointLight(QModelIndex iSrc);

    /**
     * Insert prefix after 'textures\'.
     * @brief bsShaderTextureSet
     * @param iDst
     */
    void bsShaderTextureSet(QModelIndex iDst, QModelIndex iSrc);

    void bSShaderLightingProperty(QModelIndex iDst, QModelIndex iSrc, const QString & sequenceBlockName);

    void shaderProperty(QModelIndex iDst, QModelIndex iSrc, const QString & type, const QString & sequenceBlockName);

    QModelIndex niTriStrips( QModelIndex iSrc);

    // Up to and including num triangles
    void niTriData(QModelIndex iDst, QModelIndex iSrc, Copier & c);

    void niTriStripsData( QModelIndex iSrc, QModelIndex iDst );

    void niTriShapeDataAlt(QModelIndex iDst, QModelIndex iSrc);

    /*******************************************************************************************************************
     * Block Conversion utility functions
     ******************************************************************************************************************/

    void niPSysData(const QModelIndex iDst, const QModelIndex iSrc);

    void niAVDefaultObjectPalette(QModelIndex iDst, QModelIndex iSrc);

    Controller & getController(int blockNumber);

    QString getControllerType(QString dstType);

    std::tuple<QModelIndex, QString> getControllerType(QString dstType, const QString & name);

    QModelIndex niControllerSetLink(QModelIndex iDst, const QString & name, QModelIndex iControllerDst);

    std::tuple<QModelIndex, QString> getController(
            const QString & dstType,
            const QString & valueType,
            const QString & controlledValueEffect,
            const QString & controlledValueLighting);

    std::tuple<QModelIndex, QString> getController(
            const QString & dstType,
            const QString & valueType,
            const QString & controlledValue);

    void ignoreController(QModelIndex iSrc);

    // TODO: Find best way to process blocks sharing the same controllers but with different controllers down the chain
    QModelIndex niControllerCopy(
            QModelIndex iDst,
            QModelIndex iSrc,
            QString name = "Controller",
            const QString & blockName = "",
            const int target = -1);

    void collisionObjectCopy(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Collision Object");

    void scaleVector4(QModelIndex iVector4, float scale);

    // bhkConvexVerticesShape
    void collapseScale(QModelIndex iNode, float scale);

    void collapseScaleRigidBody(QModelIndex iNode, float scale);

    void setMax(float & val, float comparison);

    void setMin(float & val, float comparison);

    bool inRange(int low, int high, int x);

    /**
     * @brief vertexRange
     * Handle vertices connected to a vertex in a sub shape but not in the sub shape range.
     * @param iVerticesSrc
     * @param vertexList
     * @param vertexMap
     * @param bInRange
     * @param index
     */
    void vertexRange(
            const QModelIndex iVerticesSrc,
            QVector<Vector3> & vertexList,
            QMap<int, int> & vertexMap,
            bool bInRange,
            int index);

    void setVertex(QModelIndex iPoint1, QModelIndex iPoint2, QMap<int, int> & vertexMap, int firstIndex, int index);

    /**
     * @brief bhkUpdateScale
     * @param bScaleSet
     * @param radius
     * @param newRadius
     * @return True if newly set, else false
     */
    bool bhkUpdateScale(bool & bScaleSet, float & radius, const float newRadius);

    QModelIndex niPSysColliderCopy(QModelIndex iSrc);

//    QModelIndex niBillBoardNode(QModelIndex iDst, QModelIndex iSrc) {
////        Copier c = Copier(fadeNode)
//    }

    void extraDataList(QModelIndex iDst, QModelIndex iSrc);

    void extraDataList(QModelIndex iDst, QModelIndex iSrc, Copier & c);

    void bsSegmentedTriShapeSegments(QModelIndex iDst, QModelIndex iSrc, Copier & c);

    bool hasProperty(QModelIndex iSrc, const QString & name);

    void materialData(QModelIndex iSrc, Copier & c);

    void niTriShapeMaterialData(QModelIndex iDst, QModelIndex iSrc, Copier & c);

    // Has no visible effect source. Maybe just bounding spheres but also has rotation.
    void niSkinDataSkinTransform(
            QModelIndex iBoneDst,
            QModelIndex iSkinTransformSrc,
            QModelIndex iSkinTransformGlobalSrc);

    void niTriShapeDataArray(const QString & name, bool bHasArray, Copier & c);

    void niTriShapeDataArray(
            QModelIndex iSrc,
            const QString & name,
            const QString & boolName,
            Copier & c,
            bool isFlag = false);

    QString updateTexturePath(QString fname);

    void setFallout4ShaderFlag(uint & flags, const QString & enumName, const QString & optionName);

    void setFallout4ShaderFlag1(uint & flags, const QString & optionName);

    void setFallout4ShaderFlag2(uint & flags, const QString & optionName);

    uint bsShaderFlagsGet(const QString & enumName, const QString & optionName);

    uint bsShaderFlags1Get(QString optionName);

    uint bsShaderFlags2Get(QString optionName);

    bool bsShaderFlagsIsSet(uint shaderFlags, const QString & enumNameSrc, const QString & optionNameSrc);

    bool bsShaderFlags1IsSet(uint shaderFlags, QString optionName);

    bool bsShaderFlags1IsSet(QModelIndex iShader, QString optionName);

    bool bsShaderFlags2IsSet(uint shaderFlags, QString optionName);

    bool bsShaderFlags2IsSet(QModelIndex iShader, QString optionName);

    void bsShaderFlagsSet(
            uint shaderFlagsSrc,
            uint & flagsDst,
            const QString & optionNameDst,
            const QString & optionNameSrc,
            const QString & enumNameDst,
            const QString & enumNameSrc);

    void bsShaderFlagsAdd(uint & flagsDst, const QString & optionNameDst, const QString & enumNameDst);

    void bsShaderFlags1Add(uint & flagsDst, const QString & optionNameDst);

    void bsShaderFlagsAdd(QModelIndex iShaderFlags, const QString & enumName, const QString & option);

    void bsShaderFlagsAdd(QModelIndex iDst, const QString & name, const QString & enumName, const QString & option);

    void bsShaderFlags1Add(QModelIndex iDst, const QString & option);

    void bsShaderFlags2Add(QModelIndex iDst, const QString & option);

    void bsShaderFlags2Add(uint & flagsDst, const QString & optionNameDst);

    void bsShaderFlags1Set(uint shaderFlagsSrc, uint & flagsDst, const QString & nameDst, const QString & nameSrc);

    void bsShaderFlags2Set(uint shaderFlagsSrc, uint & flagsDst, const QString & nameDst, const QString & nameSrc);

    void bsShaderFlags1Set(uint shaderFlagsSrc, uint & flagsDst, const QString & name);

    void bsShaderFlags2Set(uint shaderFlagsSrc, uint & flagsDst, const QString & name);

    QString bsShaderTypeGet(QModelIndex iDst);

    void bsShaderTypeSet(QModelIndex iDst, const QString & name);

    // NOTE: Finalized in Properties()
    // TODO: Set emit flag when appropriate. Creation kit currently notifies sometimes when not set.
    void bsShaderFlags(QModelIndex iShaderPropertyDst, uint shaderFlags1Src, uint shaderFlags2Src);

    // TODO: Use enumOption
    int getFlagsBSShaderFlags1(QModelIndex iDst, QModelIndex iNiTriStripsData, QModelIndex iBSShaderPPLightingProperty);

    QModelIndex getShaderProperty(QModelIndex iSrc);

    QModelIndex getShaderPropertySrc(QModelIndex iSrc);

    void properties(QModelIndex iSrc, QModelIndex iShaderPropertyDst, QModelIndex iDst);

    void niAlphaPropertyFinalize(QModelIndex iAlphaPropertyDst, QModelIndex iShaderPropertyDst);

    void properties(QModelIndex iSrc, QModelIndex shaderProperty, QModelIndex iDst, Copier & c);

    void properties(QModelIndex iDst, QModelIndex iSrc);

    void properties(QModelIndex iDst, QModelIndex iSrc, Copier & c);

    BSVertexDesc bsVectorFlags(QModelIndex iDst, QModelIndex iSrc);

    bool bsVectorFlagSet(QModelIndex iSrc, const QString & flagName);

    template<typename T> QVector<T> niTriDataGetArray(
            QModelIndex iSrc,
            Copier & c,
            const QString & boolName,
            const QString & name,
            bool isFlag = false) {
        bool isSet = false;

        if (!isFlag) {
            c.processed(boolName);

            isSet = nifSrc->get<bool>(iSrc, boolName);
        } else {
            // Use BS Vector Flags
            isSet = bsVectorFlagSet(iSrc, boolName);
        }

        if (isSet) {
            QModelIndex iArraySrc = getIndexSrc(iSrc, name);

            if (iArraySrc.isValid()) {
                c.processed(name);

                return nifSrc->getArray<T>(iSrc, name);
            }

            qDebug() << __FUNCTION__ << "Array" << name << "not found";
        }

        return QVector<T>();
    }

    QVector<Vector2> niTriDataGetUVSets(QModelIndex iSrc, Copier & c);

    /*******************************************************************************************************************
     * LOD Conversion functions
     ******************************************************************************************************************/

    void lodLandscapeTranslationZero(QModelIndex iTranslation);

    void lODLandscapeShape(
            const qint32 link,
            const QModelIndex iLink,
            const QModelIndex iLinkBlock,
            QModelIndex & iWater);

    void lODLandscape();

    float lodLandscapeMinVert(QList<QModelIndex> shapeList);

    void lodLandscapeMinVert(QModelIndex iShape, float & min);

    bool lodLandscapeIsEdgeCoord(float f);

    void lodLandscapeWaterRemoveEdgeFaces(QModelIndex iWater, float min);

    bool removeVertex(QModelIndex iVertices, QModelIndex iTriangles, int index);

    void vertRemoveListAppend(HalfVector3 v, int vertIndex, float min, QList<int> & vertRemoveList);

    bool isSameEdgeCoord(float f1, float f2, float f3);

    bool isEdgeVert(HalfVector3 v);

    bool isLowVert(HalfVector3 v, float min);

    /**
     * @brief removeTri
     * Return true if any of the given vertices is as low as the lowest vertex in a LOD water branch and
     * if all vertices have the same x or the same y coordinates and are at the edge of the LOD.
     * @param v1
     * @param v2
     * @param v3
     * @param min
     * @return
     */
    bool removeTri(HalfVector3 v1, HalfVector3 v2, HalfVector3 v3, float min);

    void lodLandscapeWaterMultiBound(QModelIndex iWater);

    QModelIndex lodLandscapeWater(QModelIndex iWater);

    void lodLandscapeWaterShader(QModelIndex iWater);

    void lodLandscapeMultiBound(QModelIndex iRoot);

    // TODO: Combine High level LODs
    void lODObjects();

    /*******************************************************************************************************************
     * Conversion utility functions
     ******************************************************************************************************************/

    QModelIndex getHandled(const QModelIndex iSrc);

    QModelIndex getHandled(const int blockNumber);

    bool isHandled(const int blockNumber);

    bool isHandled(const QModelIndex iSrc);

    bool setHandled(const QModelIndex iDst,  const QModelIndex iSrc);

    void ignoreBlock(const QModelIndex iSrc, bool ignoreChildBlocks);

    void ignoreBlock(const QModelIndex iSrc, const QString & name, bool ignoreChildBlocks);

    // Based on spCopyBlock from blocks.cpp
    QModelIndex copyBlock(const QModelIndex & iDst, const QModelIndex & iSrc, int row = -1, bool bMap = true);

    std::tuple<QModelIndex, QModelIndex> copyLink(QModelIndex iDst, QModelIndex iSrc, const QString & name = "");

    void reLinkExec();

    void reLink(QModelIndex iDst, QModelIndex iSrc);

    void reLink(QModelIndex iDst, QModelIndex iSrc, const QString & name);

    void reLinkArray(
            QModelIndex iArrayDst,
            QModelIndex iArraySrc,
            const QString arrayName = "",
            const QString & name = "");

    void reLinkRec(const QModelIndex iDst);

    uint enumOptionValue(const QString & enumName, const QString & optionName);

    QModelIndex insertNiBlock(const QString & name);

    QModelIndex getIndexSrc(QModelIndex parent, const QString & name);

    QModelIndex getIndexDst(QModelIndex parent, const QString & name);

    QModelIndex getBlockSrc(QModelIndex iLink);

    QModelIndex getBlockSrc(QModelIndex iSrc, const QString & name);

    QModelIndex getBlockDst(QModelIndex iLink);

    QModelIndex getBlockDst(QModelIndex iDst, const QString & name);

    bool setLink(QModelIndex iDst, QModelIndex iTarget);

    bool setLink(QModelIndex iDst, const QString & name, QModelIndex iTarget);

    bool checkHalfFloat(float f);

    bool checkHalfVector3(HalfVector3 v);

    bool checkHalfVector2(HalfVector2 v);

    float hfloatScale(float hf, float scale);

    void unhandledBlocks();

    /*******************************************************************************************************************
     * Conversion finalization funtions
     ******************************************************************************************************************/

    // TODO: Function needs to take entire controller chain into account.
    void niInterpolatorFinalizeEmissive(QModelIndex iController, QModelIndex iInterpolator);

    void niControllerSequencesFinalize();

    QModelIndex niInterpolatorMultToColor(QModelIndex iInterpolatorMult, Vector3 colorVector);

    void niMaterialEmittanceFinalize(const QModelIndex iBlock);

    enum RowToCopy {
        Before,
        After,
    };

    QModelIndex interpolatorDataInsertKey(QModelIndex iKeys, QModelIndex iNumKeys, int row, RowToCopy rowToCopy);

    void niControlledBlocksFinalize();

    /*******************************************************************************************************************
     * Initialization
     ******************************************************************************************************************/

    void loadMatMap();
    void loadLayerMap();

    /*******************************************************************************************************************
     * Copy Block functions from blocks.cpp
     ******************************************************************************************************************/

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

    const char * MIME_SEP = "˂"; // This is Unicode U+02C2
    const char * STR_BL = "nifskope˂niblock˂%1˂%2";

    //! Set strings array
    void setStringsArray(
            const QModelIndex & parentDst,
            const QModelIndex & parentSrc,
            const QString & arr,
            const QString & name = {});

    //! Set "Name" et al. for NiObjectNET, NiExtraData, NiPSysModifier, etc.
    void setNiObjectRootStrings( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc);

    //! Set strings for NiMesh
    void setStringsNiMesh( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc );

    void  setStringsNiSequence( const QModelIndex & iBlockDst, const QModelIndex & iBlockSrc );

    QPair<QString, QString> acceptFormat( const QString & format, const NifModel * nif );
};

void convertNif(QString fname);

#endif // SPELL_CONVERT_H
