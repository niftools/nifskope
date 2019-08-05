/*
 * TODO:
 * Remove vertical faces of LOD Water at LOD edges as these faces can be seen through other LOD Waters.
 *
 * TODO:
 * Fix Camera movement affecting LOD lighting which is possibly due to second LOD textures
 *
 * TODO:
 * Change LOD Water shader to non transparent.
 *
 * TODO:
 * Check if UV Transform controllers transform in the right direction
 *
 * TODO:
 * Set Effect Lighting on always except for shadernolightingproperties
 *
 * TODO:
 * Mutex when loading file
 *
 * TODO:
 * Mutex when saving file?
 *
 * TODO:
 * Create array of thread ids and make the index the bar index
 */

#include "convert.h"
#include "../stripify.h"
#include "../blocks.h"
#include "half.h"
#include "copier.h"
#include "progress.h"

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
#include <QThread>
#include <QtConcurrent>

#include <algorithm> // std::stable_sort

#include <time.h>

// UI
#include "ui/conversiondialog.h"
#include <QProgressDialog>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QLabel>

static QMutex mu;

class FileProperties
{
public:
    FileProperties(FileType fileType, QString fnameDst, QString fnameSrc, int lodLevel, int lodXCoord, int lodYCoord)
        : fileType(fileType),
          fnameDst(fnameDst),
          fnameSrc(fnameSrc),
          lodLevel(lodLevel),
          lodXCoord(lodXCoord),
          lodYCoord(lodYCoord) {}

    FileProperties(FileType fileType, QString fnameDst, QString fnameSrc) : fileType(fileType), fnameDst(fnameDst), fnameSrc(fnameSrc) {}

    FileProperties() : fileType(FileType::Invalid) {}

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

class EnumMap : public QMap<QString, QString>
{
    const QString enumTypeSrc;
    const QString enumTypeDst;
    const NifModel * nifSrc;
public:
    EnumMap(QString enumTypeSrc, QString enumTypeDst, NifModel * nifSrc) : enumTypeSrc(enumTypeSrc), enumTypeDst(enumTypeDst), nifSrc(nifSrc) {}

    void check() {
        bool ok;

        for (QString key : keys()) {
            NifValue::enumOptionValue(enumTypeSrc, key, &ok);
            if (!ok) {
                qCritical() << "Enum option \"" << key << "\" not found in" << enumTypeSrc;

                exit(EXIT_FAILURE);
            }
        }

        for (QString val : values()) {
            NifValue::enumOptionValue(enumTypeDst, val, &ok);
            if (!ok) {
                qCritical() << "Enum option \"" << val << "\" not found in" << enumTypeDst;

                exit(EXIT_FAILURE);
            }
        }
    }

    uint convert(NifValue option) {
        if (!option.isValid()) {
            qDebug() << __FILE__ << __LINE__ << "Invalid NifValue";

            return 0;
        }

        QString nameSrc = option.enumOptionName(enumTypeSrc, option.get<uint>());
        bool ok = false;

        quint32 val = NifValue::enumOptionValue(enumTypeDst, this->operator[](nameSrc), &ok);

        if (!ok) {
            qDebug() << __FILE__ << __LINE__ << "Enum option \"" + this->operator[](nameSrc) + "\" not found";
        }

        return val;
    }

    uint convert(QModelIndex iSrc) {
        return convert(nifSrc->getValue(iSrc));
    }
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
    Controller(int origin, const QString & name) : origin(origin), name(name) {}

    void add(int blockNumber, const QString & blockName) {
        if (clones.contains(blockNumber) || blockNumber == origin) {
            qDebug() << __FUNCTION__ << "Invalid blocknumber";

            exit(1);
        }

        clones.append(blockNumber);
        cloneBlockNames.append(blockName);
    }

    int getOrigin() const;
    QList<int> getClones() const;
    QList<QString> getCloneBlockNames() const;
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
    Converter(NifModel * nifSrc, NifModel * nifDst, FileProperties fileProps, ProgressReceiver & receiver)
            : nifSrc(nifSrc),
              nifDst(nifDst),
              progress(Progress(uint(nifSrc->getBlockCount()), receiver, fileProps.getFnameSrc())),
              fileProps(fileProps) {
        handledBlocks = QVector<bool>(nifSrc->getBlockCount(), true);

        loadMatMap();
        loadLayerMap();
    }

    void convert() {
        // TODO: Check for linking issues. AdjustLinks() is called by insertNiBlock.
        nifDst->setState(NifModel::Loading);

        bsFadeNode(nifSrc->getBlock(0));
        reLinkExec();
        niControllerSequencesFinalize();
        niControlledBlocksFinalize();
        lODLandscape();
        lODObjects();
        unhandledBlocks();
    }

    bool getConversionResult() const;
    bool getBLODLandscape() const;
    bool getBLODBuilding() const;

private:
    void loadMatMap() {
        matMap.insert("FO_HAV_MAT_STONE",                                "FO4_HAV_MAT_STONE");
        matMap.insert("FO_HAV_MAT_CLOTH",                                "FO4_HAV_MAT_CLOTH");
        matMap.insert("FO_HAV_MAT_DIRT",                                 "FO4_HAV_MAT_DIRT");
        matMap.insert("FO_HAV_MAT_GLASS",                                "FO4_HAV_MAT_GLASS");
        matMap.insert("FO_HAV_MAT_GRASS",                                "FO4_HAV_MAT_GRASS");
        matMap.insert("FO_HAV_MAT_METAL",                                "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_ORGANIC",                              "FO4_HAV_MAT_ORGANIC");
        matMap.insert("FO_HAV_MAT_SKIN",                                 "FO4_HAV_MAT_SKIN");
        matMap.insert("FO_HAV_MAT_WATER",                                "FO4_HAV_MAT_WATER");
        matMap.insert("FO_HAV_MAT_WOOD",                                 "FO4_HAV_MAT_WOOD");
        matMap.insert("FO_HAV_MAT_HEAVY_STONE",                          "FO4_HAV_MAT_STONE_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_METAL",                          "FO4_HAV_MAT_METAL_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_WOOD",                           "FO4_HAV_MAT_WOOD_HEAVY");
        matMap.insert("FO_HAV_MAT_CHAIN",                                "FO4_HAV_MAT_CHAIN");
        matMap.insert("FO_HAV_MAT_BOTTLECAP",                            "FO4_HAV_MAT_COIN");
        // Unknown
        matMap.insert("FO_HAV_MAT_ELEVATOR",                             "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_HOLLOW_METAL",                         "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_SHEET_METAL",                          "FO4_HAV_MAT_METAL_LIGHT");
        matMap.insert("FO_HAV_MAT_SAND",                                 "FO4_HAV_MAT_SAND");
        matMap.insert("FO_HAV_MAT_BROKEN_CONCRETE",                      "FO4_HAV_MAT_CONCRETE");
        matMap.insert("FO_HAV_MAT_VEHICLE_BODY",                         "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_SOLID",                   "FO4_HAV_MAT_METAL_SOLID");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_HOLLOW",                  "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_BARREL",                               "FO4_HAV_MAT_METAL_BARREL");
        matMap.insert("FO_HAV_MAT_BOTTLE",                               "FO4_HAV_MAT_BOTTLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SODA_CAN",                             "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_PISTOL",                               "FO4_HAV_MAT_WEAPON_PISTOL");
        matMap.insert("FO_HAV_MAT_RIFLE",                                "FO4_HAV_MAT_WEAPON_RIFLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SHOPPING_CART",                        "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_LUNCHBOX",                             "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_BABY_RATTLE",                          "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_RUBBER_BALL",                          "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_STONE_PLATFORM",                       "FO4_HAV_MAT_STONE");
        matMap.insert("FO_HAV_MAT_CLOTH_PLATFORM",                       "FO4_HAV_MAT_CLOTH");
        matMap.insert("FO_HAV_MAT_DIRT_PLATFORM",                        "FO4_HAV_MAT_DIRT");
        matMap.insert("FO_HAV_MAT_GLASS_PLATFORM",                       "FO4_HAV_MAT_GLASS");
        matMap.insert("FO_HAV_MAT_GRASS_PLATFORM",                       "FO4_HAV_MAT_GRASS");
        matMap.insert("FO_HAV_MAT_METAL_PLATFORM",                       "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_ORGANIC_PLATFORM",                     "FO4_HAV_MAT_ORGANIC");
        matMap.insert("FO_HAV_MAT_SKIN_PLATFORM",                        "FO4_HAV_MAT_SKIN");
        matMap.insert("FO_HAV_MAT_WATER_PLATFORM",                       "FO4_HAV_MAT_WATER");
        matMap.insert("FO_HAV_MAT_WOOD_PLATFORM",                        "FO4_HAV_MAT_WOOD");
        matMap.insert("FO_HAV_MAT_HEAVY_STONE_PLATFORM",                 "FO4_HAV_MAT_STONE_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_METAL_PLATFORM",                 "FO4_HAV_MAT_METAL_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_WOOD_PLATFORM",                  "FO4_HAV_MAT_WOOD_HEAVY");
        matMap.insert("FO_HAV_MAT_CHAIN_PLATFORM",                       "FO4_HAV_MAT_CHAIN");
        matMap.insert("FO_HAV_MAT_BOTTLECAP_PLATFORM",                   "FO4_HAV_MAT_COIN");
        // Unknown
        matMap.insert("FO_HAV_MAT_ELEVATOR_PLATFORM",                    "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_HOLLOW_METAL_PLATFORM",                "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_SHEET_METAL_PLATFORM",                 "FO4_HAV_MAT_METAL_LIGHT");
        matMap.insert("FO_HAV_MAT_SAND_PLATFORM",                        "FO4_HAV_MAT_SAND");
        matMap.insert("FO_HAV_MAT_BROKEN_CONCRETE_PLATFORM",             "FO4_HAV_MAT_CONCRETE");
        matMap.insert("FO_HAV_MAT_VEHICLE_BODY_PLATFORM",                "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_SOLID_PLATFORM",          "FO4_HAV_MAT_METAL_SOLID");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_HOLLOW_PLATFORM",         "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_BARREL_PLATFORM",                      "FO4_HAV_MAT_METAL_BARREL");
        matMap.insert("FO_HAV_MAT_BOTTLE_PLATFORM",                      "FO4_HAV_MAT_BOTTLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SODA_CAN_PLATFORM",                    "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_PISTOL_PLATFORM",                      "FO4_HAV_MAT_WEAPON_PISTOL");
        matMap.insert("FO_HAV_MAT_RIFLE_PLATFORM",                       "FO4_HAV_MAT_WEAPON_RIFLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SHOPPING_CART_PLATFORM",               "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_LUNCHBOX_PLATFORM",                    "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_BABY_RATTLE_PLATFORM",                 "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_RUBBER_BALL_PLATFORM",                 "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_STONE_STAIRS",                         "FO4_HAV_MAT_STONE_STAIRS");
        matMap.insert("FO_HAV_MAT_CLOTH_STAIRS",                         "FO4_HAV_MAT_CLOTH");
        matMap.insert("FO_HAV_MAT_DIRT_STAIRS",                          "FO4_HAV_MAT_DIRT_STAIRS");
        matMap.insert("FO_HAV_MAT_GLASS_STAIRS",                         "FO4_HAV_MAT_GLASS_STAIRS");
        matMap.insert("FO_HAV_MAT_GRASS_STAIRS",                         "FO4_HAV_MAT_GRASS_STAIRS");
        matMap.insert("FO_HAV_MAT_METAL_STAIRS",                         "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_ORGANIC_STAIRS",                       "FO4_HAV_MAT_ORGANIC");
        matMap.insert("FO_HAV_MAT_SKIN_STAIRS",                          "FO4_HAV_MAT_SKIN");
        matMap.insert("FO_HAV_MAT_WATER_STAIRS",                         "FO4_HAV_MAT_WATER");
        matMap.insert("FO_HAV_MAT_WOOD_STAIRS",                          "FO4_HAV_MAT_WOOD_STAIRS");
        matMap.insert("FO_HAV_MAT_HEAVY_STONE_STAIRS",                   "FO4_HAV_MAT_STONE_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_METAL_STAIRS",                   "FO4_HAV_MAT_METAL_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_WOOD_STAIRS",                    "FO4_HAV_MAT_WOOD_HEAVY");
        matMap.insert("FO_HAV_MAT_CHAIN_STAIRS",                         "FO4_HAV_MAT_CHAIN");
        matMap.insert("FO_HAV_MAT_BOTTLECAP_STAIRS",                     "FO4_HAV_MAT_COIN");
        // Unknown
        matMap.insert("FO_HAV_MAT_ELEVATOR_STAIRS",                      "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_HOLLOW_METAL_STAIRS",                  "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_SHEET_METAL_STAIRS",                   "FO4_HAV_MAT_METAL_LIGHT");
        matMap.insert("FO_HAV_MAT_SAND_STAIRS",                          "FO4_HAV_MAT_SAND");
        matMap.insert("FO_HAV_MAT_BROKEN_CONCRETE_STAIRS",               "FO4_HAV_MAT_CONCRETE");
        matMap.insert("FO_HAV_MAT_VEHICLE_BODY_STAIRS",                  "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_SOLID_STAIRS",            "FO4_HAV_MAT_METAL_SOLID");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_HOLLOW_STAIRS",           "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_BARREL_STAIRS",                        "FO4_HAV_MAT_METAL_BARREL");
        matMap.insert("FO_HAV_MAT_BOTTLE_STAIRS",                        "FO4_HAV_MAT_BOTTLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SODA_CAN_STAIRS",                      "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_PISTOL_STAIRS",                        "FO4_HAV_MAT_WEAPON_PISTOL");
        matMap.insert("FO_HAV_MAT_RIFLE_STAIRS",                         "FO4_HAV_MAT_WEAPON_RIFLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SHOPPING_CART_STAIRS",                 "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_LUNCHBOX_STAIRS",                      "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_BABY_RATTLE_STAIRS",                   "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_RUBBER_BALL_STAIRS",                   "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_STONE_STAIRS_PLATFORM",                "FO4_HAV_MAT_STONE_STAIRS");
        matMap.insert("FO_HAV_MAT_CLOTH_STAIRS_PLATFORM",                "FO4_HAV_MAT_CLOTH");
        matMap.insert("FO_HAV_MAT_DIRT_STAIRS_PLATFORM",                 "FO4_HAV_MAT_DIRT_STAIRS");
        matMap.insert("FO_HAV_MAT_GLASS_STAIRS_PLATFORM",                "FO4_HAV_MAT_GLASS_STAIRS");
        matMap.insert("FO_HAV_MAT_GRASS_STAIRS_PLATFORM",                "FO4_HAV_MAT_GRASS_STAIRS");
        matMap.insert("FO_HAV_MAT_METAL_STAIRS_PLATFORM",                "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_ORGANIC_STAIRS_PLATFORM",              "FO4_HAV_MAT_ORGANIC");
        matMap.insert("FO_HAV_MAT_SKIN_STAIRS_PLATFORM",                 "FO4_HAV_MAT_SKIN");
        matMap.insert("FO_HAV_MAT_WATER_STAIRS_PLATFORM",                "FO4_HAV_MAT_WATER");
        matMap.insert("FO_HAV_MAT_WOOD_STAIRS_PLATFORM",                 "FO4_HAV_MAT_WOOD_STAIRS");
        matMap.insert("FO_HAV_MAT_HEAVY_STONE_STAIRS_PLATFORM",          "FO4_HAV_MAT_STONE_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_METAL_STAIRS_PLATFORM",          "FO4_HAV_MAT_METAL_HEAVY");
        matMap.insert("FO_HAV_MAT_HEAVY_WOOD_STAIRS_PLATFORM",           "FO4_HAV_MAT_WOOD_STAIRS");
        matMap.insert("FO_HAV_MAT_CHAIN_STAIRS_PLATFORM",                "FO4_HAV_MAT_CHAIN");
        // Unknown
        matMap.insert("FO_HAV_MAT_BOTTLECAP_STAIRS_PLATFORM",            "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_ELEVATOR_STAIRS_PLATFORM",             "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_HOLLOW_METAL_STAIRS_PLATFORM",         "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_SHEET_METAL_STAIRS_PLATFORM",          "FO4_HAV_MAT_METAL_LIGHT");
        matMap.insert("FO_HAV_MAT_SAND_STAIRS_PLATFORM",                 "FO4_HAV_MAT_SAND");
        matMap.insert("FO_HAV_MAT_BROKEN_CONCRETE_STAIRS_PLATFORM",      "FO4_HAV_MAT_CONCRETE");
        matMap.insert("FO_HAV_MAT_VEHICLE_BODY_STAIRS_PLATFORM",         "FO4_HAV_MAT_METAL");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_SOLID_STAIRS_PLATFORM",   "FO4_HAV_MAT_METAL_SOLID");
        matMap.insert("FO_HAV_MAT_VEHICLE_PART_HOLLOW_STAIRS_PLATFORM",  "FO4_HAV_MAT_METAL_HOLLOW");
        matMap.insert("FO_HAV_MAT_BARREL_STAIRS_PLATFORM",               "FO4_HAV_MAT_METAL_BARREL");
        matMap.insert("FO_HAV_MAT_BOTTLE_STAIRS_PLATFORM",               "FO4_HAV_MAT_BOTTLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SODA_CAN_STAIRS_PLATFORM",             "FO4_HAV_MAT_GENERIC");
        matMap.insert("FO_HAV_MAT_PISTOL_STAIRS_PLATFORM",               "FO4_HAV_MAT_WEAPON_PISTOL");
        matMap.insert("FO_HAV_MAT_RIFLE_STAIRS_PLATFORM",                "FO4_HAV_MAT_WEAPON_RIFLE");
        // Unknown
        matMap.insert("FO_HAV_MAT_SHOPPING_CART_STAIRS_PLATFORM",        "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_LUNCHBOX_STAIRS_PLATFORM",             "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_BABY_RATTLE_STAIRS_PLATFORM",          "FO4_HAV_MAT_GENERIC");
        // Unknown
        matMap.insert("FO_HAV_MAT_RUBBER_BALL_STAIRS_PLATFORM",          "FO4_HAV_MAT_GENERIC");

        matMap.check();
    }

