#ifndef SPELL_STRINGPALETTE_H
#define SPELL_STRINGPALETTE_H

#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QStringListModel>

class NifModel;

class StringPaletteRegexDialog : public QDialog
{
	Q_OBJECT
public:
	StringPaletteRegexDialog( NifModel * nif, QPersistentModelIndex & index, QWidget * parent = 0 );

protected:
	NifModel * nif;
	QPersistentModelIndex iPalette;
	QListView * listview;
	QStringListModel * listmodel;
	QStringList * originalList;
	QGridLayout * grid;
	QLineEdit * search;
	QLineEdit * replace;

public slots:
	void stringlistRegex();
	void setStringList( QStringList & list );

};

#endif
