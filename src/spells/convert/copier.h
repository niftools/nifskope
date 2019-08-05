#ifndef COPIER_H
#define COPIER_H

#include "src/model/nifmodel.h"

/**
 * Determines whether the Copier class will search for all values under the source model index. Set to true to search
 * for values.
 */
#define FIND_UNUSED false

/**
 * @brief The ValueState enum describes whether values have been processed, ignored or not been used.
 */
enum ValueState
{
    Unused,
    Processed,
    Ignored,
};

/**
 * @brief The Copier class is used to copy values from one model index to another.
 */
class Copier
{
    QModelIndex iDst;
    QModelIndex iSrc;
    NifModel * nifDst;
    NifModel * nifSrc;
    std::map<QModelIndex, ValueState> usedValues;
public:
    /**
     * @brief Set up the Copier to copy from the source model index to destination model index.
     * @param iDst   Destination model index belonging to the destination nif
     * @param iSrc   Source model index belonging to the source nif
     * @param nifDst Destination nif
     * @param nifSrc Source nif
     */
    Copier(QModelIndex iDst, QModelIndex iSrc, NifModel * nifDst, NifModel * nifSrc);

    /**
     * Print the list of unused values on destruction.
     */
    ~Copier();

    /**
     * @brief Recursively find all children of the given model index and mark them as unused.
     * @param iSrc       Source model index
     * @param usedValues Map of values to their use state
     */
    void makeValueList(const QModelIndex iSrc, std::map<QModelIndex, ValueState> & usedValues);

    /**
     * @brief printUnused prints all unused values from the source model index.
     */
    void printUnused();

    /**
     * @brief setStatus sets the use status of a value.
     * @param iSource Source model index
     * @param status  Use status
     * @return True if the status has been successfully set.
     */
    bool setStatus(QModelIndex iSource, ValueState status);

    /**
     * @brief setStatus calls setStatus to set the status of a value by name.
     * @param iSource Parent source model index
     * @param status  Use status
     * @param name    Name of child
     * @return True if the status has been successfully set.
     */
    bool setStatus(const QModelIndex iSource, ValueState status, const QString & name);

    /**
     * @brief ignore sets a value's status to 'Ignored'.
     * @param iSource Source model index
     * @return True if the status has been successfully set.
     */
    bool ignore(const QModelIndex iSource);

    /**
     * @brief ignore sets a value's status to 'Ignored'.
     * @param iSource Source model index
     * @param name    Name of child
     * @return True if the status has been successfully set.
     */
    bool ignore(QModelIndex iSource, const QString & name) {
        return setStatus(iSource, ValueState::Ignored, name);
    }

    /**
     * @brief ignore sets a value's status to 'Ignored'.
     * @param  name Name of child
     * @return True if the status has been successfully set.
     */
    bool ignore(const QString & name);

    /**
     * @brief processed sets a value's status to 'Processed'.
     * @param iSource Source model index
     * @return True if the status has been successfully set.
     */
    bool processed(const QModelIndex iSource);

    /**
     * @brief processed sets a value's status to 'Processed'.
     * @param iSource Source model index
     * @param name    Name of child
     * @return True if the status has been successfully set.
     */
    bool processed(QModelIndex iSource, const QString & name);

    /**
     * @brief processed sets a value's status to 'Processed'.
     * @param name    Name of child
     * @return True if the status has been successfully set.
     */
    bool processed(const QString & name);