    void loadLayerMap() {
        layerMap.insert("FOL_UNIDENTIFIED",           "FO4L_UNIDENTIFIED");
        layerMap.insert("FOL_STATIC",                 "FO4L_STATIC");
        layerMap.insert("FOL_ANIM_STATIC",            "FO4L_ANIMSTATIC");
        layerMap.insert("FOL_TRANSPARENT",            "FO4L_TRANSPARENT");
        layerMap.insert("FOL_CLUTTER",                "FO4L_CLUTTER");
        layerMap.insert("FOL_WEAPON",                 "FO4L_WEAPON");
        layerMap.insert("FOL_PROJECTILE",             "FO4L_PROJECTILE");
        layerMap.insert("FOL_SPELL",                  "FO4L_SPELL");
        layerMap.insert("FOL_BIPED",                  "FO4L_BIPED");
        layerMap.insert("FOL_TREES",                  "FO4L_TREE");
        layerMap.insert("FOL_PROPS",                  "FO4L_PROP");
        layerMap.insert("FOL_WATER",                  "FO4L_WATER");
        layerMap.insert("FOL_TRIGGER",                "FO4L_TRIGGER");
        layerMap.insert("FOL_TERRAIN",                "FO4L_TERRAIN");
        layerMap.insert("FOL_TRAP",                   "FO4L_TRAP");
        layerMap.insert("FOL_NONCOLLIDABLE",          "FO4L_NONCOLLIDABLE");
        layerMap.insert("FOL_CLOUD_TRAP",             "FO4L_CLOUD_TRAP");
        layerMap.insert("FOL_GROUND",                 "FO4L_GROUND");
        layerMap.insert("FOL_PORTAL",                 "FO4L_PORTAL");
        layerMap.insert("FOL_DEBRIS_SMALL",           "FO4L_DEBRIS_SMALL");
        layerMap.insert("FOL_DEBRIS_LARGE",           "FO4L_DEBRIS_LARGE");
        layerMap.insert("FOL_ACOUSTIC_SPACE",         "FO4L_ACOUSTIC_SPACE");
        layerMap.insert("FOL_ACTORZONE",              "FO4L_ACTORZONE");
        layerMap.insert("FOL_PROJECTILEZONE",         "FO4L_PROJECTILEZONE");
        layerMap.insert("FOL_GASTRAP",                "FO4L_GASTRAP");
        layerMap.insert("FOL_SHELLCASING",            "FO4L_SHELLCASING");
        layerMap.insert("FOL_TRANSPARENT_SMALL",      "FO4L_TRANSPARENT_SMALL");
        layerMap.insert("FOL_INVISIBLE_WALL",         "FO4L_INVISIBLE_WALL");
        layerMap.insert("FOL_TRANSPARENT_SMALL_ANIM", "FO4L_TRANSPARENT_SMALL_ANIM");
        // Unknown
        layerMap.insert("FOL_DEADBIP",                "FO4L_UNIDENTIFIED");
        layerMap.insert("FOL_CHARCONTROLLER",         "FO4L_CHARACTER_CONTROLLER");
        // Unknown
        layerMap.insert("FOL_AVOIDBOX",               "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_COLLISIONBOX",           "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_CAMERASPHERE",           "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_DOORDETECTION",          "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_CAMERAPICK",             "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_ITEMPICK",               "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_LINEOFSIGHT",            "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_PATHPICK",               "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_CUSTOMPICK1",            "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_CUSTOMPICK2",            "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_SPELLEXPLOSION",         "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_DROPPINGPICK",           "FO4L_UNIDENTIFIED");
        // Unknown
        layerMap.insert("FOL_NULL",                   "FO4L_UNIDENTIFIED");

        layerMap.check();
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
        QModelIndex iControlledBlocksSrc = nifSrc->getIndex( iBlockSrc, "Controlled Blocks" );
        QModelIndex iControlledBlocksDst = nifDst->getIndex( iBlockDst, "Controlled Blocks" );

        for ( int i = 0; i < nifDst->rowCount( iControlledBlocksDst ); i++ ) {
            QModelIndex iChildDst = iControlledBlocksDst.child( i, 0 );
            QModelIndex iChildSrc = iControlledBlocksSrc.child( i, 0 );

            Copier c = Copier(iChildDst, iChildSrc, nifDst, nifSrc);

            if (nifSrc->getIndex(iChildSrc, "Target Name").isValid()) {
                c.copyValue<QString>("Target Name");
            }

            c.copyValue<QString>("Node Name");
            c.copyValue<QString>("Property Type");
            c.copyValue<QString>("Controller Type");
            c.copyValue<QString>("Controller ID");
            c.copyValue<QString>("Interpolator ID");

            // Handled in niControllerSequence()
            c.ignore("Interpolator");
            c.ignore("Controller");
            c.ignore("Priority");

            c.printUnused();
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
    QModelIndex copyBlock(const QModelIndex & iDst, const QModelIndex & iSrc, int row = -1, bool bMap = true) {
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
                QModelIndex block = nifDst->insertNiBlock( bType, row );
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

                if (bMap) {
                    setHandled(block, iSrc);
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

    QModelIndex getHandled(const QModelIndex iSrc) {
        return getHandled(nifSrc->getBlockNumber(iSrc));
    }

    QModelIndex getHandled(const int blockNumber) {
        if (isHandled(blockNumber)) {
            return nifDst->getBlock(indexMap[blockNumber]);
        }

        return QModelIndex();
    }

    bool isHandled(const int blockNumber) {
        return !handledBlocks[blockNumber];
    }

    bool isHandled(const QModelIndex iSrc) {
        return isHandled(nifSrc->getBlockNumber(iSrc));
    }

    bool setHandled(const QModelIndex iDst,  const QModelIndex iSrc) {
        if (handledBlocks[nifSrc->getBlockNumber(iSrc)] == false) {
            return false;
        }

        handledBlocks[nifSrc->getBlockNumber(iSrc)] = false;
        indexMap[nifSrc->getBlockNumber(iSrc)] = nifDst->getBlockNumber(iDst);
        progress++;

        return true;
    }

    void ignoreBlock(const QModelIndex iSrc, bool ignoreChildBlocks) {
        int lSrc = nifSrc->getBlockNumber(iSrc);

        if (lSrc == -1) {
            return;
        }

        if (!handledBlocks[lSrc] && indexMap[lSrc] != -1) {
            // Block already handled
            return;
        }

        handledBlocks[lSrc] = false;
        indexMap[lSrc] = -1;

        for (int i = linkList.count() - 1; i >= 0; i--) {
            if (std::get<0>(linkList[i]) == lSrc) {
                // Remove the current link
                nifDst->setLink(std::get<1>(linkList[i]), -1);

                linkList.remove(i);
            }
        }

        if (ignoreChildBlocks) {
            for (int link : nifSrc->getChildLinks(lSrc)) {
                ignoreBlock(nifSrc->getBlock(link), true);
            }
        }

        progress++;
    }

    void ignoreBlock(const QModelIndex iSrc, const QString & name, bool ignoreChildBlocks) {
        ignoreBlock(nifSrc->getBlock(nifSrc->getLink(iSrc, name)), ignoreChildBlocks);
    }

    void niPSysData(const QModelIndex iDst, const QModelIndex iSrc) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

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
        c.ignore("Additional Data");
        ignoreBlock(iSrc, "Additional Data", false);
        c.copyValue<bool>("Has Radii");
        c.copyValue<ushort>("Num Active");
        c.copyValue<bool>("Has Sizes");
        c.copyValue<bool>("Has Rotations");
        c.copyValue<bool>("Has Rotation Angles");
        c.copyValue<bool>("Has Rotation Axes");
        c.copyValue<bool>("Has Texture Indices");
        c.copyValue<int>("Num Subtexture Offsets");
        c.array("Subtexture Offsets");
        c.copyValue<bool>("Has Rotation Speeds");

        c.printUnused();
    }

    std::tuple<QModelIndex, QModelIndex> copyLink(QModelIndex iDst, QModelIndex iSrc, const QString & name = "") {
        int lSrc;

        if (name.length() > 0) {
            lSrc = nifSrc->getLink(iSrc, name);
        } else {
            lSrc = nifSrc->getLink(iSrc);
        }

        if (lSrc != -1) {
            QModelIndex iBlockDst;
            const QModelIndex iBlockSrc = nifSrc->getBlock(lSrc);

            const QString type = nifSrc->getBlockName(iBlockSrc);

            if (type == "NiPSysData") {
                iBlockDst = nifDst->insertNiBlock(type);
                niPSysData(iBlockDst, iBlockSrc);
            } else {
                iBlockDst = copyBlock(QModelIndex(), iBlockSrc);
            }

            if (name.length() > 0) {
                if (!nifDst->setLink(iDst, name, nifDst->getBlockNumber(iBlockDst))) {
                    qDebug() << __FUNCTION__ << "Failed to set link";

                    conversionResult = false;
                }
            } else {
                if (!nifDst->setLink(iDst, nifDst->getBlockNumber(iBlockDst))) {
                    qDebug() << __FUNCTION__ << "Failed to set link for" << nifDst->getBlockName(iBlockDst);

                    conversionResult = false;
                }
            }

            setHandled(iBlockDst, iBlockSrc);

            return {iBlockDst, iBlockSrc};
        }

        return {QModelIndex(), QModelIndex()};
    }

    void reLinkExec() {
        QModelIndex iDst;
        int lSrc;

        for (int i = 0; i < linkList.count(); i++) {
            std::tie(lSrc, iDst) = linkList[i];

            if (indexMap.count(lSrc) > 0) {
                int lDst = indexMap[lSrc];

                nifDst->setLink(iDst, lDst);
            } else {
                qDebug() << "Relinking: Link" << lSrc << "not found";
                nifDst->setLink(iDst, -1);

                conversionResult = false;
            }
        }
    }

    void reLink(QModelIndex iDst, QModelIndex iSrc) {
        if (!iDst.isValid()) {
            qDebug() << __FUNCTION__ << "Invalid iDst";

            conversionResult = false;
        }

        if (!iSrc.isValid()) {
            qDebug() << __FUNCTION__ << "Invalid iSrc";

            conversionResult = false;
        }

        int lSrc = nifSrc->getLink(iSrc);
        if (lSrc != -1) {
            linkList.append({lSrc, iDst});
        }
    }

    void reLink(QModelIndex iDst, QModelIndex iSrc, const QString & name) {
        reLink(nifDst->getIndex(iDst, name), nifSrc->getIndex(iSrc, name));
    }

    void reLinkArray(QModelIndex iArrayDst, QModelIndex iArraySrc, const QString arrayName = "", const QString & name = "") {
        if (arrayName != "") {
            iArrayDst = nifDst->getIndex(iArrayDst, arrayName);
            iArraySrc = nifSrc->getIndex(iArraySrc, arrayName);
        }

        nifDst->updateArray(iArrayDst);

        for (int i = 0; i < nifSrc->rowCount(iArraySrc); i++) {
            QModelIndex iLinkDst = iArrayDst.child(i, 0);
            QModelIndex iLinkSrc = iArraySrc.child(i, 0);

            if (name == "") {
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
            QString type = nifSrc->getBlockName(iInterpolatorSrc);

            if (type == "NiPathInterpolator") {
                copyLink(iInterpolatorDst, iInterpolatorSrc, "Path Data");
                copyLink(iInterpolatorDst, iInterpolatorSrc, "Percent Data");
            } else if (type == "NiLookAtInterpolator") {
                reLink(iInterpolatorDst, iInterpolatorSrc, "Look At");

                niInterpolator(iInterpolatorDst, iInterpolatorSrc, "Interpolator: Translation");
                niInterpolator(iInterpolatorDst, iInterpolatorSrc, "Interpolator: Roll");
                niInterpolator(iInterpolatorDst, iInterpolatorSrc, "Interpolator: Scale");
            } else {
                copyLink(iInterpolatorDst, iInterpolatorSrc, "Data");
            }
        }
    }

    QModelIndex niControllerSequence(QModelIndex iSrc) {
        // Ignore the progress because the block is finalized later.
        progress.ignoreNextIncrease();

        QModelIndex iDst = copyBlock(QModelIndex(), iSrc);

        niControllerSequenceList.append(iDst);

        // Controlled Blocks
        QModelIndex iControlledBlocksDst = nifDst->getIndex(iDst, "Controlled Blocks");
        QModelIndex iControlledBlocksSrc = nifSrc->getIndex(iSrc, "Controlled Blocks");

        if (iControlledBlocksSrc.isValid()) {
            for (int i = 0; i < nifDst->rowCount(iControlledBlocksDst); i++) {
                QModelIndex iBlockDst = iControlledBlocksDst.child(i, 0);
                QModelIndex iBlockSrc = iControlledBlocksSrc.child(i, 0);

                Copier c = Copier(iBlockDst, iBlockSrc, nifDst, nifSrc);

                // Interpolator
                niInterpolator(iBlockDst, iBlockSrc);
                c.ignore("Interpolator");

                // Controller
                reLink(iBlockDst, iBlockSrc, "Controller");
                c.ignore("Controller");

                c.copyValue("Priority");

                // Handled in copyBlock() and niControllerSequencesFinalize().
                c.ignore("Node Name");
                c.ignore("Property Type");
                c.ignore("Controller Type");
                c.ignore("Controller ID");
                c.ignore("Interpolator ID");
            }
        }

        // Text keys
        copyLink(iDst, iSrc, "Text Keys");

        // Manager
        reLink(iDst, iSrc, "Manager");

        // TODO: Anim Note Arrays

        return iDst;
    }

    /**
     * @brief niControllerSequences
     * Handle controller sequences from NiControllerManager blocks.
     * @param iDst
     * @param iSrc
     */
    void niControllerSequences(QModelIndex iDst, QModelIndex iSrc) {
        for (int i = 0; i < nifDst->rowCount(iDst); i++) {
            QModelIndex iSeqSrc = nifSrc->getBlock(nifSrc->getLink(iSrc.child(i, 0)));
            QModelIndex iSeqDst = niControllerSequence(iSeqSrc);

            nifDst->setLink(iDst.child(i, 0), nifDst->getBlockNumber(iSeqDst));
        }
    }

    Controller & getController(int blockNumber) {
        for (int i = 0; i < controllerList.count(); i++) {
            if (controllerList[i].getOrigin() == blockNumber) {
                return controllerList[i];
            }
        }

        qDebug() << __FUNCTION__ << "Controller not found" << blockNumber;

        exit(1);
    }

    // TODO: Function needs to take entire controller chain into account.
    void niInterpolatorFinalizeEmissive(QModelIndex iController, QModelIndex iInterpolator) {
        QString controllerType = nifDst->getBlockName(iController);

        if (
                (!(controllerType == "BSEffectShaderPropertyFloatController" &&
                   nifDst->get<uint>(iController, "Type of Controlled Variable") == enumOptionValue("EffectShaderControlledVariable", "EmissiveMultiple")) &&
                 !(controllerType == "BSLightingShaderPropertyFloatController" &&
                   nifDst->get<uint>(iController, "Type of Controlled Variable") == enumOptionValue("LightingShaderControlledVariable", "Emissive Multiple"))) ||
                nifDst->getBlockName(iInterpolator) != "NiFloatInterpolator"
                ) {
            return;
        }

        QModelIndex iInterpolatorData = getBlockDst(iInterpolator, "Data");
        QModelIndex iKeys = getIndexDst(getIndexDst(iInterpolatorData, "Data"), "Keys");

        qDebug() << nifDst->getBlockNumber(iInterpolator);

        for (int i = 0; i < nifDst->rowCount(iKeys); i++) {
            QModelIndex iValue = getIndexDst(iKeys.child(i, 0), "Value");

            nifDst->set<float>(iValue, nifDst->get<float>(iValue) + 1);
        }
    }

    void niControllerSequencesFinalize() {
        for (QModelIndex iDst : niControllerSequenceList) {

            QModelIndex iNumControlledBlocks = nifDst->getIndex(iDst, "Num Controlled Blocks");
            int numControlledBlocks = nifDst->get<int>(iNumControlledBlocks);
            const int numControlledBlocksOriginal = numControlledBlocks;

            // Controlled Blocks
            QModelIndex iControlledBlocksDst = nifDst->getIndex(iDst, "Controlled Blocks");
            if (!iControlledBlocksDst.isValid()) {
                qDebug() << __FUNCTION__ << "Controlled Blocks not found";

                conversionResult = false;

                return;
            }

            for (int i = 0; i < numControlledBlocksOriginal; i++) {
                QModelIndex iBlockDst = iControlledBlocksDst.child(i, 0);

                int controllerNumber = nifDst->getLink(iBlockDst, "Controller");
                QModelIndex iControllerDst = nifDst->getBlock(controllerNumber);
                QModelIndex iPropertyDst = nifDst->getBlock(nifDst->getLink(iControllerDst, "Target"));

                if (nifDst->get<QString>(iBlockDst, "Controller Type") != "") {
                    QString controllerType = nifDst->getBlockName(iControllerDst);

                    if (controllerType == "NiMultiTargetTransformController") {
                        controllerType = "NiTransformController";
                    }

                    nifDst->set<QString>(iBlockDst, "Controller Type", controllerType);
                }

                if (nifDst->get<QString>(iBlockDst, "Property Type") != "") {
                    nifDst->set<QString>(iBlockDst, "Property Type", nifDst->getBlockName(iPropertyDst));
                }

                // NOTE: Should only be true on ignored controllers
                if (controllerNumber != -1) {
                    Controller & controller = getController(controllerNumber);
                    QList<int> clones = controller.getClones();
                    int numClones = clones.count();

                    if (numClones > 0) {
                        int index = numControlledBlocks;
                        numControlledBlocks += numClones;
                        nifDst->set<uint>(iNumControlledBlocks, uint(numControlledBlocks));
                        nifDst->updateArray(iControlledBlocksDst);

                        for (int j = 0; j < numClones; j++, index++) {
                            QModelIndex iBlockClone = iControlledBlocksDst.child(index, 0);

                            Copier c = Copier(iBlockClone, iBlockDst, nifDst, nifDst);

                            nifDst->setLink(iBlockClone, "Controller", clones[j]);
                            c.processed("Controller");

                            c.copyValue("Interpolator");
                            c.copyValue("Priority");

                            // TODO: Set node name to new parent
                            const QString blockName = controller.getCloneBlockNames()[j];

                            if (blockName == "") {
                                qDebug() << __FUNCTION__ << "No block name for controlled block clone with original name:" << nifDst->get<QString>(iBlockDst, "Node Name") << i;

                                conversionResult = false;

                                return;
                            } else {
                                nifDst->set<QString>(iBlockClone, "Node Name", blockName);
                                c.processed("Node Name");
                            }

                            const QString cloneControllerType = nifDst->getBlockName(nifDst->getBlock(clones[j]));
                            QString clonePropertyType = "";

                            if (
                                    cloneControllerType == "BSEffectShaderPropertyFloatController" ||
                                    cloneControllerType == "BSEffectShaderPropertyColorController") {
                                clonePropertyType = "BSEffectShaderProperty";
                            } else if (
                                    cloneControllerType == "BSLightingShaderPropertyFloatController" ||
                                    cloneControllerType == "BSLightingShaderPropertyColorController") {
                                clonePropertyType = "BSLightingShaderProperty";
                            } else {
                                qDebug() << __FUNCTION__ << __LINE__ << "Unknown property type" << clonePropertyType;

                                conversionResult = false;
                            }

                            nifDst->set<QString>(iBlockClone, "Property Type", clonePropertyType);
                            c.processed("Property Type");

                            nifDst->set<QString>(iBlockClone, "Controller Type", cloneControllerType);
                            c.processed("Controller Type");


                            c.copyValue("Controller ID");
                            c.copyValue("Interpolator ID");
                        }
                    }
                }
            }

            progress++;
        }
    }

    QString getControllerType(QString dstType) {
        if (dstType == "BSEffectShaderProperty" ||
                dstType == "BSEffectShaderPropertyColorController" ||
                dstType == "BSEffectShaderPropertyFloatController") {
            return "Effect";
        } else if (dstType == "BSLightingShaderProperty" ||
                dstType == "BSLightingShaderPropertyColorController" ||
                dstType == "BSLightingShaderPropertyFloatController") {
            return "Lighting";
        } else if (dstType == "Controlled Blocks") {
            // In NiControllerSequence blocks.

            // TODO: Confirm
            return "Effect";
        }

        conversionResult = false;

        qDebug() << "Unknown shader property:" << dstType;

        return "";
    }

    std::tuple<QModelIndex, QString> getControllerType(QString dstType, const QString & name) {
        const QString shaderType = getControllerType(dstType);

        if (shaderType == "Effect") {
            return {nifDst->insertNiBlock("BSEffectShaderProperty" + name + "Controller"), "Effect"};
        } else if (shaderType == "Lighting") {
            return {nifDst->insertNiBlock("BSLightingShaderProperty" + name + "Controller"), "Lighting"};
        }

        qDebug() << "Unknown shader property:" << dstType;

        conversionResult = false;

        return {QModelIndex(), ""};
    }


    QModelIndex niController(QModelIndex iDst, QModelIndex iSrc, Copier & c, const QString & name = "Controller", const QString & blockName = "", const int target = -1) {
        c.processed(iSrc, name);

        return niController(iDst, iSrc, name, blockName, target);
    }

    QModelIndex niController(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Controller", const QString & blockName = "", const int target = -1) {
        if (nifSrc->getLink(iSrc, name) != -1 && !controlledBlockList.contains(iDst)) {
            controlledBlockList.append(iDst);
        } else {
            progress++;
        }

        return niControllerCopy(iDst, iSrc, name, blockName, target);
    }

    QModelIndex niControllerSetLink(QModelIndex iDst, const QString & name, QModelIndex iControllerDst) {
        // Set next controller in parent

        QModelIndex iNextControllerLink = nifDst->getIndex(iDst, name);

        // If there is already a controller linked, find a spot for the new controller.

        // The current next controller link.
        int lNextController = nifDst->getLink(iNextControllerLink);

        QModelIndex iNextControllerRoot;
        if (lNextController != -1) {
            iNextControllerRoot = nifDst->getBlock(lNextController);
        } else {
            iNextControllerRoot = iControllerDst;
        }

        QList<int> linkList;

        do {
            QModelIndex iParent = nifDst->getBlock(nifDst->getParent(iNextControllerRoot));

            if (getIndexDst(iParent, "Next Controller").isValid()) {
                iNextControllerRoot = iParent;
            } else {
                break;
            }
        } while (true);

        // Follow the controller chain until an empty spot is found.
        while (lNextController != -1) {
            linkList.append(lNextController);

            // Get next controller block.
            QModelIndex iNextControllerBlock = nifDst->getBlock(lNextController);

            // Get next controller link.
            iNextControllerLink = nifDst->getIndex(iNextControllerBlock, "Next Controller");
            lNextController = nifDst->getLink(iNextControllerLink);

            if (linkList.contains(lNextController)) {
                qDebug() << __FUNCTION__ << "Infinite Recursive link";

                conversionResult = false;

                return iNextControllerRoot;
            }
        }

        // Set the controller link to the newly created controller.
        if (!nifDst->setLink(iNextControllerLink, nifDst->getBlockNumber(iControllerDst))) {
            qDebug() << "Set link failed";

            conversionResult = false;
        }

        return iNextControllerRoot;
    }

    uint enumOptionValue(const QString & enumName, const QString & optionName) {
        bool ok = false;

        uint result = NifValue::enumOptionValue(enumName, optionName, &ok);

        if (!ok) {
            qDebug() << "Enum option value not found" << enumName << optionName;

            conversionResult = false;
        }

        return result;
    }

    std::tuple<QModelIndex, QString> getController(const QString & dstType, const QString & valueType, const QString & controlledValueEffect, const QString & controlledValueLighting) {
        QModelIndex iControllerDst;
        QString shaderType;

        std::tie(iControllerDst, shaderType) = getControllerType(dstType, valueType);

        const QString controlledValue = shaderType == "Lighting" ? controlledValueLighting : controlledValueEffect;
        const QString controlledType = valueType == "Color" ? "Color" : "Variable";
        const QString enumName =  shaderType + "ShaderControlled" + controlledType;
        const QString fieldName = "Type of Controlled " + controlledType;
        const QModelIndex iField = getIndexDst(iControllerDst, fieldName);

        if (iField.isValid()) {
            nifDst->set<uint>(iControllerDst, fieldName, enumOptionValue(enumName, controlledValue));
        } else {
            qDebug() << "Field not found" << fieldName << "in" << nifDst->getBlockName(iControllerDst);

            conversionResult = false;
        }

        return {iControllerDst, shaderType};
    }

    std::tuple<QModelIndex, QString> getController(const QString & dstType, const QString & valueType, const QString & controlledValue) {
        return getController(dstType, valueType, controlledValue, controlledValue);
    }

    void ignoreController(QModelIndex iSrc) {
        ignoreBlock(iSrc, false);
        ignoreBlock(iSrc, "Interpolator", true);
    }

    // TODO: Find best way to process blocks sharing the same controllers but with different controllers down the chain
    QModelIndex niControllerCopy(QModelIndex iDst, QModelIndex iSrc, QString name = "Controller", const QString & blockName = "", const int target = -1) {
        bool bExactCopy = false;

        const int numController = nifSrc->getLink(iSrc, name);
        if (numController == -1) {
            return QModelIndex();
        }

//        if (isHandled(numController)) {
//            return niControllerSetLink(iDst, name, getHandled(numController));
//        }

        QModelIndex iControllerSrc = nifSrc->getBlock(numController);
        QModelIndex iControllerDst;

        QString controllerType = nifSrc->getBlockName(iControllerSrc);

        QString dstType = nifDst->getBlockName(iDst);
        QString shaderType;

        // Convert controller type specific values.
        if (controllerType == "NiMaterialColorController") {
            // TODO: Process other colors
            if (nifSrc->get<uint>(iControllerSrc, "Target Color") == enumOptionValue("MaterialColor", "TC_SPECULAR")) {
                if (getControllerType(dstType) == "Effect") {
                    ignoreController(iControllerSrc);

                    return niControllerCopy(iDst, iControllerSrc, "Next Controller", blockName);
                }

                std::tie(iControllerDst, shaderType) = getController(dstType, "Color", "", "Specular Color");
            } else {
                std::tie(iControllerDst, shaderType) = getController(dstType, "Color", "Emissive Color");
            }
        } else if (controllerType == "BSMaterialEmittanceMultController") {
            std::tie(iControllerDst, shaderType) = getController(dstType, "Float", "EmissiveMultiple", "Emissive Multiple");
        } else if (controllerType == "NiAlphaController") {
            std::tie(iControllerDst, shaderType) = getController(dstType, "Float", "Alpha Transparency", "Alpha");
        } else if (controllerType == "BSRefractionFirePeriodController") {
            // TODO:

            std::tie(iControllerDst, shaderType) = getControllerType("BSLightingShaderProperty", "Float");

            if (shaderType == "Lighting") {
                bool ok = false;
                uint val = NifValue::enumOptionValue("LightingShaderControlledVariable", "U Offset", &ok);

                nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", val);

                if (!ok) {
                    qDebug() << __FUNCTION__ << __LINE__ << "Failed to get enum value";
                }
            } else {
                // shaderType == "Effect"
                qDebug() << __LINE__ << "Invalid shader type" << shaderType;
            }
        } else if (controllerType == "BSRefractionStrengthController") {
            std::tie(iControllerDst, shaderType) = getControllerType("BSLightingShaderProperty", "Float");

            if (shaderType == "Lighting") {
                bool ok = false;
                uint val = NifValue::enumOptionValue("LightingShaderControlledVariable", "Refraction Strength", &ok);

                nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", val);

                if (!ok) {
                    qDebug() << __FUNCTION__ << __LINE__ << "Failed to get enum value";
                }
            } else {
                // shaderType == "Effect"
                qDebug() << __LINE__ << "Invalid shader type" << shaderType;
            }
        } else if (controllerType == "NiTextureTransformController") {
            uint val = nifSrc->get<uint>(iControllerSrc, "Operation");
            #define U_OFFSET 20
            #define U_SCALE 21
            #define V_OFFSET 22
            #define V_SCALE 23

            std::tie(iControllerDst, shaderType) = getControllerType(dstType, "Float");

            if (shaderType == "Lighting") {
                switch (val) {
                    case 0: val = U_OFFSET; break;
                    case 1: val = V_OFFSET; break;
                    case 2: break;
                    case 3: val = U_SCALE; break;
                    case 4: val = V_SCALE; break;
                }
            } else if (shaderType == "Effect") {
                switch (val) {
                    case 0: val = 6; break;
                    case 1: val = 8; break;
                    case 2: break;
                    case 3: val = 7; break;
                    case 4: val = 9; break;
                }
            }

            nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", val);
        } else if (controllerType == "NiGeomMorpherController") {
            // NOTE: If in niControllerSequence warnings will be given by the ck.
            // TODO: Might have use bones?
            ignoreBlock(iControllerSrc, true);

            return QModelIndex();
        } else if (controllerType == "NiBSBoneLODController") {
            ignoreBlock(iControllerSrc, true);

            return QModelIndex();
        } else {
            bExactCopy = true;
        }

        if (!bExactCopy) {
            Copier c = Copier(iControllerDst, iControllerSrc, nifDst, nifSrc);

            c.copyValue<int>("Flags");
            c.copyValue<float>("Frequency");
            c.copyValue<float>("Phase");
            c.copyValue<float>("Start Time");
            c.copyValue<float>("Stop Time");

            c.ignore("Next Controller");
            c.ignore("Target");
            c.ignore("Interpolator");

            QString blockName = nifSrc->getBlockName(iControllerSrc);
            if (blockName == "NiTextureTransformController") {
                c.ignore("Texture Slot");
                c.ignore("Shader Map");
                c.ignore("Operation");
            } else if (blockName == "NiMaterialColorController" || blockName == "NiLightColorController") {
                c.ignore("Target Color");
            }

            c.printUnused();
        } else {
            // Check if this controller is meant to be copied exactly.
            if (!QList<QString>({
                                "NiMultiTargetTransformController",
                                "NiControllerManager",
                                "NiTransformController",
                                "NiPSysModifierActiveCtlr",
                                "NiPSysEmitterCtlr",
                                "NiPSysUpdateCtlr",
                                "NiPSysEmitterInitialRadiusCtlr",
                                "NiVisController",
                                "NiPSysGravityStrengthCtlr",
                                "NiPSysInitialRotAngleCtlr",
                                "NiPSysInitialRotSpeedVarCtlr",
                                "NiPSysInitialRotSpeedCtlr",
                                "NiPSysEmitterLifeSpanCtlr",
                                "NiPSysEmitterPlanarAngleVarCtlr",
                                "NiPSysEmitterPlanarAngleCtlr",
                                "NiPSysEmitterDeclinationVarCtlr",
                                "NiPSysEmitterDeclinationCtlr",
                                "NiPSysEmitterSpeedCtlr",
                                "NiFloatExtraDataController",
                                "NiLightDimmerController",
                                "NiLightColorController",
                                "bhkBlendController",
                                "BSPSysMultiTargetEmitterCtlr",
                                "BSFrustumFOVController",
                                "NiPSysResetOnLoopCtlr",

                    }).contains(nifSrc->getBlockName(nifSrc->getBlock(numController)))) {
                qDebug() << __FUNCTION__ << "Controller not found in exact copy list:" << nifSrc->getBlockName(nifSrc->getBlock(numController));

                conversionResult = false;
            }

            iControllerDst = copyBlock(QModelIndex(), nifSrc->getBlock(numController), -1, false);

            // Unset link pointing to old nif.
            nifDst->setLink(iControllerDst, "Next Controller", -1);
        }

        // Target
        if (target == -1) {
            reLink(iControllerDst, iControllerSrc, "Target");
        } else {
            nifDst->setLink(iControllerDst, "Target", target);
        }

        // Interpolator
        niInterpolator(iControllerDst, iControllerSrc);

        // Visibility Interpolator
        niInterpolator(iControllerDst, iControllerSrc, "Visibility Interpolator");

        // Object Palette
        QModelIndex iObjectPaletteDst;
        QModelIndex iObjectPaletteSrc;
        std::tie(iObjectPaletteDst, iObjectPaletteSrc) = copyLink(iControllerDst, iControllerSrc, "Object Palette");
        if (iObjectPaletteSrc.isValid()) {
            niAVDefaultObjectPalette(iObjectPaletteDst, iObjectPaletteSrc);
        }

        // Extra Targets
        if (nifSrc->getIndex(iControllerSrc, "Extra Targets").isValid()) {
            reLinkArray(iControllerDst, iControllerSrc, "Extra Targets");
        }

        // Controller Sequences
        QModelIndex iControllerSequencesSrc = nifSrc->getIndex(iControllerSrc, "Controller Sequences");
        if (iControllerSequencesSrc.isValid()) {
            niControllerSequences(nifDst->getIndex(iControllerDst, "Controller Sequences"), iControllerSequencesSrc);
        }

        // Next Controller
        niControllerCopy(iControllerDst, iControllerSrc, "Next Controller", blockName);

        // Set next controller in parent
        QModelIndex iNextControllerRoot = niControllerSetLink(iDst, name, iControllerDst);

        if (isHandled(iControllerSrc)) {
            bool bFound = false;

            for (int i = 0; i < controllerList.count(); i++) {
                if (controllerList[i].getOrigin() == nifDst->getBlockNumber(getHandled(iControllerSrc))) {
                    controllerList[i].add(nifDst->getBlockNumber(iControllerDst), blockName);

                    bFound = true;

                    break;
                }
            }

            if (!bFound) {
                qDebug() << __FUNCTION__ << "Controller not found";

                conversionResult = false;
            }
        } else {
            controllerList.append(Controller(nifDst->getBlockNumber(iControllerDst), blockName));

            setHandled(iControllerDst, iControllerSrc);
        }

        // Return the currently linked controller or the newly created one.
        return iNextControllerRoot;
    }

    void collisionObjectCopy(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Collision Object") {
        int numCollision = nifSrc->getLink(iSrc, name);
        if (numCollision != -1) {
            QModelIndex iCollisionSrc = nifSrc->getBlock(numCollision);
            QString type = nifSrc->getBlockName(iCollisionSrc);

            if (
                    type == "bhkCollisionObject" ||
                    type == "bhkBlendCollisionObject" ||
                    type == "bhkSPCollisionObject") {
                collisionObject(iDst, iCollisionSrc, type);
            } else {
                qDebug() << __FUNCTION__ << "Unknown Collision Object:" << type;

                conversionResult = false;
            }
        }
    }

    QModelIndex insertNiBlock(const QString & name) { return nifDst->insertNiBlock(name); }

    QModelIndex getIndexSrc(QModelIndex parent, const QString & name) {
        return nifSrc->getIndex(parent, name);
    }

    QModelIndex getIndexDst(QModelIndex parent, const QString & name) {
        return nifDst->getIndex(parent, name);
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
        if (scale != 1.0f) {
            scaleVector4(nifDst->getIndex(iNode, "Translation"), scale);
            scaleVector4(nifDst->getIndex(iNode, "Center"), scale);
        }
    }

    /**
     * @brief bhkPackedNiTriStripsShapeAlt
     * @param iDst bhkCompressedMeshShape
     * @param iSrc
     * @param row
     * @return
     */
    QModelIndex bhkPackedNiTriStripsShapeAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
        QModelIndex iDst = nifDst->insertNiBlock("bhkCompressedMeshShape", row);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("User Data");
        c.copyValue("Radius");
        c.copyValue("Scale");
        c.copyValue("Radius Copy");
        c.copyValue("Scale Copy");

        c.ignore("Unused 1");
        c.ignore("Unused 2");
        c.ignore("Data");

        int lDataSrc = nifSrc->getLink(iSrc, "Data");
        if (lDataSrc == -1) {
            return iDst;
        }

        QModelIndex iDataDst = bhkPackedNiTriStripsShapeDataAlt(nifSrc->getBlock(lDataSrc), iRigidBodyDst, row);
        nifDst->setLink(iDst, "Data", nifDst->getBlockNumber(iDataDst));

        return iDst;
    }

    void setMax(float & val, float comparison) {
        if (val < comparison) {
            val = comparison;
        }
    }

    void setMin(float & val, float comparison) {
        if (val > comparison) {
            val = comparison;
        }
    }

    // TODO:
    // Fix collision between chunks.
    // Handle scaling and out of bounds.
    // Chunk transforms.
    QModelIndex bhkPackedNiTriStripsShapeDataAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
        QModelIndex iDst = nifDst->insertNiBlock("bhkCompressedMeshShapeData", row);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        nifDst->set<uint>(iDst, "Bits Per Index", 17);
        nifDst->set<uint>(iDst, "Bits Per W Index", 18);
        nifDst->set<uint>(iDst, "Mask W Index", 262143);
        nifDst->set<uint>(iDst, "Mask Index", 131071);
        nifDst->set<float>(iDst, "Error", 0.001f);

        QModelIndex iVerticesSrc = getIndexSrc(iSrc, "Vertices");

        // Min/Max vertex positions

        float
                xMin = 0,
                xMax = 0,
                yMin = 0,
                yMax = 0,
                zMin = 0,
                zMax = 0;

        // Find mininums and maximums.
        for (int i = 0; i < nifSrc->rowCount(iVerticesSrc); i++) {
            Vector3 v = nifSrc->get<Vector3>(iVerticesSrc.child(i, 0));

            setMin(xMin, v[0]);
            setMin(yMin, v[1]);
            setMin(zMin, v[2]);

            setMax(xMax, v[0]);
            setMax(yMax, v[1]);
            setMax(zMax, v[2]);
        }

        qDebug() << "Mins:" << xMin << yMin << zMin;
        qDebug() << "Maxs:" << xMax << yMax << zMax;

        Vector4 minVector = Vector4(xMin, yMin, zMin, 0);

        // TODO: Chunks and big tris are not related.
        bool bChunks = true;
        if (bChunks) {
            // Chunks scaling
            nifDst->set<Vector4>(iRigidBodyDst, "Translation", (nifDst->get<Vector4>(iRigidBodyDst, "Translation") + minVector) / 10.0f);
        } else {
            // Scale
            float scale = 7.0;

            // Big Verts

            // Create vertices array.
            c.copyValue("Num Big Verts", "Num Vertices");
            nifDst->updateArray(iDst, "Big Verts");
            QModelIndex iBigVertsDst = getIndexDst(iDst, "Big Verts");

            // Set vertices.
            for (int i = 0; i < nifDst->rowCount(iBigVertsDst); i++) {
                Vector3 v = nifSrc->get<Vector3>(iVerticesSrc.child(i, 0));

                nifDst->set<Vector4>(iBigVertsDst.child(i, 0), Vector4(v) * 0.1f);
            }

            // Big Verts scaling
            nifDst->set<Vector4>(iRigidBodyDst, "Translation", nifDst->get<Vector4>(iRigidBodyDst, "Translation") / scale);

            // Big Tris.

            c.copyValue("Num Big Tris", "Num Triangles");

            // Create Big Tris array.
            QModelIndex iBigTrisArrayDst = getIndexDst(iDst, "Big Tris");
            QModelIndex iBigTrisDst;
            nifDst->updateArray(iBigTrisArrayDst);

            QModelIndex iTrianglesArraySrc = getIndexSrc(iSrc, "Triangles");
            QModelIndex iTrianglesSrc;


            // Set Triangles.
            for (int i = 0; i < nifDst->rowCount(iBigTrisArrayDst); i++) {
                iTrianglesSrc = iTrianglesArraySrc.child(i, 0);
                iBigTrisDst = iBigTrisArrayDst.child(i, 0);

                nifDst->updateArray(iBigTrisDst);

                Triangle triangle = nifSrc->get<Triangle>(iTrianglesSrc, "Triangle");

                nifDst->set<ushort>(iBigTrisDst, "Triangle 1", triangle.v1());
                nifDst->set<ushort>(iBigTrisDst, "Triangle 2", triangle.v2());
                nifDst->set<ushort>(iBigTrisDst, "Triangle 3", triangle.v3());
                nifDst->set<ushort>(iBigTrisDst, "Welding Info", nifSrc->get<ushort>(iTrianglesSrc, "Welding Info"));
            }
        }

        c.copyValue<uint, ushort>("Num Chunks", "Num Sub Shapes");
        c.copyValue<uint, ushort>("Num Materials", "Num Sub Shapes");
        c.copyValue<uint, ushort>("Num Transforms", "Num Sub Shapes");
        nifDst->updateArray(iDst, "Chunks");
        nifDst->updateArray(iDst, "Chunk Materials");
        nifDst->updateArray(iDst, "Chunk Transforms");

        QModelIndex iChunkMaterialsArrayDst = getIndexDst(iDst, "Chunk Materials");
        QModelIndex iChunkMaterialsDst;
        QModelIndex iChunksArrayDst = getIndexDst(iDst, "Chunks");
        QModelIndex iChunksDst;
        QModelIndex iSubShapesArraySrc = getIndexSrc(iSrc, "Sub Shapes");
        QModelIndex iSubShapesSrc;

        ushort vertIndex = 0;

        for (int i = 0; i < nifDst->rowCount(iChunksArrayDst); i++) {
            iChunkMaterialsDst = iChunkMaterialsArrayDst.child(i, 0);
            iChunksDst = iChunksArrayDst.child(i, 0);
            iSubShapesSrc = iSubShapesArraySrc.child(i, 0);

            // Chunk Material
            nifDst->set<uint>(iChunkMaterialsDst, "Material", matMap.convert(nifSrc->getIndex(iSubShapesSrc, "Material")));
            nifDst->set<uint>(iChunkMaterialsDst, "Layer", layerMap.convert(nifSrc->getIndex(iSubShapesSrc, "Layer")));
            nifDst->set<int>(iChunkMaterialsDst, "Flags and Part Number", nifSrc->get<int>(iSubShapesSrc, "Flags and Part Number"));
            nifDst->set<ushort>(iChunkMaterialsDst, "Group", nifSrc->get<ushort>(iSubShapesSrc, "Group"));

            // Chunk
            nifDst->set<int>(iChunksDst, "Material Index", i);
            nifDst->set<int>(iChunksDst, "Transform Index", i);
            nifDst->set<uint>(iChunksDst, "Num Vertices", nifSrc->get<uint>(iSubShapesSrc, "Num Vertices"));
            nifDst->updateArray(iChunksDst, "Vertices");
            nifDst->set<ushort>(iChunksDst, "Reference", 65535);
        }

        vertIndex = 0;
        int triIndex = 0;
        int numSubShapes = nifDst->rowCount(iChunksArrayDst);

        for (int k = 0; k < numSubShapes; k++) {
            QModelIndex iSubShapeSrc = iSubShapesArraySrc.child(k, 0);

            iChunkMaterialsDst = iChunkMaterialsArrayDst.child(k, 0);
            iChunksDst = iChunksArrayDst.child(k, 0);
            iSubShapesSrc = iSubShapesArraySrc.child(k, 0);

            // Vertices

            ushort numVertices = ushort(nifSrc->get<uint>(iSubShapeSrc, "Num Vertices"));

            nifDst->set<ushort>(iChunksDst, "Num Vertices", numVertices * 3);
            nifDst->updateArray(iChunksDst, "Vertices");

            QModelIndex iChunkVerticesDst = getIndexDst(iChunksDst, "Vertices");

            qDebug() << "Vertices...";

            for (int j = 0; j < numVertices; j++) {
                Vector3 v = nifSrc->get<Vector3>(iVerticesSrc.child(j + vertIndex, 0));

                qDebug() << v;

                nifDst->set<ushort>(iChunkVerticesDst.child(j * 3 + 0, 0), ushort((v[0] - xMin) * 100.0f));
                nifDst->set<ushort>(iChunkVerticesDst.child(j * 3 + 1, 0), ushort((v[1] - yMin) * 100.0f));
                nifDst->set<ushort>(iChunkVerticesDst.child(j * 3 + 2, 0), ushort((v[2] - zMin) * 100.0f));
            }

            // Triangles

            QModelIndex iTrianglesArraySrc = getIndexSrc(iSrc, "Triangles");
            QModelIndex iTrianglesSrc;
            int numTriangles = 0;

            if (k == numSubShapes - 1) {
                numTriangles = nifSrc->rowCount(iTrianglesArraySrc) - triIndex;
            } else for (ushort i = 0; i < nifSrc->rowCount(iTrianglesArraySrc); i++) {
                iTrianglesSrc = iTrianglesArraySrc.child(i, 0);
                Triangle triangle = nifSrc->get<Triangle>(iTrianglesSrc, "Triangle");
                if (triangle.v1() > vertIndex + numVertices - 1 || triangle.v2() > vertIndex + numVertices - 1 || triangle.v3() > vertIndex + numVertices - 1) {
                    numTriangles = i - triIndex;
                    break;
                }
            }

            // Create double sided triangles
            nifDst->set<uint>(iChunksDst, "Num Indices", uint(numTriangles) * 6);
            nifDst->set<uint>(iChunksDst, "Num Strips", uint(numTriangles) * 2);
            nifDst->set<uint>(iChunksDst, "Num Welding Info", uint(numTriangles) * 2);
            nifDst->updateArray(iChunksDst, "Strips");
            nifDst->updateArray(iChunksDst, "Welding Info");

            QModelIndex iStripLengthsDst = getIndexDst(iChunksDst, "Strips");
            for (int i = 0; i < nifDst->rowCount(iStripLengthsDst); i++) {
                nifDst->set<ushort>(iStripLengthsDst.child(i, 0), 3);
            }

            QModelIndex iIndicesDst = nifDst->getIndex(iChunksDst, "Indices");
            nifDst->updateArray(iIndicesDst);
            QModelIndex iWeldingInfoDst = nifDst->getIndex(iChunksDst, "Welding Info");

            for (int i = 0; i < numTriangles; i++) {
                iTrianglesSrc = iTrianglesArraySrc.child(triIndex + i, 0);

                Triangle triangle = nifSrc->get<Triangle>(iTrianglesSrc, "Triangle");

                if (       vertIndex > triangle.v1() ||
                           vertIndex > triangle.v2() ||
                           vertIndex > triangle.v3()) {
                    qDebug() << __FILE__ << __LINE__ << "Vertex index too low";

                    conversionResult = false;
                } else if (triangle.v1() >= vertIndex + numVertices ||
                           triangle.v2() >= vertIndex + numVertices ||
                           triangle.v3() >= vertIndex + numVertices) {
                    qDebug() << __FILE__ << __LINE__ << "Vertex index too high";

                    conversionResult = false;
                }

                nifDst->set<ushort>(iIndicesDst.child(i * 6 + 0, 0), ushort(triangle.v1() - vertIndex));
                nifDst->set<ushort>(iIndicesDst.child(i * 6 + 1, 0), ushort(triangle.v2() - vertIndex));
                nifDst->set<ushort>(iIndicesDst.child(i * 6 + 2, 0), ushort(triangle.v3() - vertIndex));

                nifDst->set<ushort>(iIndicesDst.child(i * 6 + 3, 0), ushort(triangle.v3() - vertIndex));
                nifDst->set<ushort>(iIndicesDst.child(i * 6 + 4, 0), ushort(triangle.v2() - vertIndex));
                nifDst->set<ushort>(iIndicesDst.child(i * 6 + 5, 0), ushort(triangle.v1() - vertIndex));

                nifDst->set<ushort>(iWeldingInfoDst.child(i * 2 + 0, 0), nifSrc->get<ushort>(iTrianglesSrc, "Welding Info"));
                nifDst->set<ushort>(iWeldingInfoDst.child(i * 2 + 1, 0), nifSrc->get<ushort>(iTrianglesSrc, "Welding Info"));
            }

            vertIndex += numVertices;
            triIndex += numTriangles;
        }

        c.ignore("Unknown Byte 1");

        return iDst;
    }

    QModelIndex bhkPackedNiTriStripsShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius) {
        bhkUpdateScale(bScaleSet, radius, nifSrc->get<float>(iSrc, "Radius"));

        QModelIndex iDst = nifDst->insertNiBlock("bhkNiTriStripsShape", row);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Radius");
        c.copyValue("Scale");

        c.ignore("User Data");
        c.ignore("Radius Copy");
        c.ignore("Scale Copy");
        c.ignore("Unused 1");
        c.ignore("Unused 2");
        c.ignore("Data");

        int lDataSrc = nifSrc->getLink(iSrc, "Data");
        if (lDataSrc == -1) {
            return iDst;
        }

        bhkPackedNiTriStripsShapeData(nifSrc->getBlock(lDataSrc), iDst, row);
//        nifDst->setLink(
//                    nifDst->getIndex(iDst, "Strips Data").child(0, 0),
//                    nifDst->getBlockNumber(bhkPackedNiTriStripsShapeData(nifSrc->getBlock(lDataSrc), iRigidBodyDst, iDst, row)));

        setHandled(iDst, iSrc);

        return iDst;
    }

    bool inRange(int low, int high, int x) {
        return x >= low && x <= high;
    }

    /**
     * @brief vertexRange
     * Handle vertices connected to a vertex in a sub shape but not in the sub shape range.
     * @param iVerticesSrc
     * @param vertexList
     * @param vertexMap
     * @param bInRange
     * @param index
     */
    void vertexRange(const QModelIndex iVerticesSrc, QVector<Vector3> & vertexList, QMap<int, int> & vertexMap, bool bInRange, int index) {
        if (!bInRange && !vertexMap.contains(index)) {
            // Append the vertex and map the old index to the new one.
            vertexMap[index] = vertexList.count();
            vertexList.append(nifSrc->get<Vector3>(iVerticesSrc.child(index, 0)));
        }
    }

    void setVertex(QModelIndex iPoint1, QModelIndex iPoint2, QMap<int, int> & vertexMap, int firstIndex, int index) {
        if (vertexMap.contains(index)) {
            index = vertexMap[index];
        } else {
            index -= firstIndex;
        }

        nifDst->set<ushort>(iPoint1, ushort(index));
        nifDst->set<ushort>(iPoint2, ushort(index));
    }

    QModelIndex bhkPackedNiTriStripsShapeDataSubShapeTriangles(QModelIndex iSubShapeSrc, QModelIndex iDataSrc, int firstVertIndex, int row) {
        int numVerticesSubShape = nifSrc->get<int>(iSubShapeSrc, "Num Vertices");
        int numTrianglesTotal = nifSrc->get<int>(iDataSrc, "Num Triangles");
        int vertIndexMax = firstVertIndex + numVerticesSubShape - 1;

        // Scale up (from glnode.cpp)
        float scale = 7.0;

        QVector<Vector3> vertexList;
        QModelIndex iVerticesSrc = getIndexSrc(iDataSrc, "Vertices");

        // Add vertices in sub shape range.
        for (int i = firstVertIndex; i <= vertIndexMax; i++) {
            vertexList.append(nifSrc->get<Vector3>(iVerticesSrc.child(i, 0)));
        }

        QModelIndex iTrianglesArraySrc = getIndexSrc(iDataSrc, "Triangles");

        QList<Triangle> triangleList;
        QMap<int, int> vertexMap;

        // Find all triangles and vertices related to those in the sub shape range.
        for (int i = 0; i < numTrianglesTotal; i++) {
            QModelIndex iTriangleSrc = iTrianglesArraySrc.child(i, 0);
            Triangle triangle = nifSrc->get<Triangle>(iTriangleSrc, "Triangle");

            bool bInRange1 = inRange(firstVertIndex, vertIndexMax, triangle.v1());
            bool bInRange2 = inRange(firstVertIndex, vertIndexMax, triangle.v2());
            bool bInRange3 = inRange(firstVertIndex, vertIndexMax, triangle.v3());

            if (bInRange1 || bInRange2 || bInRange3) {
                triangleList.append(triangle);

                vertexRange(iVerticesSrc, vertexList, vertexMap, bInRange1, triangle.v1());
                vertexRange(iVerticesSrc, vertexList, vertexMap, bInRange2, triangle.v2());
                vertexRange(iVerticesSrc, vertexList, vertexMap, bInRange3, triangle.v3());
            }
        }

        // Elric crashes without at least one triangle.
        if (triangleList.count() == 0) {
            return QModelIndex();
        }

        // Set sub shape fields.

        // Set Vertices

        QModelIndex iNiTriStripsDst = nifDst->insertNiBlock("NiTriStripsData", row);
        QModelIndex iVerticesDst = getIndexDst(iNiTriStripsDst, "Vertices");

        nifDst->set<bool>(iNiTriStripsDst, "Has Vertices", true);
        nifDst->set<int>(iNiTriStripsDst, "Num Vertices", vertexList.count());

        // Scale and set vertices
        nifDst->updateArray(iVerticesDst);
        for (int i = 0; i < vertexList.count(); i++) {
            nifDst->set<Vector3>(iVerticesDst.child(i, 0), vertexList[i] * scale);
        }

        // Set Triangles

        // Create Double sided triangles.
        nifDst->set<bool>(iNiTriStripsDst, "Has Points", true);
        nifDst->set<ushort>(iNiTriStripsDst, "Num Triangles", ushort(triangleList.count() * 2));
        nifDst->set<ushort>(iNiTriStripsDst, "Num Strips", ushort(triangleList.count() * 2));

        nifDst->updateArray(iNiTriStripsDst, "Strip Lengths");

        QModelIndex iStripLengthsDst = getIndexDst(iNiTriStripsDst, "Strip Lengths");
        for (int i = 0; i < nifDst->rowCount(iStripLengthsDst); i++) {
            nifDst->set<ushort>(iStripLengthsDst.child(i, 0), 3);
        }

        nifDst->updateArray(iNiTriStripsDst, "Points");

        QModelIndex iPointsArrayDst = getIndexDst(iNiTriStripsDst, "Points");

        // Set Triangles
        for (int i = 0; i < triangleList.count(); i++) {
            QModelIndex iPointsDst1 = iPointsArrayDst.child(i * 2 + 0, 0);
            QModelIndex iPointsDst2 = iPointsArrayDst.child(i * 2 + 1, 0);

            nifDst->updateArray(iPointsDst1);
            nifDst->updateArray(iPointsDst2);

            Triangle triangle = triangleList[i];

            setVertex(iPointsDst1.child(0 ,0), iPointsDst2.child(2 ,0), vertexMap, firstVertIndex, triangle.v1());
            setVertex(iPointsDst1.child(1 ,0), iPointsDst2.child(1 ,0), vertexMap, firstVertIndex, triangle.v2());
            setVertex(iPointsDst1.child(2 ,0), iPointsDst2.child(0 ,0), vertexMap, firstVertIndex, triangle.v3());
        }

        return iNiTriStripsDst;
    }

    QModelIndex bhkPackedNiTriStripsShapeData(QModelIndex iSrc, QModelIndex iBhkNiTriStripsShapeDst, int row) {
        Copier c = Copier(QModelIndex(), iSrc, nifDst, nifSrc);

        QModelIndex iSubShapesArraySrc = getIndexSrc(iSrc, "Sub Shapes");
        c.ignore(getIndexSrc(iSubShapesArraySrc.child(0, 0), "Layer"));
        c.ignore(getIndexSrc(iSubShapesArraySrc.child(0, 0), "Flags and Part Number"));
        c.ignore(getIndexSrc(iSubShapesArraySrc.child(0, 0), "Group"));
        c.ignore(getIndexSrc(iSubShapesArraySrc.child(0, 0), "Num Vertices"));
        c.ignore(getIndexSrc(iSubShapesArraySrc.child(0, 0), "Material"));

        ushort numSubShapes = nifSrc->get<ushort>(iSrc, "Num Sub Shapes");
        nifDst->set<ushort>(iBhkNiTriStripsShapeDst, "Num Strips Data", numSubShapes);
        nifDst->updateArray(iBhkNiTriStripsShapeDst, "Strips Data");
        nifDst->set<ushort>(iBhkNiTriStripsShapeDst, "Num Data Layers", numSubShapes);
        nifDst->updateArray(iBhkNiTriStripsShapeDst, "Data Layers");

        QModelIndex iDataLayersArrayDst = getIndexDst(iBhkNiTriStripsShapeDst, "Data Layers");

        int vertIndex = 0;
        int subShapeIndex = 0;

        for (int i = 0; i < numSubShapes; i++) {
            QModelIndex iSubShapeSrc = iSubShapesArraySrc.child(i, 0);

            QModelIndex iDst = bhkPackedNiTriStripsShapeDataSubShapeTriangles(iSubShapeSrc, iSrc, vertIndex, row);

            if (!iDst.isValid()) {
                continue;
            }

            c.setIDst(iDst);

            vertIndex += ushort(nifSrc->get<uint>(iSubShapeSrc, "Num Vertices"));

            nifDst->setLink(getIndexDst(iBhkNiTriStripsShapeDst, "Strips Data").child(subShapeIndex, 0), row);

            // Data Layer
            QModelIndex iDataLayerDst = iDataLayersArrayDst.child(subShapeIndex, 0);
            nifDst->set<uint>(iDataLayerDst, "Layer", layerMap.convert(nifSrc->getIndex(iSubShapeSrc, "Layer")));
            nifDst->set<int>(iDataLayerDst, "Flags and Part Number", nifSrc->get<int>(iSubShapeSrc, "Flags and Part Number"));
            nifDst->set<ushort>(iDataLayerDst, "Group", nifSrc->get<ushort>(iSubShapeSrc, "Group"));

            nifDst->set<uint>(iDst, "Material CRC", matMap.convert(getIndexSrc(iSubShapeSrc, "Material")));

            subShapeIndex++;

            setHandled(iDst, iSrc);
        }

        // Update arrays to account for deleted sub shapes.
        nifDst->set<ushort>(iBhkNiTriStripsShapeDst, "Num Strips Data", ushort(subShapeIndex));
        nifDst->updateArray(iBhkNiTriStripsShapeDst, "Strips Data");
        nifDst->set<ushort>(iBhkNiTriStripsShapeDst, "Num Data Layers", ushort(subShapeIndex));
        nifDst->updateArray(iBhkNiTriStripsShapeDst, "Data Layers");

        QModelIndex iTrianglesArraySrc = getIndexSrc(iSrc, "Triangles");
        c.ignore(getIndexSrc(iTrianglesArraySrc.child(0, 0), "Triangle"));
        c.ignore(getIndexSrc(iTrianglesArraySrc.child(0, 0), "Welding Info"));
        c.ignore("Unknown Byte 1");
        c.ignore("Num Sub Shapes");
        c.ignore("Num Triangles");
        c.ignore("Num Vertices");
        c.ignore(getIndexSrc(iSrc, "Vertices").child(0, 0));

        return QModelIndex();
    }

    QModelIndex bhkMoppBvTreeShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        nifDst->set<int>(iDst, "Build Type", 1);

        int lShapeSrc = nifSrc->getLink(iSrc, "Shape");
        if (lShapeSrc == -1) {
            return QModelIndex();
        }

        QModelIndex iShapeSrc = nifSrc->getBlock(lShapeSrc);
        // TODO: Use bkhNiTriStripsShape and bhkNiTriStripsData
        // TODO: Set block order
        QModelIndex iShapeDst = bhkShape(iShapeSrc, parent, row, bScaleSet, radius);

        nifDst->setLink(iDst, "Shape", nifDst->getBlockNumber(iShapeDst));

        return iDst;
    }

