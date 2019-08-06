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

void makeNif() {
    NifModel newNif;

    QModelIndex iHeader = newNif.getHeader();
    QModelIndex iVersion = newNif.getIndex(iHeader, "Version");
    NifValue version = newNif.getValue(newNif.getIndex(iHeader, "Version"));

//    newNif.load
    version.setFileVersion(0x14020007);
    newNif.setValue(iVersion, version);
    newNif.set<int>(iHeader, "User Version", 12);
    newNif.set<int>(iHeader, "User Version 2", 130);
    newNif.updateHeader();

    newNif.saveToFile("E:\\SteamLibrary\\steamapps\\common\\Fallout 4\\Data\\Meshes\\test\\template.nif");
}

void loadNif(NifModel & nif, const QString & fname) {
    if (!nif.loadFromFile(fname)) {
        qDebug() << "Failed to load nif" << fname;
    }
}

bool FileSaver::save(NifModel & nif, const QString & fname)
{
    QMutexLocker locker(&saveMu);

    QDir().mkpath(QFileInfo(fname).path());

    if (!nif.saveToFile(fname)) {
        qDebug() << "Failed to save nif";

        return false;
    }

    return true;
}

bool convert(
        const QString & fname,
        QString fnameDst,
        ListWrapper<HighLevelLOD> & highLevelLodList,
        ProgressReceiver & receiver,
        FileSaver & saver,
        const QString & root = "") {
    clock_t tStart = clock();

    qDebug() << QThread::currentThreadId() <<  "Processing: " + fname;

    fnameDst = QDir(fnameDst).path() + "/";

    // Get File Type and destination path.

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

    if (!saver.save(nif, fnameDst)) {
        return false;
    }

    // TODO: Emit done signal to clear progress bar here

    if (!RUN_CONCURRENT) {
        printf("Time taken: %.2fs\n", double(clock() - tStart)/CLOCKS_PER_SEC);
    }

    return c.getConversionResult();
}

void convert(
        const QString & fname,
        QString fnameDst,
        ListWrapper<HighLevelLOD> & highLevelLodList,
        ListWrapper<QString> & failedList,
        ProgressReceiver & receiver,
        FileSaver & saver,
        const QString & path = "") {
    if (!convert(fname, fnameDst, highLevelLodList, receiver, saver, path)) {
        failedList.append(fname);
    }
}

template <typename T>
void ListWrapper<T>::append(T item)
{
    QMutexLocker locker(&this->listMu);
    list.append(item);
}

