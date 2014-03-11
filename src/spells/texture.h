#ifndef SPELL_TEXTURE_H
#define SPELL_TEXTURE_H

#include <QOpenGLContext>

#include <QDialog>
//#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QListView>
#include <QStringListModel>

#include "../widgets/nifeditors.h"

class NifModel;

//! Texture selection dialog for TexFlipController
class TexFlipDialog : public QDialog
{
	Q_OBJECT
public:
	TexFlipDialog( NifModel * nif, QModelIndex & index, QWidget * parent = 0 );
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
