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

#include <time.h>

#define FIND_UNUSED true

#define UNUSED 0
#define HANDLED 1
#define IGNORED 2

class Copier
{
    QModelIndex iDst;
    QModelIndex iSrc;
    NifModel * nifDst;
    NifModel * nifSrc;
    std::map<QModelIndex, int> usedValues;
public:
    Copier(QModelIndex newIDst, QModelIndex newISrc, NifModel * newNifDst, NifModel * newNifSrc) {
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

        usedValues = std::map<QModelIndex, int>();

        if (FIND_UNUSED) {
            checkValueUse(iSrc, usedValues);
        }
    }

    ~Copier() {
        printUnused();
    }

    void checkValueUse(const QModelIndex iSrc, std::map<QModelIndex, int> & usedValues) {
        if (!FIND_UNUSED) {
            return;
        }


        for (int r = 0; r < nifSrc->rowCount(iSrc); r++) {
            QModelIndex iLink = iSrc.child(r, 0);

            // TODO: Change this to more efficient method
            if (!nifSrc->getIndex(iSrc, nifSrc->getBlockName(iLink)).isValid()) {
                continue;
            }

            if (iLink != nifSrc->getIndex(iSrc, nifSrc->getBlockName(iLink))) {
                continue;
            }
//            if (r > 0 && nifSrc->getBlockName(iLink) == nifSrc->getBlockName(iSrc.child(r - 1, 0))) {
//                continue;
//            }

            if (nifSrc->rowCount(iLink) > 0) {
                checkValueUse(iLink, usedValues);
            } else {
                NifValue v = nifSrc->getValue(iLink);
                if (v.isValid()) {
                    usedValues[iLink] = UNUSED;
                }
            }
        }
    }

    void printUnused() {
        for (std::map<QModelIndex, int>::iterator it= usedValues.begin(); it!=usedValues.end(); ++it) {
            if (it->second == UNUSED) {
                qDebug() << "Unused:" << nifSrc->getBlockName(it->first) << "from" << nifSrc->getBlockName(iSrc);
            }
        }
    }

    bool ignore(const QModelIndex iSource) {
        if (!iSource.isValid()) {
            qDebug() << "Invalid";

            return false;
        }

        if (FIND_UNUSED) {
            if (usedValues.count(iSource) != 0) {
                usedValues[iSource] = IGNORED;
            } else {
                qDebug() << "Key not found";
            }
        }

        return true;
    }

    void ignore(QModelIndex iSource, const QString & name) {
        if (!ignore(nifSrc->getIndex(iSource, name))) {
            qDebug() << name << "Invalid";
        }
    }

    void ignore(const QString & name) {
        ignore(iSrc, name);
    }


    template<typename T> T getVal(NifModel * nif, const QModelIndex iSource) {
        if (!iSource.isValid()) {
            qDebug() << "Invalid QModelIndex";
        }

        NifValue val = nif->getValue(iSource);

        if (!val.isValid()) {
            qDebug() << "Invalid value";
        }

        if (!val.ask<T>()) {
            qDebug() << "Invalid type";
        }

        if (FIND_UNUSED) {
            if (nif == nifSrc) {
                usedValues[iSource] = HANDLED;
            }
        }

        return val.get<T>();
    }

    template<typename T> T getVal(NifModel * nif, const QModelIndex iSource, const QString & name) {
        return getVal<T>(nif, nif->getIndex(iSource, name));
    }

    template<typename T> T getSrc(const QModelIndex iSource) {
        return getVal<T>(nifSrc, iSource);
    }

    template<typename T> T getDst(const QModelIndex iSource) {
        return getVal<T>(nifDst, iSource);
    }

    template<typename T> T getSrc(const QString & name) {
        return getVal<T>(nifSrc, iSrc, name);
    }

    template<typename T> T getDst(const QString & name) {
        return getVal<T>(nifDst, iDst, name);
    }

    template<typename T> T getSrc(const QModelIndex iSource, const QString & name) {
        return getVal<T>(nifSrc, iSource, name);
    }

    template<typename T> T getDst(const QModelIndex iSource, const QString & name) {
        return getVal<T>(nifDst, iSource, name);
    }

    bool copyValue(const QModelIndex iTarget, const QModelIndex iSource) {
        NifValue val = nifSrc->getValue(iSource);

        if (!val.isValid()) {
            qDebug() << "Invalid Value";

            return false;
        }

        if (!nifDst->setValue(iTarget, val)) {
            qDebug() << "Failed to set value on" << nifDst->getBlockName(iTarget);

            return false;
        }

        if (FIND_UNUSED) {
            usedValues[iSource] = HANDLED;
        }

        return true;
    }

    bool copyValue(const QString & nameDst, const QString & nameSrc) {
        QModelIndex iDstNew = nifDst->getIndex(iDst, nameDst);
        QModelIndex iSrcNew = nifSrc->getIndex(iSrc, nameSrc);

        if (!iDstNew.isValid()) {
            qDebug() << "Dst: Invalid value name" << nameDst << "in block type" << nifDst->getBlockName(iDst);

            return false;
        }

        if (!iSrcNew.isValid()) {
            qDebug() << "Src: Invalid value name" << nameSrc << "in block type" << nifSrc->getBlockName(iSrc);

            return false;
        }

        return copyValue(iDstNew, iSrcNew);
    }

