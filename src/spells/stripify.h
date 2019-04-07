#ifndef SP_STRIPIFY_H
#define SP_STRIPIFY_H

#include "spellbook.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file stripify.h
 * \brief Stripify spells header
 *
 * All classes here inherit from the Spell class.
 */

class spStichStrips final : public Spell
{
public:
    QString name() const override final;
    QString page() const override final;
    static QModelIndex getStripsData( const NifModel * nif, const QModelIndex & index );
    bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
    QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

class spUnstichStrips final : public Spell
{
public:
    QString name() const override final;
    QString page() const override final;
    bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final;
    QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final;
};

template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc );

template <typename T> void copyArray( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name );

template <typename T> void copyValue( NifModel * nif, const QModelIndex & iDst, const QModelIndex & iSrc, const QString & name );

#endif // SP_STRIPIFY_H
