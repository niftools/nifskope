#include "copier.h"

void Copier::arrayError(const QString msg) {
    qDebug() << "Array Copy error:"
             << msg << "Source:"
             << nifSrc->getBlockName(iSrc)
             << "Destination:"
             << nifDst->getBlockName(iDst);
}

void Copier::setISrc(const QModelIndex &value)
{
iSrc = value;
}

Copier::Copier(QModelIndex iDst, QModelIndex iSrc, NifModel *nifDst, NifModel *nifSrc)
    : iDst(iDst),
      iSrc(iSrc),
      nifDst(nifDst),
      nifSrc(nifSrc) {
    if (!iDst.isValid() && iDst != QModelIndex()) {
        qDebug() << __FILE__ << __LINE__ << "Invalid Destination. Source:" << nifSrc->getBlockName(iSrc);
    }

    if (!iSrc.isValid() && iSrc != QModelIndex()) {
        qDebug() << __FILE__ << __LINE__ << "Invalid Source. Destination:" << nifDst->getBlockName(iDst);
    }

    usedValues = std::map<QModelIndex, ValueState>();

    if (FIND_UNUSED) {
        makeValueList(iSrc, usedValues);
    }
}

Copier::~Copier() {
    printUnused();
}

void Copier::makeValueList(const QModelIndex iSrc, std::map<QModelIndex, ValueState> &usedValues) {
    if (!FIND_UNUSED) {
        return;
    }

    /** Loop over the children of the source model index to find values. */
    for (int r = 0; r < nifSrc->rowCount(iSrc); r++) {
        QModelIndex iLink = iSrc.child(r, 0);

        /**
             * TODO: Change this to more efficient method.
             * Check wether the value can be obtained by name.
             */
        if (
                !nifSrc->getIndex(iSrc, nifSrc->getBlockName(iLink)).isValid() ||
                iLink != nifSrc->getIndex(iSrc, nifSrc->getBlockName(iLink))) {
            continue;
        }

        /**
             * If the value is an array, find values in its children. Else, add it to the map.
             */
        if (nifSrc->rowCount(iLink) > 0) {
            makeValueList(iLink, usedValues);
        } else {
            NifValue v = nifSrc->getValue(iLink);
            if (v.isValid()) {
                usedValues[iLink] = ValueState::Unused;
            }
        }
    }
}

void Copier::printUnused() {
    for (std::map<QModelIndex, ValueState>::iterator it= usedValues.begin(); it!=usedValues.end(); ++it) {
        if (it->second == ValueState::Unused) {
            qDebug() << "Unused:" << nifSrc->getBlockName(it->first) << "from" << nifSrc->getBlockName(iSrc);
        }
    }
}

bool Copier::setStatus(QModelIndex iSource, ValueState status) {
    if (!FIND_UNUSED) {
        return true;
    }

    if (!iSource.isValid()) {
        qDebug() << __FUNCTION__ << "Invalid in" << nifSrc->getBlockName(iSrc);

        return false;
    }

    /** If the source value is an array, get its first child instead. */
    if (nifSrc->isArray(iSource)) {
        if (nifSrc->rowCount(iSource) > 0) {
            iSource = iSource.child(0, 0);
        } else {
            return true;
        }
    }

    /** Set the status. */
    if (usedValues.count(iSource) != 0) {
        usedValues[iSource] = status;
    } else {
        qDebug() << "Key" << nifSrc->getBlockName(iSource) << "not found in" << nifSrc->getBlockName(iSrc);

        return false;
    }

    return true;
}

bool Copier::setStatus(const QModelIndex iSource, ValueState status, const QString &name) {
    if (!setStatus(nifSrc->getIndex(iSource, name), status)) {
        qDebug() << __FUNCTION__ << name << "Invalid";

        return false;
    }

    return true;
}

bool Copier::ignore(const QModelIndex iSource) {
    return setStatus(iSource, ValueState::Ignored);
}

