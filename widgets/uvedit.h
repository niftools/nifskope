/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2012, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#ifndef UVEDIT_H
#define UVEDIT_H

#include <QtOpenGL>

#include <QList>
#include <QMap>
#include <QString>

class NifModel;
class QModelIndex;
class QUndoStack;
class TexCache;
class Vector2;

//! Displays and allows editing of UV coordinate data
class UVWidget : public QGLWidget
{
	Q_OBJECT
protected:
	UVWidget( QWidget * parent = 0 );
	~UVWidget();
	
public:
	//! Creates the UV editor widget
	static UVWidget * createEditor( NifModel * nif, const QModelIndex & index );
	
	//! Sets the NIF data
	bool setNifData( NifModel * nif, const QModelIndex & index );
	
	//! From QWidget; the recommended size of the widget
	QSize sizeHint() const;
	//! From QWidget; the minimum size of the widget
	QSize minimumSizeHint() const;
	
	//! Sets the size hint
	void setSizeHint( const QSize & s );
	
	//! Returns the preferred height for this widget, given the width w.
	int heightForWidth( int width ) const;
	
	//! For future use in realtime mouse-driven scaling
	enum EditingMode {
		None,
		Move,
		Scale
	} editmode;

protected:
	void initializeGL();
	void resizeGL( int width, int height );
	void paintGL();

	void mousePressEvent( QMouseEvent * e );
	void mouseReleaseEvent( QMouseEvent * e );
	void mouseMoveEvent( QMouseEvent * e );
	void wheelEvent( QWheelEvent * e );

// should this be public slots?
protected slots:
	//! Does the selection contain this vertex?
	bool isSelected( int index );
	//! Select a vertex
	void select( int index, bool yes = true );
	//! Select a region containing vertices
	void select( const QRegion & r, bool add = true );
	//! Select all
	void selectAll();
	//! Select none
	void selectNone();
	//! Select faces that use the currently selected vertices
	void selectFaces();
	//! Select vertices connected to the currently selected vertices
	void selectConnected();
	//! Move the selection by the specified vector
	void moveSelection( double dx, double dy );
	//! Scale the selection
	void scaleSelection();
	//! Rotate the selection
	void rotateSelection();
	
protected slots:
	void nifDataChanged( const QModelIndex & );
	//! Build the texture slots menu
	void getTexSlots();
	//! Select a texture slot
	void selectTexSlot();
	//! Build the coordinate sets menu
	void getCoordSets();
	//! Select a coordinate set
	void selectCoordSet();
	//! Change the coordinate set in use
	void changeCoordSet( int setToUse );
	//! Duplicate a coordinate set
	void duplicateCoordSet();
	
private:
	//! List of selected vertices
	QList< int > selection;

	QRect selectRect;
	QList<QPoint> selectPoly;
	int selectCycle;
	
	//! A UV face
	struct face {
		int index;
		
		int tc[3];
		
		bool contains( int v ) { return ( tc[0] == v || tc[1] == v || tc[2] == v ); }
		
		face() : index( -1 ) {}
		face( int idx, int tc1, int tc2, int tc3 ) : index( idx ) { tc[0] = tc1; tc[1] = tc2; tc[2] = tc3; }
	};

	QVector< Vector2 > texcoords;
	QVector< face > faces;
	QMap< int, int > texcoords2faces;
	
	QSize sHint;
	
	TexCache * textures;
	QString texfile;
	QModelIndex texsource;
	
	void drawTexCoords();
	
	void setupViewport( int width, int height );
	void updateViewRect( int width, int height );
	bool bindTexture( const QString & filename );
	bool bindTexture( const QModelIndex & iSource );
	
	QVector<int> indices( const QPoint & p ) const;
	QVector<int> indices( const QRegion & r ) const;
	
	QPoint mapFromContents( const Vector2 & v ) const;
	Vector2 mapToContents( const QPoint & p ) const;
	
	void updateNif();
	
	QPointer<NifModel> nif;
	QPersistentModelIndex iShape, iShapeData, iTexCoords, iTex;
	
	//! Submenu for texture slot selection
	QMenu * menuTexSelect;
	//! Group that holds texture slot selection actions
	QActionGroup * texSlotGroup;
	//! List of valid textures
	QStringList validTexs;
	
	//! Names of texture slots
	QStringList texnames;
	//! Texture slot currently being operated on
	int currentTexSlot;
	//! Read texcoords from the nif
	bool setTexCoords();
	//! Coordinate set currently in use
	int currentCoordSet;
	
	//! Submenu for coordinate set selection
	QMenu * coordSetSelect;
	//! Group that holds coordinate set actions
	QActionGroup * coordSetGroup;
	//! Action to trigger duplication of current coordinate set
	QAction * aDuplicateCoords;

	GLdouble glViewRect[4];

	QPointF pos;

	QPoint mousePos;
	
	GLdouble zoom;
	
	QUndoStack * undoStack;
	
	friend class UVWSelectCommand;
	friend class UVWMoveCommand;
	friend class UVWScaleCommand;
	friend class UVWRotateCommand;
	
	QAction * aTextureBlend;
};

//! Dialog for getting scaling factors
class ScalingDialog : public QDialog
{
	Q_OBJECT
public:
	ScalingDialog( QWidget * parent = 0 );
	
protected:
	/*
	QVBoxLayout * vbox;
	QHBoxLayout * hbox;
	QHBoxLayout * btnbox;
	*/
	QGridLayout * grid;
	QDoubleSpinBox * spinXScale;
	QDoubleSpinBox * spinYScale;
	QCheckBox * uniform;
	QDoubleSpinBox * spinXMove;
	QDoubleSpinBox * spinYMove;
	
public slots:
	float getXScale();
	float getYScale();
	float getXMove();
	float getYMove();
	
protected slots:
	void setUniform( bool status );
};

#endif
