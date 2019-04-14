#ifndef SPELL_CONVERT_H
#define SPELL_CONVERT_H

#include "spellbook.h"

//! \file convert.h spToFO42

//! Convert to Fallout 4
//class spToFO42 final : public Spell
//{
//public:
//    QString name() const override final { return Spell::tr( "Convert to Fallout 4" ); }
//    QString page() const override final { return Spell::tr( "Batch" ); }

//    bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
//    QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
//};

QModelIndex convertNif(QString fname);


#endif // SPELL_CONVERT_H
