#include "progress.h"

Progress::Progress(uint numBlocksSrc, ProgressReceiver &receiver, QString fileName)
    : numBlocksSrc(numBlocksSrc),
      updater(ProgressUpdater(QThread::currentThreadId(), receiver, int(numBlocksSrc), fileName))
{
    threadID = QThread::currentThreadId();
    numBlocksProcessed = 0;
    bIgnoreNextIncrease = false;
}

void Progress::operator++(int) {
    if (bIgnoreNextIncrease) {
        bIgnoreNextIncrease = false;

        return;
    }

    numBlocksProcessed++;

    if (numBlocksProcessed > numBlocksSrc) {
        return;
    }

    if (numBlocksProcessed % maxBlocks == 0) {
        if (!RUN_CONCURRENT) {
            printf("%d\tof %d\tblocks processed\n", numBlocksProcessed, numBlocksSrc);
        }
    }

//    emit updater.progress(threadID, int(numBlocksProcessed));
    updater.progress(int(numBlocksProcessed));
}

void Progress::ignoreNextIncrease() {
    bIgnoreNextIncrease = true;
}