    QModelIndex bhkShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius) {
        // Scale the collision object.
        // NOTE: scaleNode currently breaks collision for the object.
        bool bScaleNode = false;
        QModelIndex scaleNode;
        if (bScaleNode) {
            QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

            scaleNode = insertNiBlock("NiNode");
            uint numChildren = nifDst->get<uint>(parent, "Num Children");
            nifDst->set<uint>(parent, "Num Children", numChildren + 1);
            nifDst->updateArray(parent, "Children");
            nifDst->setLink(nifDst->getIndex(parent, "Children").child(int(numChildren), 0), nifDst->getBlockNumber(scaleNode));
            nifDst->set<float>(scaleNode, "Scale", nifDst->get<float>(iDst, "Radius"));

            parent = scaleNode;

            return iDst;
        } else {
            QString shapeType = nifSrc->getBlockName(iSrc);

            if (shapeType == "bhkMoppBvTreeShape") {
                return bhkMoppBvTreeShape(iSrc, parent, row, bScaleSet, radius);
            } else if (shapeType == "bhkConvexVerticesShape") {
                return bhkConvexVerticesShape(iSrc, row, bScaleSet, radius);
            } else if (shapeType == "bhkListShape") {
                return bhkListShape(iSrc, parent, row, bScaleSet, radius);
            } else if (shapeType == "bhkPackedNiTriStripsShape") {
                return bhkPackedNiTriStripsShape(iSrc, row, bScaleSet, radius);
            } else if (shapeType == "bhkNiTriStripsShape") {
                return bhkNiTriStripsShape(iSrc, row, bScaleSet, radius);
            } else if (shapeType == "bhkConvexTransformShape") {
                return  bhkConvexTransformShape(iSrc, parent, row, bScaleSet, radius);
            } else if (shapeType == "bhkTransformShape") {
                return  bhkTransformShape(iSrc, parent, row, bScaleSet, radius);
            } else if (shapeType == "bhkBoxShape") {
                return  bhkBoxShape(iSrc, row, bScaleSet, radius);
            } else if (shapeType == "bhkCapsuleShape") {
                return  bhkCapsuleShape(iSrc, row, bScaleSet, radius);
            } else if (shapeType == "bhkSphereShape") {
                return bhkSphereShape(iSrc, row, bScaleSet, radius);
            } else {
                qDebug() << __FUNCTION__ << "Unknown collision shape:" << shapeType;

                conversionResult = false;
            }
        }

        return QModelIndex();
    }

    QModelIndex bhkNiTriStripsShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius) {
        bhkUpdateScale(bScaleSet, radius, nifSrc->get<float>(iSrc, "Radius"));

        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        QModelIndex iStripsDataArrayDst = getIndexDst(iDst, "Strips Data");
        QModelIndex iStripsDataArraySrc = getIndexSrc(iSrc, "Strips Data");

        for (int i = 0; i < nifSrc->rowCount(iStripsDataArraySrc); i++) {
            QModelIndex iStripsDataDst = bhkNiTriStripsShapeData(getBlockSrc(iStripsDataArraySrc.child(i, 0)), row);

            nifDst->setLink(iStripsDataArrayDst.child(i, 0), nifDst->getBlockNumber(iStripsDataDst));
        }

        nifDst->set<uint>(iDst, "Material", matMap.convert(getIndexSrc(iSrc, "Material")));

        QModelIndex iDataLayerArrayDst = getIndexDst(iDst, "Data Layers");
        QModelIndex iDataLayerArraySrc = getIndexSrc(iSrc, "Data Layers");

        for (int i = 0; i < nifSrc->rowCount(iDataLayerArraySrc); i++) {
            nifDst->set<uint>(iDataLayerArrayDst.child(i, 0),
                              "Layer",
                              layerMap.convert(getIndexSrc(iDataLayerArraySrc.child(i, 0), "Layer")));
        }

        return iDst;
    }

    QModelIndex bhkNiTriStripsShapeData(QModelIndex iSrc, int row) {
        QModelIndex iDst = nifDst->insertNiBlock("NiTriStripsData", row);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Group ID");
        c.copyValue("Num Vertices");
        c.copyValue("Keep Flags");
        c.copyValue("Compress Flags");

        c.copyValue<bool>("Has Vertices");
        if (nifSrc->get<bool>(iSrc, "Has Vertices")) {
            c.array("Vertices");
        }

        c.copyValue("BS Vector Flags");

        c.copyValue<bool>("Has Normals");
        if (nifSrc->get<bool>(iSrc, "Has Normals")) {
            c.array("Normals");
        }

        c.copyValue("Center");
        c.copyValue("Radius");

        c.copyValue<bool>("Has Vertex Colors");
        if (nifSrc->get<bool>(iSrc, "Has Vertex Colors")) {
            c.array("Vertex Colors");
        }

        c.array("UV Sets");
        c.copyValue("Consistency Flags");
        c.ignore("Additional Data");
        ignoreBlock(iSrc, "Additional Data", false);
        c.copyValue("Num Triangles");
        c.copyValue("Num Strips");
        c.array("Strip Lengths");
        c.copyValue<bool>("Has Points");
        c.array("Points");

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex bhkSphereShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = nifDst->insertNiBlock("bhkSphereShape", row);

        nifDst->set<uint>(iDst, "Material", matMap.convert(getIndexSrc(iSrc, "Material")));

        // NOTE: Seems to be always scaled by 10 in source.
        bhkUpdateScale(bScaleSet, radius, 0.1f);
        nifDst->set<float>(iDst, "Radius", nifSrc->get<float>(iSrc, "Radius") * 0.1f);

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex bhkConvexTransformShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));

        bhkUpdateScale(bScaleSet, radius, nifSrc->get<float>(iSrc, "Radius"));

        Matrix4 scaleMatrix = Matrix4();

        scaleMatrix.compose(Vector3(), Matrix(), Vector3(radius, radius, radius));
        nifDst->set<Matrix4>(iDst, "Transform", nifSrc->get<Matrix4>(iSrc, "Transform") * scaleMatrix);

        QModelIndex iShapeSrc = getBlockSrc(iSrc, "Shape");
        QModelIndex iShapeDst = bhkShape(iShapeSrc, parent, nifDst->getBlockNumber(iDst), bScaleSet, radius);

        nifDst->setLink(iDst, "Shape", nifDst->getBlockNumber(iShapeDst));

        return iDst;
    }

    QModelIndex bhkTransformShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));

        QModelIndex iShapeSrc = getBlockSrc(iSrc, "Shape");
        QModelIndex iShapeDst = bhkShape(iShapeSrc, parent, nifDst->getBlockNumber(iDst), bScaleSet, radius);

        Matrix4 m = nifSrc->get<Matrix4>(iSrc, "Transform");

        // Translation seems to always be scaled by 10 in source
        m(3, 0) = m(3, 0) * 0.1f;
        m(3, 1) = m(3, 1) * 0.1f;
        m(3, 2) = m(3, 2) * 0.1f;

        QString blockType = nifSrc->getBlockName(iShapeSrc);

        if (blockType == "bhkMoppBvTreeShape") {
            blockType = nifSrc->getBlockName(nifSrc->getBlock(nifSrc->getLink(iShapeSrc, "Shape")));
        }

        if (blockType == "bhkPackedNiTriStripsShape") {
            m(0, 0) = m(0, 0) * 7.0f;
            m(1, 1) = m(1, 1) * 7.0f;
            m(2, 2) = m(2, 2) * 7.0f;
        } else if (blockType == "bhkBoxShape" || blockType == "bhkSphereShape") {
            // Don't scale
        } else {
            qDebug() << __FUNCTION__ << __LINE__ << "Unknown block:" << blockType;

            conversionResult = false;
        }

        nifDst->set<Matrix4>(iDst, "Transform",  m);

        nifDst->setLink(iDst, "Shape", nifDst->getBlockNumber(iShapeDst));

        return iDst;
    }



    /**
     * @brief bhkUpdateScale
     * @param bScaleSet
     * @param radius
     * @param newRadius
     * @return True if newly set, else false
     */
    bool bhkUpdateScale(bool & bScaleSet, float & radius, const float newRadius) {
        if (newRadius == 0.0f) {
            qDebug() << "Radius of 0";

            conversionResult = false;

            return  false;
        }

        if (!bScaleSet) {
            radius = newRadius;
            bScaleSet = true;

            return true;
        } else if (radius - newRadius != 0.0f) {
            qDebug() << __FUNCTION__ << "Different radii:" << radius << "and" << newRadius << ", cannot scale rigidBody";

            conversionResult = false;
        }

        return false;
    }

    QModelIndex bhkBoxShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));

        if (bhkUpdateScale(bScaleSet, radius, nifSrc->get<float>(iSrc, "Radius"))) {
            nifDst->set<Vector3>(iDst, "Dimensions", nifSrc->get<Vector3>(iSrc, "Dimensions") * radius);
        }

        return iDst;
    }

    QModelIndex bhkCapsuleShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));

        // Capsule seems to always be scaled by 10
        float scale = 0.1f;

        bhkUpdateScale(bScaleSet, radius, scale);

        float newRadius = nifSrc->get<float>(iSrc, "Radius") * scale;

        nifDst->set<float>(iDst, "Radius", newRadius);
        nifDst->set<float>(iDst, "Radius 1", newRadius);
        nifDst->set<float>(iDst, "Radius 2", newRadius);

        QModelIndex iFirstPointDst = getIndexDst(iDst, "First Point");
        QModelIndex iSecondPointDst = getIndexDst(iDst, "Second Point");

        nifDst->set<Vector3>(iFirstPointDst, nifDst->get<Vector3>(iFirstPointDst) * scale);
        nifDst->set<Vector3>(iSecondPointDst, nifDst->get<Vector3>(iSecondPointDst) * scale);

