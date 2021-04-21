#ifndef SPELL_STRINGPALETTE_H
#define SPELL_STRINGPALETTE_H

#include <QDialog> // Inherited
#include <QPersistentModelIndex>


//! \file stringpalette.h StringPaletteRegexDialog

class NifModel;

class QGridLayout;
class QLineEdit;
class QListView;
class QStringListModel;
class QWidget;

//! String palette QRegularExpression dialog for spEditStringEntries
class StringPaletteRegexDialog final : public QDialog
{
	Q_OBJECT

public:
	//! Constructor. Sets widgets and layout.
	StringPaletteRegexDialog( NifModel * nif, QPersistentModelIndex & index, QWidget * parent = nullptr );

protected:
	//! Model used
	NifModel * nif;
	//! Index of the string palette
	QPersistentModelIndex iPalette;
	//! Lists the strings in the palette
	QListView * listview;
	//! Internal representation of the current state of the palette
	QStringListModel * listmodel;
	//! Original palette
	QStringList * originalList;
	//! Grid layout
	QGridLayout * grid;
	//! Regular expression to search for
	QLineEdit * search;
	//! String to replace with
	QLineEdit * replace;

public slots:
	//! Performs regex replacement of \link search \endlink to \link replace \endlink in \link listmodel \endlink
	void stringlistRegex();
	//! Set the string palette entries
	void setStringList( QStringList & list );
	//! Get the modified string palette
	QStringList getStringList();
};

#endif
