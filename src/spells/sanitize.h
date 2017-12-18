#ifndef SANITIZE_H
#define SANITIZE_H

//! Reorders blocks
class spSanitizeBlockOrder final : public Spell
{
public:
    QString name() const override final { return Spell::tr( "Reorder Blocks" ); }
    QString page() const override final { return Spell::tr( "Sanitize" ); }
    // Prevent this from running during auto-sanitize for the time being
    //	Can really only cause issues with rendering and textureset overrides via the CK
    bool sanity() const { return false; }

    bool isApplicable( const NifModel *, const QModelIndex & index ) override final;

    // check whether the block is of a type that comes before the parent or not
    bool childBeforeParent( NifModel * nif, qint32 block );

    // build the nif tree at node block; the block itself and its children are recursively added to
    // the newblocks list
    void addTree( NifModel * nif, qint32 block, QList<qint32> & newblocks );

    QModelIndex cast( NifModel * nif, const QModelIndex & ) override final;
};


#endif // SANITIZE_H
