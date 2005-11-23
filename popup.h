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


#ifndef POPUP_H
#define POPUP_H

#include <QFrame>

class Popup : public QFrame
{
	Q_OBJECT
public:
	Popup( const QString & title, QWidget * parent );
	virtual ~Popup();
	
	QAction	*	popupAction() const { return aPopup; }
	
signals:
	void	aboutToHide();
	
public slots:
	virtual void popup();
	virtual void popup( const QPoint & p );
	
protected:
	virtual void showEvent( QShowEvent * );
	virtual void hideEvent( QHideEvent * );
	
private slots:
	virtual void popup( bool );
	
private:
	QAction	*	aPopup;
};


#endif