    bool copyValue(const QString & name) {
        return copyValue(name, name);
    }


    template<typename TDst, typename TSrc> bool copyValue(const QModelIndex iTarget, const QModelIndex iSource) {
        if (!nifDst->set<TDst>( iTarget, nifSrc->get<TSrc>( iSource ) )) {
            qDebug() << "Failed to set value on" << nifDst->getBlockName(iTarget);

            return false;
        }

        if (FIND_UNUSED) {
            usedValues[iSource] = HANDLED;
        }

        return true;
    }

    template<typename TDst, typename TSrc> bool copyValue() {
        return copyValue<TDst, TSrc>(iDst, iSrc);
    }

    template<typename TDst, typename TSrc> bool copyValue(const QString & nameDst, const QString & nameSrc) {
        QModelIndex iDstNew = nifDst->getIndex(iDst, nameDst);
        QModelIndex iSrcNew = nifSrc->getIndex(iSrc, nameSrc);

        if (!iDstNew.isValid()) {
            qDebug() << "Dst: Invalid value name" << nameDst << "in block type" << nifDst->getBlockName(iDst);
        }

        if (!iSrcNew.isValid()) {
            qDebug() << "Src: Invalid value name" << nameSrc << "in block type" << nifSrc->getBlockName(iSrc);
        }

        return copyValue<TDst, TSrc>(iDstNew, iSrcNew);
    }

    template<typename TDst, typename TSrc> bool copyValue(const QString & name) {
        return copyValue<TDst, TSrc>(name, name);
    }

    template<typename T> bool copyValue() {
        return copyValue<T, T>();
    }

    template<typename T> bool copyValue(const QString & nameDst, const QString & nameSrc) {
        return copyValue<T, T>(nameDst, nameSrc);
    }

    template<typename T> bool copyValue(const QString & name) {
        return copyValue<T, T>(name, name);
    }

    void setIDst(const QModelIndex &value);
};


class Converter
{
    // Texture root folder
    #define TEXTURE_ROOT "textures\\"
    // Texture folder inside root folder
    #define TEXTURE_FOLDER "new_vegas\\"

    NifModel * nifSrc;
    NifModel * nifDst;
    bool * handledBlocks;

    std::map<int, int> indexMap = std::map<int, int>();
    QVector<std::tuple<int, QModelIndex>> linkList = QVector<std::tuple<int, QModelIndex>>();

    QMap<QString, QString> matMap = QMap<QString, QString>();
    QMap<QString, QString> layerMap = QMap<QString, QString>();

public:
    Converter(NifModel * newNifSrc, NifModel * newNifDst, uint blockCount) {
        nifSrc = newNifSrc;
        nifDst = newNifDst;
        handledBlocks = new bool[blockCount];

        loadMatMap();
        loadLayerMap();
    }

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

        bool ok = false;

        // Verify map

        for (QString key : matMap.keys()) {
            NifValue::enumOptionValue("Fallout3HavokMaterial", key, &ok);
            if (!ok) {
                qCritical() << "Enum option \"" << key << "\" not found";

                exit(EXIT_FAILURE);
            }
        }

        for (QString val : matMap.values()) {
            NifValue::enumOptionValue("Fallout4HavokMaterial", val, &ok);
            if (!ok) {
                qCritical() << "Enum option \"" << val << "\" not found";

                exit(EXIT_FAILURE);
            }
        }
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
        // Unknown

        bool ok = false;

        // Verify map

        for (QString key : layerMap.keys()) {
            NifValue::enumOptionValue("Fallout3Layer", key, &ok);
            if (!ok) {
                qCritical() << "Enum option \"" << key << "\" not found";

                exit(EXIT_FAILURE);
            }
        }

        for (QString val : layerMap.values()) {
            NifValue::enumOptionValue("Fallout4Layer", val, &ok);
            if (!ok) {
                qCritical() << "Enum option \"" << val << "\" not found";

                exit(EXIT_FAILURE);
            }
        }
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

            c.copyValue("Target Name");
            c.copyValue("Node Name");
            c.copyValue("Property Type");
            c.copyValue("Controller Type");
            c.copyValue("Controller ID");
            c.copyValue("Interpolator ID");

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

