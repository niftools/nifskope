/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>

class QAbstractButton;
class QAction;
class QColor;
class QCheckBox;
class QDialog;
class QLineEdit;
class QListView;
class QModelIndex;
class QRadioButton;
class QSpinBox;
class QStringListModel;
class QTimer;

class AlphaSlider;
class ColorWheel;
class FileSelector;
class GroupBox;

class Options : public QObject
{
	Q_OBJECT
public:
	static Options * get();
	static QList<QAction*> actions();
	
	static QStringList textureFolders();
	static bool textureAlternatives();
	
	static bool antialias();
	static bool texturing();
	static bool shaders();
	
	static bool blending() { return true; }
	
	static QColor bgColor();
	static QColor nlColor();
	static QColor hlColor();
	
	static QRegExp cullExpression();
	static bool onlyTextured();
	
	static bool drawAxes();
	static bool drawNodes();
	static bool drawHavok();
	static bool drawConstraints();
	static bool drawFurn();
	static bool drawHidden();
	static bool drawStats();
	
	static bool benchmark();
	
	typedef enum {
		ZAxis, YAxis, XAxis
	} Axis;

	static Axis upAxis();
	
	static QColor ambient();
	static QColor diffuse();
	static QColor specular();
	
	static bool lightFrontal();
	static int lightDeclination();
	static int lightPlanarAngle();

	static QString startupVersion();
	
signals:
	void sigChanged();

public slots:
	void save();
	
protected slots:
	void textureFolderAction( int );
	void textureFolderIndex( const QModelIndex & );
	void textureFolderAutoDetect( int game );
	void activateLightPreset( int );
	
protected:
	Options();
	~Options();
	
	bool eventFilter( QObject * o, QEvent * e );
	
	QAction * aDrawAxes;
	QAction * aDrawNodes;
	QAction * aDrawHavok;
	QAction * aDrawConstraints;
	QAction * aDrawFurn;
	QAction * aDrawHidden;
	QAction * aDrawStats;
	
	QAction * aSettings;
	
	GroupBox * dialog;
	
	QStringListModel * TexFolderModel;
	QListView * TexFolderView;
	FileSelector * TexFolderSelect;
	QCheckBox * TexAlternatives;
	QAbstractButton * TexFolderButtons[3];
	
	ColorWheel * colors[3];
	AlphaSlider * alpha[3];
	
	QCheckBox * AntiAlias;
	QCheckBox * Textures;
	QCheckBox * Shaders;
	
	QCheckBox * CullNoTex;
	QCheckBox * CullByID;
	QLineEdit * CullExpr;
	
	QRadioButton * AxisX;
	QRadioButton * AxisY;
	QRadioButton * AxisZ;
	
	ColorWheel * LightColor[3];
	
	QCheckBox * LightFrontal;
	QSpinBox * LightDeclination;
	QSpinBox * LightPlanarAngle;
	
	
	QTimer * tSave, * tEmit;

	//Misc Optoins
	QLineEdit * StartVer;

};

#define glNormalColor() glColor( Color4( Options::nlColor() ) )
#define glHighlightColor() glColor( Color4( Options::hlColor() ) )


#endif