void Converter::loadMatMap() {
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

void Converter::loadLayerMap() {
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

void Converter::setStringsArray(const QModelIndex &parentDst, const QModelIndex &parentSrc, const QString &arr, const QString &name)
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

void Converter::setNiObjectRootStrings(const QModelIndex &iBlockDst, const QModelIndex &iBlockSrc)
{
    for ( int i = 0; i < nifDst->rowCount( iBlockDst ); i++ ) {
            auto iStringSrc = iBlockSrc.child( i, 0 );
            auto iStringDst = iBlockDst.child( i, 0 );
            if ( rootStringList.contains( nifSrc->itemName( iStringSrc ) ) ) {
                nifDst->set<QString>( iStringDst,  nifSrc->string( iStringSrc ) );
            }
        }
}

void Converter::setStringsNiMesh(const QModelIndex &iBlockDst, const QModelIndex &iBlockSrc)
{
    QStringList strings;
        auto iDataSrc = nifSrc->getIndex( iBlockSrc, "Datastreams" );
        auto iDataDst = nifDst->getIndex( iBlockDst, "Datastreams" );
        for ( int i = 0; i < nifSrc->rowCount( iDataSrc ); i++ )
            setStringsArray( iDataDst.child( i, 0 ), iDataSrc.child( i, 0 ), "Component Semantics", "Name" );
}

void Converter::setStringsNiSequence(const QModelIndex &iBlockDst, const QModelIndex &iBlockSrc) {
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

QPair<QString, QString> Converter::acceptFormat(const QString &format, const NifModel *nif)
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

QModelIndex Converter::copyBlock(const QModelIndex &iDst, const QModelIndex &iSrc, int row, bool bMap) {
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

QModelIndex Converter::getHandled(const QModelIndex iSrc) {
    return getHandled(nifSrc->getBlockNumber(iSrc));
}

QModelIndex Converter::getHandled(const int blockNumber) {
    if (isHandled(blockNumber)) {
            return nifDst->getBlock(indexMap[blockNumber]);
        }

        return QModelIndex();
}

bool Converter::isHandled(const int blockNumber) {
    return !handledBlocks[blockNumber];
}

bool Converter::isHandled(const QModelIndex iSrc) {
    return isHandled(nifSrc->getBlockNumber(iSrc));
}

bool Converter::setHandled(const QModelIndex iDst, const QModelIndex iSrc) {
    if (handledBlocks[nifSrc->getBlockNumber(iSrc)] == false) {
            return false;
        }

        handledBlocks[nifSrc->getBlockNumber(iSrc)] = false;
        indexMap[nifSrc->getBlockNumber(iSrc)] = nifDst->getBlockNumber(iDst);
        progress++;

        return true;
}

void Converter::ignoreBlock(const QModelIndex iSrc, bool ignoreChildBlocks) {
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

void Converter::ignoreBlock(const QModelIndex iSrc, const QString &name, bool ignoreChildBlocks) {
    ignoreBlock(nifSrc->getBlock(nifSrc->getLink(iSrc, name)), ignoreChildBlocks);
}

void Converter::niPSysData(const QModelIndex iDst, const QModelIndex iSrc) {
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

std::tuple<QModelIndex, QModelIndex> Converter::copyLink(QModelIndex iDst, QModelIndex iSrc, const QString &name) {
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

void Converter::reLinkExec() {
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

void Converter::reLink(QModelIndex iDst, QModelIndex iSrc) {
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

void Converter::reLink(QModelIndex iDst, QModelIndex iSrc, const QString &name) {
    reLink(nifDst->getIndex(iDst, name), nifSrc->getIndex(iSrc, name));
}

void Converter::reLinkArray(QModelIndex iArrayDst, QModelIndex iArraySrc, const QString arrayName, const QString &name) {
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

void Converter::reLinkRec(const QModelIndex iDst) {
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

void Converter::niAVDefaultObjectPalette(QModelIndex iDst, QModelIndex iSrc) {
    reLink(iDst, iSrc, "Scene");
        reLinkArray(iDst, iSrc, "Objs", "AV Object");
}

void Converter::niInterpolator(QModelIndex iDst, QModelIndex iSrc, const QString &name) {
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

QModelIndex Converter::niControllerSequence(QModelIndex iSrc) {
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

void Converter::niControllerSequences(QModelIndex iDst, QModelIndex iSrc) {
    for (int i = 0; i < nifDst->rowCount(iDst); i++) {
            QModelIndex iSeqSrc = nifSrc->getBlock(nifSrc->getLink(iSrc.child(i, 0)));
            QModelIndex iSeqDst = niControllerSequence(iSeqSrc);

            nifDst->setLink(iDst.child(i, 0), nifDst->getBlockNumber(iSeqDst));
        }
}

Controller &Converter::getController(int blockNumber) {
    for (int i = 0; i < controllerList.count(); i++) {
            if (controllerList[i].getOrigin() == blockNumber) {
                return controllerList[i];
            }
        }

        qDebug() << __FUNCTION__ << "Controller not found" << blockNumber;

        exit(1);
}

void Converter::niInterpolatorFinalizeEmissive(QModelIndex iController, QModelIndex iInterpolator) {
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

void Converter::niControllerSequencesFinalize() {
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

QString Converter::getControllerType(QString dstType) {
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

std::tuple<QModelIndex, QString> Converter::getControllerType(QString dstType, const QString &name) {
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

QModelIndex Converter::niController(QModelIndex iDst, QModelIndex iSrc, Copier &c, const QString &name, const QString &blockName, const int target) {
    c.processed(iSrc, name);

        return niController(iDst, iSrc, name, blockName, target);
}

QModelIndex Converter::niController(QModelIndex iDst, QModelIndex iSrc, const QString &name, const QString &blockName, const int target) {
    if (nifSrc->getLink(iSrc, name) != -1 && !controlledBlockList.contains(iDst)) {
            controlledBlockList.append(iDst);
        } else {
            progress++;
        }

        return niControllerCopy(iDst, iSrc, name, blockName, target);
}

QModelIndex Converter::niControllerSetLink(QModelIndex iDst, const QString &name, QModelIndex iControllerDst) {
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

uint Converter::enumOptionValue(const QString &enumName, const QString &optionName) {
    bool ok = false;

        uint result = NifValue::enumOptionValue(enumName, optionName, &ok);

        if (!ok) {
            qDebug() << "Enum option value not found" << enumName << optionName;

            conversionResult = false;
        }

        return result;
}

std::tuple<QModelIndex, QString> Converter::getController(const QString &dstType, const QString &valueType, const QString &controlledValueEffect, const QString &controlledValueLighting) {
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

std::tuple<QModelIndex, QString> Converter::getController(const QString &dstType, const QString &valueType, const QString &controlledValue) {
    return getController(dstType, valueType, controlledValue, controlledValue);
}

void Converter::ignoreController(QModelIndex iSrc) {
    ignoreBlock(iSrc, false);
        ignoreBlock(iSrc, "Interpolator", true);
}

QModelIndex Converter::niControllerCopy(QModelIndex iDst, QModelIndex iSrc, QString name, const QString &blockName, const int target) {
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

void Converter::collisionObjectCopy(QModelIndex iDst, QModelIndex iSrc, const QString &name) {
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

QModelIndex Converter::insertNiBlock(const QString &name) { return nifDst->insertNiBlock(name); }

QModelIndex Converter::getIndexSrc(QModelIndex parent, const QString &name) {
    return nifSrc->getIndex(parent, name);
}

QModelIndex Converter::getIndexDst(QModelIndex parent, const QString &name) {
    return nifDst->getIndex(parent, name);
}

void Converter::scaleVector4(QModelIndex iVector4, float scale) {
    Vector4 v = nifDst->get<Vector4>(iVector4);
        v *= scale;
        nifDst->set<Vector4>(iVector4, v);
}

void Converter::collapseScale(QModelIndex iNode, float scale) {
    QModelIndex iVertices = nifDst->getIndex(iNode, "Vertices");
        for (int i = 0; i < nifDst->rowCount(iVertices); i++) {
            QModelIndex iVertex = iVertices.child(i, 0);
            scaleVector4(iVertex, scale);
        }

        // Don't have to scale normals right?
}

void Converter::collapseScaleRigidBody(QModelIndex iNode, float scale) {
    if (scale != 1.0f) {
            scaleVector4(nifDst->getIndex(iNode, "Translation"), scale);
            scaleVector4(nifDst->getIndex(iNode, "Center"), scale);
        }
}

QModelIndex Converter::bhkPackedNiTriStripsShapeAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
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

void Converter::setMax(float &val, float comparison) {
    if (val < comparison) {
            val = comparison;
        }
}

void Converter::setMin(float &val, float comparison) {
    if (val > comparison) {
            val = comparison;
        }
}

QModelIndex Converter::bhkPackedNiTriStripsShapeDataAlt(QModelIndex iSrc, QModelIndex iRigidBodyDst, int row) {
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

QModelIndex Converter::bhkPackedNiTriStripsShape(QModelIndex iSrc, int row, bool &bScaleSet, float &radius) {
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

bool Converter::inRange(int low, int high, int x) {
    return x >= low && x <= high;
}

void Converter::vertexRange(const QModelIndex iVerticesSrc, QVector<Vector3> &vertexList, QMap<int, int> &vertexMap, bool bInRange, int index) {
    if (!bInRange && !vertexMap.contains(index)) {
            // Append the vertex and map the old index to the new one.
            vertexMap[index] = vertexList.count();
            vertexList.append(nifSrc->get<Vector3>(iVerticesSrc.child(index, 0)));
        }
}

void Converter::setVertex(QModelIndex iPoint1, QModelIndex iPoint2, QMap<int, int> &vertexMap, int firstIndex, int index) {
    if (vertexMap.contains(index)) {
            index = vertexMap[index];
        } else {
            index -= firstIndex;
        }

        nifDst->set<ushort>(iPoint1, ushort(index));
        nifDst->set<ushort>(iPoint2, ushort(index));
}

QModelIndex Converter::bhkPackedNiTriStripsShapeDataSubShapeTriangles(QModelIndex iSubShapeSrc, QModelIndex iDataSrc, int firstVertIndex, int row) {
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

QModelIndex Converter::bhkPackedNiTriStripsShapeData(QModelIndex iSrc, QModelIndex iBhkNiTriStripsShapeDst, int row) {
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

QModelIndex Converter::bhkMoppBvTreeShape(QModelIndex iSrc, QModelIndex &parent, int row, bool &bScaleSet, float &radius) {
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

QModelIndex Converter::bhkShape(QModelIndex iSrc, QModelIndex &parent, int row, bool &bScaleSet, float &radius) {
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

QModelIndex Converter::bhkNiTriStripsShape(QModelIndex iSrc, int row, bool &bScaleSet, float &radius) {
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

QModelIndex Converter::bhkNiTriStripsShapeData(QModelIndex iSrc, int row) {
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

QModelIndex Converter::bhkSphereShape(QModelIndex iSrc, int row, bool &bScaleSet, float &radius) {
    QModelIndex iDst = nifDst->insertNiBlock("bhkSphereShape", row);

        nifDst->set<uint>(iDst, "Material", matMap.convert(getIndexSrc(iSrc, "Material")));

        // NOTE: Seems to be always scaled by 10 in source.
        bhkUpdateScale(bScaleSet, radius, 0.1f);
        nifDst->set<float>(iDst, "Radius", nifSrc->get<float>(iSrc, "Radius") * 0.1f);

        setHandled(iDst, iSrc);

        return iDst;
}

QModelIndex Converter::bhkConvexTransformShape(QModelIndex iSrc, QModelIndex &parent, int row, bool &bScaleSet, float &radius) {
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

QModelIndex Converter::bhkTransformShape(QModelIndex iSrc, QModelIndex &parent, int row, bool &bScaleSet, float &radius) {
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

bool Converter::bhkUpdateScale(bool &bScaleSet, float &radius, const float newRadius) {
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

QModelIndex Converter::bhkBoxShape(QModelIndex iSrc, int row, bool &bScaleSet, float &radius) {
    QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

    bhkUpdateScale(bScaleSet, radius, nifSrc->get<float>(iSrc, "Radius"));

    nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));
    nifDst->set<Vector3>(iDst, "Dimensions", nifSrc->get<Vector3>(iSrc, "Dimensions") * radius);
    nifDst->set<float>(iDst, "Radius", 0);

    return iDst;
}

QModelIndex Converter::bhkCapsuleShape(QModelIndex iSrc, int row, bool &bScaleSet, float &radius) {
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

void Converter::collisionObject(QModelIndex parent, QModelIndex iSrc, QString type) {
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

QModelIndex Converter::bhkRigidBody(QModelIndex iSrc, QModelIndex &parent, int row) {
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

QModelIndex Converter::bhkConstraint(QModelIndex iSrc) {
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

QModelIndex Converter::bhkConvexVerticesShape(QModelIndex iSrc, int row, bool &bScaleSet, float &radius) {
    QModelIndex iDst = copyBlock(QModelIndex(), iSrc, row);

        bhkUpdateScale(bScaleSet, radius, nifDst->get<float>(iDst, "Radius"));

        // NOTE: Radius seems to mean scale in FNV context.
        nifDst->set<float>(iDst, "Radius", 0.0f);

        collapseScale(iDst, radius);

        // TODO: Check mat value
        nifDst->set<uint>(iDst, "Material", matMap.convert(nifSrc->getIndex(iSrc, "Material")));

        return iDst;
}

QModelIndex Converter::getBlockSrc(QModelIndex iLink) {
    return nifSrc->getBlock(nifSrc->getLink(iLink));
}

QModelIndex Converter::getBlockSrc(QModelIndex iSrc, const QString &name) {
    return getBlockSrc(getIndexSrc(iSrc, name));
}

QModelIndex Converter::getBlockDst(QModelIndex iLink) {
    return nifDst->getBlock(nifDst->getLink(iLink));
}

QModelIndex Converter::getBlockDst(QModelIndex iDst, const QString &name) {
    return getBlockDst(getIndexDst(iDst, name));
}

QModelIndex Converter::bhkListShape(QModelIndex iSrc, QModelIndex &parent, int row, bool &bScaleSet, float &radius) {
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

bool Converter::setLink(QModelIndex iDst, QModelIndex iTarget) {
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

bool Converter::setLink(QModelIndex iDst, const QString &name, QModelIndex iTarget) {
    return setLink(getIndexDst(iDst, name), iTarget);
}

void Converter::niTexturingProperty(QModelIndex iDst, QModelIndex iSrc, const QString &sequenceBlockName) {
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

void Converter::niMaterialProperty(QModelIndex iDst, QModelIndex iSrc, const QString &sequenceBlockName) {
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

void Converter::particleSystemModifiers(QModelIndex iDst, QModelIndex iSrc, Copier &c) {
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

QModelIndex Converter::niParticleSystem(QModelIndex iSrc) {
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

void Converter::niPSys(QModelIndex iLinkDst, QModelIndex iLinkSrc) {
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

QModelIndex Converter::niPSysColliderManager(QModelIndex iDst, QModelIndex iSrc) {
    QModelIndex iColliderSrc = getBlockSrc(iSrc, "Collider");

        if (iColliderSrc.isValid()) {
            reLink(iDst, iSrc, "Target");
            nifDst->setLink(iDst, "Collider", nifDst->getBlockNumber(niPSysCollider(iColliderSrc)));
        }

        return iDst;
}

QModelIndex Converter::niPSysCollider(QModelIndex iSrc) {
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

QModelIndex Converter::niPSysColliderCopy(QModelIndex iSrc) {
    QModelIndex iDst = copyBlock(QModelIndex(), iSrc);

        reLink(iDst, iSrc, "Spawn Modifier");
        reLink(iDst, iSrc, "Parent");
        nifDst->setLink(iDst, "Next Collider", nifDst->getBlockNumber(niPSysCollider(getBlockSrc(iSrc, "Next Collider"))));
        reLink(iDst, iSrc, "Collider Object");

        return iDst;
}

QModelIndex Converter::bsFurnitureMarker(QModelIndex iSrc) {
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

void Converter::extraDataList(QModelIndex iDst, QModelIndex iSrc) {
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

void Converter::extraDataList(QModelIndex iDst, QModelIndex iSrc, Copier &c) {
    c.ignore("Num Extra Data List");

        if (nifSrc->get<int>(iSrc, "Num Extra Data List") > 0) {
            c.ignore(getIndexSrc(iSrc, "Extra Data List").child(0, 0));
        }

        extraDataList(iDst, iSrc);
}

QModelIndex Converter::bsFadeNode(QModelIndex iSrc) {
    const QString blockType = nifSrc->getBlockName(iSrc);
    QModelIndex iDst = insertNiBlock(blockType);

    Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

    c.ignore("Num Extra Data List");
    c.ignore("Controller");
    c.ignore("Num Properties");
    c.ignore("Collision Object");
    c.ignore("Num Effects");

    if (nifSrc->rowCount(getIndexSrc(iSrc, "Children")) > 0) {
        c.ignore(getIndexSrc(iSrc, "Children").child(0, 0));
    }

    // This block might be a controlled block in which case it will be finalized later.
    progress.ignoreNextIncrease();
    setHandled(iDst, iSrc);

    c.copyValue("Name");

    extraDataList(iDst, iSrc, c);

    niController(iDst, iSrc, "Controller", nifSrc->get<QString>(iSrc, "Name"));

    c.copyValue("Flags");
    c.copyValue("Translation");
    c.copyValue("Rotation");
    c.copyValue("Scale");

    collisionObjectCopy(iDst, iSrc);

    c.copyValue("Num Children");
    nifDst->updateArray(iDst, "Children");
    QVector<qint32> links = nifSrc->getLinkArray(iSrc, "Children");
    for (int i = 0; i < links.count(); i++) {
        if (links[i] == -1) {
            continue;
        }

        QModelIndex linkNode = nifSrc->getBlock(links[i]);
        QModelIndex iChildDst = nifDst->getIndex(iDst, "Children").child(i, 0);

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

        if (nifSrc->get<int>(iSrc, "Num Particle Systems") > 0) {
            QModelIndex iParticleSystemsSrc = getIndexSrc(iSrc, "Particle Systems");
            QModelIndex iParticleSystemsDst = getIndexDst(iDst, "Particle Systems");
            nifDst->updateArray(iParticleSystemsDst);

            for (int i = 0; i < nifSrc->rowCount(iParticleSystemsSrc); i++) {
                reLink(iParticleSystemsDst.child(i, 0), iParticleSystemsSrc.child(i, 0));
            }

            c.processed(iParticleSystemsSrc.child(0, 0));
        }
    } else if (blockType == "BSMultiBoundNode") {
        setLink(iDst, "Multi Bound", bsMultiBound(getBlockSrc(iSrc, "Multi Bound")));
        c.processed("Multi Bound");
    }

    return iDst;
}

void Converter::bsSegmentedTriShapeSegments(QModelIndex iDst, QModelIndex iSrc, Copier &c) {
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

bool Converter::hasProperty(QModelIndex iSrc, const QString &name) {
    QVector<qint32> links = nifSrc->getLinkArray(iSrc, "Properties");

        for (qint32 link : links) {
            if (nifSrc->getBlockName(nifSrc->getBlock(link)).compare(name) == 0) {
                return true;
            }
        }

        return false;
}

QModelIndex Converter::bsSegmentedTriShape(QModelIndex iSrc) {
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

QModelIndex Converter::bsMultiBound(QModelIndex iSrc) {
    QModelIndex iDst = nifDst->insertNiBlock("BSMultiBound");

        copyLink(iDst, iSrc, "Data");

        setHandled(iDst, iSrc);

        return iDst;
}

void Converter::materialData(QModelIndex iSrc, Copier &c) {
    QModelIndex iMaterialDataSrc = getIndexSrc(iSrc, "Material Data");

        c.ignore(iMaterialDataSrc, "Num Materials");

        if (nifSrc->get<uint>(iMaterialDataSrc, "Num Materials") > 0) {
            c.ignore(getIndexSrc(iMaterialDataSrc, "Material Name").child(0, 0));
            c.ignore(getIndexSrc(iMaterialDataSrc, "Material Extra Data").child(0, 0));
        }

        c.ignore(iMaterialDataSrc, "Active Material");
        c.ignore(iMaterialDataSrc, "Material Needs Update");
}

QModelIndex Converter::bsStripParticleSystem(QModelIndex iSrc) {
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

QModelIndex Converter::bsStripPSysData(QModelIndex iSrc) {
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

QModelIndex Converter::niCamera(QModelIndex iSrc) {
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

void Converter::niTriShapeMaterialData(QModelIndex iDst, QModelIndex iSrc, Copier &c) {
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

QModelIndex Converter::niTriShape(QModelIndex iSrc) {
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

QModelIndex Converter::niTriShapeAlt(QModelIndex iSrc) {
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

void Converter::niTriShapeDataArray(const QString &name, bool bHasArray, Copier &c) {
    if (!bHasArray) {
            return;
        }

        c.array(name);
}

void Converter::niTriShapeDataArray(QModelIndex iSrc, const QString &name, const QString &boolName, Copier &c, bool isFlag) {
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

QModelIndex Converter::niTriShapeData(QModelIndex iSrc) {
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

QModelIndex Converter::niPointLight(QModelIndex iSrc) {
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

QString Converter::updateTexturePath(QString fname) {
    if (fname == "") {
            return "";
        }

        int offset = int(strlen(TEXTURE_ROOT));

        if (fname.left(5).compare("Data\\", Qt::CaseInsensitive) == 0) {
            fname.remove(0, 5);
        }

        return fname.insert(offset, TEXTURE_FOLDER);
}

void Converter::bsShaderTextureSet(QModelIndex iDst, QModelIndex iSrc) {
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

void Converter::setFallout4ShaderFlag(uint &flags, const QString &enumName, const QString &optionName) {
    flags |= bsShaderFlagsGet(enumName, optionName);
}

void Converter::setFallout4ShaderFlag1(uint &flags, const QString &optionName) {
    setFallout4ShaderFlag(flags, "Fallout4ShaderPropertyFlags1", optionName);
}

void Converter::setFallout4ShaderFlag2(uint &flags, const QString &optionName) {
    setFallout4ShaderFlag(flags, "Fallout4ShaderPropertyFlags2", optionName);
}

uint Converter::bsShaderFlagsGet(const QString &enumName, const QString &optionName) {
    bool ok = false;

        uint result = NifValue::enumOptionValue(enumName, optionName, &ok);

        if (!ok) {
            qDebug() << __FUNCTION__ << "Failed to find option:" << optionName;

            conversionResult = false;
        }

        return result;
}

uint Converter::bsShaderFlags1Get(QString optionName) {
    return bsShaderFlagsGet("BSShaderFlags", optionName);
}

uint Converter::bsShaderFlags2Get(QString optionName) {
    return bsShaderFlagsGet("BSShaderFlags2", optionName);
}

bool Converter::bsShaderFlagsIsSet(uint shaderFlags, const QString &enumNameSrc, const QString &optionNameSrc) {
    return shaderFlags & bsShaderFlagsGet(enumNameSrc, optionNameSrc);
}

bool Converter::bsShaderFlags1IsSet(uint shaderFlags, QString optionName) {
    return bsShaderFlagsIsSet(shaderFlags, "BSShaderFlags", optionName);
}

bool Converter::bsShaderFlags1IsSet(QModelIndex iShader, QString optionName) {
    return bsShaderFlagsIsSet(nifSrc->get<uint>(iShader, "Shader Flags"), "BSShaderFlags", optionName);
}

bool Converter::bsShaderFlags2IsSet(uint shaderFlags, QString optionName) {
    return bsShaderFlagsIsSet(shaderFlags, "BSShaderFlags2", optionName);
}

bool Converter::bsShaderFlags2IsSet(QModelIndex iShader, QString optionName) {
    return bsShaderFlagsIsSet(nifSrc->get<uint>(iShader, "Shader Flags 2"), "BSShaderFlags2", optionName);
}

void Converter::bsShaderFlagsSet(uint shaderFlagsSrc, uint &flagsDst, const QString &optionNameDst, const QString &optionNameSrc, const QString &enumNameDst, const QString &enumNameSrc) {
    if (bsShaderFlagsIsSet(shaderFlagsSrc, enumNameSrc, optionNameSrc)) {
            bsShaderFlagsAdd(flagsDst, optionNameDst, enumNameDst);
        }
}

void Converter::bsShaderFlagsAdd(uint &flagsDst, const QString &optionNameDst, const QString &enumNameDst) {
    bool ok = false;
        uint flag = NifValue::enumOptionValue(enumNameDst, optionNameDst, & ok);

        if (!ok) {
            qDebug() << __FUNCTION__ << "Failed to find option:" << optionNameDst;

            conversionResult = false;
        }

        // Set the flag
        flagsDst |= flag;
}

void Converter::bsShaderFlags1Add(uint &flagsDst, const QString &optionNameDst) {
    bsShaderFlagsAdd(flagsDst, optionNameDst, "Fallout4ShaderPropertyFlags1");
}

void Converter::bsShaderFlagsAdd(QModelIndex iShaderFlags, const QString &enumName, const QString &option) {
    uint flags = nifDst->get<uint>(iShaderFlags);

        bsShaderFlagsAdd(flags, option, enumName);

        nifDst->set<uint>(iShaderFlags, flags);
}

void Converter::bsShaderFlagsAdd(QModelIndex iDst, const QString &name, const QString &enumName, const QString &option) {
    bsShaderFlagsAdd(getIndexDst(iDst, name), enumName, option);
}

void Converter::bsShaderFlags1Add(QModelIndex iDst, const QString &option) {
    bsShaderFlagsAdd(iDst, "Shader Flags 1", "Fallout4ShaderPropertyFlags1", option);
}

void Converter::bsShaderFlags2Add(QModelIndex iDst, const QString &option) {
    bsShaderFlagsAdd(iDst, "Shader Flags 2", "Fallout4ShaderPropertyFlags2", option);
}

void Converter::bsShaderFlags2Add(uint &flagsDst, const QString &optionNameDst) {
    bsShaderFlagsAdd(flagsDst, optionNameDst, "Fallout4ShaderPropertyFlags2");
}

void Converter::bsShaderFlags1Set(uint shaderFlagsSrc, uint &flagsDst, const QString &nameDst, const QString &nameSrc) {
    bsShaderFlagsSet(shaderFlagsSrc, flagsDst, nameDst, nameSrc, "Fallout4ShaderPropertyFlags1", "BSShaderFlags");
}

void Converter::bsShaderFlags2Set(uint shaderFlagsSrc, uint &flagsDst, const QString &nameDst, const QString &nameSrc) {
    bsShaderFlagsSet(shaderFlagsSrc, flagsDst, nameDst, nameSrc, "Fallout4ShaderPropertyFlags2", "BSShaderFlags2");
}

void Converter::bsShaderFlags1Set(uint shaderFlagsSrc, uint &flagsDst, const QString &name) {
    bsShaderFlags1Set(shaderFlagsSrc, flagsDst, name, name);
}

void Converter::bsShaderFlags2Set(uint shaderFlagsSrc, uint &flagsDst, const QString &name) {
    bsShaderFlags2Set(shaderFlagsSrc, flagsDst, name, name);
}

QString Converter::bsShaderTypeGet(QModelIndex iDst) {
    return NifValue::enumOptionName("BSLightingShaderPropertyShaderType", nifDst->get<uint>(iDst));
}

void Converter::bsShaderTypeSet(QModelIndex iDst, const QString &name) {
    bool ok = false;

        nifDst->set<uint>(iDst, NifValue::enumOptionValue("BSLightingShaderPropertyShaderType", name, &ok));

        if (!ok) {
            qDebug() << "Failed to find enum option" << name;

            conversionResult = false;
        }
}

void Converter::bsShaderFlags(QModelIndex iShaderPropertyDst, uint shaderFlags1Src, uint shaderFlags2Src) {
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

int Converter::getFlagsBSShaderFlags1(QModelIndex iDst, QModelIndex iNiTriStripsData, QModelIndex iBSShaderPPLightingProperty) {
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

void Converter::bSShaderLightingProperty(QModelIndex iDst, QModelIndex iSrc, const QString &sequenceBlockName) {
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

QModelIndex Converter::getShaderProperty(QModelIndex iSrc) {
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

QModelIndex Converter::getShaderPropertySrc(QModelIndex iSrc) {
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

void Converter::properties(QModelIndex iSrc, QModelIndex iShaderPropertyDst, QModelIndex iDst) {
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

void Converter::niAlphaPropertyFinalize(QModelIndex iAlphaPropertyDst, QModelIndex iShaderPropertyDst) {
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

void Converter::shaderProperty(QModelIndex iDst, QModelIndex iSrc, const QString &type, const QString &sequenceBlockName) {
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

void Converter::properties(QModelIndex iSrc, QModelIndex shaderProperty, QModelIndex iDst, Copier &c) {
    c.ignore("Num Properties");

        if (nifSrc->get<int>(iSrc, "Num Properties") > 0) {
            c.ignore(getIndexSrc(iSrc, "Properties").child(0, 0));

            properties(iSrc, shaderProperty, iDst);
        }
}

void Converter::properties(QModelIndex iDst, QModelIndex iSrc) {
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

void Converter::properties(QModelIndex iDst, QModelIndex iSrc, Copier &c) {
    c.ignore("Num Properties");

        if (nifSrc->get<int>(iSrc, "Num Properties") > 0) {
            c.ignore(getIndexSrc(iSrc, "Properties").child(0, 0));
        }

        properties(iDst, iSrc);
}

QModelIndex Converter::niTriStrips(QModelIndex iSrc) {
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

BSVertexDesc Converter::bsVectorFlags(QModelIndex iDst, QModelIndex iSrc) {
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

bool Converter::bsVectorFlagSet(QModelIndex iSrc, const QString &flagName) {
    bool ok = false;
        bool isSet = nifSrc->get<ushort>(iSrc, "BS Vector Flags") & NifValue::enumOptionValue("BSVectorFlags", flagName, &ok);

        if (!ok) {
            qDebug() << __FUNCTION__ << "Enum Option" << flagName << "not found";
        }

        return isSet;
}

QVector<Vector2> Converter::niTriDataGetUVSets(QModelIndex iSrc, Copier &c) {
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

bool Converter::checkHalfFloat(float f) {
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

bool Converter::checkHalfVector3(HalfVector3 v) {
    return checkHalfFloat(v[0]) && checkHalfFloat(v[1]) && checkHalfFloat(v[2]);
}

bool Converter::checkHalfVector2(HalfVector2 v) {
    return checkHalfFloat(v[0]) && checkHalfFloat(v[1]);
}

float Converter::hfloatScale(float hf, float scale) {
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

void Converter::niTriData(QModelIndex iDst, QModelIndex iSrc, Copier &c) {
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

void Converter::niTriStripsData(QModelIndex iSrc, QModelIndex iDst) {
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

void Converter::niTriShapeDataAlt(QModelIndex iDst, QModelIndex iSrc) {
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

void Converter::unhandledBlocks() {
    for ( int i = 0; i < nifSrc->getBlockCount(); i++ ) {
            if (handledBlocks[i]) {
                qDebug() << "Unhandled block:" << i << "\t" << nifSrc->getBlockName(nifSrc->getBlock(i));

                conversionResult = false;
            }
        }
}

void Converter::niControlledBlocksFinalize() {
    for (QModelIndex iBlock : controlledBlockList) {
            niMaterialEmittanceFinalize(iBlock);
            progress++;
        }
}

Converter::Converter(NifModel *nifSrc, NifModel *nifDst, FileProperties fileProps, ProgressReceiver &receiver)
    : nifSrc(nifSrc),
      nifDst(nifDst),
      progress(Progress(uint(nifSrc->getBlockCount()), receiver, fileProps.getFnameSrc())),
      fileProps(fileProps) {
    handledBlocks = QVector<bool>(nifSrc->getBlockCount(), true);

    loadMatMap();
    loadLayerMap();
}

void Converter::convert() {
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

HighLevelLOD::HighLevelLOD(QString fname, QString worldspace, int x, int y) : fname(fname), worldspace(worldspace), x(x), y(y) {
    //
}

bool HighLevelLOD::compare(int level, int otherX, int otherY) {
    int x = this->x;
    int y = this->y;

    adjustToGrid(level, x, y);

    return x == otherX && y == otherY;
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

FileProperties::FileProperties(FileType fileType, QString fnameDst, QString fnameSrc, int lodLevel, int lodXCoord, int lodYCoord)
    : fileType(fileType),
      fnameDst(fnameDst),
      fnameSrc(fnameSrc),
      lodLevel(lodLevel),
      lodXCoord(lodXCoord),
      lodYCoord(lodYCoord) {}

FileProperties::FileProperties(FileType fileType, QString fnameDst, QString fnameSrc) : fileType(fileType), fnameDst(fnameDst), fnameSrc(fnameSrc) {}

FileProperties::FileProperties() : fileType(FileType::Invalid) {}

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

Controller::Controller(int origin, const QString &name) : origin(origin), name(name) {}

void Controller::add(int blockNumber, const QString &blockName) {
    if (clones.contains(blockNumber) || blockNumber == origin) {
        qDebug() << __FUNCTION__ << "Invalid blocknumber";

        exit(1);
    }

    clones.append(blockNumber);
    cloneBlockNames.append(blockName);
}

int Controller::getOrigin() const
{
    return origin;
}

EnumMap::EnumMap(QString enumTypeSrc, QString enumTypeDst, NifModel *nifSrc) : enumTypeSrc(enumTypeSrc), enumTypeDst(enumTypeDst), nifSrc(nifSrc) {}

void EnumMap::check() {
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

uint EnumMap::convert(NifValue option) {
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

uint EnumMap::convert(QModelIndex iSrc) {
    return convert(nifSrc->getValue(iSrc));
}

#include <QFileDialog>

void convertNif() {
    const QString pathSrc = QFileDialog::getExistingDirectory(nullptr, "Select the source directory");

    if (pathSrc == "") {
        return;
    }

    const QString pathDst = QFileDialog::getExistingDirectory(nullptr, "Select the destination directory");

    if (pathDst == "") {
        return;
    }

    convertNif(pathDst, pathSrc);
}

void convertNif(const QString pathDst, QString pathSrc) {
    if (pathDst == "" || !QDir(pathDst).exists()) {
        qDebug() << "Invalid destination";

        return;
    }

    clock_t tStart = clock();
    bool isCanceled = false;

    QList<QString> failedList;
    QList<HighLevelLOD> highLevelLodList;

    ListWrapper<QString> listWrapperFailed = ListWrapper<QString>(failedList);
    ListWrapper<HighLevelLOD> listWrapperHLLODs = ListWrapper<HighLevelLOD>(highLevelLodList);
    FileSaver saver;

    // Progress dialog
    QFutureWatcher<void> futureWatcher;
    ProgressGrid dialog(futureWatcher);
    ProgressReceiver receiver = ProgressReceiver(&dialog);

    if (QFileInfo(pathSrc).isFile()) {
        convert(pathSrc, pathDst, listWrapperHLLODs, listWrapperFailed, receiver, saver);
    } else if (QDir(pathSrc).exists() && pathSrc != "") {
        QDirIterator it(pathSrc, QStringList() << "*.nif", QDir::Files, QDirIterator::Subdirectories);
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
                        convert(elem, pathDst, listWrapperHLLODs, listWrapperFailed, receiver, saver, pathSrc);

                        if (futureWatcher.future().isCanceled()) {
                            return;
                        }
                    }));

            // Display the dialog and start the event loop.
            dialog.exec();

            futureWatcher.waitForFinished();

            if (futureWatcher.future().isCanceled()) {
                isCanceled = true;

                qDebug() << "Canceled";
            }
        } else {
            while (it.hasNext()) {
                convert(it.next(), pathDst, listWrapperHLLODs, listWrapperFailed, receiver, saver, pathSrc);
            }
        }
    } else {
        qDebug() << "Source path not found";

        return;
    }

    if (isCanceled) {
        return;
    }

    // Make sure the progress dialog doesn't appear after this point.
    futureWatcher.finished();

    if (highLevelLodList.count() > 0) {
        QLabel label("Processing High Level LODs");

        label.setAlignment(Qt::AlignCenter);
        label.setMinimumSize(200, 50);
        label.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);

        label.show();

        combineHighLevelLODs(highLevelLodList, failedList, pathDst, receiver);
    }

    if (failedList.count() > 0) {
        qDebug() << "Failed to convert:";

        for (QString s : failedList) {
            qDebug() << s;
        }
    }

    qDebug("Total time elapsed: %.2fs", double(clock() - tStart)/CLOCKS_PER_SEC);

    QMessageBox msgBox;

    msgBox.setText("Done");
    msgBox.exec();
}