    bool setHandled(const QModelIndex iDst,  const QModelIndex iSrc) {
        if (handledBlocks[nifSrc->getBlockNumber(iSrc)] == false) {
            return false;
        }

        handledBlocks[nifSrc->getBlockNumber(iSrc)] = false;
        indexMap[nifSrc->getBlockNumber(iSrc)] = nifDst->getBlockNumber(iDst);

        return true;
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

    std::tuple<QModelIndex, QString> getControllerType(QString dstType, const QString & name) {
        if (dstType == "BSEffectShaderProperty" ||
                dstType == "BSEffectShaderPropertyColorController" ||
                dstType == "BSEffectShaderPropertyFloatController") {
            return {nifDst->insertNiBlock("BSEffectShaderProperty" + name + "Controller"), "Effect"};
        } else if (dstType == "BSLightingShaderProperty" ||
                dstType == "BSLightingShaderPropertyColorController" ||
                dstType == "BSLightingShaderPropertyFloatController") {
            return {nifDst->insertNiBlock("BSLightingShaderProperty" + name + "Controller"), "Lighting"};
        }

        qDebug() << "Unknown shader property:" << dstType;

        return {QModelIndex(), ""};
    }

    QModelIndex niControllerCopy(QModelIndex iDst, QModelIndex iSrc, const QString & name = "Controller", const int target = -1) {
        bool bExactCopy = false;

        int numController = nifSrc->getLink(iSrc, name);
        if (numController == -1) {
            return QModelIndex();
        }

        QModelIndex iControllerSrc = nifSrc->getBlock(numController);
        QModelIndex iControllerDst;

        QString controllerType = nifSrc->getBlockName(iControllerSrc);
        QString dstType = nifDst->getBlockName(iDst);
        QString shaderType;

        if (controllerType == "NiMaterialColorController") {
            std::tie(iControllerDst, shaderType) = getControllerType(dstType, "Float");

            nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", 0);
        } else if (controllerType == "BSMaterialEmittanceMultController") {
            std::tie(iControllerDst, shaderType) = getControllerType(dstType, "Color");

            nifDst->set<uint>(iControllerDst, "Type of Controlled Color", 0);
        } else if (controllerType == "NiAlphaController") {
            std::tie(iControllerDst, shaderType) = getControllerType(dstType, "Float");

            if (shaderType == "Lighting") {
            // Alpha = 12
                nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", 12);
            } else if (shaderType == "Effect") {
                // Alpha Transparency = 5
                nifDst->set<uint>(iControllerDst, "Type of Controlled Variable", 5);
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
            } else if (blockName == "NiMaterialColorController") {
                c.ignore("Target Color");
            }

            c.printUnused();
        } else {
            iControllerDst = copyBlock(QModelIndex(), nifSrc->getBlock(numController));
        }

        QModelIndex iNextController = iDst;

        iNextController = nifDst->getIndex(iDst, name);
        int lNextController = nifDst->getLink(iNextController);

        QModelIndex iNextControllerBlock;

        while (lNextController != -1) {
            iNextControllerBlock = nifDst->getBlock(lNextController);

            // TODO: Find another way
            if (iNextControllerBlock.isValid()) {
                iNextController = nifDst->getIndex(iNextControllerBlock, "Next Controller");
                lNextController = nifDst->getLink(iNextController);
            } else {
                lNextController = -1;
            }
        }

        nifDst->setLink(iNextController, nifDst->getBlockNumber(iControllerDst));


//            nifDst->setLink(iNextController, name, nifDst->getBlockNumber(iControllerDst));

        niController(iControllerDst, iControllerSrc, target);

        return iControllerDst;
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

    void niController(QModelIndex iDst, QModelIndex iSrc, const int target = -1) {
        setHandled(iDst, iSrc);

        // Target
        if (target == -1) {
            reLink(iDst, iSrc, "Target");
        } else {
            nifDst->setLink(iDst, "Target", target);
        }

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

    QModelIndex getIndexSrc(QModelIndex parent, const QString & name) {
        return nifSrc->getIndex(parent, name);
    }

    QModelIndex getIndexDst(QModelIndex parent, const QString & name) {
        return nifDst->getIndex(parent, name);
    }

    // NOTE: Props collision not rendered correctly in nifSkope but should work in-game.
    void bhkRigidBody(QModelIndex iDst, QModelIndex iSrc) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        nifDst->set<float>(iDst, "Time Factor", 1.0);
        c.copyValue<float>("Friction");
        nifDst->set<float>(iDst, "Rolling Friction Multiplier", 0.0);
        c.copyValue<float>("Restitution");
        c.copyValue<float>("Max Linear Velocity");
        c.copyValue<float>("Max Angular Velocity");
        c.copyValue<float>("Penetration Depth");
        c.copyValue<int>("Motion System");
        // Deactivator Type
        c.copyValue<int>("Solver Deactivation");
        c.copyValue<int>("Quality Type");
        c.copyValue("Num Constraints");
        // Constraints
        // Body Flags

        c.ignore("Shape");
        c.ignore("Layer");
        c.ignore("Flags and Part Number");
        c.ignore("Group");
        c.ignore("Broad Phase Type");
        c.ignore("Collision Response");
        c.ignore("Collision Response 2");
        c.ignore("Process Contact Callback Delay");
        c.ignore("Process Contact Callback Delay 2");
        c.ignore("Translation");
        c.ignore("Rotation");
        c.ignore("Linear Velocity");
        c.ignore("Angular Velocity");
        c.ignore("Center");
        c.ignore("Mass");
        c.ignore("Linear Damping");
        c.ignore("Angular Damping");
        c.ignore("Deactivator Type");
        c.ignore("Body Flags");

        c.ignore("Unused Byte 1");
        c.ignore("Unused Byte 2");
        c.ignore("Unknown Int 1");
        c.ignore("Unknown Int 2");
        c.ignore(nifSrc->getIndex(iSrc, "Unused Bytes"), "Unused Bytes");
        c.ignore(nifSrc->getIndex(iSrc, "Unknown Bytes 1"), "Unknown Bytes 1");
        c.ignore(nifSrc->getIndex(iSrc, "Unused 2"), "Unused 2");
        c.ignore(nifSrc->getIndex(iSrc, "Unused"), "Unused");

        QModelIndex iHavokFilterCopySrc = nifSrc->getIndex(iSrc, "Havok Filter Copy");
        c.ignore(iHavokFilterCopySrc, "Layer");
        c.ignore(iHavokFilterCopySrc, "Flags and Part Number");
        c.ignore(iHavokFilterCopySrc, "Group");

        QModelIndex iCInfoPropertySrc = nifSrc->getIndex(iSrc, "Cinfo Property");
        c.ignore(iCInfoPropertySrc, "Data");
        c.ignore(iCInfoPropertySrc, "Size");
        c.ignore(iCInfoPropertySrc, "Capacity and Flags");

        QModelIndex iInertiaTensorSrc = nifSrc->getIndex(iSrc, "Inertia Tensor");
        c.ignore(iInertiaTensorSrc, "m11");
        c.ignore(iInertiaTensorSrc, "m12");
        c.ignore(iInertiaTensorSrc, "m13");
        c.ignore(iInertiaTensorSrc, "m14");
        c.ignore(iInertiaTensorSrc, "m21");
        c.ignore(iInertiaTensorSrc, "m22");
        c.ignore(iInertiaTensorSrc, "m23");
        c.ignore(iInertiaTensorSrc, "m24");
        c.ignore(iInertiaTensorSrc, "m31");
        c.ignore(iInertiaTensorSrc, "m32");
        c.ignore(iInertiaTensorSrc, "m33");
        c.ignore(iInertiaTensorSrc, "m34");
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

    QModelIndex bhkPackedNiTriStripsShapeDataAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
        QModelIndex iDst = nifDst->insertNiBlock("bhkCompressedMeshShapeData", row);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.copyValue("Num Big Verts", "Num Vertices");
        nifDst->updateArray(iDst, "Big Verts");
        QModelIndex iBigVertsDst = getIndexDst(iDst, "Big Verts");
        QModelIndex iVerticesSrc = getIndexSrc(iSrc, "Vertices");

        // Scale
        float scale = 7.0;

        nifDst->set<Vector4>(iRigidBodyDst, "Translation", nifDst->get<Vector4>(iRigidBodyDst, "Translation") / scale);

        for (int i = 0; i < nifDst->rowCount(iBigVertsDst); i++) {
            nifDst->set<Vector4>(iBigVertsDst.child(i, 0), Vector4(nifSrc->get<Vector3>(iVerticesSrc.child(i, 0))) * 0.1f);
        }

        c.copyValue("Num Big Tris", "Num Triangles");

        QModelIndex iTrianglesArraySrc = getIndexSrc(iSrc, "Triangles");
        QModelIndex iTrianglesSrc;
        QModelIndex iBigTrisArrayDst = getIndexDst(iDst, "Big Tris");
        QModelIndex iBigTrisDst;

        nifDst->updateArray(iBigTrisArrayDst);

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

        c.copyValue<uint, ushort>("Num Chunks", "Num Sub Shapes");
        c.copyValue<uint, ushort>("Num Materials", "Num Sub Shapes");
        nifDst->updateArray(iDst, "Chunks");
        nifDst->updateArray(iDst, "Chunk Materials");

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
            nifDst->set<uint>(iChunkMaterialsDst, "Material", nifSrc->get<uint>(iSubShapesSrc, "Material"));
            nifDst->set<int>(iChunkMaterialsDst, "Layer", nifSrc->get<int>(iSubShapesSrc, "Layer"));
            nifDst->set<int>(iChunkMaterialsDst, "Flags and Part Number", nifSrc->get<int>(iSubShapesSrc, "Flags and Part Number"));
            nifDst->set<ushort>(iChunkMaterialsDst, "Group", nifSrc->get<ushort>(iSubShapesSrc, "Group"));

            // Chunk
            nifDst->set<int>(iChunksDst, "Material Index", i);
            nifDst->set<uint>(iChunksDst, "Num Vertices", nifSrc->get<uint>(iSubShapesSrc, "Num Vertices"));
            nifDst->updateArray(iChunksDst, "Vertices");

            QModelIndex iChunkVerticesDst = getIndexDst(iChunksDst, "Vertices");
            for (ushort j = 0; j < nifDst->rowCount(iChunkVerticesDst); j++) {
                nifDst->set<ushort>(iChunkVerticesDst.child(j, 0), vertIndex++);
            }
        }

        c.ignore("Unknown Byte 1");

        return iDst;
    }

    QModelIndex bhkPackedNiTriStripsShape(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
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

        bhkPackedNiTriStripsShapeData(nifSrc->getBlock(lDataSrc), iRigidBodyDst, iDst, row);
//        nifDst->setLink(
//                    nifDst->getIndex(iDst, "Strips Data").child(0, 0),
//                    nifDst->getBlockNumber(bhkPackedNiTriStripsShapeData(nifSrc->getBlock(lDataSrc), iRigidBodyDst, iDst, row)));

        return iDst;
    }

    QModelIndex bhkPackedNiTriStripsShapeData(QModelIndex iSrc, QModelIndex iRigidBodyDst, QModelIndex iBhkNiTriStripsShapeDst, int row) {
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

        // Scale up (from glnode.cpp)
        float scale = 7.0;

        nifDst->set<Vector4>(iRigidBodyDst, "Translation", nifDst->get<Vector4>(iRigidBodyDst, "Translation") / scale);

        int vertIndex = 0;
        int triIndex = 0;

        for (int k = 0; k < numSubShapes; k++) {
            QModelIndex iSubShapeSrc = iSubShapesArraySrc.child(k, 0);
            QModelIndex iDst = nifDst->insertNiBlock("NiTriStripsData", row);

            c.setIDst(iDst);

            // Vertices

            ushort numVertices = ushort(nifSrc->get<uint>(iSubShapeSrc, "Num Vertices"));
            nifDst->set<ushort>(iDst, "Num Vertices", numVertices);
            nifDst->set<bool>(iDst, "Has Vertices", true);
            nifDst->updateArray(iDst, "Vertices");

            QModelIndex iVerticesDst = getIndexDst(iDst, "Vertices");
            QModelIndex iVerticesSrc = getIndexSrc(iSrc, "Vertices");

            // Scale and set vertices
            for (int i = 0; i < numVertices; i++) {
                nifDst->set<Vector3>(iVerticesDst.child(i, 0), nifSrc->get<Vector3>(iVerticesSrc.child(i + vertIndex, 0)) * scale);
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

            nifDst->set<ushort>(iDst, "Num Triangles", ushort(numTriangles));
            nifDst->set<ushort>(iDst, "Num Strips", ushort(numTriangles));
            nifDst->updateArray(iDst, "Strip Lengths");

            QModelIndex iStripLengthsDst = getIndexDst(iDst, "Strip Lengths");
            for (int i = 0; i < nifDst->rowCount(iStripLengthsDst); i++) {
                nifDst->set<ushort>(iStripLengthsDst.child(i, 0), 3);
            }

            nifDst->set<bool>(iDst, "Has Points", true);
            nifDst->updateArray(iDst, "Points");


            QModelIndex iPointsArrayDst = getIndexDst(iDst, "Points");
            QModelIndex iPointsDst;

            for (int i = 0; i < nifDst->rowCount(iPointsArrayDst); i++) {
                iTrianglesSrc = iTrianglesArraySrc.child(triIndex + i, 0);
                iPointsDst = iPointsArrayDst.child(i, 0);

                nifDst->updateArray(iPointsDst);

                Triangle triangle = nifSrc->get<Triangle>(iTrianglesSrc, "Triangle");

                if (       vertIndex > triangle.v1() ||
                           vertIndex > triangle.v2() ||
                           vertIndex > triangle.v3()) {
                    qDebug() << __FILE__ << __LINE__ << "Vertex index too low";
                } else if (triangle.v1() >= vertIndex + numVertices ||
                           triangle.v2() >= vertIndex + numVertices ||
                           triangle.v3() >= vertIndex + numVertices) {
                    qDebug() << __FILE__ << __LINE__ << "Vertex index too high";
                }

                nifDst->set<ushort>(iPointsDst.child(0, 0), ushort(triangle.v1() - vertIndex));
                nifDst->set<ushort>(iPointsDst.child(1, 0), ushort(triangle.v2() - vertIndex));
                nifDst->set<ushort>(iPointsDst.child(2, 0), ushort(triangle.v3() - vertIndex));
            }

            vertIndex += numVertices;
            triIndex += numTriangles;

            nifDst->setLink(getIndexDst(iBhkNiTriStripsShapeDst, "Strips Data").child(k, 0), row);
            QModelIndex iDataLayerDst = iDataLayersArrayDst.child(k, 0);
            nifDst->set<int>(iDataLayerDst, "Layer", nifSrc->get<int>(iSubShapeSrc, "Layer"));
            nifDst->set<int>(iDataLayerDst, "Flags and Part Number", nifSrc->get<int>(iSubShapeSrc, "Flags and Part Number"));
            nifDst->set<ushort>(iDataLayerDst, "Group", nifSrc->get<ushort>(iSubShapeSrc, "Group"));

            nifDst->set<uint>(iDst, "Material CRC", convertMaterial(getIndexSrc(iSubShapeSrc, "Material")));
        }

        QModelIndex iTrianglesArraySrc = getIndexSrc(iSrc, "Triangles");
        c.ignore(getIndexSrc(iTrianglesArraySrc.child(0, 0), "Triangle"));
        c.ignore(getIndexSrc(iTrianglesArraySrc.child(0, 0), "Welding Info"));
        c.ignore("Unknown Byte 1");
        c.ignore("Num Sub Shapes");
        c.ignore(getIndexSrc(iSrc, "Vertices").child(0, 0));

        return QModelIndex();
    }

    uint convertMaterial(QModelIndex iSrc) {
        NifValue material = nifSrc->getValue(iSrc);
        QString nameSrc = material.enumOptionName("Fallout3HavokMaterial", nifSrc->get<uint>(iSrc));
        bool ok = false;

        quint32 val = NifValue::enumOptionValue("Fallout4HavokMaterial", matMap[nameSrc], &ok);

        if (!ok) {
            throw "Enum option not found";
        }

        return val;
    }

    void bhkMoppBvTreeShape(QModelIndex iDst, QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
        nifDst->set<int>(iDst, "Build Type", 1);

        int lShapeSrc = nifSrc->getLink(iSrc, "Shape");
        if (lShapeSrc == -1) {
            return;
        }

        QModelIndex iShapeSrc = nifSrc->getBlock(lShapeSrc);
        QModelIndex iShapeDst;

        if (nifSrc->getBlockName(iShapeSrc) == "bhkPackedNiTriStripsShape") {
//            iShapeDst = copyBlock(QModelIndex(), iShapeSrc);
            // TODO: Use bkhNiTriStripsShape and bhkNiTriStripsData
            // TODO: Set block order
//            iShapeDst = bhkPackedNiTriStripsShape(iShapeSrc, iRigidBodyDst, row);
            iShapeDst = bhkPackedNiTriStripsShape(iShapeSrc, iRigidBodyDst, row);
            nifDst->setLink(iDst, "Shape", nifDst->getBlockNumber(iShapeDst));
        }
    }

    void collisionObject( QModelIndex parent, QModelIndex iNode ) {
        QModelIndex iRigidBodySrc = nifSrc->getBlock(nifSrc->getLink(iNode, "Body"));

        QModelIndex iShapeSrc = nifSrc->getBlock(nifSrc->getLink(iRigidBodySrc, "Shape"));
        QModelIndex iShapeDst = copyBlock(QModelIndex(), iShapeSrc);
        int rShapeDst = nifDst->getBlockNumber(iShapeDst);

        // NOTE: Copy of rigidBody is only correct up to and including Angular Damping
        QModelIndex iRigidBodyDst = copyBlock(QModelIndex(), iRigidBodySrc);
        QModelIndex colNode = insertNiBlock("bhkCollisionObject");

        Copier c = Copier(colNode, iNode, nifDst, nifSrc);

        // Collision Object
        c.copyValue("Flags");

        c.ignore("Target");
        c.ignore("Body");

        handledBlocks[nifSrc->getBlockNumber(iNode)] = false;
        nifDst->setLink(colNode, "Body", nifDst->getBlockNumber(iRigidBodyDst));

        // TODO: Skyrim layers
        bhkRigidBody(iRigidBodyDst, iRigidBodySrc);
        handledBlocks[nifSrc->getBlockNumber(iRigidBodySrc)] = false;

        // Shape
        // NOTE: Radius not rendered? Seems to be at 10 times always
        nifDst->setLink(iRigidBodyDst, "Shape", nifDst->getBlockNumber(iShapeDst));
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
            nifDst->set<float>(scaleNode, "Scale", nifDst->get<float>(iShapeDst, "Radius"));

            parent = scaleNode;
        } else {
            qDebug() << nifDst->getBlockName(iShapeDst);
            if (nifDst->getBlockName(iShapeDst) == "bhkMoppBvTreeShape") {
                bhkMoppBvTreeShape(iShapeDst, iShapeSrc, iRigidBodyDst, rShapeDst);
            } else {
                collapseScale(iShapeDst, nifDst->get<float>(iShapeDst, "Radius"));
                collapseScaleRigidBody(iRigidBodyDst, nifDst->get<float>(iShapeDst, "Radius"));
            }
        }

        // Link to parent
        nifDst->setLink(parent, "Collision Object", nifDst->getBlockNumber(colNode));
        nifDst->setLink(colNode, "Target", nifDst->getBlockNumber(parent));
    }

    void niTexturingProperty(QModelIndex iDst, QModelIndex iSrc) {
        setHandled(iDst, iSrc);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        niControllerCopy(iDst, iSrc, "Controller", nifDst->getBlockNumber(iDst));
        c.ignore("Controller");

        c.ignore("Name");
        c.ignore("Num Extra Data List");
        c.ignore("Flags");
        c.ignore("Texture Count");
        c.ignore("Num Shader Textures");

        QString type = nifDst->getBlockName(iDst);
        if (type == "BSEffectShaderProperty") {
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

                    if (s == "Base") {
                        QModelIndex iNiSourceTexture = nifSrc->getBlock(nifSrc->getLink(iTextureSrc, "Source"));
                        c.ignore(iTextureSrc, "Source");
                        setHandled(iDst, iNiSourceTexture);
                        QString path = nifSrc->string(iNiSourceTexture, QString("File Name"));
                        nifDst->set<QString>(iDst, "Source Texture", updateTexturePath(path));
                    }
                }
            }
        } else if (type == "BSLightingShaderProperty") {
        }