//        bhkUpdateScale(bScaleSet, radius, newRadius);



        return iDst;
    }

    void collisionObject( QModelIndex parent, QModelIndex iSrc, QString type = "bhkCollisionObject" ) {
        QModelIndex iDst = insertNiBlock(type);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        // Collision Object
        c.copyValue("Flags");

        c.ignore("Target");
        c.ignore("Body");

        QModelIndex iRigidBodySrc = nifSrc->getBlock(nifSrc->getLink(iSrc, "Body"));
        QModelIndex iRigidBodyDst = bhkRigidBody(iRigidBodySrc, parent, nifDst->getBlockNumber(iDst));
        nifDst->setLink(iDst, "Body", nifDst->getBlockNumber(iRigidBodyDst));

        // Link to parent
        nifDst->setLink(parent, "Collision Object", nifDst->getBlockNumber(iDst));
        nifDst->setLink(iDst, "Target", nifDst->getBlockNumber(parent));

        if (type == "bhkBlendCollisionObject") {
            c.copyValue("Heir Gain");
            c.copyValue("Vel Gain");
        }

        setHandled(iDst, iSrc);
    }

    // NOTE: Copy of rigidBody is only correct up to and including Angular Damping
    // NOTE: Some props have weird collision e.g: 9mmammo.nif.
    QModelIndex bhkRigidBody(QModelIndex iSrc, QModelIndex & parent, int row) {
        QString type = nifSrc->getBlockName(iSrc);

        QModelIndex iDst = nifDst->insertNiBlock(type, row);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        QModelIndex iShapeSrc = nifSrc->getBlock(nifSrc->getLink(iSrc, "Shape"));

        float radius = 1.0f;
        bool bScaleSet = false;
        QModelIndex iShapeDst = bhkShape(iShapeSrc, parent, nifDst->getBlockNumber(iDst), bScaleSet, radius);

        // Shape
        // NOTE: Radius not rendered? Seems to be at 10 times always
        nifDst->setLink(iDst, "Shape", nifDst->getBlockNumber(iShapeDst));
        c.processed("Shape");

        nifDst->set<uint>(iDst, "Layer", layerMap.convert(getIndexSrc(iSrc, "Layer")));
        c.processed("Layer");

        c.copyValue("Flags and Part Number");
        c.copyValue("Group");
        c.copyValue("Broad Phase Type");

        c.ignore(nifSrc->getIndex(iSrc, "Unused Bytes"), "Unused Bytes");
        c.ignore(nifSrc->getIndex(iSrc, "Unused"), "Unused");
        c.ignore(nifSrc->getIndex(iSrc, "Unused 2"), "Unused 2");

        QModelIndex iCInfoPropertyDst = nifDst->getIndex(iDst, "Cinfo Property");
        QModelIndex iCInfoPropertySrc = nifSrc->getIndex(iSrc, "Cinfo Property");
        c.copyValue(iCInfoPropertyDst, iCInfoPropertySrc, "Data");
        c.copyValue(iCInfoPropertyDst, iCInfoPropertySrc, "Size");
        c.copyValue(iCInfoPropertyDst, iCInfoPropertySrc, "Capacity and Flags");

        if (type == "bhkRigidBody" || type == "bhkRigidBodyT") {
            c.copyValue("Translation");
            c.copyValue("Center");

            // TODO: Always 0.1?
            if (bScaleSet) {
                collapseScaleRigidBody(iDst, 0.1f);
            }

            // TODO: Material

            nifDst->set<float>(iDst, "Time Factor", 1.0);
            c.copyValue<float>("Friction");
            nifDst->set<float>(iDst, "Rolling Friction Multiplier", 0.0);
            c.copyValue<float>("Restitution");
            c.copyValue<float>("Max Linear Velocity");
            c.copyValue<float>("Max Angular Velocity");
            c.copyValue<float>("Penetration Depth");

            if (nifSrc->get<float>(iSrc, "Mass") == 0.0f) {
                nifDst->set<uint>(iDst, "Motion System", enumOptionValue("hkMotionType", "MO_SYS_INVALID"));
                nifDst->set<bool>(iDst, "Enable Deactivation", false);
                nifDst->set<uint>(iDst, "Quality Type", enumOptionValue("hkQualityType", "MO_QUAL_FIXED"));

                c.processed("Motion System");
                c.processed("Solver Deactivation");
                c.processed("Quality Type");
            } else {
                if (c.getSrc<int>("Motion System") == int(enumOptionValue("hkMotionType", "MO_SYS_FIXED"))) {
                    nifDst->set<uint>(iDst, "Motion System", enumOptionValue("hkMotionType", "MO_SYS_INVALID"));
                } else {
                    c.copyValue<int>("Motion System");
                }

                // Deactivator Type
                if (c.getSrc<int>("Solver Deactivation") > 1) {
                    nifDst->set<bool>(iDst, "Enable Deactivation", true);
                    c.copyValue<int>("Solver Deactivation");
                }

                c.copyValue<int>("Quality Type");
            }

            c.copyValue("Collision Response");
            c.copyValue("Collision Response 2");
            c.copyValue("Process Contact Callback Delay");
            c.copyValue("Process Contact Callback Delay 2");
            c.copyValue("Rotation");

            c.copyValue("Linear Velocity");
            c.copyValue("Angular Velocity");
            c.copyValue("Mass");

            c.copyValue("Linear Damping");
            c.copyValue("Angular Damping");

            c.ignore("Deactivator Type");

            // Unused values
            c.ignore("Unused Byte 1");
            c.ignore("Unused Byte 2");
            c.ignore("Unknown Int 1");
            c.ignore("Unknown Int 2");
            c.ignore(nifSrc->getIndex(iSrc, "Unknown Bytes 1"), "Unknown Bytes 1");

            QModelIndex iHavokFilterCopyDst = nifDst->getIndex(iDst, "Havok Filter Copy");
            QModelIndex iHavokFilterCopySrc = nifSrc->getIndex(iSrc, "Havok Filter Copy");
            // TODO: Convert Layer
            c.copyValue(iHavokFilterCopyDst, iHavokFilterCopySrc, "Layer");
            c.copyValue(iHavokFilterCopyDst, iHavokFilterCopySrc, "Flags and Part Number");
            c.copyValue(iHavokFilterCopyDst, iHavokFilterCopySrc, "Group");

            QModelIndex iInertiaTensorDst = nifDst->getIndex(iDst, "Inertia Tensor");
            QModelIndex iInertiaTensorSrc = nifSrc->getIndex(iSrc, "Inertia Tensor");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m11");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m12");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m13");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m14");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m21");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m22");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m23");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m24");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m31");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m32");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m33");
            c.copyValue(iInertiaTensorDst, iInertiaTensorSrc, "m34");

            c.copyValue("Num Constraints");

            if (nifSrc->get<uint>(iSrc, "Num Constraints") > 0) {
                QModelIndex iConstraintsDst = getIndexDst(iDst, "Constraints");
                QModelIndex iConstraintsSrc = getIndexSrc(iSrc, "Constraints");

                nifDst->updateArray(iConstraintsDst);

                for (int i = 0; i < nifSrc->rowCount(iConstraintsSrc); i++) {
                    setLink(iConstraintsDst.child(i, 0), bhkConstraint(getBlockSrc(iConstraintsSrc.child(i, 0))));
                }

                c.processed(iConstraintsSrc.child(0, 0));
            }

//            c.copyValue<ushort, uint>("Body Flags");
            c.copyValue<ushort, uint>(getIndexDst(iDst, "Body Flags"), getIndexSrc(iSrc, "Body Flags"));
        } else if (type == "bhkSimpleShapePhantom") {
            // TODO: Scale?
            c.copyValue("Transform");
        } else {
            qDebug() << __FUNCTION__ << "Unknown Type:" << type;

            conversionResult = false;
        }

        setHandled(iDst, iSrc);

        return iDst;
    }

    // NOTE: Block number does not appear to matter
    QModelIndex bhkConstraint(QModelIndex iSrc) {
        QString type = nifSrc->getBlockName(iSrc);

        if (
                type == "bhkLimitedHingeConstraint" ||
                type == "bhkRagdollConstraint" ||
                type == "bhkHingeConstraint" ||
                type == "bhkMalleableConstraint" ||
                type == "bhkPrismaticConstraint" ||
                type == "bhkBreakableConstraint" ||
                type == "bhkStiffSpringConstraint") {
            QModelIndex iDst = copyBlock(QModelIndex(), iSrc);

            reLinkArray(iDst, iSrc, "Entities");

            return iDst;
        } else if (type == "bhkOrientHingedBodyAction") {
            QModelIndex iDst = copyBlock(QModelIndex(), iSrc);

            reLink(iDst, iSrc, "Body");

            return iDst;
        } else if (type == "bhkLiquidAction") {
            // NOTE: Cannot be converted at runtime and untested
            QModelIndex iDst = copyBlock(QModelIndex(), iSrc);

            // Seems to cause crash in elric if other than 0
            nifDst->set<uint>(iDst, "User Data", 0);

            return iDst;
        }

        qDebug() << __FUNCTION__ << "Unknown constraint type:" << type;

        conversionResult = false;

        return QModelIndex();
    }

    QModelIndex bhkConvexVerticesShape(QModelIndex iSrc, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        bhkUpdateScale(bScaleSet, radius, nifDst->get<float>(iDst, "Radius"));

        // NOTE: Radius seems to mean scale in FNV context.
        nifDst->set<float>(iDst, "Radius", 0.0f);

        collapseScale(iDst, radius);

        // TODO: Check mat value
        nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));

        return iDst;
    }

    QModelIndex getBlockSrc(QModelIndex iLink) {
        return nifSrc->getBlock(nifSrc->getLink(iLink));
    }

    QModelIndex getBlockSrc(QModelIndex iSrc, const QString & name) {
        return getBlockSrc(getIndexSrc(iSrc, name));
    }

    QModelIndex getBlockDst(QModelIndex iLink) {
        return nifDst->getBlock(nifDst->getLink(iLink));
    }

    QModelIndex getBlockDst(QModelIndex iDst, const QString & name) {
        return getBlockDst(getIndexDst(iDst, name));
    }

    QModelIndex bhkListShape(QModelIndex iSrc, QModelIndex & parent, int row, bool & bScaleSet, float & radius) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        QModelIndex iSubShapesArrayDst = nifDst->getIndex(iDst, "Sub Shapes");
        QModelIndex iSubShapesArraySrc = nifSrc->getIndex(iSrc, "Sub Shapes");

        for (int i = 0; i < nifDst->get<int>(iDst, "Num Sub Shapes"); i++) {
            QModelIndex iShapeSrc = getBlockSrc(iSubShapesArraySrc.child(i, 0));
            QModelIndex iShapeDst = bhkShape(iShapeSrc, parent, row, bScaleSet, radius);

            nifDst->setLink(iSubShapesArrayDst.child(i, 0), nifDst->getBlockNumber(iShapeDst));
        }

        return iDst;
    }

    bool setLink(QModelIndex iDst, QModelIndex iTarget) {
        if (nifDst->getLink(iDst) != -1 && nifDst->getLink(iDst) != nifDst->getBlockNumber(iTarget)) {
            qDebug() << "Link already set to"
                     << nifDst->getLink(iDst)
                     << "in" << nifDst->getBlockName(iDst)
                     << ". Setting to"
                     << nifDst->getBlockNumber(iTarget);

            conversionResult = false;

            return false;
        }

        return nifDst->setLink(iDst, nifDst->getBlockNumber(iTarget));
    }

    bool setLink(QModelIndex iDst, const QString & name, QModelIndex iTarget) {
        return setLink(getIndexDst(iDst, name), iTarget);
    }

    void niTexturingProperty(QModelIndex iDst, QModelIndex iSrc, const QString & sequenceBlockName) {
        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        setHandled(iDst, iSrc);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        niController(iDst, iSrc, c, "Controller", sequenceBlockName, nifDst->getBlockNumber(iDst));

        c.ignore("Name");
        c.ignore("Num Extra Data List");
        c.ignore("Flags");
        c.ignore("Texture Count");
        c.ignore("Num Shader Textures");

        QString typeDst = nifDst->getBlockName(iDst);
        QString textures[] = {
            "Base",
            "Dark",
            "Detail",
            "Gloss",
            "Glow",
            "Bump Map",
            "Normal",
            "Parallax",
            "Decal 0",
        };

        for (QString s : textures) {
            c.ignore("Has " + s + " Texture");
            if (c.getSrc<bool>("Has " + s + " Texture")) {
                QModelIndex iTextureSrc = nifSrc->getIndex(iSrc, s + " Texture");

                c.ignore(iTextureSrc, "Flags");

                if (c.getSrc<bool>(iTextureSrc, "Has Texture Transform")) {
                    c.ignore(iTextureSrc, "Translation");
                    c.ignore(iTextureSrc, "Scale");
                    c.ignore(iTextureSrc, "Rotation");
                    c.ignore(iTextureSrc, "Transform Method");
                    c.ignore(iTextureSrc, "Center");
                }

                QModelIndex iNiSourceTexture = nifSrc->getBlock(nifSrc->getLink(iTextureSrc, "Source"));
                setHandled(iDst, iNiSourceTexture);
                QString path = nifSrc->string(iNiSourceTexture, QString("File Name"));

                if (typeDst == "BSEffectShaderProperty") {
                    if (s == "Base") {
                        nifDst->set<QString>(iDst, "Source Texture", updateTexturePath(path));
                        c.processed(iTextureSrc, "Source");
                    }
                } else if (typeDst == "BSLightingShaderProperty") {
                    c.ignore(iTextureSrc, "Source");
                } else {
                    qDebug() << __FUNCTION__ << "Unknown shader property" << typeDst;

                    conversionResult = false;

                    return;
                }
            }
        }
    }

    void niMaterialProperty(QModelIndex iDst, QModelIndex iSrc, const QString & sequenceBlockName) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        QString typeDst = nifDst->getBlockName(iDst);

        c.ignore("Name");
        c.ignore("Num Extra Data List");

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", sequenceBlockName);

        if (
                nifSrc->getUserVersion2() == 14 ||
                nifSrc->getUserVersion2() == 11 ||
                nifSrc->getUserVersion2() == 21) {
            c.ignore("Ambient Color");
            c.ignore("Diffuse Color");
        } else if (typeDst == "BSSkyShaderProperty" || typeDst == "BSWaterShaderProperty") {
            c.ignore("Emissive Mult");
        } else {
            c.copyValue<float>("Emissive Multiple", "Emissive Mult");
        }

        c.ignore("Specular Color");
        c.ignore("Emissive Color");
        nifDst->set<Color4>(iDst, "Emissive Color", Color4(c.getSrc<Color3>("Emissive Color")));
        c.ignore("Glossiness");
        c.ignore("Alpha");

        setHandled(iDst, iSrc);

        c.printUnused();
    }

    void particleSystemModifiers(QModelIndex iDst, QModelIndex iSrc, Copier & c) {
        c.copyValue<uint>("Num Modifiers");
        // TODO: Modifiers
        QModelIndex iModifiersDst = nifDst->getIndex(iDst, "Modifiers");
        QModelIndex iModifiersSrc = nifSrc->getIndex(iSrc, "Modifiers");
        nifDst->updateArray(iModifiersDst);

        if (nifSrc->rowCount(iModifiersSrc) > 0) {
            c.ignore(iModifiersSrc.child(0, 0));
        }

        for (int i = 0; i < nifSrc->rowCount(iModifiersSrc); i++) {
            niPSys(iModifiersDst.child(i, 0), iModifiersSrc.child(i, 0));
        }
    }

    QModelIndex niParticleSystem(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("NiParticleSystem");

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        setHandled(iDst, iSrc);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);
        c.copyValue<QString>("Name");
        c.ignore("Num Extra Data List");
        // TODO: Extra data
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue<int>("Flags");
        c.copyValue<Vector3>("Translation");
        c.copyValue<Matrix>("Rotation");
        c.copyValue<float>("Scale");
        // TODO: Properties

        c.ignore("Num Properties");
        QModelIndex iPropertiesSrc = nifSrc->getIndex(iSrc, "Properties");
        if (nifSrc->rowCount(iPropertiesSrc) > 0) {
            c.ignore(iPropertiesSrc.child(0, 0));

            QModelIndex iShaderProperty = nifDst->insertNiBlock("BSEffectShaderProperty");

            nifDst->setLink(iDst, "Shader Property", nifDst->getBlockNumber(iShaderProperty));

            properties(iSrc, iShaderProperty, iDst);
        }

        collisionObjectCopy(iDst, iSrc);
        c.ignore("Collision Object");

        copyLink(iDst, iSrc, "Data");
        c.ignore("Data");

        c.ignore("Skin Instance");
        QModelIndex iMaterialDataSrc = nifSrc->getIndex(iSrc, "Material Data");
        if (iMaterialDataSrc.isValid()) {
            c.ignore(iMaterialDataSrc, "Num Materials");
            c.ignore(iMaterialDataSrc, "Active Material");
            c.ignore(iMaterialDataSrc, "Material Needs Update");
        }

        c.copyValue<int>("World Space");

        particleSystemModifiers(iDst, iSrc, c);

        return iDst;
    }

    void niPSys(QModelIndex iLinkDst, QModelIndex iLinkSrc) {
        QModelIndex iDst = std::get<0>(copyLink(iLinkDst, iLinkSrc));
        QModelIndex iSrc = getBlockSrc(iLinkSrc);
        QString type = nifSrc->getBlockName(iSrc);

        if (type == "NiPSysColliderManager") {
            niPSysColliderManager(iDst, iSrc);
        } else if (type == "NiPSysColorModifier") {
            copyLink(iDst, iSrc, "Data");
        } else {
            reLinkRec(iDst);
        }
    }

    QModelIndex niPSysColliderManager(QModelIndex iDst, QModelIndex iSrc) {
        QModelIndex iColliderSrc = getBlockSrc(iSrc, "Collider");

        if (iColliderSrc.isValid()) {
            reLink(iDst, iSrc, "Target");
            nifDst->setLink(iDst, "Collider", nifDst->getBlockNumber(niPSysCollider(iColliderSrc)));
        }

        return iDst;
    }

    QModelIndex niPSysCollider(QModelIndex iSrc) {
        if (!iSrc.isValid()) {
            return QModelIndex();
        }

        QString type = nifSrc->getBlockName(iSrc);

        if (
                type == "NiPSysSphericalCollider" ||
                type == "NiPSysPlanarCollider") {
            return niPSysColliderCopy(iSrc);
        } else {
            qDebug() << __FUNCTION__ << "Unknown collider:" << type;

            conversionResult = false;
        }

        return QModelIndex();
    }

    QModelIndex niPSysColliderCopy(QModelIndex iSrc) {
        QModelIndex iDst = copyBlock(QModelIndex(), iSrc);

        reLink(iDst, iSrc, "Spawn Modifier");
        reLink(iDst, iSrc, "Parent");
        nifDst->setLink(iDst, "Next Collider", nifDst->getBlockNumber(niPSysCollider(getBlockSrc(iSrc, "Next Collider"))));
        reLink(iDst, iSrc, "Collider Object");

        return iDst;
    }

