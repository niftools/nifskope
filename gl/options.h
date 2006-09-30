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

#ifndef GLOPTIONS_H
#define GLOPTIONS_H

#include <QObject>

class QAction;
class QColor;
class QCheckBox;
class QDialog;
class QLineEdit;
class QTimer;

class ColorWheel;
class GroupBox;

class GLOptions : public QObject
{
	Q_OBJECT
public:
	static GLOptions * get();
	static QList<QAction*> actions();
	
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
	static bool drawFurn();
	static bool drawHidden();
	static bool drawStats();
	
	static bool benchmark();
	
	typedef enum {
		ZAxis, YAxis, XAxis
	} Axis;

	static Axis upAxis();
	
signals:
	void sigChanged();

public slots:
	void save();
	
protected:
	GLOptions();
	~GLOptions();
	
	QAction * aDrawAxes;
	QAction * aDrawNodes;
	QAction * aDrawHavok;
	QAction * aDrawFurn;
	QAction * aDrawHidden;
	QAction * aDrawStats;
	
	QAction * aSettings;
	
	GroupBox * dialog;
	
	enum {
		colorBack = 0,
		colorNorm = 1,
		colorHigh = 2
	};
	
	
	ColorWheel * colors[3];
	
	QCheckBox * AntiAlias;
	QCheckBox * Textures;
	QCheckBox * Shaders;
	
	QCheckBox * CullNoTex;
	QCheckBox * CullByID;
	QLineEdit * CullExpr;
	
	QCheckBox * AxisX;
	QCheckBox * AxisY;
	QCheckBox * AxisZ;
	
	
	QTimer * tSave;
};

#define glNormalColor() glColor( Color3( GLOptions::nlColor() ) )
#define glHighlightColor() glColor( Color3( GLOptions::hlColor() ) )


#endif
