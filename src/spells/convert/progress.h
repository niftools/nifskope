#ifndef PROGRESS_H
#define PROGRESS_H

#include "ui/conversiondialog.h"

/**
 * @brief The Progress class is used to determine the progress of nif conversion.
 * Progress is determined by the number of source blocks which have been processed.
 */
class Progress
{
public:
    /**
     * @brief Progress constructor.
     * @param numBlocksSrc The number of blocks in the source nif
     */
    Progress(uint numBlocksSrc, ProgressReceiver & receiver, QString fileName);

    /**
     * @brief operator ++ increases progress and prints progress on certain conditions.
     */
    void operator++(int);

    void ignoreNextIncrease();
private:
    uint numBlocksSrc;
    uint numBlocksProcessed;
    uint maxBlocks = 100;
    bool bIgnoreNextIncrease;
    ProgressUpdater updater;
    Qt::HANDLE threadID;
};

#endif // PROGRESS_H