//    QModelIndex niBillBoardNode(QModelIndex iDst, QModelIndex iSrc) {
////        Copier c = Copier(fadeNode)
//    }

    QModelIndex bsFurnitureMarker(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("BSFurnitureMarkerNode");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");
        c.copyValue("Num Positions");

        QModelIndex iPositionsArrayDst = nifDst->getIndex(iDst, "Positions");
        QModelIndex iPositionsArraySrc = nifSrc->getIndex(iSrc, "Positions");

        nifDst->updateArray(iPositionsArrayDst);

        for (int i = 0; i < nifSrc->rowCount(iPositionsArraySrc); i++) {
            QModelIndex iPositionDst = iPositionsArrayDst.child(i, 0);
            QModelIndex iPositionSrc = iPositionsArraySrc.child(i, 0);

            c.setIDst(iPositionDst);
            c.setISrc(iPositionSrc);

            c.copyValue("Offset");

            // TODO:
            if (i == 0) {
                c.ignore("Orientation");
                c.ignore("Position Ref 1");
                c.ignore("Position Ref 2");
            }
        }

        setHandled(iDst, iSrc);

        return iDst;
    }

    void extraDataList(QModelIndex iDst, QModelIndex iSrc) {
        QVector<qint32> links = nifSrc->getLinkArray(iSrc, "Extra Data List");

        for (int link:links) {
            QModelIndex iExtraDataSrc = nifSrc->getBlock(link);

            QString type = nifSrc->getBlockName(iExtraDataSrc);
            if (type == "BSXFlags") {
                copyBlock(iDst, iExtraDataSrc);
            } else if (
                    type == "NiStringExtraData" ||
                    type == "NiTextKeyExtraData" ||
                    type == "NiFloatExtraData" ||
                    type == "BSBound" ||
                    type == "NiIntegerExtraData" ||
                    type == "NiBinaryExtraData" ||
                    type == "BSWArray") {
                // NOTE: Simulation_Geometry causes crash if processed at runtime
                copyBlock(iDst, iExtraDataSrc);
            } else if (type == "BSFurnitureMarker") {
                blockLink(nifDst, iDst, bsFurnitureMarker(iExtraDataSrc));
            } else if (type == "BSDecalPlacementVectorExtraData") {
                ignoreBlock(iExtraDataSrc, false);
            } else {
                qDebug() << "Unknown Extra Data block:" << type;

                conversionResult = false;
            }
        }
    }

    void extraDataList(QModelIndex iDst, QModelIndex iSrc, Copier & c) {
        c.ignore("Num Extra Data List");

        if (nifSrc->get<int>(iSrc, "Num Extra Data List") > 0) {
            c.ignore(getIndexSrc(iSrc, "Extra Data List").child(0, 0));
        }

        extraDataList(iDst, iSrc);
    }

    QModelIndex bsFadeNode( QModelIndex iNode) {
        QModelIndex linkNode;
        QModelIndex fadeNode;
        const QString blockType = nifSrc->getBlockName(iNode);

        fadeNode = insertNiBlock(blockType);

        Copier c = Copier(fadeNode, iNode, nifDst, nifSrc);

        c.ignore("Num Extra Data List");
        c.ignore("Controller");
        c.ignore("Num Properties");
        c.ignore("Collision Object");
        c.ignore("Num Effects");

        if (nifSrc->rowCount(getIndexSrc(iNode, "Children")) > 0) {
            c.ignore(getIndexSrc(iNode, "Children").child(0, 0));
        }

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        setHandled(fadeNode, iNode);

        c.copyValue("Name");

        extraDataList(fadeNode, iNode, c);

        niController(fadeNode, iNode, "Controller", nifSrc->get<QString>(iNode, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");

        collisionObjectCopy(fadeNode, iNode);

        c.copyValue("Num Children");
        nifDst->updateArray(fadeNode, "Children");
        QVector<qint32> links = nifSrc->getLinkArray(iNode, "Children");
        for (int i = 0; i < links.count(); i++) {
            if (links[i] == -1) {
                continue;
            }

            linkNode = nifSrc->getBlock(links[i]);
            QModelIndex iChildDst = nifDst->getIndex(fadeNode, "Children").child(i, 0);

            QString type = nifSrc->getBlockName(linkNode);
            if (nifSrc->getBlockName(linkNode) == "NiTriStrips") {
                setLink(iChildDst, niTriStrips(linkNode));
            } else if (
                    type == "BSFadeNode" ||
                    type == "NiNode" ||
                    type == "BSOrderedNode" ||
                    type == "NiBillboardNode" ||
                    type == "BSValueNode" ||
                    type == "BSDamageStage" ||
                    type == "BSBlastNode" ||
                    type == "BSMasterParticleSystem" ||
                    type == "BSMultiBoundNode" ||
                    type == "BSDebrisNode") {
                QModelIndex iFadeNodeChild = bsFadeNode(linkNode);
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(iFadeNodeChild));
            } else if (type == "NiParticleSystem") {
                QModelIndex iNiParticleSystemDst = niParticleSystem(linkNode);
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(iNiParticleSystemDst));
            } else if (type == "NiPointLight" || type == "NiAmbientLight") {
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(niPointLight(linkNode)));
            } else if (type == "NiCamera") {
                nifDst->setLink(iChildDst, nifDst->getBlockNumber(niCamera(linkNode)));
            } else if (type == "NiTriShape") {
                setLink(iChildDst, niTriShapeAlt(linkNode));
            } else if (type == "BSStripParticleSystem") {
                setLink(iChildDst, bsStripParticleSystem(linkNode));
            } else if (type == "BSSegmentedTriShape") {
                setLink(iChildDst, bsSegmentedTriShape(linkNode));
            } else {
                qDebug() << __FUNCTION__ << "Unknown child type:" << type;

                conversionResult = false;
            }
        }

        // TODO: Num Effects
        // TODO: Effects

        if (blockType == "BSOrderedNode") {
            c.copyValue("Alpha Sort Bound");
            c.copyValue("Static Bound");
        } else if (blockType == "NiBillboardNode") {
            c.copyValue("Billboard Mode");
        } else if (blockType == "BSValueNode") {
            c.copyValue("Value");
            c.copyValue("Value Node Flags");
        } else if (
                    blockType == "BSDamageStage" ||
                    blockType == "BSBlastNode" ||
                    blockType == "BSDebrisNode") {
            c.copyValue("Min");
            c.copyValue("Max");
            c.copyValue("Current");
        } else if (blockType == "BSMasterParticleSystem") {
            c.copyValue("Max Emitter Objects");
            c.copyValue("Num Particle Systems");

            if (nifSrc->get<int>(iNode, "Num Particle Systems") > 0) {
                QModelIndex iParticleSystemsSrc = getIndexSrc(iNode, "Particle Systems");
                QModelIndex iParticleSystemsDst = getIndexDst(fadeNode, "Particle Systems");
                nifDst->updateArray(iParticleSystemsDst);

                for (int i = 0; i < nifSrc->rowCount(iParticleSystemsSrc); i++) {
                    reLink(iParticleSystemsDst.child(i, 0), iParticleSystemsSrc.child(i, 0));
                }

                c.processed(iParticleSystemsSrc.child(0, 0));
            }
        } else if (blockType == "BSMultiBoundNode") {
            setLink(fadeNode, "Multi Bound", bsMultiBound(getBlockSrc(iNode, "Multi Bound")));
            c.processed("Multi Bound");
        }

        return fadeNode;
    }

    void bsSegmentedTriShapeSegments(QModelIndex iDst, QModelIndex iSrc, Copier & c) {
        c.copyValue("Num Segments");
        c.copyValue("Total Segments", "Num Segments");

        QModelIndex iSegmentArrayDst = getIndexDst(iDst, "Segment");
        QModelIndex iSegmentArraySrc = getIndexSrc(iSrc, "Segment");
        uint numPrimitives = 0;

        nifDst->updateArray(iSegmentArrayDst);

        for (int i = 0; i < nifSrc->rowCount(iSegmentArraySrc); i++) {
            QModelIndex iSegmentDst = iSegmentArrayDst.child(i, 0);
            QModelIndex iSegmentSrc = iSegmentArraySrc.child(i, 0);

            c.copyValue(iSegmentDst, iSegmentSrc, "Start Index", "Index");
            c.copyValue(iSegmentDst, iSegmentSrc, "Num Primitives", "Num Tris in Segment");

            if (i == 0) {
                c.ignore(iSegmentSrc, "Flags");
            }

            numPrimitives += nifSrc->get<uint>(iSegmentSrc, "Num Tris in Segment");
        }

        nifDst->set<uint>(iDst, "Num Primitives", numPrimitives);
    }

    bool hasProperty(QModelIndex iSrc, const QString & name) {
        QVector<qint32> links = nifSrc->getLinkArray(iSrc, "Properties");

        for (qint32 link : links) {
            if (nifSrc->getBlockName(nifSrc->getBlock(link)).compare(name) == 0) {
                return true;
            }
        }

        return false;
    }

    QModelIndex bsSegmentedTriShape(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("BSSubIndexTriShape");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Name");
        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");

        QModelIndex iShaderPropertyDst = getShaderProperty(iSrc);

        setLink(iDst, "Shader Property", iShaderPropertyDst);
        properties(iSrc, iShaderPropertyDst, iDst, c);

        collisionObjectCopy(iDst, iSrc);
        c.processed("Collision Object");

        niTriShapeDataAlt(iDst, getBlockSrc(iSrc, "Data"));
        c.processed("Data");

        niSkinInstance(iDst, iShaderPropertyDst, getBlockSrc(iSrc, "Skin Instance"));
        c.processed("Skin Instance");

        materialData(iSrc, c);

        bsSegmentedTriShapeSegments(iDst, iSrc, c);

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex bsMultiBound(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("BSMultiBound");

        copyLink(iDst, iSrc, "Data");

        setHandled(iDst, iSrc);

        return iDst;
    }

    void materialData(QModelIndex iSrc, Copier & c) {
        QModelIndex iMaterialDataSrc = getIndexSrc(iSrc, "Material Data");

        c.ignore(iMaterialDataSrc, "Num Materials");

        if (nifSrc->get<uint>(iMaterialDataSrc, "Num Materials") > 0) {
            c.ignore(getIndexSrc(iMaterialDataSrc, "Material Name").child(0, 0));
            c.ignore(getIndexSrc(iMaterialDataSrc, "Material Extra Data").child(0, 0));
        }

        c.ignore(iMaterialDataSrc, "Active Material");
        c.ignore(iMaterialDataSrc, "Material Needs Update");
    }

    QModelIndex bsStripParticleSystem(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("BSStripParticleSystem");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);
        QModelIndex iShaderPropertyDst = getShaderProperty(iSrc);

        setLink(iDst, "Shader Property", iShaderPropertyDst);

        c.copyValue("Name");
        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");
        properties(iSrc, iShaderPropertyDst, iDst, c);
        collisionObjectCopy(iDst, iSrc);
        c.processed("Collision Object");
        niSkinInstance(iDst, iShaderPropertyDst, getBlockSrc(iSrc, "Skin Instance"));
        c.processed("Skin Instance");
        materialData(iSrc, c);
        c.copyValue("World Space");

        particleSystemModifiers(iDst, iSrc, c);

        setLink(iDst, "Data", bsStripPSysData(getBlockSrc(iSrc, "Data")));
        c.processed("Data");

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex bsStripPSysData(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("NiPSysData");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Group ID");

        // Causes crash
        c.ignore("BS Max Vertices");

        c.copyValue("Keep Flags");
        c.copyValue("Compress Flags");
        c.copyValue("Has Vertices");
        c.copyValue("BS Vector Flags");
        c.copyValue("Has Normals");
        c.copyValue("Center");
        c.copyValue("Radius");
        c.copyValue("Has Vertex Colors");
        c.copyValue("Consistency Flags");
        c.ignore("Additional Data");
        ignoreBlock(iSrc, "Additional Data", false);
        c.copyValue("Has Radii");
        c.copyValue("Num Active");
        c.copyValue("Has Sizes");
        c.copyValue("Has Rotations");
        c.copyValue("Has Rotation Angles");
        c.copyValue("Has Rotation Axes");
        c.copyValue("Has Texture Indices");
        c.copyValue<uint>("Num Subtexture Offsets");
        c.array("Subtexture Offsets");
        c.copyValue("Has Rotation Speeds");
        c.ignore("Max Point Count");
        c.ignore("Start Cap Size");
        c.ignore("End Cap Size");
        c.ignore("Do Z Prepass");

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex niCamera(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("NiCamera");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");
        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");
        properties(iDst, iSrc, c);
        collisionObjectCopy(iDst, iSrc);
        c.processed("Collision Object");
        c.copyValue("Camera Flags");
        c.copyValue("Frustum Left");
        c.copyValue("Frustum Right");
        c.copyValue("Frustum Top");
        c.copyValue("Frustum Bottom");
        c.copyValue("Frustum Near");
        c.copyValue("Frustum Far");
        c.copyValue("Use Orthographic Projection");
        c.copyValue("Viewport Left");
        c.copyValue("Viewport Right");
        c.copyValue("Viewport Top");
        c.copyValue("Viewport Bottom");
        c.copyValue("LOD Adjust");
        c.ignore("Scene");
        c.copyValue("Num Screen Polygons");
        c.copyValue("Num Screen Textures");

        setHandled(iDst, iSrc);

        return iDst;
    }

    void niTriShapeMaterialData(QModelIndex iDst, QModelIndex iSrc, Copier & c) {
        QModelIndex iMaterialDataDst = getIndexDst(iDst, "Material Data");
        QModelIndex iMaterialDataSrc = getIndexSrc(iSrc, "Material Data");

        c.setIDst(iMaterialDataDst);
        c.setISrc(iMaterialDataSrc);

        c.copyValue("Num Materials");
        // TODO: Convert material names.
        c.array("Material Name");
        c.array("Material Extra Data");
        c.copyValue("Active Material");
        c.copyValue("Material Needs Update");

        c.setIDst(iDst);
        c.setISrc(iSrc);
    }

    QModelIndex niTriShape(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("NiTriShape");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");

        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");

        QModelIndex iShaderPropertyDst = getShaderProperty(iSrc);

        setLink(iDst, "Shader Property", iShaderPropertyDst);

        properties(iSrc, iShaderPropertyDst, iDst, c);

        collisionObjectCopy(iDst, iSrc);
        c.ignore("Collision Object");

        setLink(iDst, "Data", niTriShapeData(getBlockSrc(iSrc, "Data")));
        c.ignore("Data");

        niTriShapeMaterialData(iDst, iSrc, c);

        niSkinInstance(iDst, iShaderPropertyDst, getBlockSrc(iSrc, "Skin Instance"));
        c.ignore("Skin Instance");

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex niTriShapeAlt(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("BSTriShape");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");

        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");

        QModelIndex iShaderPropertyDst = getShaderProperty(iSrc);

        setLink(iDst, "Shader Property", iShaderPropertyDst);

        properties(iSrc, iShaderPropertyDst, iDst, c);

        collisionObjectCopy(iDst, iSrc);
        c.ignore("Collision Object");

        niTriShapeDataAlt(iDst, getBlockSrc(iSrc, "Data"));
        c.processed("Data");

        niSkinInstance(iDst, iShaderPropertyDst, getBlockSrc(iSrc, "Skin Instance"));
        c.processed("Skin Instance");

        // TODO: Material Data
        QModelIndex iMaterialDataSrc = getIndexSrc(iSrc, "Material Data");
        c.ignore(iMaterialDataSrc, "Num Materials");
        c.ignore(iMaterialDataSrc, "Material Name");
        c.ignore(iMaterialDataSrc, "Material Extra Data");
        c.ignore(iMaterialDataSrc, "Active Material");
        c.ignore(iMaterialDataSrc, "Material Needs Update");

        setHandled(iDst, iSrc);

        return iDst;
    }

    // NOTE: Apply after vertices have been created for example by niTriShapeDataAlt()
    QModelIndex niSkinInstance(QModelIndex iBSTriShapeDst, QModelIndex iShaderPropertyDst, QModelIndex iSrc) {
        if (!iSrc.isValid()) {
            return QModelIndex();
        }

        QModelIndex iDst = nifDst->insertNiBlock("BSSkin::Instance");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.ignore("Data");
        c.ignore("Skin Partition");

        reLink(iDst, iSrc, "Skeleton Root");
        c.processed("Skeleton Root");

        c.copyValue("Num Bones");

        if (nifSrc->get<int>(iSrc, "Num Bones") > 0) {
            reLinkArray(iDst, iSrc, "Bones");
            c.processed(iSrc, "Bones");
        }

        setLink(iBSTriShapeDst, "Skin", iDst);

        BSVertexDesc vertexFlagsDst = nifDst->get<BSVertexDesc>(iBSTriShapeDst, "Vertex Desc");
        vertexFlagsDst.SetFlag(VertexFlags::VF_SKINNED);
        vertexFlagsDst.ResetAttributeOffsets(nifDst->getUserVersion2());

        bool ok;

        // Set shader and vertex flags.
        nifDst->set<BSVertexDesc>(iBSTriShapeDst, "Vertex Desc", vertexFlagsDst);
        nifDst->set<uint>(iShaderPropertyDst, "Shader Flags 1",
                    nifDst->get<uint>(iShaderPropertyDst, "Shader Flags 1") |
                    NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Skinned", &ok));

        if (!ok) {
            qDebug() << __FUNCTION__ << "Enum option not found";

            conversionResult = false;
        }

        ushort numVertices = nifDst->get<ushort>(iBSTriShapeDst, "Num Vertices");
        uint numTriangles = nifDst->get<uint>(iBSTriShapeDst, "Num Triangles");

        nifDst->updateArray(iBSTriShapeDst, "Vertex Data");
        nifDst->set<uint>(iBSTriShapeDst, "Data Size", vertexFlagsDst.GetVertexSize() * numVertices + numTriangles * 6);

        setLink(iDst, "Data", niSkinData(iBSTriShapeDst, getBlockSrc(iSrc, "Data")));
        // TODO: Set bone weights in vertex data

        // Only used for optimization?
        ignoreBlock(iSrc, "Skin Partition", false);

        if (nifSrc->getBlockName(iSrc) == "BSDismemberSkinInstance") {
            c.ignore("Num Partitions");

            if (nifSrc->get<int>(iSrc, "Num Partitions") > 0) {
                c.ignore(getIndexSrc(getIndexSrc(iSrc, "Partitions").child(0, 0), "Part Flag"));
                c.ignore(getIndexSrc(getIndexSrc(iSrc, "Partitions").child(0, 0), "Body Part"));
            }
        }

        setHandled(iDst, iSrc);

        return iDst;
    }

    // Has no visible effect source. Maybe just bounding spheres but also has rotation.
    void niSkinDataSkinTransform(QModelIndex iBoneDst, QModelIndex iSkinTransformSrc,  QModelIndex iSkinTransformGlobalSrc) {
        Matrix skinRotation = nifSrc->get<Matrix>(iSkinTransformGlobalSrc, "Rotation");
        Vector3 skinTranslation = nifSrc->get<Vector3>(iSkinTransformGlobalSrc, "Translation");
        float skinScale = nifSrc->get<float>(iSkinTransformGlobalSrc, "Scale");

        Matrix rotation = nifSrc->get<Matrix>(iSkinTransformSrc, "Rotation");
        for (uint r = 0; r < 3; r++) {
            for (uint c = 0; c < 3; c++) {
                rotation(r, c) += skinRotation(r, c);
            }
        }

        nifDst->set<Matrix>(iBoneDst, "Rotation", rotation);
        nifDst->set<Vector3>(iBoneDst, "Translation", nifSrc->get<Vector3>(iSkinTransformSrc, "Translation") + skinTranslation);
        nifDst->set<float>(iBoneDst, "Scale", nifSrc->get<float>(iSkinTransformSrc, "Scale") * skinScale);
    }

    QModelIndex niSkinData(QModelIndex iBSTriShapeDst, QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("BSSkin::BoneData");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Num Bones");

        QModelIndex iBoneListSrc = getIndexSrc(iSrc, "Bone List");
        QModelIndex iBoneListDst = getIndexDst(iDst, "Bone List");

        // Processed by niSkinDataSkinTransform()
        QModelIndex iSkinTransformGlobalSrc = getIndexSrc(iSrc, "Skin Transform");
        c.ignore(iSkinTransformGlobalSrc, "Rotation");
        c.ignore(iSkinTransformGlobalSrc, "Translation");
        c.ignore(iSkinTransformGlobalSrc, "Scale");


        nifDst->updateArray(iBoneListDst);

        for (int i = 0; i < nifSrc->rowCount(iBoneListSrc); i++) {
            QModelIndex iBoneDst = iBoneListDst.child(i, 0);
            QModelIndex iBoneSrc = iBoneListSrc.child(i, 0);
            QModelIndex iSkinTransformSrc = getIndexSrc(iBoneSrc, "Skin Transform");
            QModelIndex iBoundingSphereDst = getIndexDst(iBoneDst, "Bounding Sphere");

            c.copyValue(iBoneDst, iSkinTransformSrc, "Rotation");
            c.copyValue(iBoneDst, iSkinTransformSrc, "Translation");
            c.copyValue(iBoneDst, iSkinTransformSrc, "Scale");
            c.copyValue(iBoundingSphereDst, iBoneSrc, "Center", "Bounding Sphere Offset");
            c.copyValue(iBoundingSphereDst, iBoneSrc, "Radius", "Bounding Sphere Radius");
        }

        if (nifSrc->get<bool>(iSrc, "Has Vertex Weights")) {
            QModelIndex iVertexDataArrayDst = getIndexDst(iBSTriShapeDst, "Vertex Data");
            QMap<ushort, int> weightCounts;

            QModelIndex iBoneListArraySrc = getIndexSrc(iSrc, "Bone List");

            for (int i = 0; i < nifSrc->rowCount(iBoneListArraySrc); i++) {
                QModelIndex iBoneSrc = iBoneListArraySrc.child(i, 0);
                QModelIndex iVertexWeightsArraySrc = getIndexSrc(iBoneSrc, "Vertex Weights");

                for (int j = 0; j < nifSrc->rowCount(iVertexWeightsArraySrc); j++) {
                    QModelIndex iVertexWeightSrc = iVertexWeightsArraySrc.child(j, 0);

                    ushort vertexIndex = nifSrc->get<ushort>(iVertexWeightSrc, "Index");
                    float vertexWeight = nifSrc->get<float>(iVertexWeightSrc, "Weight");

                    if (weightCounts[vertexIndex] >= 4) {
                        // TODO: conversionResult = false;
                        if (!RUN_CONCURRENT) {
                            qDebug() << "Too many boneweights for one vertex. Blocknr.:" << nifSrc->getBlockNumber(iSrc);
                        }

                        continue;
                    }

                    QModelIndex iVertexDataDst = iVertexDataArrayDst.child(vertexIndex, 0);
                    QModelIndex iBoneWeightsDst = getIndexDst(iVertexDataDst, "Bone Weights");
                    QModelIndex iBoneIndicesDst = getIndexDst(iVertexDataDst, "Bone Indices");

                    nifDst->updateArray(iBoneWeightsDst);
                    nifDst->updateArray(iBoneIndicesDst);

                    int weightIndex = weightCounts[vertexIndex];

                    if (!iBoneWeightsDst.child(weightIndex, 0).isValid()) {
                        qDebug() << __FUNCTION__ << "Bone Weights not found";

                        conversionResult = false;

                        break;
                    }

                    if (!iBoneIndicesDst.child(weightIndex, 0).isValid()) {
                        qDebug() << __FUNCTION__ << "Bone Indices not found";

                        conversionResult = false;

                        break;
                    }

                    nifDst->set<ushort>(iBoneIndicesDst.child(weightIndex, 0), ushort(i));
                    nifDst->set<float>(iBoneWeightsDst.child(weightIndex, 0), vertexWeight);

                    weightCounts[vertexIndex]++;
                }
            }

            if (nifSrc->rowCount(iBoneListArraySrc) > 0) {
                c.processed(iBoneListArraySrc.child(0, 0), "Num Vertices");

                for (int i = 0; i < nifSrc->rowCount(iBoneListArraySrc); i++) {
                    QModelIndex iVertexWeightsArraySrc = getIndexSrc(iBoneListArraySrc.child(i, 0), "Vertex Weights");

                    if (nifSrc->rowCount(iVertexWeightsArraySrc) > 0) {
                        c.processed(iVertexWeightsArraySrc.child(0, 0), "Index");
                        c.processed(iVertexWeightsArraySrc.child(0, 0), "Weight");

                        break;
                    }
                }
            }
        }

        c. processed("Has Vertex Weights");

        setHandled(iDst, iSrc);

        return iDst;
    }

    void niTriShapeDataArray(const QString & name, bool bHasArray, Copier & c) {
        if (!bHasArray) {
            return;
        }

        c.array(name);
    }

    void niTriShapeDataArray(QModelIndex iSrc, const QString & name, const QString & boolName, Copier & c, bool isFlag = false) {
        bool isSet = false;

        if (!isFlag) {
            if (nifSrc->get<bool>(iSrc, boolName)) {
                c.copyValue<bool>(boolName);
                isSet = true;
            } else {
                c.ignore(boolName);
            }
        } else {
            // Use BS Vector Flags

            bool ok = false;
            isSet = nifSrc->get<ushort>(iSrc, "BS Vector Flags") & NifValue::enumOptionValue("BSVectorFlags", boolName, &ok);

            if (!ok) {
                qDebug() << __FUNCTION__ << "Enum Option" << boolName << "not found";
            }
        }

        niTriShapeDataArray(name, isSet, c);
    }

    QModelIndex niTriShapeData(QModelIndex iSrc) {
        QModelIndex iDst = nifDst->insertNiBlock("NiTriShapeData");

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Group ID");
        c.copyValue("Num Vertices");
        c.copyValue("Keep Flags");
        c.copyValue("Compress Flags");
        niTriShapeDataArray(iSrc, "Vertices", "Has Vertices", c);
        c.copyValue("BS Vector Flags");
        niTriShapeDataArray(iSrc, "Normals", "Has Normals", c);
        niTriShapeDataArray(iSrc, "Tangents", "Has_Tangents", c, true);
        niTriShapeDataArray(iSrc, "Bitangents", "Has_Tangents", c, true);
        c.copyValue("Center");
        c.copyValue("Radius");
        niTriShapeDataArray(iSrc, "Vertex Colors", "Has Vertex Colors", c);
        niTriShapeDataArray(iSrc, "UV Sets", "Has_UV", c, true);
        c.copyValue("Consistency Flags");
        // TODO: Additional Data
        c.ignore("Additional Data");
        ignoreBlock(iSrc, "Additional Data", false);
        c.copyValue("Num Triangles");
        c.copyValue("Num Triangle Points");
        niTriShapeDataArray(iSrc, "Triangles", "Has Triangles", c);
        c.copyValue("Num Match Groups");
        c.array("Match Groups");

        setHandled(iDst, iSrc);

        return iDst;
    }

    QModelIndex niPointLight(QModelIndex iSrc) {
        QString type = nifSrc->getBlockName(iSrc);

        // Copies up to "Num Properties"
        QModelIndex iDst = nifDst->insertNiBlock(type);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");

        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");

        // Values copied up to here

        properties(iDst, iSrc, c);

        // Collision Object
        nifDst->setLink(iDst, "Collision Object", -1);
        collisionObjectCopy(iDst, iSrc);
        c.processed("Collision Object");

        c.ignore("Switch State");
        c.ignore("Num Affected Nodes");

        if (nifSrc->get<int>(iSrc, "Num Affected Nodes") > 0) {
            c.ignore(getIndexSrc(iSrc, "Affected Nodes").child(0, 0));
        }

        c.copyValue("Dimmer");
        c.copyValue("Ambient Color");
        c.copyValue("Diffuse Color");
        c.copyValue("Specular Color");

        if (type == "NiPointLight") {
            c.copyValue("Constant Attenuation");
            c.copyValue("Linear Attenuation");
            c.copyValue("Quadratic Attenuation");
        }

        setHandled(iDst, iSrc);

        return iDst;
    }

    QString updateTexturePath(QString fname) {
        if (fname == "") {
            return "";
        }

        int offset = int(strlen(TEXTURE_ROOT));

        if (fname.left(5).compare("Data\\", Qt::CaseInsensitive) == 0) {
            fname.remove(0, 5);
        }

        return fname.insert(offset, TEXTURE_FOLDER);
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

    void setFallout4ShaderFlag(uint & flags, const QString & enumName, const QString & optionName) {
        flags |= bsShaderFlagsGet(enumName, optionName);
    }

    void setFallout4ShaderFlag1(uint & flags, const QString & optionName) {
        setFallout4ShaderFlag(flags, "Fallout4ShaderPropertyFlags1", optionName);
    }

    void setFallout4ShaderFlag2(uint & flags, const QString & optionName) {
        setFallout4ShaderFlag(flags, "Fallout4ShaderPropertyFlags2", optionName);
    }

    uint bsShaderFlagsGet(const QString & enumName, const QString & optionName) {
        bool ok = false;

        uint result = NifValue::enumOptionValue(enumName, optionName, &ok);

        if (!ok) {
            qDebug() << __FUNCTION__ << "Failed to find option:" << optionName;

            conversionResult = false;
        }

        return result;
    }

    uint bsShaderFlags1Get(QString optionName) {
        return bsShaderFlagsGet("BSShaderFlags", optionName);
    }

    uint bsShaderFlags2Get(QString optionName) {
        return bsShaderFlagsGet("BSShaderFlags2", optionName);
    }

    bool bsShaderFlagsIsSet(uint shaderFlags, const QString & enumNameSrc, const QString & optionNameSrc) {
        return shaderFlags & bsShaderFlagsGet(enumNameSrc, optionNameSrc);
    }

    bool bsShaderFlags1IsSet(uint shaderFlags, QString optionName) {
        return bsShaderFlagsIsSet(shaderFlags, "BSShaderFlags", optionName);
    }

    bool bsShaderFlags1IsSet(QModelIndex iShader, QString optionName) {
        return bsShaderFlagsIsSet(nifSrc->get<uint>(iShader, "Shader Flags"), "BSShaderFlags", optionName);
    }

    bool bsShaderFlags2IsSet(uint shaderFlags, QString optionName) {
        return bsShaderFlagsIsSet(shaderFlags, "BSShaderFlags2", optionName);
    }

    bool bsShaderFlags2IsSet(QModelIndex iShader, QString optionName) {
        return bsShaderFlagsIsSet(nifSrc->get<uint>(iShader, "Shader Flags 2"), "BSShaderFlags2", optionName);
    }

    void bsShaderFlagsSet(
            uint shaderFlagsSrc,
            uint & flagsDst,
            const QString & optionNameDst,
            const QString & optionNameSrc,
            const QString & enumNameDst,
            const QString & enumNameSrc) {
        if (bsShaderFlagsIsSet(shaderFlagsSrc, enumNameSrc, optionNameSrc)) {
            bsShaderFlagsAdd(flagsDst, optionNameDst, enumNameDst);
        }
    }

    void bsShaderFlagsAdd(uint & flagsDst, const QString & optionNameDst, const QString & enumNameDst) {
        bool ok = false;
        uint flag = NifValue::enumOptionValue(enumNameDst, optionNameDst, & ok);

        if (!ok) {
            qDebug() << __FUNCTION__ << "Failed to find option:" << optionNameDst;

            conversionResult = false;
        }

        // Set the flag
        flagsDst |= flag;
    }

    void bsShaderFlags1Add(uint & flagsDst, const QString & optionNameDst) {
        bsShaderFlagsAdd(flagsDst, optionNameDst, "Fallout4ShaderPropertyFlags1");
    }

    void bsShaderFlagsAdd(QModelIndex iShaderFlags, const QString & enumName, const QString & option) {
        uint flags = nifDst->get<uint>(iShaderFlags);

        bsShaderFlagsAdd(flags, option, enumName);

        nifDst->set<uint>(iShaderFlags, flags);
    }

    void bsShaderFlagsAdd(QModelIndex iDst, const QString & name, const QString & enumName, const QString & option) {
        bsShaderFlagsAdd(getIndexDst(iDst, name), enumName, option);
    }

    void bsShaderFlags1Add(QModelIndex iDst, const QString & option) {
        bsShaderFlagsAdd(iDst, "Shader Flags 1", "Fallout4ShaderPropertyFlags1", option);
    }

    void bsShaderFlags2Add(QModelIndex iDst, const QString & option) {
        bsShaderFlagsAdd(iDst, "Shader Flags 2", "Fallout4ShaderPropertyFlags2", option);
    }

    void bsShaderFlags2Add(uint & flagsDst, const QString & optionNameDst) {
        bsShaderFlagsAdd(flagsDst, optionNameDst, "Fallout4ShaderPropertyFlags2");
    }

    void bsShaderFlags1Set(uint shaderFlagsSrc, uint & flagsDst, const QString & nameDst, const QString & nameSrc) {
        bsShaderFlagsSet(shaderFlagsSrc, flagsDst, nameDst, nameSrc, "Fallout4ShaderPropertyFlags1", "BSShaderFlags");
    }

    void bsShaderFlags2Set(uint shaderFlagsSrc, uint & flagsDst, const QString & nameDst, const QString & nameSrc) {
        bsShaderFlagsSet(shaderFlagsSrc, flagsDst, nameDst, nameSrc, "Fallout4ShaderPropertyFlags2", "BSShaderFlags2");
    }

    void bsShaderFlags1Set(uint shaderFlagsSrc, uint & flagsDst, const QString & name) {
        bsShaderFlags1Set(shaderFlagsSrc, flagsDst, name, name);
    }

    void bsShaderFlags2Set(uint shaderFlagsSrc, uint & flagsDst, const QString & name) {
        bsShaderFlags2Set(shaderFlagsSrc, flagsDst, name, name);
    }

    QString bsShaderTypeGet(QModelIndex iDst) {
        return NifValue::enumOptionName("BSLightingShaderPropertyShaderType", nifDst->get<uint>(iDst));
    }

    void bsShaderTypeSet(QModelIndex iDst, const QString & name) {
        bool ok = false;

        nifDst->set<uint>(iDst, NifValue::enumOptionValue("BSLightingShaderPropertyShaderType", name, &ok));

        if (!ok) {
            qDebug() << "Failed to find enum option" << name;

            conversionResult = false;
        }
    }

    // NOTE: Finalized in Properties()
    // TODO: Set emit flag when appropriate. Creation kit currently notifies sometimes when not set.
    void bsShaderFlags(QModelIndex iShaderPropertyDst, uint shaderFlags1Src, uint shaderFlags2Src) {
        if (!iShaderPropertyDst.isValid()) {
            return;
        }

        uint flags1Dst = nifDst->get<uint>(iShaderPropertyDst, "Shader Flags 1");

        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Specular");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Skinned");
        // LowDetail
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Vertex_Alpha");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "GreyscaleToPalette_Color", "Unknown_1");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "GreyscaleToPalette_Alpha", "Single_Pass");
        // Empty
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Environment_Mapping");
        if (bsShaderFlags1IsSet(shaderFlags1Src, "Environment_Mapping")) {
            if (bsShaderTypeGet(getIndexDst(iShaderPropertyDst, "Skyrim Shader Type")).compare("Default") != 0) {
                qDebug() << __FUNCTION__ "Shader Type already set";

                conversionResult = false;
            }

            bsShaderTypeSet(getIndexDst(iShaderPropertyDst, "Skyrim Shader Type"), "Environment Map");
        }

        // Alpha_Texture
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Cast_Shadows", "Unknown_2");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Face", "FaceGen");
        // Parallax_Shader_Index_15
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Model_Space_Normals", "Unknown_3");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Non_Projective_Shadows");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Landscape", "Unknown_4");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Refraction");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Fire_Refraction");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Eye_Environment_Mapping");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Hair");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Screendoor_Alpha_Fade", "Dynamic_Alpha");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Localmap_Hide_Secret");
        // Window_Environment_Mapping
        // Tree_Billboard
        // Shadow_Frustum
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Multiple_Textures");
        // Remappable_Textures
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Decal", "Decal_Single_Pass");
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "Dynamic_Decal", "Dynamic_Decal_Single_Pass");
        // Parallax_Occulsion
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "External_Emittance");
        // Shadow_Map
        bsShaderFlags1Set(shaderFlags1Src, flags1Dst, "ZBuffer_Test");

        uint flags2Dst = nifDst->get<uint>(iShaderPropertyDst, "Shader Flags 2");

        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "ZBuffer_Write");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "LOD_Landscape");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "LOD_Objects", "LOD_Building");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "No_Fade");
        // Refraction_Tint
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Vertex_Colors");
        // Unknown 1
        // 1st_Light_is_Point_Light
        // 2nd_Light
        // 3rd_Light
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Grass_Vertex_Lighting", "Vertex_Lighting");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Grass_Uniform_Scale", "Uniform_Scale");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Grass_Fit_Slope", "Fit_Slope");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Grass_Billboard", "Billboard_and_Envmap_Light_Fade");
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "No_LOD_Land_Blend");
        // Envmap_Light_Fade
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Wireframe");
        // VATS_Selection
        if (!bsShaderFlags2IsSet(shaderFlags2Src, "Show_in_Local_Map")) {
            setFallout4ShaderFlag2(flags2Dst, "Hide_On_Local_Map");
        }
        bsShaderFlags2Set(shaderFlags2Src, flags2Dst, "Premult_Alpha");
        // Skip_Normal_Maps
        // Alpha_Decal
        // No_Transparecny_Multisampling
        // Unknown 2
        // Unknown 3
        // Unknown 4
        // Unknown 5
        // Unknown 6
        // Unknown 7
        // Unknown 8
        // Unknown 9
        // Unknown 10

        if (nifDst->getBlockName(iShaderPropertyDst).compare("BSEffectShaderProperty") == 0) {
            bsShaderFlags1Add(flags1Dst, "Use_Falloff");
            bsShaderFlags2Add(flags2Dst, "Double_Sided");
        }

        nifDst->set<uint>(iShaderPropertyDst, "Shader Flags 1", flags1Dst);
        nifDst->set<uint>(iShaderPropertyDst, "Shader Flags 2", flags2Dst);

        if (bsShaderFlags2IsSet(shaderFlags2Src, "LOD_Landscape")) {
            bLODLandscape = true;
        }

        if (bsShaderFlags2IsSet(shaderFlags2Src, "LOD_Building")) {
            bLODBuilding = true;
        }
    }

    // TODO: Use enumOption
    int getFlagsBSShaderFlags1(QModelIndex iDst, QModelIndex iNiTriStripsData, QModelIndex iBSShaderPPLightingProperty) {
        if (!iBSShaderPPLightingProperty.isValid()) {
            return nifDst->get<int>(iDst, "Shader Flags 1");
        }

        int flags = 0;

        if (iNiTriStripsData.isValid()) {
            // Set Model_Space_Normals in case there are no tangents
            if (!(nifSrc->get<int>(iNiTriStripsData, "BS Vector Flags") & (1 << 12))) {
                flags = flags | (1 << 12);
            }
        }

        // Add the currently set flags
        flags = flags | nifDst->get<int>(iDst, "Shader Flags 1");

        return flags;
    }

    void bSShaderLightingProperty(QModelIndex iDst, QModelIndex iSrc, const QString & sequenceBlockName) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.ignore("Name");
        c.ignore("Num Extra Data List");

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", sequenceBlockName);

        c.ignore("Flags");
        c.ignore("Shader Type");
        c.ignore("Shader Flags");
        c.ignore("Environment Map Scale");
        c.ignore("Texture Clamp Mode");