        c.printUnused();
    }

    void niMaterialProperty(QModelIndex iDst, QModelIndex iSrc) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.ignore("Name");
        c.ignore("Num Extra Data List");
        niControllerCopy(iDst, iSrc);
        c.ignore("Controller");
        c.ignore("Specular Color");
        c.ignore("Emissive Color");
//        nifDst->set<Color4>(iDst, "Emissive Color", Color4(c.getSrc<Color3>("Emissive Color")));
        c.ignore("Glossiness");
        c.ignore("Alpha");
        c.copyValue<float>("Emissive Multiple", "Emissive Mult");

        setHandled(iDst, iSrc);

        c.printUnused();
    }

    void niParticleSystem(QModelIndex iDst, QModelIndex iSrc) {
        setHandled(iDst, iSrc);

        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);
        c.copyValue<QString>("Name");
        c.ignore("Num Extra Data List");
        // TODO: Extra data
        niControllerCopy(iDst, iSrc);
        c.ignore("Controller");

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
        c.copyValue<uint>("Num Modifiers");
        // TODO: Modifiers
        QModelIndex iModifiersDst = nifDst->getIndex(iDst, "Modifiers");
        QModelIndex iModifiersSrc = nifSrc->getIndex(iSrc, "Modifiers");
        nifDst->updateArray(iModifiersDst);

        if (nifSrc->rowCount(iModifiersSrc) > 0) {
            c.ignore(iModifiersSrc.child(0, 0));
        }

        for (int i = 0; i < nifSrc->rowCount(iModifiersSrc); i++) {
            QModelIndex iModifierDst  = std::get<0>(copyLink(iModifiersDst.child(i, 0), iModifiersSrc.child(i, 0)));
            reLinkRec(iModifierDst);
        }

        c.printUnused();
    }

