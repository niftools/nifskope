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
#include <QMenu>
#include <QWidget>


class NifModel;
class NifProxyModel;
class GLView;
class Popup;
 
class QListView;
class QTreeView;
class QModelIndex;

class QAbstractButton;
class QButtonGroup;
class QCheckBox;
class QGroupBox;
class QSplitter;
class QTextEdit;

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
	
	void addMessage( const QString & );
	
protected slots:
	void clearRoot();
	void selectRoot( const QModelIndex & );
	
	void setListMode( QAbstractButton * );
	
	void updateConditionZero();
	
	void dataChanged( const QModelIndex &, const QModelIndex & );
	void contextMenu( const QPoint & pos );
	
	void toggleMessages();
	void delayedToggleMessages();
	
protected:
	void showHideRows( QModelIndex );
	
private:
	NifModel * model;
	NifProxyModel * proxy;
	
	QTreeView * list;
	QTreeView * tree;
	GLView * ogl;
	
	Popup * popOpts;

	QLineEdit * lineLoad;
	QLineEdit * lineSave;
	
	QButtonGroup * listMode;
	
	QCheckBox * conditionZero;
	QCheckBox * autoSettings;
	
	QGroupBox * msgroup;
	QTextEdit * msgview;
	
	QSplitter * split;
};


#endif