//        c.copyValue("Texture Clamp Mode");
        nifDst->set<int>(iDst, "Texture Clamp Mode", 3);

        c.ignore("Shader Flags 2");

        const QString blockName = nifDst->getBlockName(iDst);
        const QString shaderTypeSrc = nifSrc->getBlockName(iSrc);

        if (blockName == "BSLightingShaderProperty") {
            if (shaderTypeSrc == "BSShaderPPLightingProperty") {
                QModelIndex iTextureSetSrc = nifSrc->getBlock(nifSrc->getLink(iSrc, "Texture Set"));
                QModelIndex iTextureSet = copyBlock(iDst, iTextureSetSrc);
                c.ignore("Texture Set");

                bsShaderTextureSet(iTextureSet, iTextureSetSrc);
                nifDst->setLink(iDst, "Texture Set", nifDst->getBlockNumber(iTextureSet));

                if (nifSrc->getUserVersion2() == 34) {
                    c.copyValue("Refraction Strength");
                    c.ignore("Refraction Fire Period");

                    c.ignore("Parallax Max Passes");
                    c.ignore("Parallax Scale");
                }
            } else if (shaderTypeSrc == "BSShaderNoLightingProperty") {
//                QString fileNameSrc = nifSrc->string(iSrc, QString("File Name"));
                QString fileNameSrc = c.getSrc<QString>("File Name");

                if (fileNameSrc.length() > 0) {
                    QModelIndex iTextureSetDst = nifDst->insertNiBlock("BSShaderTextureSet");

                    nifDst->set<int>(iTextureSetDst, "Num Textures", 10);
                    nifDst->updateArray(nifDst->getIndex(iTextureSetDst, "Textures"));
                    nifDst->set<QString>(nifDst->getIndex(iTextureSetDst, "Textures").child(0, 0), updateTexturePath(fileNameSrc));
                    nifDst->setLink(iDst, "Texture Set", nifDst->getBlockNumber(iTextureSetDst));
                }
            }
        } else if (blockName == "BSEffectShaderProperty") {
            if (shaderTypeSrc == "BSShaderPPLightingProperty") {
                QModelIndex iTextureSetSrc = getBlockSrc(iSrc, "Texture Set");

                if (iTextureSetSrc.isValid()) {
                    if (nifSrc->get<int>(iTextureSetSrc, "Num Textures") < 6) {
                        qDebug() << __FUNCTION__ << "Texture set too small";

                        conversionResult = false;

                        return;
                    }

                    QModelIndex iTexturesArraySrc = getIndexSrc(iTextureSetSrc, "Textures");

                    const QString textureMain =
                            updateTexturePath(nifSrc->get<QString>(iTexturesArraySrc.child(0, 0)));
                    const QString textureNormal =
                            updateTexturePath(nifSrc->get<QString>(iTexturesArraySrc.child(1, 0)));
                    // TODO: Glow map and Height/Parallax map.
                    const QString textureEnvironment =
                            updateTexturePath(nifSrc->get<QString>(iTexturesArraySrc.child(4, 0)));
                    const QString textureEnvMask =
                            updateTexturePath(nifSrc->get<QString>(iTexturesArraySrc.child(5, 0)));

                    nifDst->set<QString>(iDst, "Source Texture", textureMain);
                    nifDst->set<QString>(iDst, "Normal Texture", textureNormal);
                    nifDst->set<QString>(iDst, "Env Map Texture", textureEnvironment);
                    nifDst->set<QString>(iDst, "Env Mask Texture", textureEnvMask);

                    setHandled(iDst, iTextureSetSrc);
                }

                c.processed("Texture Set");

                c.ignore("Refraction Strength");
                c.ignore("Refraction Fire Period");
                c.ignore("Parallax Max Passes");
                c.ignore("Parallax Scale");

                // Needs to be set after material is processed
                nifDst->set<Color4>(iDst, "Emissive Color", Color4(1, 1, 1, 1));

                nifDst->set<float>(iDst, "Falloff Start Angle", 1);
                nifDst->set<float>(iDst, "Falloff Stop Angle", 1);
                nifDst->set<float>(iDst, "Falloff Start Opacity", 1);
                nifDst->set<float>(iDst, "Falloff Stop Opacity", 1);
            } else if (shaderTypeSrc == "BSShaderNoLightingProperty") {
                c.ignore("File Name");

                if (nifSrc->getUserVersion2() == 34) {
                    // Does not appear to be used
                    c.ignore("Falloff Start Angle");
                    c.ignore("Falloff Stop Angle");
                    c.ignore("Falloff Start Opacity");
                    c.ignore("Falloff Stop Opacity");

                    nifDst->set<float>(iDst, "Falloff Start Angle", 1);
                    nifDst->set<float>(iDst, "Falloff Stop Angle", 1);
                    nifDst->set<float>(iDst, "Falloff Start Opacity", 1);
                    nifDst->set<float>(iDst, "Falloff Stop Opacity", 1);
                }
            }
        }

        setHandled(iDst, iSrc);
    }

    QModelIndex getShaderProperty(QModelIndex iSrc) {
        QModelIndex iResult;

        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iSrc));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            QString type = nifSrc->getBlockName(linkNode);

            if (type == "BSShaderNoLightingProperty") {
                // TODO: Set falloff opacity to 1 on all unless falloff flags is set in source and always set use_falloff flag
                // TODO: Set Double sided on always
                iResult = nifDst->insertNiBlock("BSEffectShaderProperty");
            } else if (type == "BSShaderPPLightingProperty") {
                bool bAlphaBlending = false;
                bool isDecal = false;

                for (int j = 0; j < links.count(); j++) {
                    QModelIndex iAlphaBlockSrc = nifSrc->getBlock(links[j]);

                    if (nifSrc->getBlockName(iAlphaBlockSrc) != "NiAlphaProperty") {
                        continue;
                    }

                    if (nifSrc->get<int>(iAlphaBlockSrc, "Flags") & 1) {
                        bAlphaBlending = true;
                    }
                }

                if (
                        bsShaderFlags1IsSet(linkNode, "Decal_Single_Pass") ||
                        bsShaderFlags1IsSet(linkNode, "Dynamic_Decal_Single_Pass")) {
                    isDecal = true;
                }

                /**
                 * NOTE: Couldn't find a way to have alpha blending on a BSLightingShaderProperty and not have the shape
                 *       turn invisible unless a decal flag was set.
                 */
                if (bAlphaBlending && !isDecal) {
                    iResult = nifDst->insertNiBlock("BSEffectShaderProperty");
                } else {
                    iResult = nifDst->insertNiBlock("BSLightingShaderProperty");

                    nifDst->set<float>(iResult, "Specular Strength", 0);
                }
            } else if (type == "TallGrassShaderProperty") {
                iResult = nifDst->insertNiBlock("BSLightingShaderProperty");

                nifDst->set<float>(iResult, "Specular Strength", 0);
            } else if (type == "SkyShaderProperty") {
                iResult = nifDst->insertNiBlock("BSSkyShaderProperty");
            } else if (type == "WaterShaderProperty") {
                iResult = nifDst->insertNiBlock("BSWaterShaderProperty");
            }
        }

        if (!iResult.isValid()) {
            iResult = nifDst->insertNiBlock( "BSEffectShaderProperty" );
        }

        nifDst->set<qint32>(iResult, "Shader Flags 1", 0);
        nifDst->set<qint32>(iResult, "Shader Flags 2", 0);

        return iResult;
    }

    QModelIndex getShaderPropertySrc(QModelIndex iSrc) {
        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iSrc));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            QString type = nifSrc->getBlockName(linkNode);

            if (type == "BSShaderNoLightingProperty" || type == "BSShaderPPLightingProperty") {
                return linkNode;
            }
        }

        return QModelIndex();
    }

    void properties(QModelIndex iSrc, QModelIndex iShaderPropertyDst, QModelIndex iDst) {
        QVector<qint32> links = nifSrc->getLinkArray(iSrc, "Properties");
        QString dstType = nifDst->getBlockName(iDst);
        QModelIndex iNiAlphaPropertyDst;
        QModelIndex iShaderPropertySrc;

        for (int link:links) {
            QModelIndex iPropertySrc = nifSrc->getBlock(link);
            QModelIndex iPropertyDst;

            QString type = nifSrc->getBlockName(iPropertySrc);;

            if (type == "NiAlphaProperty") {
                iPropertyDst = copyBlock(QModelIndex(), iPropertySrc);

                if (!setLink(iDst, "Alpha Property", iPropertyDst)) {
                    qDebug() << __FUNCTION__ << "no way to handle" << type << "for" << dstType;

                    conversionResult = false;
                }

                iNiAlphaPropertyDst = iPropertyDst;
            } else {
                // Shader property properties

                if (!iShaderPropertyDst.isValid()) {
                    qDebug() << __FUNCTION__ << "No Shader Property - required for:" << type;

                    conversionResult = false;

                    return;
                }

                if (type == "BSShaderPPLightingProperty" || type == "BSShaderNoLightingProperty") {
                    bSShaderLightingProperty(iShaderPropertyDst, iPropertySrc, nifSrc->get<QString>(iSrc, "Name"));

                    iShaderPropertySrc = iPropertySrc;
                } else if (type == "TileShaderProperty" ||
                           type == "TallGrassShaderProperty" ||
                           type == "SkyShaderProperty" ||
                           type == "WaterShaderProperty") {
                    shaderProperty(iShaderPropertyDst, iPropertySrc, type, nifSrc->get<QString>(iSrc, "Name"));

                    iShaderPropertySrc = iPropertySrc;
                } else if (type == "NiMaterialProperty") {
                    niMaterialProperty(iShaderPropertyDst, iPropertySrc, nifSrc->get<QString>(iSrc, "Name"));
                } else if (type == "NiTexturingProperty") {
                    // Needs to be copied
                    niTexturingProperty(iShaderPropertyDst, iPropertySrc, nifSrc->get<QString>(iSrc, "Name"));
                } else if (type == "NiStencilProperty") {
                    // TODO: NiStencilProperty
                    setHandled(QModelIndex(), iPropertySrc);
                } else {
                    qDebug() << __FUNCTION__ << "Unknown Property:" << nifSrc->getBlockName(iPropertySrc);

                    conversionResult = false;
                }
            }
        }

        if (iNiAlphaPropertyDst.isValid() || iShaderPropertySrc.isValid()) {
            bsShaderFlags(
                    iShaderPropertyDst,
                    nifSrc->get<uint>(iShaderPropertySrc, "Shader Flags"),
                    nifSrc->get<uint>(iShaderPropertySrc, "Shader Flags 2"));
        }

        if (!iShaderPropertySrc.isValid()) {
            QModelIndex iEmissive = getIndexDst(iShaderPropertyDst, "Emissive Color");
            Color4 colorEmmissive = nifDst->get<Color4>(iEmissive);

            colorEmmissive.setAlpha(0);
            nifDst->set<Color4>(iEmissive, colorEmmissive);
        }

        if (iNiAlphaPropertyDst.isValid() && iShaderPropertySrc.isValid()) {
            if (
                    nifSrc->getBlockName(iShaderPropertySrc) == "BSShaderPPLightingProperty" &&
                    nifDst->getBlockName(iShaderPropertyDst) == "BSEffectShaderProperty") {
                bsShaderFlags2Add(iShaderPropertyDst, "Effect_Lighting");
                nifDst->set<Color4>(iShaderPropertyDst, "Emissive Color", Color4(1, 1, 1, 1));
            }
        }
    }

    void niAlphaPropertyFinalize(QModelIndex iAlphaPropertyDst, QModelIndex iShaderPropertyDst) {
        if (nifDst->getBlockName(iShaderPropertyDst) == "BSLightingShaderProperty") {
            QModelIndex iAlphaFlags = getIndexDst(iAlphaPropertyDst, "Flags");
            uint alphaFlags = nifDst->get<uint>(iAlphaFlags);

            if (alphaFlags & 1) {
                bsShaderFlags1Add(iShaderPropertyDst, "Decal");
                bsShaderFlags1Add(iShaderPropertyDst, "Dynamic_Decal");
            }

//            // Disable Blending
//            alphaFlags &= uint(~1);

//            nifDst->set<uint>(iAlphaFlags, alphaFlags);
        }

    }

    void shaderProperty(QModelIndex iDst, QModelIndex iSrc, const QString & type, const QString & sequenceBlockName) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.ignore("Name");
        extraDataList(iDst, iSrc, c);

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        niController(iDst, iSrc, c, "Controller", sequenceBlockName);

        c.ignore("Flags");
        c.ignore("Shader Type");

        // Processed by properties()
        c.processed("Shader Flags");
        c.processed("Shader Flags 2");

        if (
                type != "WaterShaderProperty" &&
                type != "TallGrassShaderProperty") {
            if (!nifDst->set<QString>(iDst, "Source Texture", updateTexturePath(nifSrc->get<QString>(iSrc, "File Name")))) {
                qDebug() << __FUNCTION__ << "Failed to set source texture on:" << nifDst->getBlockName(iDst);

                conversionResult = false;
            }

            c.processed("File Name");
        }

        if (type == "tileShaderProperty") {
            c.copyValue<int>("Texture Clamp Mode");
        }

        if (type == "SkyShaderProperty") {
            c.ignore("Texture Clamp Mode");
            c.ignore("Environment Map Scale");
            c.copyValue("Sky Object Type");
        } else if (type == "WaterShaderProperty") {
            float scale = nifSrc->get<float>(iSrc, "Environment Map Scale");

            if (scale != 1.0f) {
                nifDst->set<Vector2>(iDst, "UV Scale", Vector2(1, 1) * scale);
            }

            c.processed("Environment Map Scale");
        } else if (type == "TallGrassShaderProperty") {
            QModelIndex iTextureSetDst = nifDst->insertNiBlock("BSShaderTextureSet");
            QModelIndex iTextureArray = getIndexDst(iTextureSetDst, "Textures");

            nifDst->set<int>(iTextureSetDst, "Num Textures", 10);
            nifDst->updateArray(iTextureArray);
            nifDst->set<QString>(iTextureArray.child(0, 0), updateTexturePath(c.getSrc<QString>("File Name")));
            setLink(iDst, "Texture Set", iTextureSetDst);
        } else {
            c.copyValue("Environment Map Scale");
        }

        setHandled(iDst, iSrc);
    }

    void properties(QModelIndex iSrc, QModelIndex shaderProperty, QModelIndex iDst, Copier & c) {
        c.ignore("Num Properties");

        if (nifSrc->get<int>(iSrc, "Num Properties") > 0) {
            c.ignore(getIndexSrc(iSrc, "Properties").child(0, 0));

            properties(iSrc, shaderProperty, iDst);
        }
    }

    void properties(QModelIndex iDst, QModelIndex iSrc) {
        QVector<qint32> links = nifSrc->getLinkArray(iSrc, "Properties");

        QString dstType = nifDst->getBlockName(iDst);

        for (int link:links) {
            QModelIndex iPropertySrc = nifSrc->getBlock(link);
            QModelIndex iPropertyDst;

            QString type = nifSrc->getBlockName(iPropertySrc);

            if (type == "NiAlphaProperty") {
                iPropertyDst = copyBlock(QModelIndex(), iPropertySrc);

                if (dstType == "NiTriShape") {
                    setLink(iDst, "Alpha Property", iPropertyDst);
                } else {
                    qDebug() << __FUNCTION__ << "no way to handle" << type << "for" << dstType;

                    conversionResult = false;
                }
            } else {
                qDebug() << "Unknown Property block:" << type;

                conversionResult = false;
            }
        }

//        for (int i = 0; i < nifSrc->rowCount(iSrc); i++) {
////            QModelIndex linkNode = iSrc.child(i, 0);
//            QModelIndex iPropertySrc = nifSrc->getBlock(nifSrc->getLink(iSrc.child(i, 0)));
//            QString type = nifSrc->getBlockName(iPropertySrc);

//            if (type == "NiAlphaProperty") {
//                int lAlphaProperty = nifDst->getBlockNumber(copyBlock(QModelIndex(), iPropertySrc));

//                if (triShape.isValid()) {
//                    nifDst->setLink(triShape, "Alpha Property", lAlphaProperty);
//                } else {
//                    nifDst->setLink(shaderProperty, "Alpha Property", lAlphaProperty);
//                }
//            } else if (type == "BSShaderPPLightingProperty" || type == "BSShaderNoLightingProperty") {
//                bSShaderLightingProperty(shaderProperty, iPropertySrc);
//                iBSShaderLightingProperty = iPropertySrc;
//            } else if (type == "NiMaterialProperty") {
//                niMaterialProperty(shaderProperty, iPropertySrc);
//            } else if (type == "NiTexturingProperty") {
//                // Needs to be copied
//                niTexturingProperty(shaderProperty, iPropertySrc);
//            } else if (type == "NiStencilProperty") {
//                // TODO: NiStencilProperty
//                setHandled(QModelIndex(), iPropertySrc);
//            } else {
//                qDebug() << __FUNCTION__ << "Unknown Property:" << nifSrc->getBlockName(iPropertySrc);

//                conversionResult = false;
//            }
//        }
    }

    void properties(QModelIndex iDst, QModelIndex iSrc, Copier & c) {
        c.ignore("Num Properties");

        if (nifSrc->get<int>(iSrc, "Num Properties") > 0) {
            c.ignore(getIndexSrc(iSrc, "Properties").child(0, 0));
        }

        properties(iDst, iSrc);
    }

    QModelIndex niTriStrips( QModelIndex iSrc) {
        const QModelIndex iDst = nifDst->insertNiBlock( "BSTriShape" );

        // This block might be a controlled block in which case it will be finalized later.
        progress.ignoreNextIncrease();
        setHandled(iDst, iSrc);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");
        if (nifDst->string(iDst, QString("Name")).length() == 0) {
            qDebug() << "triShape has no name!";

            conversionResult = false;
        }

        niController(iDst, iSrc, c, "Controller", nifSrc->get<QString>(iSrc, "Name"));

        QModelIndex shaderProperty = getShaderProperty(iSrc);
        QModelIndex iBSShaderLightingPropertySrc = getShaderPropertySrc(iSrc);

        properties(iSrc, shaderProperty, iDst, c);

        setLink( iDst, "Shader Property", shaderProperty);

        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue<uint>("Flags");
        c.copyValue("Scale");

        c.ignore("Collision Object");
        c.ignore("Data");
        c.ignore("Num Extra Data List");

        QModelIndex iMaterialDataSrc = getIndexSrc(iSrc, "Material Data");

        if (nifSrc->getUserVersion2() == 11) {
            c.ignore(iMaterialDataSrc, "Has Shader");

            if (nifSrc->get<bool>(iMaterialDataSrc, "Has Shader")) {
                c.ignore(iMaterialDataSrc, "Shader Name");
                c.ignore(iMaterialDataSrc, "Shader Extra Data");
            }
        } else {
            c.ignore(iMaterialDataSrc, "Num Materials");
            c.ignore(iMaterialDataSrc, "Active Material");
            c.ignore(iMaterialDataSrc, "Material Needs Update");
        }

        extraDataList(iDst, iSrc, c);

        QModelIndex iNiTriStripsData;

        // TODO: Go per item
        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iSrc));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            QString type = nifSrc->getBlockName(linkNode);

            if (type == "NiTriStripsData") {
                niTriStripsData(linkNode, iDst);

                iNiTriStripsData = linkNode;
            }
        }

