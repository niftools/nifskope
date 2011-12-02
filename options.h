/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2010, NIF File Format Library and Tools
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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QObject>
#include "widgets/groupbox.h"

#define NifSkopeDisplayRole (Qt::UserRole + 42)

//! \file options.h Options class

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
class QTabWidget;
class QTimer;
class QComboBox;

class AlphaSlider;
class ColorWheel;
class FileSelector;
class GroupBox;

//! Global options menu and dialog
class Options : public QObject
{
	Q_OBJECT
public:
	//! Global instance
	static Options * get();
	//! The list of currently enabled actions
	static QList<QAction*> actions();
	
	//! Texture folders
	static QStringList textureFolders();
	//! Whether to use alternative textures
	static bool textureAlternatives();
	
	//! Whether to enable antialiasing
	static bool antialias();
	//! Whether to enable texturing
	static bool texturing();
	//! Whether to enable shaders
	static bool shaders();
	
	//! Whether to enable blending
	static bool blending() { return true; }
	
	//! The background color of the main view window
	static QColor bgColor();
	//! The colour to normally use for drawing
	static QColor nlColor();
	//! The colour to use when highlighting
	static QColor hlColor();
	
	//! Regular expression to use for culling
	static QRegExp cullExpression();
	//! Whether to only draw textured shapes
	static bool onlyTextured();
	
	//! Whether to draw the axes
	static bool drawAxes();
	//! Whether to draw nodes
	static bool drawNodes();
	//! Whether to draw Havok shapes
	static bool drawHavok();
	//! Whether to draw constraints
	static bool drawConstraints();
	//! Whether to draw furniture markers
	static bool drawFurn();
	//! Whether to draw hidden shapes
	static bool drawHidden();
	//! Whether to draw stats
	static bool drawStats();
	//! Whether to draw meshes
	static bool drawMeshes();
	
	//! Whether to benchmark FPS
	static bool benchmark();
	
	//! The possible axes
	typedef enum {
		ZAxis, YAxis, XAxis
	} Axis;
	
	//! The axis defined as up
	static Axis upAxis();
	
	//! The ambient lighting color
	static QColor ambient();
	//! The diffuse lighting color
	static QColor diffuse();
	//! The specular lighting color
	static QColor specular();
	
	//! Whether to use frontal lighting
	static bool lightFrontal();
	//! The angle between the Z axis and the light
	static int lightDeclination();
	//! The angle between the X axis and the light
	static int lightPlanarAngle();
	
	//! Whether to override material colors
	static bool overrideMaterials();
	//! The ambient color to override materials with
	static QColor overrideAmbient();
	//! The diffuse color to override materials with
	static QColor overrideDiffuse();
	//! The specular color to override materials with
	static QColor overrideSpecular();
	//! The emissive color to override materials with
	static QColor overrideEmissive();
	
	//! The NIF version to use at start
	static QString startupVersion();
	//! The current translation locale
	static QLocale translationLocale();
	// Maximum string length (see NifIStream::init for the current usage)
	//static int maxStringLength();
	
signals:
	//! Signal emitted when a value changes
	void sigChanged();
	//! Signal emitted when material overrides change
	void materialOverridesChanged();
	//! Signal emitted when the locale changes
	void sigLocaleChanged();
	
protected slots:
	//! Texture folder button actions
	void textureFolderAction( int );
	//! Per-index texture folder options
	void textureFolderIndex( const QModelIndex & );
	//! Automatic detection of texture folders
	void textureFolderAutoDetect();
	//! Set lighting presets
	void activateLightPreset( int );
	
public slots:
	void save();
	
protected:
	friend class TexturesPage;
	friend class ColorsOptionPage;
	friend class MaterialOverrideOptionPage;
	
	Options();
	~Options();
	
	bool eventFilter( QObject * o, QEvent * e );
	
	//////////////////////////////////////////////////////////////////////////
	// Menu
	
	QAction * aDrawAxes;
	QAction * aDrawNodes;
	QAction * aDrawHavok;
	QAction * aDrawConstraints;
	QAction * aDrawFurn;
	QAction * aDrawHidden;
	QAction * aDrawStats;
	
	QAction * aSettings;
	
	QTimer * tSave, * tEmit;
	
	//////////////////////////////////////////////////////////////////////////
	// General Settings page
	
	QComboBox * RegionOpt;
	QLineEdit * StartVer;
	//QSpinBox * StringLength;
	
	//////////////////////////////////////////////////////////////////////////
	// Rendering Settings page
	
	QStringListModel * TexFolderModel;
	QListView * TexFolderView;
	FileSelector * TexFolderSelect;
	QCheckBox * TexAlternatives;
	QAbstractButton * TexFolderButtons[4];
	
	QCheckBox * AntiAlias;
	QCheckBox * Textures;
	QCheckBox * Shaders;
	
	QCheckBox * CullNoTex;
	QCheckBox * CullByID;
	QLineEdit * CullExpr;
	
	QRadioButton * AxisX;
	QRadioButton * AxisY;
	QRadioButton * AxisZ;
	
	//////////////////////////////////////////////////////////////////////////
	// Colors Settings page
	
	ColorWheel * colors[3];
	AlphaSlider * alpha[3];
	
	ColorWheel * LightColor[3];
	
	QCheckBox * LightFrontal;
	QSpinBox * LightDeclination;
	QSpinBox * LightPlanarAngle;
	
	//////////////////////////////////////////////////////////////////////////
	// Materials Settings page
	
	QCheckBox * overrideMatCheck;
	ColorWheel * matColors[4];
	
	//////////////////////////////////////////////////////////////////////////
	
	GroupBox * dialog;
	QTabWidget *tab;
	
	bool showMeshes;
};

//! Gets the color normally used for drawing from Options::nlColor()
#define glNormalColor() glColor( Color4( Options::nlColor() ) )
//! Gets the color used for highlighting from Options::hlColor()
#define glHighlightColor() glColor( Color4( Options::hlColor() ) )

#endif
