#ifndef SPELL_CONVERT_H
#define SPELL_CONVERT_H

#include "spellbook.h"
#include <QThread>
#include <QMutex>

//! \file convert.h spToFO42

template <typename T>
class ListWrapper
{
public:
    ListWrapper(QList<T> & list) : list(list) {}
    void append(T s);
private:
    QList<T> & list;
    QMutex mu;
};

//! Convert to Fallout 4
//class spToFO42 final : public Spell
//{
//public:
//    QString name() const override final { return Spell::tr( "Convert to Fallout 4" ); }
//    QString page() const override final { return Spell::tr( "Batch" ); }

//    bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
//    QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
//};

void convertNif(QString fname);


#endif // SPELL_CONVERT_H