//        properties(nifSrc->getIndex(iSrc, "Properties"));

        // Multinode dependant values

        // Shader Flags
        nifDst->set<int>(shaderProperty, "Shader Flags 1", getFlagsBSShaderFlags1(shaderProperty, iNiTriStripsData, iBSShaderLightingPropertySrc));

        niSkinInstance(iDst, shaderProperty, getBlockSrc(iSrc, "Skin Instance"));
        c.processed("Skin Instance");

        return iDst;
    }

    BSVertexDesc bsVectorFlags(QModelIndex iDst, QModelIndex iSrc) {
        int vf = 0;

        if (nifSrc->getUserVersion2() == 11) {
            vf = nifSrc->get<int>(iSrc, "Vector Flags");
        } else {
            vf = nifSrc->get<int>(iSrc, "BS Vector Flags");
        }

        BSVertexDesc newVf = nifDst->get<BSVertexDesc>(iDst, "Vertex Desc");

        if (vf & 1) {
           newVf.SetFlag(VertexFlags::VF_UV);
        }

        if (vf & 4096) {
           newVf.SetFlag(VertexFlags::VF_TANGENT);
        }

        if (nifSrc->get<bool>( iSrc, "Has Vertices")) {
            newVf.SetFlag(VertexFlags::VF_VERTEX);
        }

        if (nifSrc->get<bool>( iSrc, "Has Normals")) {
            newVf.SetFlag(VertexFlags::VF_NORMAL);
        }

        if (nifSrc->get<bool>( iSrc, "Has Vertex Colors")) {
            newVf.SetFlag(VertexFlags::VF_COLORS);
        }

        newVf.ResetAttributeOffsets(nifDst->getUserVersion2());

        nifDst->set<BSVertexDesc>(iDst, "Vertex Desc", newVf);

        return newVf;
    }

    bool bsVectorFlagSet(QModelIndex iSrc, const QString & flagName) {
        bool ok = false;
        bool isSet = nifSrc->get<ushort>(iSrc, "BS Vector Flags") & NifValue::enumOptionValue("BSVectorFlags", flagName, &ok);

        if (!ok) {
            qDebug() << __FUNCTION__ << "Enum Option" << flagName << "not found";
        }

        return isSet;
    }

    template<typename T> QVector<T> niTriDataGetArray(QModelIndex iSrc, Copier & c, const QString & boolName, const QString & name, bool isFlag = false) {
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

    QVector<Vector2> niTriDataGetUVSets(QModelIndex iSrc, Copier & c) {
        if (!bsVectorFlagSet(iSrc, "Has_UV")) {
            return QVector<Vector2>();
        }

        QModelIndex iUVSetsSrc = getIndexSrc(iSrc, "UV Sets");
        QVector<Vector2> result;

        for (int i = 0; i < nifSrc->rowCount(iUVSetsSrc); i++) {
            result.append(nifSrc->getArray<Vector2>(iUVSetsSrc.child(i, 0)));
        }

        c.processed(iUVSetsSrc.child(0, 0).child(0, 0));

        return result;
    }

    bool checkHalfFloat(float f) {
        // Check if the float is already nan or inf
        // TODO: Set to 0 if these values cause crashes
        if (isnan(f) || isinf(f)) {
            return true;
        }

        uint32_t floatData;

        memcpy(&floatData, &f, sizeof floatData);

        uint32_t halfData = half_to_float(half_from_float(floatData));

        memcpy(&f, &halfData, sizeof f);

        bool result = !(isnan(f) || isinf(f));

        if (!result) {
            qDebug() << __FUNCTION__ << "Float to HFloat conversion failed";

            conversionResult = false;
        }

        return result;
    }

    bool checkHalfVector3(HalfVector3 v) {
        return checkHalfFloat(v[0]) && checkHalfFloat(v[1]) && checkHalfFloat(v[2]);
    }

    bool checkHalfVector2(HalfVector2 v) {
        return checkHalfFloat(v[0]) && checkHalfFloat(v[1]);
    }

    float hfloatScale(float hf, float scale) {
        // hfloat max equals (2 - 2^-11) * 2^15 = 65520
        const float fHfLimitMax = 65520;

        // hfloat max equals -(2 - 2^-11) * 2^15 = -65520
        const float fHfLimitMin = -65520;

        if (hf >= fHfLimitMax) {
            float fNewScale = ceil(hf / fHfLimitMax);

            if (fNewScale > scale) {
                scale = fNewScale;
            }
        } else if (hf <= fHfLimitMin) {
            float fNewScale = ceil(hf / fHfLimitMin);

            if (fNewScale > scale) {
                scale = fNewScale;
            }
        }

        return scale;
    }

    // Up to and including num triangles
    void niTriData(QModelIndex iDst, QModelIndex iSrc, Copier & c) {
        c.ignore("Group ID");

        if (fileProps.getLodLevel() > 0) {
            nifDst->set<float>(iDst, "Scale", nifDst->get<float>(iDst, "Scale") * fileProps.getLodLevel());
        }

        uint numVertices = nifSrc->get<uint>( iSrc, "Num Vertices");
        c.processed("Num Vertices");

        c.ignore("Keep Flags");
        c.ignore("Compress Flags");

        QVector<Vector3> verts = niTriDataGetArray<Vector3>(iSrc, c, "Has Vertices", "Vertices");

        BSVertexDesc newVf = bsVectorFlags(iDst, iSrc);
        if (nifSrc->getUserVersion2() == 11) {
            c.processed("Vector Flags");
        } else {
            c.processed("BS Vector Flags");
        }

        QVector<Vector3> norms = niTriDataGetArray<Vector3>(iSrc, c, "Has Normals", "Normals");
        QVector<Vector3> tangents = niTriDataGetArray<Vector3>(iSrc, c, "Has_Tangents", "Tangents", true);
        QVector<Vector3> bitangents = niTriDataGetArray<Vector3>(iSrc, c, "Has_Tangents", "Bitangents", true);

        QModelIndex iBoundingSphereDst = nifDst->getIndex(iDst, "Bounding Sphere");
        c.copyValue(iBoundingSphereDst, iSrc, "Center");
        c.copyValue(iBoundingSphereDst, iSrc, "Radius");

        QVector<Color4> vertexColors = niTriDataGetArray<Color4>(iSrc, c, "Has Vertex Colors", "Vertex Colors");
        QVector<Vector2> uvSets = niTriDataGetUVSets(iSrc, c);

        c.ignore("Consistency Flags");
        c.ignore("Additional Data");
        ignoreBlock(iSrc, "Additional Data", false);

        ushort numTriangles = nifSrc->get<ushort>(iSrc, "Num Triangles");
        c.processed("Num Triangles");

        // TODO: Vertex Colors

        nifDst->set<uint>(iDst, "Num Vertices", numVertices);
        nifDst->set<uint>(iDst, "Num Triangles", numTriangles);
        nifDst->set<uint>(iDst, "Data Size", newVf.GetVertexSize() * numVertices + numTriangles * 6);
        nifDst->updateArray( nifDst->getIndex( iDst, "Vertex Data" ) );
        nifDst->updateArray( nifDst->getIndex( iDst, "Triangles" ) );

        QModelIndex data = nifDst->getIndex(iDst, "Vertex Data");

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

        // TODO: Process for no tangents
        // TODO: Fix for multiple arrays
        QVector<HalfVector3> newVerts = QVector<HalfVector3>(verts.count());

        bool bUvScaled = false;
        float fUvScale = 1;

        // Find UV Offsets exceeding hfloat limtis and set scale accordingly.
        for ( int i = 0; i < verts.count(); i++ ) {
            if (verts.count() == uvSets.count()) {
                HalfVector2 hv = HalfVector2(uvSets[i]);

                fUvScale = hfloatScale(hv[0], fUvScale);
                fUvScale = hfloatScale(hv[1], fUvScale);
            }
        }

        if (fUvScale > 1) {
            bUvScaled = true;
            QModelIndex iUVScaleDst = getIndexDst(getBlockDst(iDst, "Shader Property"), "UV Scale");

            if (iUVScaleDst.isValid()) {
                nifDst->set<Vector2>(iUVScaleDst, nifDst->get<Vector2>(iUVScaleDst) * fUvScale);
            } else {
                qDebug() << __FUNCTION__ << "UV Scaled but no shader property to apply scale to";

                conversionResult = false;
            }
        }

        // Create vertex data
        for ( int i = 0; i < verts.count(); i++ ) {
            HalfVector3 hv = HalfVector3(verts[i]);
            if (fileProps.getLodLevel() > 0) {
                hv /= fileProps.getLodLevel();
            }

            if (!checkHalfVector3(hv)) {
                qDebug() << nifDst->getBlockNumber(iDst);
            }

            nifDst->set<HalfVector3>(data.child(i, 0).child(0, 0), hv);

            // TODO: Check float conversions
            if (verts.count() == norms.count()) {
                nifDst->set<ByteVector3>(data.child(i, 0).child(7, 0), ByteVector3(norms[i]));
            }

            // TODO: Check float conversions
            if (verts.count() == tangents.count()) {
                nifDst->set<ByteVector3>(data.child(i, 0).child(9, 0), ByteVector3(tangents[i]));
            }

            if (verts.count() == uvSets.count()) {
                HalfVector2 hv = HalfVector2(uvSets[i]);

                if (bUvScaled) {
                    hv /= fUvScale;
                }

                if (!checkHalfVector2(hv)) {
                    qDebug() << nifDst->getBlockNumber(iDst);
                }

                nifDst->set<HalfVector2>(data.child(i, 0).child(6, 0), hv);
            }

            if (verts.count() == vertexColors.count()) {
                ByteColor4 color;
                color.fromQColor(vertexColors[i].toQColor());
                nifDst->set<ByteColor4>( data.child(i, 0).child(11, 0), color);
            }

            if (verts.count() == bitangents.count()) {
                nifDst->set<float>( data.child( i, 0 ).child(1,0),  bitangents[i][0]);

                // TODO: Set Bitangent Y and Z
            }
        }

        setHandled(iDst, iSrc);
    }

    void niTriStripsData( QModelIndex iSrc, QModelIndex iDst ) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        niTriData(iDst, iSrc, c);

        // Points

        QModelIndex points = nifSrc->getIndex(iSrc, "Points");
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

        c.processed("Has Points");
        c.processed("Num Strips");
        c.processed(getIndexSrc(iSrc, "Strip Lengths").child(0, 0));

        if (nifSrc->get<bool>(iSrc, "Has Points")) {
            c.processed(getIndexSrc(iSrc, "Points").child(0, 0).child(0, 0));
        }

        nifDst->updateArray(iDst, "Triangles");
        nifDst->setArray<Triangle>(nifDst->getIndex(iDst, "Triangles"), arr);
     }

    void niTriShapeDataAlt(QModelIndex iDst, QModelIndex iSrc) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        niTriData(iDst, iSrc, c);

        c.ignore("Num Triangle Points");

        if (nifSrc->get<bool>(iSrc, "Has Triangles")) {
            c.array("Triangles");
        }

        c.processed("Has Triangles");

        // TODO:
        c.ignore("Num Match Groups");
        if (nifSrc->get<int>(iSrc, "Num Match Groups") > 0) {
            QModelIndex iMatchGroupsArraySrc = getIndexSrc(iSrc, "Match Groups");

            c.ignore(getIndexSrc(iMatchGroupsArraySrc.child(0, 0), "Num Vertices"));

            if (nifSrc->get<int>(iMatchGroupsArraySrc.child(0, 0), "Num Vertices") > 0) {
                c.ignore(getIndexSrc(iMatchGroupsArraySrc.child(0, 0), "Vertex Indices").child(0, 0));
            }
        }
    }

    void unhandledBlocks() {
        for ( int i = 0; i < nifSrc->getBlockCount(); i++ ) {
            if (handledBlocks[i]) {
                qDebug() << "Unhandled block:" << i << "\t" << nifSrc->getBlockName(nifSrc->getBlock(i));

                conversionResult = false;
            }
        }
    }

    void lodLandscapeTranslationZero(QModelIndex iTranslation) {
        Vector3 translation = nifDst->get<Vector3>(iTranslation);
        if (4096 * fileProps.getLodXCoord() - translation[0] != 0.0f || 4096 * fileProps.getLodYCoord() - translation[1] != 0.0f) {
            qDebug() << __FUNCTION__ << "File name translation does not match file data translation";

            conversionResult = false;
        }

        translation[0] = 0;
        translation[1] = 0;

        nifDst->set<Vector3>(iTranslation, translation);
    }

    void lODLandscapeShape(const qint32 link, const QModelIndex iLink, const QModelIndex iLinkBlock, QModelIndex & iWater) {
        QModelIndex iShaderProperty = getBlockDst(iLinkBlock, "Shader Property");

        if (!iShaderProperty.isValid()) {
            qDebug() << __FUNCTION__ << "Shader Property not found";

            conversionResult = false;

            return;
        }

        Vector3 translation = nifDst->get<Vector3>(iLinkBlock, "Translation");

        if (translation[0] != 0.0f || translation[1] != 0.0f) {
            if (link != 1) {
                translation[0] = 0.0f;
                translation[1] = 0.0f;

                nifDst->set<Vector3>(iLinkBlock, "Translation", translation);
            } else {
                qDebug() << __FUNCTION__ << "Unknown case of first trishape in LOD to be translated";

                conversionResult = false;
            }
        }

        if (link != 1) {
            if (iWater.isValid()) {
                nifDst->setLink(iLink, -1);

                lodLandscapeWaterShader(iLinkBlock);
                nifDst->set<int>(iWater, "Num Children", nifDst->get<int>(iWater, "Num Children") + 1);

                QModelIndex iWaterChildren = getIndexDst(iWater, "Children");

                nifDst->updateArray(iWaterChildren);
                nifDst->setLink(iWaterChildren.child(nifDst->rowCount(iWaterChildren) - 1, 0), link);

                return;
            }

            iWater = lodLandscapeWater(iLinkBlock);

            // Make water branch
            nifDst->setLink(iLink, nifDst->getBlockNumber(iWater));

            return;
        }

        bool ok = false;

        nifDst->set<uint>(iShaderProperty, "Skyrim Shader Type", NifValue::enumOptionValue("BSLightingShaderPropertyShaderType", "LOD Landscape Noise", & ok));

        uint flags = 0;

        bsShaderFlags1Add(flags, "Model_Space_Normals");
        bsShaderFlags1Add(flags, "Own_Emit");
        bsShaderFlags1Add(flags, "ZBuffer_Test");

        nifDst->set<uint>(iShaderProperty, "Shader Flags 1", flags);

        flags = 0;

        bsShaderFlags2Add(flags, "ZBuffer_Write");
        bsShaderFlags2Add(flags, "LOD_Landscape");

        nifDst->set<uint>(iShaderProperty, "Shader Flags 2", flags);

        nifDst->set<QString>(iLinkBlock, "Name", "Land");

        // TODO: Set shader type
    }

    void lODLandscape() {
        if (!bLODLandscape) {
            return;
        }

        QModelIndex iRoot = nifDst->getBlock(0);

        if (nifDst->getBlockName(iRoot) != "BSMultiBoundNode") {
            qDebug() << __FUNCTION__ << "Invalid root" << nifDst->getBlockName(iRoot);

            conversionResult = false;

            return;
        }

        nifDst->set<QString>(iRoot, "Name", "chunk");
        nifDst->set<uint>(iRoot, "Culling Mode", 1);

        lodLandscapeTranslationZero(getIndexDst(iRoot, "Translation"));
        lodLandscapeMultiBound(iRoot);

        QModelIndex iChildren = getIndexDst(iRoot, "Children");
        QVector<qint32> links = nifDst->getLinkArray(iRoot, "Children");
        QModelIndex iWater;
        QList<QModelIndex> shapeList;

        for (qint32 i = 0; i < nifDst->rowCount(iChildren); i++) {
            QModelIndex iLink = iChildren.child(i, 0);
            qint32 link = nifDst->getLink(iLink);
            QModelIndex iLinkBlock = nifDst->getBlock(link);
            QString type = nifDst->getBlockName(iLinkBlock);

            if (type == "BSTriShape" || type == "BSSubIndexTriShape") {
                lODLandscapeShape(link, iLink, iLinkBlock, iWater);

                shapeList.append(iLinkBlock);
            } else if (type == "BSMultiBoundNode") {
                nifDst->set<QString>(iLinkBlock, "Name", "WATER");
            } else {
                qDebug() << __FUNCTION__ << "Unknown LOD Structure with block:" << type;

                conversionResult = false;

                return;
            }
        }

        int numChildren = nifDst->get<int>(iRoot, "Num Children");

        for (int i = numChildren - 1; i >= 0; i--) {
            if (nifDst->getLink(iChildren.child(i, 0)) == -1) {
                numChildren--;
                nifDst->removeRow(i, iChildren);
            }
        }

        nifDst->set<int>(iRoot, "Num Children", numChildren);

        if (iWater.isValid()) {
            lodLandscapeWaterMultiBound(iWater);
//            lodLandscapeWaterRemoveEdgeFaces(iWater, lodLandscapeMinVert(shapeList));
        }
    }

    float lodLandscapeMinVert(QList<QModelIndex> shapeList) {
        float min = HUGE_VALF;

        for (QModelIndex iBlock : shapeList) {
            QModelIndex iVertices = getIndexDst(iBlock, "Vertices");

            if (!iVertices.isValid()) {
                continue;
            }

            lodLandscapeMinVert(iVertices, min);
        }

        return min;
    }

    void lodLandscapeMinVert(QModelIndex iShape, float & min) {
        for (int i = 0; i < nifDst->rowCount(iShape); i++) {
            HalfVector3 v = nifDst->get<Vector3>(iShape.child(i, 0), "Vertex");

            if (v[2] < min) {
                min = v[2];
            }
        }
    }

    bool lodLandscapeIsEdgeCoord(float f) {
        return f == 0.0f || f == 4096.0f;
    }

    void lodLandscapeWaterRemoveEdgeFaces(QModelIndex iWater, float min) {
        QList<QPair<QModelIndex, QModelIndex>> vertexDataArrayList;
        QList<QPair<QModelIndex, QModelIndex>> triangleArrayList;

        for (qint32 link : nifDst->getLinkArray(iWater, "Children")) {
            QModelIndex iBlock = nifDst->getBlock(link);

            QModelIndex iVertexDataArray = getIndexDst(iBlock, "Vertex Data");
            QModelIndex iNumVertices = getIndexDst(iBlock, "Num Vertices");
            QModelIndex iTriangleArray = getIndexDst(iBlock, "Triangles");
            QModelIndex iNUmTriangles = getIndexDst(iBlock, "Num Triangles");

            if (
                    !iVertexDataArray.isValid() ||
                    nifDst->rowCount(iVertexDataArray) == 0 ||
                    !getIndexDst(iVertexDataArray.child(0, 0), "Vertex").isValid() ||
                    !iTriangleArray.isValid() ||
                    !iNumVertices.isValid() ||
                    !iNUmTriangles.isValid()) {
                continue;
            }

            vertexDataArrayList.append(QPair<QModelIndex, QModelIndex>(iNumVertices, iVertexDataArray));
            triangleArrayList.append(QPair<QModelIndex, QModelIndex>(iNUmTriangles, iTriangleArray));
        }

        if (vertexDataArrayList.count() == 0) {
            return;
        }

//        float min = nifDst->get<float>(vertexDataArrayList[0].second.child(0, 0), "Vertex");

        // Set min to the lowest vertex.
        for (QPair pair : vertexDataArrayList) {
            lodLandscapeMinVert(pair.second, min);
        }

        for (int j = 0; j < vertexDataArrayList.count(); j++) {
            QPair vertPair = vertexDataArrayList[j];
            QPair triPair = triangleArrayList[j];

            QModelIndex iNumTriangles = triPair.first;
            QModelIndex iTriangleArray = triPair.second;
            QModelIndex iNumVertices = vertPair.first;
            QModelIndex iVertexDataArray = vertPair.second;
            QList<int> vertRemoveList;
            int numTriangles = nifDst->get<int>(iNumTriangles);
            int numVertices = nifDst->get<int>(iNumVertices);

            for (int i = nifDst->rowCount(iTriangleArray) - 1; i >= 0; i--) {
                Triangle tri = nifDst->get<Triangle>(iTriangleArray.child(i, 0));

                int vertIndex1 = tri.v1();
                int vertIndex2 = tri.v2();
                int vertIndex3 = tri.v3();

                HalfVector3 v1 = nifDst->get<HalfVector3>(iVertexDataArray.child(vertIndex1, 0), "Vertex");
                HalfVector3 v2 = nifDst->get<HalfVector3>(iVertexDataArray.child(vertIndex2, 0), "Vertex");
                HalfVector3 v3 = nifDst->get<HalfVector3>(iVertexDataArray.child(vertIndex3, 0), "Vertex");

                if (removeTri(v1, v2, v3, min)) {
                    vertRemoveListAppend(v1, vertIndex1, min, vertRemoveList);
                    vertRemoveListAppend(v2, vertIndex2, min, vertRemoveList);
                    vertRemoveListAppend(v3, vertIndex3, min, vertRemoveList);

                    nifDst->removeRow(i, iTriangleArray);
                    numTriangles--;
                }
            }

            nifDst->set<int>(iNumTriangles, numTriangles);

            // NOTE: Removal of vertices not required. There can still be detached vertices.

            std::sort(vertRemoveList.begin(), vertRemoveList.end(), std::greater<int>());

            for (int vertIndex : vertRemoveList) {
                // Remove verts and update triangle vert indices
                if (removeVertex(iVertexDataArray, iTriangleArray, vertIndex)) {
                    numVertices--;
                }
            }

            nifDst->set<int>(iNumVertices, numVertices);
        }
    }

    bool removeVertex(QModelIndex iVertices, QModelIndex iTriangles, int index) {
        for (int i = 0; i < nifDst->rowCount(iTriangles); i++) {
            Triangle tri = nifDst->get<Triangle>(iTriangles.child(i, 0));

            if (tri[0] == index || tri[1] == index || tri[2] == index) {
                qDebug() << __FUNCTION__ << "Deleting referenced vertex" << nifDst->get<Vector3>(iVertices.child(index, 0), "Vertex");

                conversionResult = false;

                return false;
            }

            if (tri[0] > index) {
                tri[0]--;
            } else if (tri[1] > index) {
                tri[1]--;
            } else if (tri[2] > index) {
                tri[2]--;
            }
        }

        nifDst->removeRow(index, iVertices);

        return true;
    }

    void vertRemoveListAppend(HalfVector3 v, int vertIndex, float min, QList<int> & vertRemoveList) {
        if (isLowVert(v, min)) {
            if (!vertRemoveList.contains(vertIndex)) {
                vertRemoveList.append(vertIndex);
            }
        }
    }

    bool isSameEdgeCoord(float f1, float f2, float f3) {
        return lodLandscapeIsEdgeCoord(f1) && (f1 - f2 == 0.0f && f2 - f3 == 0.0f);
    }

    bool isEdgeVert(HalfVector3 v) {
        return lodLandscapeIsEdgeCoord(v[0]) || lodLandscapeIsEdgeCoord(v[1]);
    }

    bool isLowVert(HalfVector3 v, float min) {
        return v[2] - min == 0.0f;
    }

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
    bool removeTri(HalfVector3 v1, HalfVector3 v2, HalfVector3 v3, float min) {
        return isLowVert(v1, min) || isLowVert(v2, min) || isLowVert(v3, min);
    }

    void lodLandscapeWaterMultiBound(QModelIndex iWater) {
        QVector<qint32> links = nifDst->getLinkArray(iWater, "Children");
        float vert_max = nifDst->get<Vector3>(getIndexDst(nifDst->getBlock(links[0]), "Vertex Data").child(0, 0), "Vertex")[2];
        float vert_min = vert_max;

        for (qint32 link : links) {
            QModelIndex iVertices = getIndexDst(nifDst->getBlock(link), "Vertex Data");

            for (int i = 0; i < nifDst->rowCount(iVertices); i++) {
                float z = nifDst->get<Vector3>(iVertices.child(i, 0), "Vertex")[2];

                if (z < vert_min) {
                    vert_min = z;
                }

                if (z > vert_max) {
                    vert_max = z;
                }
            }
        }

        float extent = (vert_max - vert_min) * fileProps.getLodLevel() / 2;
        float position = vert_max * fileProps.getLodLevel() - extent;

        QModelIndex iAABB = getBlockDst(getBlockDst(iWater, "Multi Bound"), "Data");
        QModelIndex iPosition = getIndexDst(iAABB, "Position");
        QModelIndex iExtent = getIndexDst(iAABB, "Extent");

        Vector3 vPosition = nifDst->get<Vector3>(iPosition);
        Vector3 vExtent = nifDst->get<Vector3>(iExtent);

        vPosition[2] = position;
        vExtent[2] = extent;

        nifDst->set<Vector3>(iPosition, vPosition);
        nifDst->set<Vector3>(iExtent, vExtent);
    }

    QModelIndex lodLandscapeWater(QModelIndex iWater) {
        QModelIndex iMultiBoundNode = nifDst->insertNiBlock("BSMultiBoundNode");

        nifDst->set<QString>(iMultiBoundNode, "Name", "WATER");
        nifDst->set<uint>(iMultiBoundNode, "Culling Mode", 1);
        nifDst->set<uint>(iMultiBoundNode, "Num Children", 1);
        nifDst->updateArray(iMultiBoundNode, "Children");
        nifDst->setLink(getIndexDst(iMultiBoundNode, "Children").child(0, 0), nifDst->getBlockNumber(iWater));

        QModelIndex iMultiBound = nifDst->insertNiBlock("BSMultiBound");

        setLink(iMultiBoundNode, "Multi Bound", iMultiBound);

        QModelIndex iMultiBoundAABB = nifDst->insertNiBlock("BSMultiBoundAABB");
        int level = fileProps.getLodLevel();

        setLink(iMultiBound, "Data", iMultiBoundAABB);
        nifDst->set<Vector3>(iMultiBoundAABB, "Position", Vector3(4096 * level / 2, 4096 * level / 2, 0));
        nifDst->set<Vector3>(iMultiBoundAABB, "Extent", Vector3(4096 * level / 2, 4096 * level/ 2, 0));

        lodLandscapeWaterShader(iWater);

        return iMultiBoundNode;
    }

    void lodLandscapeWaterShader(QModelIndex iWater) {
        QModelIndex iShaderProperty = getBlockDst(iWater, "Shader Property");

        nifDst->set<uint>(iShaderProperty, "Shader Flags 1", bsShaderFlagsGet("Fallout4ShaderPropertyFlags1", "ZBuffer_Test"));
        nifDst->set<uint>(iShaderProperty, "Shader Flags 2", bsShaderFlagsGet("Fallout4ShaderPropertyFlags2", "ZBuffer_Write"));
        nifDst->set<int>(iShaderProperty, "Texture Clamp Mode", 3);
        nifDst->set<int>(iShaderProperty, "Lighting influence", 255);
        nifDst->set<float>(iShaderProperty, "Falloff Start Angle", 0);
        nifDst->set<float>(iShaderProperty, "Falloff Stop Angle", 0);
        nifDst->set<float>(iShaderProperty, "Emissive Multiple", 1.0f);
        nifDst->set<float>(iShaderProperty, "Soft Falloff Depth", 100.0f);
        nifDst->set<float>(iShaderProperty, "Environment Map scale", 1.0f);
    }

    void lodLandscapeMultiBound(QModelIndex iRoot) {
        QModelIndex iMultiBoundAABB = getBlockDst(getBlockDst(iRoot, "Multi Bound"), "Data");

        if (!iMultiBoundAABB.isValid()) {
            qDebug() << "Invalid Multi Bound in LOD";

            conversionResult = false;

            return;
        }

        Vector3 position = nifDst->get<Vector3>(iMultiBoundAABB, "Position");

        if (
                4096 * fileProps.getLodXCoord() + 4096 * fileProps.getLodLevel() / 2 - position[0] != 0.0f ||
                4096 * fileProps.getLodYCoord() + 4096 * fileProps.getLodLevel() / 2 - position[1] != 0.0f) {
            qDebug() << "Unknown MulitboundAABB positioning" << position;

            conversionResult = false;
        }

        position[0] = 4096 * fileProps.getLodLevel() / 2;
        position[1] = 4096 * fileProps.getLodLevel() / 2;

        nifDst->set<Vector3>(iMultiBoundAABB, "Position", position);
    }

    // TODO: Combine High level LODs
    void lODObjects() {
        if (!bLODBuilding) {
            return;
        }

        QModelIndex iRoot = nifDst->getBlock(0);

        // Check if the destination nif already has a processed LOD.
        if (nifDst->getBlockName(iRoot) == "NiNode" && nifDst->get<QString>(iRoot, "Name") == "obj") {
            QModelIndex iRootOld = iRoot;
            iRoot = QModelIndex();

            // Find the root of the newly created LOD.
            for (int i = 1; i < nifDst->getBlockCount(); i++) {
                if (nifDst->getParent(i) == -1) {
                    iRoot = nifDst->getBlock(i);

                    break;
                }
            }

            if (!iRoot.isValid()) {
                qDebug() << __FUNCTION__ << "Failed to find new root";

                conversionResult = false;

                return;
            }

            uint numChildren = nifDst->get<uint>(iRootOld, "Num Children");
            QModelIndex iChildren = getIndexDst(iRootOld, "Children");

            nifDst->set<uint>(iRootOld, "Num Children", numChildren + 1);
            nifDst->updateArray(iChildren);
            nifDst->setLink(iChildren.child(int(numChildren), 0), nifDst->getBlockNumber(iRoot));
        } else {
            QModelIndex iNewRoot = nifDst->insertNiBlock("NiNode", 0);

            nifDst->set<QString>(iNewRoot, "Name", "obj");
            nifDst->set<uint>(iNewRoot, "Num Children", 1);
            nifDst->updateArray(iNewRoot, "Children");
            nifDst->setLink(getIndexDst(iNewRoot, "Children").child(0, 0), 1);
        }

        if (nifDst->getBlockName(iRoot) != "BSMultiBoundNode") {
            qDebug() << __FUNCTION__ << "Invalid root" << nifDst->getBlockName(iRoot);

            conversionResult = false;

            return;
        }

        nifDst->set<QString>(iRoot, "Name", "");

        QVector<qint32> links = nifDst->getLinkArray(iRoot, "Children");

        for (qint32 link : links) {
            QModelIndex iLinkBlock = nifDst->getBlock(link);

            if (nifDst->getBlockName(iLinkBlock) == "BSSubIndexTriShape") {
                nifDst->set<QString>(iLinkBlock, "Name", "obj");

                QModelIndex iShaderProperty = getBlockDst(iLinkBlock, "Shader Property");

                if (!iShaderProperty.isValid()) {
                    qDebug() << __FUNCTION__ << "Shader Property not found";

                    conversionResult = false;

                    return;
                }

                QModelIndex iNiAlphaProperty = nifDst->insertNiBlock("NiAlphaProperty");

                nifDst->set<uint>(iNiAlphaProperty, "Flags", 4844);
                nifDst->set<int>(iNiAlphaProperty, "Threshold", 128);

                setLink(iLinkBlock, "Alpha Property", iNiAlphaProperty);

                // TODO: Set shader type
            } else {
                qDebug() << __FUNCTION__ << "Unknown LOD Structure";

                conversionResult = false;

                return;
            }
        }
    }

    QModelIndex niInterpolatorMultToColor(QModelIndex iInterpolatorMult, Vector3 colorVector) {
        QModelIndex iInterpolatorColor = nifDst->insertNiBlock("NiPoint3Interpolator");
        QModelIndex iInterpolatorMultData = getBlockDst(iInterpolatorMult, "Data");

        if (iInterpolatorMultData.isValid()) {
            QModelIndex iInterpolatorColorData = nifDst->insertNiBlock("NiPosData");

            QModelIndex iDataMult = getIndexDst(iInterpolatorMultData, "Data");
            QModelIndex iDataColor = getIndexDst(iInterpolatorColorData, "Data");

            nifDst->set<uint>(iDataColor, "Num Keys", nifDst->get<uint>(iDataMult, "Num Keys"));
            nifDst->set<uint>(iDataColor, "Interpolation", enumOptionValue("KeyType", "LINEAR_KEY"));

            QModelIndex iKeysArrayMult = getIndexDst(iDataMult, "Keys");
            QModelIndex iKeysArrayColor = getIndexDst(iDataColor, "Keys");

            nifDst->updateArray(iKeysArrayColor);

            for (int i = 0; i < nifDst->rowCount(iKeysArrayMult); i++) {
                nifDst->set<float>(iKeysArrayColor.child(i, 0), "Time", nifDst->get<float>(iKeysArrayMult.child(i, 0), "Time"));
                nifDst->set<Vector3>(iKeysArrayColor.child(i, 0), "Value", colorVector);
            }

            setLink(iInterpolatorColor, "Data", iInterpolatorColorData);
        }

        return iInterpolatorColor;
    }

    void niMaterialEmittanceFinalize(const QModelIndex iBlock) {
        if (nifDst->getBlockName(iBlock) != "BSEffectShaderProperty") {
            return;
        }

        QModelIndex iController = getBlockDst(iBlock, "Controller");
        QModelIndex iControllerMult;
        QModelIndex iControllerColor;
        QModelIndex iLastController;

        QModelIndex iMult = getIndexDst(iBlock, "Emissive Multiple");
        QModelIndex iColor = getIndexDst(iBlock, "Emissive Color");
        float mult = nifDst->get<float>(iMult);
        Color4 color = nifDst->get<Color4>(iColor);

        while (iController.isValid()) {
            iLastController = iController;

            const QString controllerType = nifDst->getBlockName(iController);

            if (controllerType == "BSEffectShaderPropertyFloatController") {
                if (
                        nifDst->get<uint>(iController, "Type of Controlled Variable") ==
                        enumOptionValue("EffectShaderControlledVariable", "EmissiveMultiple")) {
                    if (!iControllerMult.isValid()) {
                        iControllerMult = iController;
                    } else {
                        qDebug() << __FUNCTION__ << "Multiple Mult Controllers";

                        conversionResult = false;
                    }
                }
            } else if (controllerType == "BSEffectShaderPropertyColorController") {
                if (
                        nifDst->get<uint>(iController, "Type of Controlled Color") ==
                        enumOptionValue("EffectShaderControlledColor", "Emissive Color")) {
                    if (!iControllerColor.isValid()) {
                        iControllerColor = iController;
                    } else {
                        qDebug() << __FUNCTION__ << "Multiple Color Controllers";

                        conversionResult = false;
                    }
                }
            }

            iController = getBlockDst(iController, "Next Controller");
        }

        if (!iControllerMult.isValid() && !iControllerColor.isValid()) {
            if (mult == 0.0f) {
                nifDst->set<float>(iMult, 1);
                nifDst->set<Color4>(iColor, Color4(1, 1, 1, 1));
            }

            return;
        }

        if (iControllerMult.isValid() && !iControllerColor.isValid()) {
            QModelIndex iInterpolatorMult = getBlockDst(iControllerMult, "Interpolator");

            if (!iInterpolatorMult.isValid()) {
                return;
            }

            Vector3 colorVector = Vector3(color[0], color[1], color[2]);

            iControllerColor = nifDst->insertNiBlock("BSEffectShaderPropertyColorController");
            nifDst->set<uint>(iControllerColor, "Type of Controlled Color", enumOptionValue("EffectShaderControlledColor", "Emissive Color"));

            int lNextController = nifDst->getLink(iControllerMult, "Next Controller");

            nifDst->setLink(iControllerMult, "Next Controller", nifDst->getBlockNumber(iControllerColor));
            nifDst->setLink(iControllerColor, "Next Controller", lNextController);

            const QString interpolatorType = nifDst->getBlockName(iInterpolatorMult);

            if (interpolatorType == "NiBlendFloatInterpolator") {
                QModelIndex iInterpolatorColor = nifDst->insertNiBlock("NiBlendPoint3Interpolator");

                Copier c = Copier(iInterpolatorColor, iInterpolatorMult, nifDst, nifDst);

                c.copyValue("Flags");
                c.copyValue("Array Size");
                c.copyValue("Weight Threshold");
                c.ignore("Value");

                for (QModelIndex iSequence : niControllerSequenceList) {
                    QModelIndex iControlledBlocksArray = getIndexDst(iSequence, "Controlled Blocks");
                    QModelIndex iNumControlledBlocks = getIndexDst(iSequence, "Num Controlled Blocks");

                    const int numControlledBlocksOrginal = nifDst->get<int>(iNumControlledBlocks);
                    int numControlledBlocks = numControlledBlocksOrginal;

                    for (int i = 0; i < numControlledBlocksOrginal; i++) {
                        QModelIndex iControlledBlockMult = iControlledBlocksArray.child(i, 0);

                        if (nifDst->getLink(iControlledBlockMult, "Controller") == nifDst->getBlockNumber(iControllerMult)) {
                            numControlledBlocks++;
                            nifDst->set<int>(iNumControlledBlocks, numControlledBlocks);
                            nifDst->updateArray(iControlledBlocksArray);

                            QModelIndex iControlledBlockColor = iControlledBlocksArray.child(numControlledBlocks - 1, 0);

                            Copier c = Copier(iControlledBlockColor, iControlledBlockMult, nifDst, nifDst);

                            c.tree();

                            QModelIndex iInterpolatorMult = getBlockDst(iControlledBlockMult, "Interpolator");
                            QModelIndex iInterpolatorColor = niInterpolatorMultToColor(iInterpolatorMult, colorVector);

                            nifDst->setLink(iControlledBlockColor, "Interpolator", nifDst->getBlockNumber(iInterpolatorColor));
                            nifDst->setLink(iControlledBlockColor, "Controller", nifDst->getBlockNumber(iControllerColor));
                        }
                    }
                }

                setLink(iControllerColor, "Interpolator", iInterpolatorColor);
            } else if (interpolatorType == "NiFloatInterpolator") {
                QModelIndex iInterpolatorColor = niInterpolatorMultToColor(iInterpolatorMult, colorVector);

                setLink(iControllerColor, "Interpolator", iInterpolatorColor);
            } else {
                qDebug() << __FUNCTION__ << __LINE__ << "Unknown interpolator type" << interpolatorType;

                conversionResult = false;

                return;
            }

            Copier c = Copier(iControllerColor, iControllerMult, nifDst, nifDst);

            c.processed("Next Controller");
            c.copyValue("Flags");
            c.copyValue("Frequency");
            c.copyValue("Phase");
            c.copyValue("Start Time");
            c.copyValue("Stop Time");
            c.copyValue("Target");
            c.processed("Interpolator");
            c.processed("Type of Controlled Variable");
        }

        if (!iControllerMult.isValid() && iControllerColor.isValid()) {
            if (mult == 0.0f) {
                nifDst->set<float>(iMult, 1);
                nifDst->set<Color4>(iColor, Color4(1, 1, 1, 1));

                for (QModelIndex iSequence : niControllerSequenceList) {
                    QModelIndex iControlledBlocksArray = getIndexDst(iSequence, "Controlled Blocks");

                    for (int i = 0; i < nifDst->rowCount(iControlledBlocksArray); i++) {
                        QModelIndex iControlledBlock = iControlledBlocksArray.child(i, 0);

                        if (nifDst->getLink(iControlledBlock, "Controller") == nifDst->getBlockNumber(iControllerMult)) {
                            QModelIndex iInterpolator = getBlockDst(getBlockDst(iControlledBlock, "Interpolator"), "Data");

                            if (!iInterpolator.isValid()) {
                                continue;
                            }

                            QModelIndex iData = getIndexDst(iInterpolator, "Data");
                            QModelIndex iKeys = getIndexDst(iInterpolator, "Keys");

                            if (!iInterpolator.isValid() || !iData.isValid() || !iKeys.isValid()) {
                                qDebug() << __FUNCTION__ << __LINE__ << "No Interpolator";

                                conversionResult = false;

                                return;
                            }

                            nifDst->set<uint>(iData, "Num Keys", 1);
                            nifDst->updateArray(iKeys);
                            nifDst->set<float>(iKeys.child(0, 0), "Time", 0);
                            nifDst->set<Vector3>(iKeys.child(0, 0), "Value", Vector3(1, 1, 1));
                        }
                    }
                }
            }

            return;
        }

        QModelIndex iInterpolatorMult = getBlockDst(iControllerMult, "Interpolator");
        QModelIndex iFlagsMult = getIndexDst(iInterpolatorMult, "Flags");

        if (iFlagsMult.isValid() && nifDst->get<int>(iFlagsMult) == int(enumOptionValue("InterpBlendFlags", "MANAGER_CONTROLLED"))) {
            iInterpolatorMult = QModelIndex();
        }

        QModelIndex iInterpolatorColor = getBlockDst(iControllerColor, "Interpolator");
        QModelIndex iFlagsColor = getIndexDst(iInterpolatorColor, "Flags");

        if (iFlagsColor.isValid() && nifDst->get<int>(iFlagsColor) == int(enumOptionValue("InterpBlendFlags", "MANAGER_CONTROLLED"))) {
            iInterpolatorColor = QModelIndex();
        }

        QList<QPair<QModelIndex, QModelIndex>> interpolatorList;
        int linkMult = nifDst->getBlockNumber(iControllerMult);
        int linkColor = nifDst->getBlockNumber(iControllerColor);

        if (iInterpolatorMult.isValid() && iInterpolatorColor.isValid()) {
            interpolatorList.append(QPair(iInterpolatorMult, iInterpolatorColor));
        } else {
            for (QModelIndex iSequence : niControllerSequenceList) {
                QModelIndex iControlledBlocksArray = getIndexDst(iSequence, "Controlled Blocks");
                QPair interpolatorPair = QPair(QModelIndex(), QModelIndex());

                if (iInterpolatorMult.isValid()) {
                    interpolatorPair.first = iInterpolatorMult;
                } else if (iInterpolatorColor.isValid()) {
                    interpolatorPair.second = iInterpolatorColor;
                }

                for (int i = 0; i < nifDst->rowCount(iControlledBlocksArray); i++) {
                    QModelIndex iControlledBlock = iControlledBlocksArray.child(i, 0);

                    if (!iInterpolatorMult.isValid() && nifDst->getLink(iControlledBlock, "Controller") == linkMult) {
                        if (interpolatorPair.first.isValid()) {
                            qDebug() << __FUNCTION__ << "Mult interpolator already set";

                            conversionResult = false;
                        }

                        interpolatorPair.first = getBlockDst(iControlledBlock, "Interpolator");
                    } else if (!iInterpolatorColor.isValid() && nifDst->getLink(iControlledBlock, "Controller") == linkColor) {
                        if (interpolatorPair.second.isValid()) {
                            qDebug() << __FUNCTION__ << "Color interpolator already set";

                            conversionResult = false;
                        }

                        interpolatorPair.second = getBlockDst(iControlledBlock, "Interpolator");
                    }
                }

                if (interpolatorPair.first.isValid() ^ interpolatorPair.second.isValid()) {
                    qDebug() << __FUNCTION__ << "No Interpolator Pair";

                    conversionResult = false;

                    return;
                }

                if (interpolatorPair.first.isValid() && interpolatorPair.second.isValid()) {
                    interpolatorList.append(interpolatorPair);
                }
            }
        }

        for (QPair<QModelIndex, QModelIndex> pair : interpolatorList) {
            QModelIndex iInterpolatorDataMult = getIndexDst(getBlockDst(pair.first, "Data"), "Data");
            QModelIndex iInterpolatorDataColor = getIndexDst(getBlockDst(pair.second, "Data"), "Data");

            // TODO: Process case where color interpolator is not valid
            if (!iInterpolatorDataMult.isValid() || !iInterpolatorDataColor.isValid()) {
                continue;
            }

            QModelIndex iNumKeysMult = getIndexDst(iInterpolatorDataMult, "Num Keys");
            QModelIndex iNumKeysColor = getIndexDst(iInterpolatorDataColor, "Num Keys");

            QModelIndex iKeysArrayMult = getIndexDst(iInterpolatorDataMult, "Keys");
            QModelIndex iKeysArrayColor = getIndexDst(iInterpolatorDataColor, "Keys");

            for (uint i = 0; i < uint(nifDst->rowCount(iKeysArrayMult) - 1); i++) {
                QModelIndex iKey1 = iKeysArrayMult.child(int(i), 0);
                QModelIndex iKey2 = iKeysArrayMult.child(int(i + 1), 0);

                QModelIndex iValue1 = getIndexDst(iKey1, "Value");
                QModelIndex iValue2 = getIndexDst(iKey2, "Value");

                float t1 = nifDst->get<float>(iKey1, "Time");
                float t2 = nifDst->get<float>(iKey2, "Time");

                if (nifDst->get<float>(iValue1) == 0.0f) {
                    if (nifDst->get<float>(iValue2) == 0.0f) {
                        if (i > 0) {
                            iKey1 = interpolatorDataInsertKey(iKeysArrayMult, iNumKeysMult, iKey1.row() + 1, RowToCopy::Before);
                            iValue1 = getIndexDst(iKey1, "Value");

                            // Update key 2
                            iKey2 = iKeysArrayMult.child(iKey2.row() + 1, 0);
                        }

                        iKey2 = interpolatorDataInsertKey(iKeysArrayMult, iNumKeysMult, iKey2.row(), RowToCopy::After);
                        iValue2 = getIndexDst(iKey2, "Value");

                        nifDst->set<float>(iValue1, 1);
                        nifDst->set<float>(iValue2, 1);

                        QModelIndex iColorKey1;
                        QModelIndex iColorKey2;

                        for (int j = 0; j < nifDst->rowCount(iKeysArrayColor); j++) {
                            QModelIndex iColorKey = iKeysArrayColor.child(j, 0);
                            float t = nifDst->get<float>(iColorKey, "Time");

                            if (t - t1 <= 0) {
                                // Get the last t1 key
                                iColorKey1 = iColorKey;
                            } else if (t - t1 > 0 && t - t2 < 0) {
                                // inbetweeners
                                nifDst->set<Vector3>(iColorKey, "Value", Vector3(1, 1, 1));
                            } else if (t - t2 >= 0) {
                                // Get the first t2 key
                                iColorKey2 = iColorKey;

                                break;
                            }
                        }

                        if (iColorKey1.isValid() && iColorKey2.isValid()) {
                            if (i > 0) {
                                iColorKey1 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey1.row() + 1, RowToCopy::Before);

                                // Update Color key 2
                                iColorKey2 = iKeysArrayColor.child(iColorKey2.row() + 1, 0);
                            }

                            if (nifDst->get<float>(iColorKey1, "Time") - t1 < 0) {
                                float tBegin = nifDst->get<float>(iColorKey1, "Time");
                                float tEnd = nifDst->get<float>(iColorKey2, "Time");
                                float tRange = tEnd - tBegin;
                                float changeFactor = (t1 - tBegin) / tRange;
                                Vector3 colorBefore = nifDst->get<Vector3>(iColorKey1, "Value");
                                Vector3 colorAfter = nifDst->get<Vector3>(iColorKey2, "Value");
                                Vector3 colorChange = (colorAfter - colorBefore) * changeFactor;
                                Vector3 color = colorBefore + colorChange;

                                // Set color to white
                                nifDst->set<float>(iColorKey1, "Time", t1);
                                nifDst->set<Vector3>(iColorKey1, "Value", color);

                                iColorKey1 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey1.row() + 1, RowToCopy::Before);
                            }

                            // Set color before to the color right before changing to white and color after to the color at t2
                            Vector3 colorBefore = nifDst->get<Vector3>(iColorKey1, "Value");
                            Vector3 colorAfter = nifDst->get<Vector3>(iColorKey2, "Value");

                            // Set color to white
                            nifDst->set<float>(iColorKey1, "Time", t1);
                            nifDst->set<Vector3>(iColorKey1, "Value", Vector3(1, 1, 1));

                            // Insert the color key at the end of the white color phase
                            iColorKey2 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey2.row(), RowToCopy::After);

                            // Set the color key's color to white and it's time to t2
                            nifDst->set<float>(iColorKey2, "Time", t2);
                            nifDst->set<Vector3>(iColorKey2, "Value", Vector3(1, 1, 1));

                            // If the color end key is after the mult end key, create a new color with its value being the color at that time in the timeline without white phases.
                            if (nifDst->get<float>(iColorKey2, "Time") - t2 > 0) {
                                // t > t2
                                float t = nifDst->get<float>(iColorKey2, "Time");

                                float keyRange = t2 - t1;
                                float keyInRange = t - t1;
                                float changeFactor = keyRange / keyInRange;
                                Vector3 colorChange = (colorAfter - colorBefore) * changeFactor;
                                Vector3 color = colorBefore + colorChange;

                                iColorKey2 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey2.row(), RowToCopy::After);

                                nifDst->set<Vector3>(iColorKey2, "Value", color);
                            }
                        } else {
                            qDebug() << __FUNCTION__ << "Missing color key" << iColorKey1 << iColorKey2;

                            conversionResult = false;

                            return;
                        }

                        i += 2;
                    }
                }
            }

            // Set the final key

            QModelIndex iKey = iKeysArrayMult.child(nifDst->rowCount(iKeysArrayMult) - 1, 0);

            if (nifDst->get<float>(iKey, "Value") == 0.0f) {
                float t = nifDst->get<float>(iKey, "Time");

                QModelIndex iColorKeyLast;

                for (int i = 0; i < nifDst->rowCount(iKeysArrayColor); i++) {
                    QModelIndex iColorKey = iKeysArrayColor.child(i, 0);

                    if (nifDst->get<float>(iColorKey, "Time") >= t) {
                        if (!iColorKeyLast.isValid()) {
                            iColorKeyLast = iColorKey;
                        } else {
                            nifDst->set<Vector3>(iColorKey, "Value", Vector3(1, 1, 1));
                        }
                    }
                }

                if (!iColorKeyLast.isValid()) {
                    // Add a key with the last color
                    interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, nifDst->rowCount(iKeysArrayColor), RowToCopy::Before);
                } else if (nifDst->get<float>(iColorKeyLast, "Time") > t) {
                    // Add a key with the color at the timeline point.

                    QModelIndex iColorKeyBeforeLast = iKeysArrayColor.child(iColorKeyLast.row() - 1, 0);

                    float tBegin = nifDst->get<float>(iColorKeyBeforeLast, "Time");
                    float tEnd = nifDst->get<float>(iColorKeyBeforeLast, "Time");
                    float tRange = tEnd - tBegin;
                    float tInRange = tEnd - t;
                    float changeFactor = tInRange / tRange;

                    Vector3 iColorBefore = nifDst->get<Vector3>(iColorKeyBeforeLast, "Value");
                    Vector3 iColorAfter = nifDst->get<Vector3>(iColorKeyLast, "Value");
                    Vector3 colorChange = (iColorAfter - iColorBefore) * changeFactor;
                    Vector3 color = iColorBefore + colorChange;

                    iColorKeyLast = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, nifDst->rowCount(iKeysArrayColor), RowToCopy::Before);

                    nifDst->set<Vector3>(iColorKeyLast, "Value", color);
                }

                iColorKeyLast = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, nifDst->rowCount(iKeysArrayColor), RowToCopy::Before);

                nifDst->set<Vector3>(iColorKeyLast, "Value", Vector3(1, 1, 1));
            }
        }

        if (mult == 0.0f) {
            nifDst->set<float>(iMult, 1);
            nifDst->set<Color4>(iColor, Color4(1, 1, 1, 1));
        }
    }

    enum RowToCopy {
        Before,
        After,
    };

    QModelIndex interpolatorDataInsertKey(QModelIndex iKeys, QModelIndex iNumKeys, int row, RowToCopy rowToCopy) {
        uint numKeys = nifDst->get<uint>(iNumKeys) + 1;
        nifDst->set<uint>(iNumKeys, numKeys);
        nifDst->updateArray(iKeys);

        if (rowToCopy == RowToCopy::Before) {
            if (row == 0) {
                qDebug() << __FUNCTION__ << "Cannot insert at 0";

                conversionResult = false;

                return QModelIndex();
            }

            for (int i = int(numKeys) - 1; i >= row; i--) {
                Copier c = Copier(iKeys.child(i, 0), iKeys.child(i - 1, 0), nifDst, nifDst);

                c.tree();
            }
        } else if (rowToCopy == RowToCopy::After) {
            for (int i = int(numKeys) - 1; i > row; i--) {
                Copier c = Copier(iKeys.child(i, 0), iKeys.child(i - 1, 0), nifDst, nifDst);

                c.tree();
            }
        }

        return iKeys.child(row, 0);
    }

    void niControlledBlocksFinalize() {
        for (QModelIndex iBlock : controlledBlockList) {
            niMaterialEmittanceFinalize(iBlock);
            progress++;
        }
    }
};

