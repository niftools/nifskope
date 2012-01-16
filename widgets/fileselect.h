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

#ifndef FILESELECT_H
#define FILESELECT_H

#include <QAction>
#include <QBoxLayout>
#include <QLineEdit>
#include <QWidget>

class QCompleter;
class QDirModel;

//! A widget for file selection, both via a browser window and via editing a string.
/*!
 * This widget displays a button (typically Load or Save) and a filename string. If the button is
 * pressed, then a file selection window is displayed.
 *
 * Emits sigActivated when a file has been chosen, or when the filename string is activated (that
 * is, pressing enter after editing the string). This signal can be connected to the load/save
 * slot of the application.
 *
 * Emits sigEdited when the string is edited.
 */
class FileSelector : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString file READ file WRITE setFile NOTIFY sigEdited USER true)
	Q_PROPERTY(QStringList filter READ filter WRITE setFilter)
	Q_PROPERTY(Modes mode READ mode WRITE setMode)
	Q_PROPERTY(States state READ state WRITE setState RESET rstState)
	Q_ENUMS(Modes)
	Q_ENUMS(States)
	
public:
	enum Modes
	{
		LoadFile, SaveFile, Folder
	};

	enum States
	{
		stNeutral = 0, stSuccess = 1, stError = 2
	};
	
	FileSelector( Modes m, const QString & buttonText = "browse", QBoxLayout::Direction dir = QBoxLayout::LeftToRight, QKeySequence keySeq = QKeySequence::UnknownKey );
	
	QString text() const { return file(); }
	QString file() const;
	
	void setFilter( const QStringList & f );
	QStringList filter() const;
	
	Modes mode() const { return Mode; }
	void setMode( Modes m ) { Mode = m; }

	States state() const { return State; }
	void setState( States );
	
signals:
	void sigEdited( const QString & );
	void sigActivated( const QString & );

public slots:
	void rstState();

	void setText( const QString & );
	void setFile( const QString & );

	void replaceText( const QString & );
	
	void setCompletionEnabled( bool );
	
protected slots:
	void browse();
	void activate();
	
protected:
	bool eventFilter( QObject * o, QEvent * e );
	
	QAction * completionAction();
	
	Modes Mode;
	States State;

	QLineEdit * line;
	QAction   * action;
	
	QDirModel * dirmdl;
	QCompleter * completer;
	QStringList fltr;
	
	QTimer * timer;
};

class CompletionAction : public QAction
{
	Q_OBJECT
public:
	CompletionAction( QObject * parent = 0 );
	~CompletionAction();
	
protected slots:
	void sltToggled( bool );
};

#endif
