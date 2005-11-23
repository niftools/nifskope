/****************************************************************************
**
** nifscope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#ifndef NIFSKOPE_H
#define NIFSKOPE_H

#include <QLineEdit>
#include <QWidget>


class NifModel;
class GLView;
class Popup;
 
class QCheckBox;
class QListView;
class QTreeView;
class QModelIndex;

class NifSkope : public QWidget
{
Q_OBJECT
public:
	NifSkope();
	~NifSkope();
	
public slots:
	void load();
	void save();
	
	void loadBrowse();
	void saveBrowse();
	
	void saveOptions();
	
	void about();
	
protected slots:
	void selectRoot();
	void updateConditionZero();
	
	void dataChanged( const QModelIndex &, const QModelIndex & );
	
protected:
	void showHideRows( QModelIndex );
	
public:
	QLineEdit * lineLoad;
	QLineEdit * lineSave;
	
	Popup * popOpts;

	NifModel * model;
	
	QListView * list;
	QTreeView * tree;
	GLView * ogl;
	
	QCheckBox * conditionZero;
	QCheckBox * autoSettings;
};


#endif
