#ifndef SANITIZE_H
#define SANITIZE_H

#include "spellbook.h"

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

// Base class for all warning and error checkers
class spChecker : public Spell
{
public:
	QString name() const override { return {}; }
	QString page() const override { return {}; }
	bool sanity() const override { return true; }
	bool constant() const override { return true; }
	bool checker() const override { return true; }

	bool isApplicable(const NifModel *, const QModelIndex & index) override
	{
		return false;
	}

	QModelIndex cast(NifModel * nif, const QModelIndex &) override { return {};	}

	static QString message() { return {}; }
};

class spErrorNoneRefs : public spChecker
{
public:
	QString name() const override { return Spell::tr( "None Refs" ); }
	QString page() const override final { return Spell::tr( "Error Checking" ); }

	bool isApplicable( const NifModel *, const QModelIndex & index ) override;

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final;

	static QString message()
	{
		return tr("One or more blocks contain None Refs.");
	}

	void checkArray( NifModel * nif, const QModelIndex & idx, QVector<QString> rows );
	void checkRef( NifModel * nif, const QModelIndex & idx, const QString & name );
};


class spErrorInvalidPaths : public spChecker
{
public:
	QString name() const override { return Spell::tr( "Invalid Paths" ); }
	QString page() const override final { return Spell::tr( "Error Checking" ); }

	bool isApplicable( const NifModel *, const QModelIndex & index ) override;

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final;

	typedef enum
	{
		P_EMPTY,
		P_ABSOLUTE,
		P_NO_EXT,
		P_DEFAULT = (P_ABSOLUTE | P_NO_EXT)
	} InvalidPath;

	static QString message()
	{
		return tr("One or more asset path strings contain an invalid path.");
	}

	void checkPath( NifModel * nif, const QModelIndex & idx, const QString & name, InvalidPath invalid = P_DEFAULT );
};


class spWarningEnvironmentMapping : public spChecker
{
public:
	QString name() const override { return Spell::tr("Environment Mapping Flags"); }
	QString page() const override final { return Spell::tr("Error Checking"); }

	bool isApplicable(const NifModel *, const QModelIndex & index) override;

	QModelIndex cast(NifModel * nif, const QModelIndex &) override final;

	static QString message()
	{
		return tr("Potential problems detected with Shader Flags.");
	}
};

#endif // SANITIZE_H
