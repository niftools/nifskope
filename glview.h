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


#ifndef GLVIEW
#define GLVIEW

class NifModel;

#ifdef QT_OPENGL_LIB

#include <QGLWidget>
#include <QStack>

class GLView : public QGLWidget
{
	Q_OBJECT

public:
	GLView();
	~GLView();
	
	int xRotation() const { return xRot; }
	int yRotation() const { return yRot; }
	int zRotation() const { return zRot; }
	
	bool lighting() const { return lightsOn; }
	
	QString textureFolder() const { return texfolder; }
	
	QSize minimumSizeHint() const { return QSize( 50, 50 ); }
	QSize sizeHint() const { return QSize( 400, 400 ); }

public slots:
	void setNif( NifModel * );

	void compile();

	void setXRotation(int angle);
	void setYRotation(int angle);
	void setZRotation(int angle);
	void setZoom( int zoom );
	void setXTrans( int );
	void setYTrans( int );
	
	void setLighting( bool );
	
	void setTextureFolder( const QString & );

signals: 
	void xRotationChanged(int angle);
	void yRotationChanged(int angle);
	void zRotationChanged(int angle);
	void zoomChanged( int zoom );

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseDoubleClickEvent( QMouseEvent * );
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent( QWheelEvent * event );

private slots:
	void advanceGears();
	
	void dataChanged();

private:
	void normalizeAngle(int *angle);
	
	bool compileNode( int b, bool alphatoggle );
	GLuint compileTexture( QString filename );

	GLuint nif;
	QList<GLuint> textures;
	
	GLuint click_tex;
	
	int xRot;
	int yRot;
	int zRot;
	int zoom;
	
	int xTrans;
	int yTrans;
	
	bool updated;
	bool doCompile;
	
	QPoint lastPos;
	
	NifModel * model;
	
	QString texfolder;
	
	bool lightsOn;
	
	QStack<int> nodestack;
};

#else
class GLView {};
#endif


#endif
