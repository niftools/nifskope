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
	StringPaletteRegexDialog( NifModel * nif, QModelIndex & index, QWidget * parent = 0 );

protected:
	NifModel * nif;
	QModelIndex iPalette;
	QStringListModel * listmodel;
	QListView * listview;
	QGridLayout * grid;

protected slots:
	void stringlistRegex();

};

#endif