bool Copier::ignore(const QString &name) {
    return setStatus(iSrc, ValueState::Ignored, name);
}

bool Copier::processed(const QModelIndex iSource) {
    return setStatus(iSource, ValueState::Processed);
}

bool Copier::processed(QModelIndex iSource, const QString &name) {
    return setStatus(iSource, ValueState::Processed, name);
}

bool Copier::processed(const QString &name) {
    return setStatus(iSrc, ValueState::Processed, name);
}

bool Copier::copyValue(const QModelIndex iTarget, const QModelIndex iSource, const QString &nameDst, const QString &nameSrc) {
    QModelIndex iDstNew = nifDst->getIndex(iTarget, nameDst);
    QModelIndex iSrcNew = nifSrc->getIndex(iSource, nameSrc);

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

bool Copier::copyValue(const QModelIndex iTarget, const QModelIndex iSource, const QString &name) {
    return copyValue(iTarget, iSource, name, name);
}

bool Copier::copyValue(const QString &name) {
    return copyValue(name, name);
}

bool Copier::copyValue(const QString &nameDst, const QString &nameSrc) {
    return copyValue(iDst, iSrc, nameDst, nameSrc);
}

bool Copier::copyValue(const QModelIndex iTarget, const QModelIndex iSource) {
    NifValue val = nifSrc->getValue(iSource);

    if (!val.isValid()) {
        qDebug() << "Invalid Value";

        return false;
    }

    /** Make sure strings are copied as strings. */
    if (
            val.type() == NifValue::tStringIndex ||
            val.type() == NifValue::tSizedString ||
            val.type() == NifValue::tText ||
            val.type() == NifValue::tShortString ||
            val.type() == NifValue::tHeaderString ||
            val.type() == NifValue::tLineString ||
            val.type() == NifValue::tChar8String) {
        return copyValue<QString>(iTarget, iSource);
        /** Copy flags as their raw data. */
    } else if (val.type() == NifValue::tFlags) {
        return copyValue<uint>(iTarget, iSource);
        /** Copy normally */
    } else if (!nifDst->setValue(iTarget, val)) {
        qDebug() << "Failed to set value on" << nifDst->getBlockName(iTarget);

        return false;
    }

    if (FIND_UNUSED) {
        usedValues[iSource] = ValueState::Processed;
    }

    return true;
}

bool Copier::tree(const QModelIndex iDst, const QModelIndex iSrc) {
    /** Copy the value as an array. */
    if (nifSrc->isArray(iSrc)) {
        array(iDst, iSrc);
        /** Recursively copy values. */
    } else if (nifSrc->rowCount(iSrc) > 0) {
        for (int i = 0; i < nifSrc->rowCount(iSrc); i++) {
            tree(iDst.child(i, 0), iSrc.child(i, 0));
        }
        /** Copy the value normally */
    } else if (!copyValue(iDst, iSrc)) {
        arrayError("Failed to copy value.");

        return false;
    }

    return true;
}

bool Copier::tree() {
    return tree(iDst, iSrc);
}

bool Copier::array(const QModelIndex iDst, const QModelIndex iSrc) {
    int rows = nifSrc->rowCount(iSrc);

    /** Make sure the target array is initialized. */
    nifDst->updateArray(iDst);

    if (rows != nifDst->rowCount(iDst)) {
        arrayError("Mismatched arrays.");

        return false;
    }

    /** Copy the values */
    for (int i = 0; i < rows; i++) {
        tree(iDst.child(i, 0), iSrc.child(i, 0));
    }

    return true;
}

bool Copier::array(const QString &nameDst, const QString &nameSrc) {
    return array(nifDst->getIndex(iDst, nameDst), nifSrc->getIndex(iSrc, nameSrc));
}

bool Copier::array(const QString &name) {
    return array(name, name);
}

void Copier::setIDst(const QModelIndex &value)
{
    iDst = value;
}