//    QModelIndex niBillBoardNode(QModelIndex iDst, QModelIndex iSrc) {
////        Copier c = Copier(fadeNode)
//    }

    QModelIndex bsFadeNode( QModelIndex iNode ) {
        QModelIndex linkNode;
        QModelIndex fadeNode;
        const QString blockType = nifSrc->getBlockName(iNode);


        if (blockType == "NiBillBoardNode") {
            fadeNode = insertNiBlock("NiBillBoardNode");
        } else {
            fadeNode = insertNiBlock("BSFadeNode");
        }

        Copier c = Copier(fadeNode, iNode, nifDst, nifSrc);

        if (blockType == "NiBillBoardNode") {
            c.copyValue("Billboard Mode");
        }

        c.ignore("Num Extra Data List");
        c.ignore("Controller");
        c.ignore("Num Properties");
        c.ignore("Collision Object");
        c.ignore("Num Effects");

        if (nifSrc->rowCount(getIndexSrc(iNode, "Extra Data List")) > 0) {
            c.ignore(getIndexSrc(iNode, "Extra Data List").child(0, 0));
        }

        if (nifSrc->rowCount(getIndexSrc(iNode, "Children")) > 0) {
            c.ignore(getIndexSrc(iNode, "Children").child(0, 0));
        }


        setHandled(fadeNode, iNode);
//        indexMap.insert(nifSrc->getBlockNumber(iNode), nifDst->getBlockNumber(fadeNode));
        indexMap[nifSrc->getBlockNumber(iNode)] = nifDst->getBlockNumber(fadeNode);
        c.copyValue("Name");
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

        c.copyValue("Flags");
        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Scale");
        c.copyValue("Num Children");

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
            } else if (type == "NiBillBoardNode") {
                bsFadeNode(linkNode);
            }
        }

        // TODO: Num Effects
        // TODO: Effects

        return fadeNode;
    }

    QString updateTexturePath(QString fname) {
        return fname.insert(int(strlen(TEXTURE_ROOT)), TEXTURE_FOLDER);
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

    int getFlagsBSShaderFlags1(QModelIndex iDst, QModelIndex iNiTriStripsData, QModelIndex iBSShaderPPLightingProperty) {
        if (!iBSShaderPPLightingProperty.isValid()) {
            return 0;
        }

        int mask = ~(
                (1 << 2 ) +
                (1 << 4 ) +
                (1 << 5 ) +
                (1 << 6 ) +
                (1 << 7 ) +
                (1 << 8 ) +
                (1 << 9 ) +
                (1 << 10) + // Maybe?
                (1 << 11) +
                (1 << 12) +
                (1 << 14) +
                (1 << 19) +
                (1 << 21) +
                (1 << 22) +
                (1 << 23) +
                (1 << 25) +
                (1 << 28) +
                (1 << 30));
        int flags = nifSrc->get<int>(iBSShaderPPLightingProperty, "Shader Flags") & mask;

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

    void bSShaderLightingProperty(QModelIndex iDst, QModelIndex iSrc) {
        Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

        c.ignore("Name");
        c.ignore("Num Extra Data List");
        c.ignore("Controller");
        c.ignore("Flags");
        c.ignore("Shader Type");
        c.ignore("Shader Flags");
        c.ignore("Environment Map Scale");
        c.ignore("Texture Clamp Mode");

//        c.copyValue("Texture Clamp Mode");
        nifDst->set<int>(iDst, "Texture Clamp Mode", 3);

        c.copyValue("Shader Flags 2");

        QString blockName = nifDst->getBlockName(iDst);

        if (blockName == "BSLightingShaderProperty") {
            if (nifSrc->getBlockName(iSrc) == "BSShaderPPLightingProperty") {
                QModelIndex iTextureSetSrc = nifSrc->getBlock(nifSrc->getLink(iSrc, "Texture Set"));
                QModelIndex iTextureSet = copyBlock(iDst, iTextureSetSrc);
                c.ignore("Texture Set");

                bsShaderTextureSet(iTextureSet, iTextureSetSrc);
                nifDst->setLink(iDst, "Texture Set", nifDst->getBlockNumber(iTextureSet));
                c.copyValue("Refraction Strength");

                c.ignore("Refraction Fire Period");
                c.ignore("Parallax Max Passes");
                c.ignore("Parallax Scale");
            } else {
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
            c.ignore("File Name");
            c.copyValue("Falloff Start Angle");
            c.copyValue("Falloff Stop Angle");
            c.copyValue("Falloff Start Opacity");
            c.copyValue("Falloff Stop Opacity");

            // Set Use_Falloff flag
            nifDst->set<int>(iDst, "Shader Flags 1", c.getDst<int>("Shader Flags 1") | (1 << 6));
        }

        setHandled(iDst, iSrc);

        c.printUnused();
    }

    QModelIndex getShaderProperty(QModelIndex iSrc) {
        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iSrc));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            if (nifSrc->getBlockName(linkNode) == "BSShaderNoLightingProperty") {
                return nifDst->insertNiBlock("BSEffectShaderProperty");
            }
        }

        return nifDst->insertNiBlock( "BSLightingShaderProperty" );
    }

    QModelIndex niTriStrips( QModelIndex iSrc) {
        const QModelIndex triShape = nifDst->insertNiBlock( "BSTriShape" );
        setHandled(triShape, iSrc);

        Copier c = Copier(triShape, iSrc, nifDst, nifSrc);

        c.copyValue<QString>("Name");
        if (nifDst->string(triShape, QString("Name")).length() == 0) {
            qDebug() << "Important Warning: triShape has no name!";
        }

//        QModelIndex shaderProperty = nifDst->insertNiBlock( "BSLightingShaderProperty" );
        QModelIndex shaderProperty = getShaderProperty(iSrc);

        nifDst->setLink( triShape, "Shader Property", nifDst->getBlockNumber( shaderProperty ) );

        c.copyValue("Translation");
        c.copyValue("Rotation");
        c.copyValue("Flags");
        c.copyValue("Scale");

        c.ignore("Controller");
        c.ignore("Num Properties");
        c.ignore("Collision Object");
        c.ignore("Data");
        c.ignore("Skin Instance");
        c.ignore("Num Extra Data List");

        QModelIndex iMaterialDataSrc = getIndexSrc(iSrc, "Material Data");
        c.ignore(iMaterialDataSrc, "Num Materials");
        c.ignore(iMaterialDataSrc, "Active Material");
        c.ignore(iMaterialDataSrc, "Material Needs Update");

        if (nifSrc->rowCount(getIndexSrc(iSrc, "Properties")) > 0) {
            c.ignore(getIndexSrc(iSrc, "Properties").child(0, 0));
        }

        if (nifSrc->rowCount(getIndexSrc(iSrc, "Extra Data List")) > 0) {
            c.ignore(getIndexSrc(iSrc, "Extra Data List").child(0, 0));
        }

        QModelIndex iNiTriStripsData;
        QModelIndex iBSShaderLightingProperty;

        QList<int> links = nifSrc->getChildLinks(nifSrc->getBlockNumber(iSrc));
        for (int i = 0; i < links.count(); i++) {
            QModelIndex linkNode = nifSrc->getBlock(links[i]);
            QString type = nifSrc->getBlockName(linkNode);

            if (type == "NiTriStripsData") {
                niTriStripsData(linkNode, triShape);
                handledBlocks[nifSrc->getBlockNumber(linkNode)] = false;

                iNiTriStripsData = linkNode;
            } else if (type == "NiAlphaProperty") {
                copyBlock(triShape, linkNode);
            } else if (type == "BSShaderPPLightingProperty" || type == "BSShaderNoLightingProperty") {
                bSShaderLightingProperty(shaderProperty, linkNode);
                iBSShaderLightingProperty = linkNode;
            } else if (type == "NiMaterialProperty") {
                niMaterialProperty(shaderProperty, linkNode);
            } else if (type == "NiTexturingProperty") {
                // Needs to be copied
                niTexturingProperty(shaderProperty, linkNode);
            }
        }

        // Multinode dependant values

        // Shader Flags
        nifDst->set<int>(shaderProperty, "Shader Flags 1", getFlagsBSShaderFlags1(shaderProperty, iNiTriStripsData, iBSShaderLightingProperty));

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
         QVector<Color4> vertexColors = nifSrc->getArray<Color4>( iNode, "Vertex Colors" );


         // TODO: Process for no tangents

         // TODO: Fix for multiple arrays
         QVector<Vector2> uvSets = nifSrc->getArray<Vector2>( nifSrc->getIndex( iNode, "UV Sets"), "UV Sets");
    //             QVector<ushort> points = nif->getArray<ushort>( nif->getIndex( iNode, "Points"), "Points" );
    //             QVector<QModelIndex> points = nif->getArray<QModelIndex>(triShape, "Points");

         QVector<HalfVector3> newVerts = QVector<HalfVector3>(verts.count());

         // Create vertex data
         for ( int i = 0; i < verts.count(); i++ ) {
             nifDst->set<HalfVector3>( data.child( i, 0 ).child(0,0), HalfVector3(verts[i]));
             verts.count() == norms.count()        && nifDst->set<ByteVector3>( data.child( i, 0 ).child(7,0), ByteVector3(norms[i]));
             verts.count() == tangents.count()     && nifDst->set<ByteVector3>( data.child( i, 0 ).child(9,0), ByteVector3(tangents[i]));
             verts.count() == uvSets.count()       && nifDst->set<HalfVector2>( data.child( i, 0 ).child(6,0), HalfVector2(uvSets[i]));
             if (verts.count() == vertexColors.count()) {
                 ByteColor4 color;
                 color.fromQColor(vertexColors[i].toQColor());
                 nifDst->set<ByteColor4>( data.child( i, 0 ).child(11,0), color);
             }

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
    clock_t tStart = clock();

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

//    fnameDst = "D:\\Games\\Fallout New Vegas\\FNVFo4 Converted\\test\\" + fnameDst;
    fnameDst = "E:\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data\\Meshes\\test\\" + fnameDst;
    qDebug() << "Destination: " + fnameDst;

    if (!newNif.saveToFile(fnameDst)) {
        fprintf(stderr, "Failed to save nif\n");
    }

    printf("Time taken: %.2fs\n", double(clock() - tStart)/CLOCKS_PER_SEC);
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

void Copier::setIDst(const QModelIndex &value)
{
iDst = value;
}