//bool isHighLevelLOD(const QString & fname) {
//    QString fileName = QFileInfo(fname).fileName();
//    QStringList fileNameParts = fileName.split('.');
//}

bool checkLODFields(const QString & sLevel, const QString & sXCoord, const QString & sYCoord) {
    bool ok = false;

    sLevel.toInt(&ok);
    if (!ok) {
        qDebug() << "Could not parse LOD level coord from:" << sLevel;

        return false;
    }

    sXCoord.toInt(&ok);
    if (!ok) {
        qDebug() << "Could not parse LOD X coord from:" << sXCoord;

        return false;
    }

    sYCoord.toInt(&ok);
    if (!ok) {
        qDebug() << "Could not parse LOD Y coord from:" << sYCoord;

        return false;
    }

    return true;
}

void adjustToGrid(const int level, int & x, int & y) {
    if (x < 0 && x % level != 0) {
        x -= level;
    }

    x = x / level * level;

    if (y < 0 && y % level != 0) {
        y -= level;
    }

    y = y / level * level;
}

class HighLevelLOD
{
private:
    const QString fname;
    const QString worldspace;
    const int x;
    const int y;
public:
    HighLevelLOD(QString fname, QString worldspace, int x, int y) : fname(fname), worldspace(worldspace), x(x), y(y) {
        //
    }

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
    bool compare(int level, int otherX, int otherY) {
        int x = this->x;
        int y = this->y;

        adjustToGrid(level, x, y);

        return x == otherX && y == otherY;
    }

    int getX() const;
    int getY() const;
    QString getFname() const;
    QString getWorldspace() const;
};

void combineHighLevelLODs(QList<HighLevelLOD> & list, QList<QString> & failedList, const QString & pathDst, ProgressReceiver & receiver) {
    int maxX = 0;
    int maxY = 0;
    int minX = 0;
    int minY = 0;

    QStringList worldspaceList;

    for (HighLevelLOD lod : list) {
        if (!worldspaceList.contains(lod.getWorldspace(), Qt::CaseInsensitive)) {
            worldspaceList.append(lod.getWorldspace());
        }
    }

    for (QString worldspace : worldspaceList) {
        QList<HighLevelLOD> filteredList;

        for (HighLevelLOD lod : list) {
            if (worldspace.compare(lod.getWorldspace(), Qt::CaseInsensitive) == 0) {
                filteredList.append(lod);
            }
        }

        for (HighLevelLOD lod : filteredList) {
            int x = lod.getX();
            int y = lod.getY();

            if (x > maxX) {
                maxX = x;
            } else if (x < minX) {
                minX = x;
            }

            if (y > maxY) {
                maxY = y;
            } else if (y < minY) {
                minY = y;
            }
        }

        for (int level = 8; level <= 32; level *= 2) {
            int minXNew = minX;
            int minYNew = minY;

            adjustToGrid(level, minXNew, minYNew);

            for (int x = minXNew; x <= maxX; x += level) {
                for (int y = minYNew; y <= maxY; y += level) {
                    NifModel nifCombined = NifModel();
                    if (!nifCombined.loadFromFile("D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\template.nif")) {
                        fprintf(stderr, "Failed to load template\n");
                    }

                    for (HighLevelLOD lod : filteredList) {
                        if (!lod.compare(level, x, y)) {
                            continue;
                        }

                        qDebug() << level << x << y;

                        NifModel nif = NifModel();
                        if (!nif.loadFromFile(lod.getFname())) {
                            qDebug() << "Failed to load nif";

                            return;
                        }

                        // Convert

                        Converter c = Converter(&nif, &nifCombined, FileProperties(FileType::Standard, "", lod.getFname(), 0, 0, 0), receiver);

                        c.convert();

                        if (!c.getBLODBuilding()) {
                            qDebug() << __FUNCTION__ << "No LOD Building values found in:" << lod.getFname();

                            return;
                        }

                        if (!c.getConversionResult()) {
                            failedList.append(lod.getFname());
                        }
                    }

                    if (nifCombined.getBlockCount() > 0) {
                        QString fileName =
                                pathDst +
                                "Terrain\\" +
                                worldspace +
                                "\\Objects\\" +
                                worldspace +
                                "." +
                                QString::number(level) +
                                "." +
                                QString::number(x) +
                                "." +
                                QString::number(y) +
                                ".BTO";

                        qDebug() << "Destination: " + fileName;

                        QDir().mkpath(QFileInfo(fileName).path());

                        if (!nifCombined.saveToFile(fileName)) {
                            fprintf(stderr, "Failed to save nif\n");

                            return;
                        }
                    }
                }
            }
        }
    }
}

/**
 * Get the LOD type from directory structure.
 * @brief getLODType
 * @param dirSrc
 * @param isLOD
 * @param isLODObject
 */
FileProperties getFileType(const QString & fnameSrc, QString & fnameDst, ListWrapper<HighLevelLOD> & highLevelLodList) {
    QString fileName = QFileInfo(fnameSrc).fileName();
    QStringList fileNameParts = fileName.split('.');
    QDir dirSrc = QFileInfo(fnameSrc).dir();
    bool isLOD = false;
    bool isLODObject = false;

    if (dirSrc.dirName().compare("blocks", Qt::CaseInsensitive) == 0) {
        isLODObject = true;
        dirSrc.cdUp();
    }

    dirSrc.cdUp();

    if (dirSrc.dirName().compare("lod", Qt::CaseInsensitive) == 0) {
        dirSrc.cdUp();

        if (dirSrc.dirName().compare("landscape", Qt::CaseInsensitive) == 0) {
            isLOD = true;
        }
    }

    if (fileNameParts.last().compare("nif", Qt::CaseInsensitive) != 0) {
        qDebug() << __FUNCTION__ << "File not recognized" << fnameSrc;

        return FileProperties();
    }

    if (!isLOD) {
        return FileProperties(FileType::Standard, fnameDst, fnameSrc);
    }

    int coordStartIndex = 2;
    bool bHighLevel = false;

    if (fileNameParts.count() != 5) {
        if (isLODObject && fileNameParts[2] == "high") {
            coordStartIndex = 3;
            bHighLevel = true;
        } else {
            qDebug() << __FUNCTION__ << "Unknown LOD name format" << fnameSrc;

            return FileProperties();
        }
    }

    QString sWorldSpace = fileNameParts[0];

    // Get the level field and remove the "level" part.
    QString sLevel = fileNameParts[1].remove(0, 5);

    // Get the coordinate fields and remove the first 'x'/'y'.
    QString sXCoord = fileNameParts[coordStartIndex + 0].remove(0, 1);
    QString sYCoord = fileNameParts[coordStartIndex + 1].remove(0, 1);

    if (!checkLODFields(sLevel, sXCoord, sYCoord)) {
        return FileProperties();
    }

    // Set destination name
    if (bHighLevel) {
        highLevelLodList.append(HighLevelLOD(fnameSrc, sWorldSpace, sXCoord.toInt(), sYCoord.toInt()));

        return FileProperties(FileType::LODObjectHigh, "", fnameSrc);
    } else {
        fnameDst +=
                "Terrain\\" +
                sWorldSpace +
                (isLODObject ? "\\Objects\\" : "\\") +
                sWorldSpace +
                "." +
                sLevel +
                "." +
                sXCoord +
                "." +
                sYCoord +
                (isLODObject ? ".BTO" : ".BTR");
    }

    if (isLODObject) {
        return FileProperties(FileType::LODObject, fnameDst, fnameSrc);
    }

    return FileProperties(FileType::LODLandscape, fnameDst, fnameSrc, sLevel.toInt(), sXCoord.toInt(), sYCoord.toInt());
}

NifModel * makeNif() {
    NifModel * newNif = new NifModel();

    QModelIndex iHeader = newNif->getHeader();
    QModelIndex iVersion = newNif->getIndex(iHeader, "Version");
    NifValue version = newNif->getValue(newNif->getIndex(iHeader, "Version"));
    version.setFileVersion(0x14020007);
    newNif->setValue(iVersion, version);
    newNif->set<int>(iHeader, "User Version", 12);
    newNif->set<int>(iHeader, "User Version 2", 130);
    newNif->updateHeader();

    newNif->saveToFile("E:\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data\\Meshes\\test\\template.nif");

    return newNif;
}

void loadNif(NifModel & nif, const QString & fname) {
    if (!nif.loadFromFile(fname)) {
        qDebug() << "Failed to load nif" << fname;
    }
}

bool saveNif(NifModel & nif, const QString & fname) {
//    qDebug() << QThread::currentThreadId() << "Destination: " + fname;

    QMutexLocker locker(&mu);

    QDir().mkpath(QFileInfo(fname).path());

    if (!nif.saveToFile(fname)) {
        qDebug() << "Failed to save nif";

        return false;
    }

    return true;
}

bool convert(const QString & fname, ListWrapper<HighLevelLOD> & highLevelLodList, ProgressReceiver & receiver, const QString & root = "") {
    clock_t tStart = clock();

    qDebug() << QThread::currentThreadId() <<  "Processing: " + fname;

    // Get File Type and destination path.

    QString fnameDst = "E:\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data\\Meshes\\test\\";
    FileProperties fileProps = getFileType(fname, fnameDst, highLevelLodList);
    int fileType = fileProps.getFileType();

    if (fileType == FileType::Invalid) {
        return false;
    }

    if (fileType == FileType::LODObjectHigh) {
        qDebug() << "High level LOD";

        return true;
    }

    if (fileType == FileType::Standard) {
        if (root.length() > 0) {
            fnameDst += QString(fname).remove(0, root.length() + (root.endsWith('/') ? 0 : 1));
        } else {
            fnameDst += QFileInfo(fname).fileName();
        }

        fnameDst = fnameDst.replace('/', '\\');
    }

    // Load

    NifModel nif;
    loadNif(nif, fname);

    NifModel newNif;
    loadNif(newNif, "D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\template.nif");

    // Convert

    Converter c = Converter(&nif, &newNif, fileProps, receiver);

    c.convert();

    // Save

    if (
            ((c.getBLODBuilding()) ^ (fileType == FileType::LODObject)) ||
            ((c.getBLODLandscape()) ^ (fileType == FileType::LODLandscape))) {
        qDebug()
                << "Filename type does not match file data"
                << fname
                << "Building name:" << (fileType == FileType::LODObject) << "data:" << c.getBLODBuilding()
                << "Landscape name:" << (fileType == FileType::LODLandscape) << "data:" << c.getBLODLandscape()
                << "Filetype:" << fileType;

        return false;
    }

    if (!saveNif(newNif, fnameDst)) {
        return false;
    }

    // TODO: Emit done signal to clear progress bar here

    if (!RUN_CONCURRENT) {
        printf("Time taken: %.2fs\n", double(clock() - tStart)/CLOCKS_PER_SEC);
    }

    return c.getConversionResult();
}

void search() {
    const QString path = "D:\\Games\\Fallout 4\\FO4Extracted\\Data\\Meshes";
    QDirIterator it(path, QStringList() << "*.nif", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fname = it.next();

        NifModel nif = NifModel();
        if (!nif.loadFromFile(fname)) {
            fprintf(stderr, "Failed to load nif\n");
        }

        for (int i = 0; i < nif.getBlockCount(); i++) {
            QModelIndex iBlock = nif.getBlock(i);

            if (nif.getBlockName(iBlock) != "BSTriShape") {
                continue;
            }

            QModelIndex iAlpha = nif.getBlock(nif.getLink(iBlock, "Alpha Property"));
            QModelIndex iShader = nif.getBlock(nif.getLink(iBlock, "Shader Property"));

            if (!iAlpha.isValid() || !iShader.isValid()) {
                continue;
            }

            if (nif.getBlockName(iShader) != "BSLightingShaderProperty") {
                continue;
            }

            // Check whether enable blending is set
            if (nif.get<int>(iAlpha, "Flags") & 1) {
                uint flags = nif.get<uint>(iShader, "Shader Flags 1");

                if (
                        !(flags & NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Decal")) &&
                        !(flags & NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Dynamic_Decal"))) {
                    qDebug() << fname;

                    break;
                }
            }

//            if (nif.get<int>(iAlpha, "Flags") == 4333) {
//                uint flags = nif.get<uint>(iShader, "Shader Flags 1");
//                if (
//                        !(flags & NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Decal")) &&
//                        !(flags & NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Dynamic_Decal"))) {
//                    qDebug() << fname;

//                    break;
//                }
//            }
        }
    }

    exit(0);
}

void search2() {
    const QString path = "E:\\SteamLibrary\\steamapps\\common\\Fallout New Vegas\\Data\\meshes";
    QDirIterator it(path, QStringList() << "*.nif", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fname = it.next();

        NifModel nif = NifModel();
        if (!nif.loadFromFile(fname)) {
            fprintf(stderr, "Failed to load nif\n");
        }

        for (int i = 0; i < nif.getBlockCount(); i++) {
            QModelIndex iBlock = nif.getBlock(i);
            QModelIndex iProperties = nif.getIndex(iBlock, "Properties");

            if (!iProperties.isValid()) {
                continue;
            }

            QModelIndex iAlpha;
            QModelIndex iShader;

            for (int j = 0; j < nif.rowCount(iProperties); j++) {
                QModelIndex iLinkBlock = nif.getBlock(nif.getLink(iProperties.child(j, 0)));

                if (nif.getBlockName(iLinkBlock) == "BSShaderPPLightingProperty") {
                    iShader = iLinkBlock;
                }

                if (nif.getBlockName(iLinkBlock) == "NiAlphaProperty") {
                    iAlpha = iLinkBlock;
                }
            }

            if (!iAlpha.isValid() || !iShader.isValid()) {
                continue;
            }

            // Check whether enable blending is set
            if (nif.get<int>(iAlpha, "Flags") & 1) {
                uint flags = nif.get<uint>(iShader, "Shader Flags 1");

                if (
                        !(flags & NifValue::enumOptionValue("BSShaderFlags", "Decal_Single_Pass")) &&
                        !(flags & NifValue::enumOptionValue("BSShaderFlags2", "Dynamic_Decal_Single_Pass"))) {
                    qDebug() << fname;

                    break;
                }
            }

//            if (nif.get<int>(iAlpha, "Flags") == 4333) {
//                uint flags = nif.get<uint>(iShader, "Shader Flags 1");
//                if (
//                        !(flags & NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Decal")) &&
//                        !(flags & NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Dynamic_Decal"))) {
//                    qDebug() << fname;

//                    break;
//                }
//            }
        }
    }

    exit(0);
}

void convert(
        const QString & fname,
        ListWrapper<HighLevelLOD> & highLevelLodList,
        ListWrapper<QString> & failedList,
        ProgressReceiver & receiver,
        const QString & path = "") {
    if (!convert(fname, highLevelLodList, receiver, path)) {
        failedList.append(fname);
    }
}

template <typename T>
void ListWrapper<T>::append(T s)
{
    QMutexLocker locker(&this->listMu);
    list.append(s);
}

void convertNif(QString path) {
////    testConverterController();

//    ConversionDialog w;
////    w.show();
//    w.exec();

//    return;

    clock_t tStart = clock();
    bool bProcessHLLODs = true;

    QList<QString> failedList;
    QList<HighLevelLOD> highLevelLodList;

    ListWrapper<QString> listWrapperFailed = ListWrapper<QString>(failedList);
    ListWrapper<HighLevelLOD> listWrapperHLLODs = ListWrapper<HighLevelLOD>(highLevelLodList);

    // Progress dialog
    QFutureWatcher<void> futureWatcher;
    ProgressGrid dialog(futureWatcher);
    ProgressReceiver receiver = ProgressReceiver(&dialog);

    if (QFileInfo(path).isFile()) {
        convert(path, listWrapperHLLODs, listWrapperFailed, receiver);
    } else if (QDir(path).exists()) {
        QDirIterator it(path, QStringList() << "*.nif", QDir::Files, QDirIterator::Subdirectories);
        QStringList fileList;

        if (RUN_CONCURRENT) {
            // Prepare the file list.
            while (it.hasNext()) {
                fileList.append(it.next());
            }

            qRegisterMetaType<Qt::HANDLE>("Qt::HANDLE");

            // Start the conversion.
            futureWatcher.setFuture(QtConcurrent::map(
                    fileList,
                    [&](QString & elem) {
                        convert(elem, listWrapperHLLODs, listWrapperFailed, receiver, path);

                        if (futureWatcher.future().isCanceled()) {
                            return;
                        }
                    }));

            // Display the dialog and start the event loop.
            dialog.exec();

            futureWatcher.waitForFinished();

            if (futureWatcher.future().isCanceled()) {
                bProcessHLLODs = false;

                qDebug() << "Canceled";
            }
        } else {
            while (it.hasNext()) {
                convert(it.next(), listWrapperHLLODs, listWrapperFailed, receiver, path);
            }
        }
    } else {
        qDebug() << "Path not found";

        return;
    }

    if (bProcessHLLODs) {
        if (highLevelLodList.count() > 0) {
            combineHighLevelLODs(highLevelLodList, failedList, "E:\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data\\Meshes\\test\\", receiver);
        }

        if (failedList.count() > 0) {
            qDebug() << "Failed to convert:";

            for (QString s : failedList) {
                qDebug() << s;
            }
        }
    }

    printf("Total time elapsed: %.2fs\n", double(clock() - tStart)/CLOCKS_PER_SEC);
}

bool Converter::getBLODLandscape() const
{
return bLODLandscape;
}

bool Converter::getBLODBuilding() const
{
return bLODBuilding;
}

bool Converter::getConversionResult() const
{
return conversionResult;
}

QString HighLevelLOD::getFname() const
{
return fname;
}

QString HighLevelLOD::getWorldspace() const
{
return worldspace;
}

int HighLevelLOD::getX() const
{
return x;
}

int HighLevelLOD::getY() const
{
return y;
}

int FileProperties::getLodLevel() const
{
return lodLevel;
}

int FileProperties::getLodXCoord() const
{
return lodXCoord;
}

int FileProperties::getLodYCoord() const
{
return lodYCoord;
}

QString FileProperties::getFnameSrc() const
{
return fnameSrc;
}

FileType FileProperties::getFileType() const
{
return fileType;
}

QList<int> Controller::getClones() const
{
return clones;
}

QList<QString> Controller::getCloneBlockNames() const
{
return cloneBlockNames;
}

int Controller::getOrigin() const
{
return origin;
}