    template<typename T>
    /**
     * @brief getVal gets the value at the given model index as type T.
     * @param nif     Nif of the given model index
     * @param iSource Value's model index
     * @return Value as T
     */
    T getVal(NifModel * nif, const QModelIndex iSource) {
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
                usedValues[iSource] = ValueState::Processed;
            }
        }

        return val.get<T>();
    }

    template<typename T>
    /**
     * @brief Overload to get the value by name.
     * @param nif     Nif of the given model index
     * @param iSource Value's parent model index
     * @param name    Name of value
     * @return Value as T
     */
    T getVal(NifModel * nif, const QModelIndex iSource, const QString & name) {
        return getVal<T>(nif, nif->getIndex(iSource, name));
    }

    template<typename T>
    /**
     * @brief getSrc calls getVal to get a value from the class source nif only.
     * @param iSource Value's model index
     * @return Value as T
     */
    T getSrc(const QModelIndex iSource) {
        return getVal<T>(nifSrc, iSource);
    }

    template<typename T>
    /**
     * @brief getDst calls getVal to get a value from the class destination nif only.
     * @param iSource Value's model index
     * @return Value as T
     */
    T getDst(const QModelIndex iSource) {
        return getVal<T>(nifDst, iSource);
    }

    template<typename T>
    /**
     * @brief Overload to get the value by name with the class source model index as the parent.
     * @param name Name of value
     * @return Value as T
     */
    T getSrc(const QString & name) {
        return getVal<T>(nifSrc, iSrc, name);
    }

    template<typename T>
    /**
     * @brief Overload to get the value by name with the class destination model index as the parent.
     * @param name Name of value
     * @return Value as T
     */
    T getDst(const QString & name) {
        return getVal<T>(nifDst, iDst, name);
    }

    template<typename T>
    /**
     * @brief Overload to get the value by name.
     * @param iSource Source model index
     * @param name    Name of value
     * @return Value as T
     */
    T getSrc(const QModelIndex iSource, const QString & name) {
        return getVal<T>(nifSrc, iSource, name);
    }

    template<typename T>
    /**
     * @brief Overload to get the value by name.
     * @param iSource Source model index
     * @param name    Name of value
     * @return Value as T
     */
    T getDst(const QModelIndex iSource, const QString & name) {
        return getVal<T>(nifDst, iSource, name);
    }

    /**
     * @brief copyValue copies the value at the source model index to the destination model index.
     * @param iTarget Target model index
     * @param iSource Source model index
     * @return True if successful
     */
    bool copyValue(const QModelIndex iTarget, const QModelIndex iSource);

    /**
     * @brief Overload to copy values by name.
     * @param iTarget Target parent model index
     * @param iSource Source parent model index
     * @param nameDst Name of value in target
     * @param nameSrc Name of value in source
     * @return True if successful
     */
    bool copyValue(
            const QModelIndex iTarget,
            const QModelIndex iSource,
            const QString & nameDst,
            const QString & nameSrc);

    /**
     * @brief Overload to copy values by name.
     * @param iTarget Target parent model index
     * @param iSource Source parent model index
     * @param name    Name of value in target and source
     * @return True if successful
     */
    bool copyValue(const QModelIndex iTarget, const QModelIndex iSource, const QString & name);

    /**
     * @brief Overload to copy values by name in the class destination and source model indices.
     * @param nameDst Name of value in class target
     * @param nameSrc Name of value in class source
     * @return True if successful
     */
    bool copyValue(const QString & nameDst, const QString & nameSrc);

    /**
     * @brief Overload to copy values by name in the class destination and source model indices.
     * @param name Name of value in class target and source
     * @return True if successful
     */
    bool copyValue(const QString & name);

    template<typename TDst, typename TSrc>
    /**
     * @brief Overload to copy values as type TSrc to type TDst.
     * @param iTarget Target model index
     * @param iSource Source model index
     * @return True if successful
     */
    bool copyValue(const QModelIndex iTarget, const QModelIndex iSource) {
        if (!nifDst->set<TDst>(iTarget, nifSrc->get<TSrc>(iSource))) {
            qDebug() << "Failed to set value on" << nifDst->getBlockName(iTarget);

            return false;
        }

        if (FIND_UNUSED) {
            usedValues[iSource] = ValueState::Processed;
        }

        return true;
    }

    template<typename T>
    /**
     * @brief Overload to copy values as type T.
     * @param iTarget Target model index
     * @param iSource Source model index
     * @return True if successful
     */
    bool copyValue(const QModelIndex iTarget, const QModelIndex iSource) {
        return copyValue<T, T>(iTarget, iSource);
    }

    template<typename TDst, typename TSrc>
    /**
     * @brief Overload to copy from class source model index to class destination model index.
     * @return True if successful
     */
    bool copyValue() {
        return copyValue<TDst, TSrc>(iDst, iSrc);
    }

    template<typename TDst, typename TSrc>
    /**
     * @brief Overload to copy values by name in the class destination and source model indices.
     * @param nameDst Name of value in class target
     * @param nameSrc Name of value in class source
     * @return True if successful
     */
    bool copyValue(const QString & nameDst, const QString & nameSrc) {
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

    template<typename TDst, typename TSrc>
    /**
     * @brief Overload to copy values by name in the class destination and source model indices.
     * @param name Name of value in class target and source
     * @return True if successful
     */
    bool copyValue(const QString & name) {
        return copyValue<TDst, TSrc>(name, name);
    }

    template<typename T>
    /**
     * @brief Overload to copy from class source model index to class destination model index.
     * @return True if successful
     */
    bool copyValue() {
        return copyValue<T, T>();
    }

    template<typename T>
    /**
     * @brief Overload to copy values by name in the class destination and source model indices.
     * @param nameDst Name of value in class target
     * @param nameSrc Name of value in class source
     * @return True if successful
     */
    bool copyValue(const QString & nameDst, const QString & nameSrc) {
        return copyValue<T, T>(nameDst, nameSrc);
    }

    template<typename T>
    /**
     * @brief Overload to copy values by name in the class destination and source model indices.
     * @param name Name of value in class target and source
     * @return True if successful
     */
    bool copyValue(const QString & name) {
        return copyValue<T, T>(name, name);
    }

    // Traverse all child nodes
    /**
     * @brief tree copies the value and all child values from the given source model index to the given destination
     *        model index.
     * @param iDst Destination model index
     * @param iSrc Source model index
     * @return True if successful
     */
    bool tree(const QModelIndex iDst, const QModelIndex iSrc);

    /**
     * @brief Overload to call tree with the class destination model index and the class source model index.
     * @return True if successful
     */
    bool tree();

    /**
     * @brief array copies value arrays.
     * @param iDst Destination model index
     * @param iSrc Source model index
     * @return True if successful
     */
    bool array(const QModelIndex iDst, const QModelIndex iSrc);

    /**
     * @brief Overload to copy arrays by name.
     * @param nameDst Name of array in destination model index
     * @param nameSrc Name of array in source model index
     * @return True if successful
     */
    bool array(const QString & nameDst, const QString & nameSrc);

    /**
     * @brief Overload to copy arrays by name.
     * @param name Name of array in destination and source model indices
     * @return True if successful
     */
    bool array(const QString & name);

    /** iDst setter */
    void setIDst(const QModelIndex &value);

    /** iSrc setter */
    void setISrc(const QModelIndex &value);
private:
    /**
     * @brief arrayError makes an error message for failed array copying.
     * @param msg Message
     */
    void arrayError(const QString msg = "");
};

#endif // COPIER_H
