#ifndef SPELL_TEXTURE_H
#define SPELL_TEXTURE_H

#include <QDialog> // Inherited
#include <QModelIndex>


class NifFloatEdit;
class NifModel;
class QGridLayout;
class QListView;
class QPushButton;
class QStringListModel;

//! Texture selection dialog for TexFlipController
class TexFlipDialog final : public QDialog
{
	Q_OBJECT

public:
	TexFlipDialog( NifModel * nif, QModelIndex & index, QWidget * parent = nullptr );
	QStringList flipList();

protected:
	NifModel * nif;
	QModelIndex baseIndex;
	NifFloatEdit * startTime;
	NifFloatEdit * stopTime;
	QStringList flipnames;
	QGridLayout * grid;
	QListView * listview;
	QStringListModel * listmodel;
	QPushButton * textureButtons[4];

protected slots:
	void textureAction( int i );
	void texIndex( const QModelIndex & idx );
	void listFromNif();
};

#endif
