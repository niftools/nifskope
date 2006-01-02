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

#ifdef QT_OPENGL_LIB

#include "glview.h"

#include <QtOpenGL>

#include <math.h>

#include "nifmodel.h"

#include "glscene.h"

#include "popup.h"

#include <QTimer>

/* XPM */
static char * click_xpm[] = {
"256 32 43 1",
" 	c None",
".	c #1B130F","+	c #361609","@	c #452215","#	c #362B25","$	c #402919",
"%	c #51240F","&	c #752B13","*	c #623922","=	c #4C423D","-	c #70381A",
";	c #95381D",">	c #924424",",	c #AA3C1A","'	c #A34119",")	c #9A471C",
"!	c #655A54","~	c #7F523B","{	c #8B502B","]	c #BE5121","^	c #C84D1E",
"/	c #B45C2D","(	c #7B726C","_	c #AC6935",":	c #AD6644","<	c #A1714E",
"[	c #D4642B","}	c #CA6A29","|	c #9C7864","1	c #C76C3A","2	c #E46227",
"3	c #908782","4	c #E77231","5	c #BF8849","6	c #CC8344","7	c #DF8045",
"8	c #D98A3A","9	c #BB926A","0	c #E6863A","a	c #B1A299","b	c #BD9F8A",
"c	c #E79E50","d	c #E0B560",
"                                                                                                                                                                                                                                                                ",
"                                                                                                                                                                                                                                                                ",
"                                                                                                                                                                                                                                                                ",
"                                                                                                                                                                                                                                                                ",
"                                                                                                                                                                                                                                                                ",
"                 b9dd9                        a/,;|         b:75|                                  |/,;|                  9c6:b                                                                               9dd9a                                             ",
"                 <cdd63                       :2;,&a        6[40{a                                 ]24^>a b9b            a7007|                                                                              b7ddc~                                             ",
"                 70dc6=                      b1[''&=       b71/8{(                                |,][})|ccc7~           |7776*a                  :/1|                                                       <0ddc*               a<6:|                         ",
"                 68dd_$                      b0}1})!       b8//c{!                                |1'/}}{8c01*a          |7711*3                 :4[41a                                                      :ccd6*               :44[>                         ",
"                 6c5c6$                      |88}8_!       9}]/8{!                                |}]}0}{100]@3          |11/1-3                 70[4}!                                                      <c55c*               6[17/(                        ",
"              abbcc566#     bab     b      bb|]'/]'*bbb    |8/'}<!    aaba                  bba   |]]']>${}0>#     abb   <1:11*3               b|2];)]>b      bba               a      a    ba bbb         bb6cc57*      aba     |7[>),{b      bbb              ",
"            9d72cc_:5/$  b5c7c71| b171:   1[4}2')]]^44[/:  |0/)8{!  b677c769              91442[1a<[]']>*_80:$   95c77:< 9761/-3a171:         1422-@-']^19  1cdcc5|           b6[1<  b1[[1:}42}422}:    bd640d8{5_*   b5c0c4:9a174[-@%/^^[| b1}2221/b           ",
"           500>/6_**5_$ |cc77:111|124[)3 |^[[[2'%&)'[],]^>a|0))7{! 9c07:1607<            <2[]]]^2]/[]'^'{700>*  94d611776_71/1>|}2[[)(       b22]&+=#&;]}_9/17811c6<          17^4{a :^^[422[,']]}[^{  9001)1_-$<6*  5cc71:1171241&.==%'1[><[2[[}]]^'|          ",
"          :00{#%*..(56~971{$**$>:1:];';$ :^,,,,;#$+$%%$;][_<0>>8)!5dc~$*@$*7/(          b^]%+@+%,]>']']]]4}0>-3:01~$~@~1/<11:/:16-%>&#       ~,,'>= a{^^}}c1>$$*$<65|        a17>;&! ;]/,,,&...$%@&],:|70_$@*$#=<6{a17/~#**@~171,;&= a>,,,,^^;@%%@%;]-          ",
"         b2/_)>1c5<386567;{<71:~:11,>/;# &,;&,';=3]][^]';]/80>/8}_7d{<571:~>/*          ;,&#]^^^,'%'^,^,]]6};)|c0-~dd_11;~/11161~@~]%!       (;;,^'|:22])8}>-558_<_c*        |11;;>! ,,;;;;&!<,]^/>;]^[[_)>185_a585<7/%{:71:~::;;/];|:^,;&;^&&>^^]-&'&3         ",
"         >^-{}//d}6<c_:1:-:7777::11]]1'$ >]''^^'-[4^]']]&;){)'_d{_0{~711[>!1_$         |2,-]2]'''))[^']]]'6/>_d0{~d8:/:>{<71{:1~@:4>.3        +.%]2]^^&+%_)%_d8175{6*3       3:/]]{= /]',^^;~22]^2'-,^^&{/1/58/<56{11><7777161{@+%/]/[2&+%);&>2^4]-{]%(         ",
"         /^%{]%.#)__c-~_{~}{$-1~:71;;>;# &,;'],,;^'...-]->;${/}c*~_*{}6:_-<0{#         {,%&,&.....%}^]^]],/'/-5}*_c-.....~7{$*@$:^&.(          3=>][2]+==/>~06$*{__}<(       31:;;-! ;;;;,,,;,&.@)'/2^]&~]-.#-/_8_*5{*11-*1171:#(=&/,,;.=*,&*,^)]&~^^+3         ",
"         ;,)1}.3a)6/0{(5:_0%!~111111;;;# >';'']''^&#  -';,)={]8c$<_$:65_-6c7*!         ;]&>,#! 3(3~]]]],,]1})$51*{}$3 3(3<7{!!=(11&!             )]]]'# ~}}{8-=(_}70<!       |:1/'&= ;]','';]^-!(]/]^],)_4%( {1_0/=9_~8-#!::6:>= a;,,''# ~'-!'')>)[2>.          ",
"         ',)1[*  11_}{(6_)7*a<7~>:,;&;;@a,,&;]]]'^)!  /]-;]*_}}5$|_$!*~_[748{3        a;,&&,-a|[24/[']]''}}]_$51{_8~a9646_7_(574>,;-             '}']'+ <}]{0{3<6{{6*!       3>;&;;~a,,&;,//,^&(|^))]],)/[-a :1_}_!{6/7{!b7]:/>#  ;;;;;. {]&!%&)]22]'~          ",
"         >'-{})99[{{6_|6{~/_9c7*{1/;%&,;;,;&']]')'];:/2'->}-1}}8$<7$b<8c7}1cc/a        &;%&]]^222^)']']],}'}_~5/*:8c6cc71{_6:c71[;;;~            )^,''$ ~/)*88_18-<_$(       a{:@&;;;^&&;]^'']]>]2&*]]]>*}1<97)~_8!_6*_}<90:{>/#  &,;;;# {,&(<]22^]]^'~         ",
"         -;_{)1001**5_<c_~/_cc{:11/,&~;',,&@&^,;%&&;,,;&>,0>}18}{58<90c71//67_!        &;;>;;],''&@&;',''0'}/_8/{6_566_/)~57c71:1&&;;|           &;,;;# |>/~}_17__58#3       a{1*;;;,;%%;,'&%),];&%>,''/~_)007-${6~c_~)1cc7*={_@  ;,,,;$ ~''~^^;;,;>';*         ",
"         *)1{~-/{%#(8_556}~{:{~<7**>[1%...@~,,,;*$@&;%@>2]@*/)4/{_60555_)-$:8*!        !>['&+.%;,;-,,^');1//)){)0_{$.<_8>~7671~@{/{1[-a          ;,,,'$  -/>~-){*<d-#         ~/[1$..+%~^,,&=@&;&@~^;%-}>~-_)-$!55_d60{{__*$(:/*a )^[^)# 3&,,>;;;&@-,)$         ",
"         3%)}/>--_95d5$*5}/16557>.=@///::>,^^]',;:,,,;;^'@.!-)[)%$*6c_5~*/_c_$(        a@&,^>>,^&++&,,;%@)}/)%=*<6c5<dd{%*ccc:@!@:[][-(          &,,;&#  *-)}/6_5d5.3         (%>]>::/,^^,'&!~;,;;^;+##>}/>{*_<5cd*~511/:_5_:7/@3 >}',&#  @&,,'%%&;,,%#         ",
"          (+&]]]^[][}%=($-/41}1-.! !+-)/[,1);%&&,;^^,/]>+.a @&>%.3=%>]}0}27{$#          3.@&;,;%.#(@-&%.=%>;-.3!#%_/]_*.=!>1]>.a!-//_@(          -)')%=  a$%',}/]-$#           =@-/111'/)&;&~]^,],;.# 3+-'^^^[]}}{.!${]4}}41[7_#3 ~>/>+!  3+%;,][^},%.(         ",
"           (.++&%&%%+.3 a..%%%..(   3#..........+.+%*+..=   3...3  !..---%+.=             !.....3  3...3 #...=   =....#3  @%%.=  =+@$.           !@*$.a    #.+%%..!             (........+&;;^;%@..=    ..+%&%%%%.! 3..%-%+.+@.#  !%*+.     ..+---$..a          ",
"             !###====3    3===(        333     3!(!!=!(              (=###!                  a            33a      aa     3=#=    =#=3            !##3      3===!                  33a   *,;;,%#=(       (=###===(    (=#=!3!=!    !#=3      3==#==3            ",
"                                                                                                                                                                                         ~;,;;@(                                                                ",
"                                                                                                                                                                                         ~&,;;+3                                                                ",
"                                                                                                                                                                                         (&;;&.                                                                 ",
"                                                                                                                                                                                          +...=                                                                 ",
"                                                                                                                                                                                           (!3                                                                  ",
"                                                                                                                                                                                                                                                                "};

#define FPS 35

GLView::GLView()
	: QGLWidget()
{
	setFocusPolicy( Qt::ClickFocus );
	
	xRot = yRot = zRot = 0;
	zoom = 1.0;
	zInc = 1;
	
	updated = false;
	doCenter = false;
	
	model = 0;
	
	time = 0.0;
	lastTime = QTime::currentTime();
	
	scene = new Scene();

	click_tex = 0;
	
	timer = new QTimer(this);
	connect( timer, SIGNAL( timeout() ), this, SLOT( advanceGears() ) );
	
	aTexturing = new QAction( "texturing", this );
	aTexturing->setToolTip( "enable texturing" );
	aTexturing->setCheckable( true );
	aTexturing->setChecked( true );
	connect( aTexturing, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aTexturing );
	
	aBlending = new QAction( "blending", this );
	aBlending->setToolTip( "enable alpha blending" );
	aBlending->setCheckable( true );
	aBlending->setChecked( true );
	connect( aBlending, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aBlending );
	
	aLighting = new QAction( "lighting", this );
	aLighting->setToolTip( "enable lighting" );
	aLighting->setCheckable( true );
	aLighting->setChecked( true );
	connect( aLighting, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aLighting );
	
	aDrawAxis = new QAction( "draw axis", this );
	aDrawAxis->setToolTip( "draw xyz-Axis" );
	aDrawAxis->setCheckable( true );
	aDrawAxis->setChecked( true );
	connect( aDrawAxis, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawAxis );
	
	aDrawNodes = new QAction( "draw nodes", this );
	aDrawNodes->setToolTip( "draw bones/nodes" );
	aDrawNodes->setCheckable( true );
	aDrawNodes->setChecked( true );
	connect( aDrawNodes, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawNodes );
	
	aDrawHidden = new QAction( "show hidden", this );
	aDrawHidden->setToolTip( "if checked nodes and meshes are allways displayed<br>wether they are hidden ( flags & 1 ) or not" );
	aDrawHidden->setCheckable( true );
	aDrawHidden->setChecked( false );
	connect( aDrawHidden, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aDrawHidden );
	
	aHighlight = new QAction( "highlight selected", this );
	aHighlight->setToolTip( "highlight selected meshes and nodes" );
	aHighlight->setCheckable( true );
	aHighlight->setChecked( true );
	connect( aHighlight, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aHighlight );
	
	aRotate = new QAction( "rotate", this );
	aRotate->setToolTip( "slowly rotate the object around the z axis" );
	aRotate->setCheckable( true );
	aRotate->setChecked( true );
	connect( aRotate, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aRotate );
	
	aAnimate = new QAction( "animation", this );
	aAnimate->setToolTip( "enables evaluation of animation controllers" );
	aAnimate->setCheckable( true );
	aAnimate->setChecked( true );
	connect( aAnimate, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );
	addAction( aAnimate );

	aAnimPlay = new QAction( "play", this );
	aAnimPlay->setCheckable( true );
	aAnimPlay->setChecked( true );
	connect( aAnimPlay, SIGNAL( toggled( bool ) ), this, SLOT( checkActions() ) );

	aTexFolder = new QAction( "set texture folder", this );
	aTexFolder->setToolTip( "tell me where your textures are" );
	connect( aTexFolder, SIGNAL( triggered() ), this, SLOT( selectTexFolder() ) );
	addAction( aTexFolder );
}

GLView::~GLView()
{
	makeCurrent();
	delete scene;
}

void GLView::initializeGL()
{
	GLTex::initialize( context() );
	
	qglClearColor( palette().color( QPalette::Active, QPalette::Background ) );

	static const GLfloat L0position[4] = { 5.0f, 5.0f, 10.0f, 1.0f };
	static const GLfloat L0ambient[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	static const GLfloat L0diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	static const GLfloat L0specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv( GL_LIGHT0, GL_POSITION, L0position );
	glLightfv( GL_LIGHT0, GL_AMBIENT, L0ambient );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, L0diffuse );
	glLightfv( GL_LIGHT0, GL_SPECULAR, L0specular );
	glEnable( GL_LIGHT0 );
	
	glShadeModel( GL_SMOOTH );
	glEnable( GL_POINT_SMOOTH );
	glEnable( GL_LINE_SMOOTH );

	click_tex = bindTexture( QImage( click_xpm ) );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (init) : " << (const char *) gluErrorString( err );
}

void GLView::glPerspective( int x, int y )
{
	GLdouble r = qMax( scene->boundRadius[0], qMax( scene->boundRadius[1], scene->boundRadius[2] ) );
	if ( r > radius )
		radius = r;

	GLint	viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if ( x >= 0 && y >= 0 )
		gluPickMatrix( (GLdouble) x, (GLdouble) (viewport[3]-y), 5.0f, 5.0f, viewport);
	gluPerspective( 45.0 / zoom, (GLdouble) viewport[2] / (GLdouble) viewport[3], 1.0 * radius, 8.0 * radius );
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef( xTrans / 20.0, yTrans / 20.0, -4.0 * radius );
}

void GLView::glOrtho()
{
	GLint	viewport[4];
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	::glOrtho (0.0, viewport[2], 0.0, viewport[3], -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void GLView::compile( bool center )
{
	updated = false;
	scene->make( model );
	time = scene->timeMin;
	emit sigFrame( (int) ( time * FPS ), (int) ( scene->timeMin * FPS ), (int) ( scene->timeMax * FPS ) );
	
	doCenter = center;
	updateGL();
}

void GLView::paintGL()
{
	if ( ! ( isVisible() && height() ) )	return;
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// center the model
	
	if ( doCenter )
	{
		xTrans = 0;
		yTrans = - scene->boundCenter[1];
		
		xRot = - 90*16;
		yRot = 0;
		
		zoom = 1.0;
		
		doCenter = false;
		
		radius = 0.0;
	}
	
	// transform the scene
	
	Transform viewTrans;
	viewTrans.rotation = Matrix::fromEuler( xRot / 16.0 / 180 * PI, yRot / 16.0 / 180 * PI, zRot / 16.0 / 180 * PI );
	
	scene->transform( viewTrans, time );
	
	// setup projection mode

	glPerspective();
	
	// draw the axis
	
	if ( aDrawAxis->isChecked() )
	{
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_ALWAYS );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		glLineWidth( 1.2 );
		
		glPushMatrix();
		viewTrans.glMultMatrix();
		
		GLfloat axis = qMax( scene->boundRadius[0], qMax( scene->boundRadius[1], scene->boundRadius[2] ) ) * 2.1;
		GLfloat arrow = axis / 10.0;
		glBegin( GL_LINES );
		glColor3f( 1.0, 0.0, 0.0 );
		glVertex3f( - axis, 0, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, + arrow, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, - arrow, 0 );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, 0, + arrow );
		glVertex3f( + axis, 0, 0 );
		glVertex3f( + axis - arrow, 0, - arrow );
		glColor3f( 0.0, 1.0, 0.0 );
		glVertex3f( 0, - axis, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( + arrow, + axis - arrow, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( - arrow, + axis - arrow, 0 );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( 0, + axis - arrow, + arrow );
		glVertex3f( 0, + axis, 0 );
		glVertex3f( 0, + axis - arrow, - arrow );
		glColor3f( 0.0, 0.0, 1.0 );
		glVertex3f( 0, 0, - axis );
		glVertex3f( 0, 0, + axis );
		glVertex3f( 0, 0, + axis );
		glVertex3f( 0, + arrow, + axis - arrow );
		glVertex3f( 0, 0, + axis );
		glVertex3f( 0, - arrow, + axis - arrow );
		glVertex3f( 0, 0, + axis );
		glVertex3f( + arrow, 0, + axis - arrow );
		glVertex3f( 0, 0, + axis );
		glVertex3f( - arrow, 0, + axis - arrow );
		glEnd();
		glPopMatrix();
		/*
		glColor3f( 1.0, 0.0, 1.0 );
		glBegin( GL_LINE_STRIP );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMax[2] );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMin[2] );
		glEnd();
		glBegin( GL_LINE_STRIP );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMin[2] );
		glEnd();
		glBegin( GL_LINES );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMin[2] );
		glVertex3f( scene->boundMin[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMax[1], scene->boundMax[2] );
		glVertex3f( scene->boundMin[0], scene->boundMin[1], scene->boundMax[2] );
		glVertex3f( scene->boundMax[0], scene->boundMin[1], scene->boundMax[2] );
		glEnd();
		*/
	}
	
	// draw the model

	if ( aLighting->isChecked() )
		glEnable( GL_LIGHTING );
	else
		glDisable( GL_LIGHTING );
	
	scene->draw();
	
	// draw the double click pixmap
	
	if ( updated && click_tex )
	{
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );
		glDisable( GL_NORMALIZE );
		glDisable( GL_LIGHTING );
		glEnable( GL_TEXTURE_2D );
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_COLOR_MATERIAL );
		
		glOrtho();
		
		glBindTexture( GL_TEXTURE_2D, click_tex );
		glColor4f( 1.0, 1.0, 1.0, 1.0 );
		glBegin( GL_QUADS );
		glTexCoord2d( 0.0, 0.0 );		glVertex2d( 0.0, 0.0 );
		glTexCoord2d( 1.0, 0.0 );		glVertex2d( 255.0, 0.0 );
		glTexCoord2d( 1.0, 1.0 );		glVertex2d( 255.0, 31.0 );
		glTexCoord2d( 0.0, 1.0 );		glVertex2d( 0.0, 31.0 );
		glEnd();
	}
	
	// check for errors
	
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR )
		qDebug() << "GL ERROR (paint): " << (const char *) gluErrorString( err );
}

int GLView::pickGL( int x, int y )
{
	if ( ! ( isVisible() && height() ) )
		return -1;
	
	makeCurrent();

	GLuint	buffer[512];
	glSelectBuffer( 512, buffer );

	glRenderMode( GL_SELECT );	
	glInitNames();
	glPushName( 0 );
	
	glPerspective( pressPos.x(), pressPos.y() );

	scene->draw();
	
	GLint hits = glRenderMode( GL_RENDER );
	if ( hits > 0 )
	{
		int	choose = buffer[3];
		int	depth = buffer[1];
		for (int loop = 1; loop < hits; loop++)
		{
			if (buffer[loop*4+1] < GLuint(depth))
			{
				choose = buffer[loop*4+3];
				depth = buffer[loop*4+1];
			}       
		}
		return choose;
	}
	return -1;
}

void GLView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height );
}

void GLView::setNif( NifModel * nif )
{
	if ( model )
	{
		disconnect( model, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( dataChanged() ) );
		disconnect( model, SIGNAL( linksChanged() ), this, SLOT( dataChanged() ) );
		disconnect( model, SIGNAL( modelReset() ), this, SLOT( dataChanged() ) );
	}
	model = nif;
	if ( model )
	{
		connect( model, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ), this, SLOT( dataChanged() ) );
		connect( model, SIGNAL( linksChanged() ), this, SLOT( dataChanged() ) );
		connect( model, SIGNAL( modelReset() ), this, SLOT( dataChanged() ) );
	}
	updated = true;
}

void GLView::setCurrentIndex( const QModelIndex & index )
{
	if ( ! ( model && index.model() == model ) )
		return;
	
	scene->currentNode = model->getBlockNumber( index );
	updateGL();
}

void GLView::sltFrame( int f )
{
	time = (float) f / FPS;
	updateGL();
}

void GLView::selectTexFolder()
{
	//QString tf = QFileDialog::getExistingDirectory( this, "select texture folder", textureFolder() );
	setTextureFolder( selectMultipleDirs( "select texture folders", scene->texfolders, this ).join( ";" ) );
}

void GLView::setTextureFolder( const QString & tf )
{
	scene->texfolders = tf.split( ";" );
	updated = true;
}

QString GLView::textureFolder() const
{
	return scene->texfolders.join( ";" );
}

void GLView::save( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	settings.setValue( "texture folder", scene->texfolders.join( ";" ) );
	settings.setValue( "enable textures", aTexturing->isChecked() );
	settings.setValue( "enable lighting", aLighting->isChecked() );
	settings.setValue( "enable blending", aBlending->isChecked() );
	settings.setValue( "highlight meshes", aHighlight->isChecked() );
	settings.setValue( "draw axis", aDrawAxis->isChecked() );
	settings.setValue( "draw nodes", aDrawNodes->isChecked() );
	settings.setValue( "draw hidden", aDrawHidden->isChecked() );
	settings.setValue( "rotate", aRotate->isChecked() );
	settings.setValue( "enable animations", aAnimate->isChecked() );
	settings.setValue( "play animation", aAnimPlay->isChecked() );
	//settings.endGroup();
}

void GLView::restore( QSettings & settings )
{
	//settings.beginGroup( "OpenGL" );
	scene->texfolders = settings.value( "texture folder" ).toString().split( ";" );
	aTexturing->setChecked( settings.value( "enable textures", true ).toBool() );
	aLighting->setChecked( settings.value( "enable lighting", true ).toBool() );
	aBlending->setChecked( settings.value( "enable blending", true ).toBool() );
	aHighlight->setChecked( settings.value( "highlight meshes", true ).toBool() );
	aDrawAxis->setChecked( settings.value( "draw axis", true ).toBool() );
	aDrawNodes->setChecked( settings.value( "draw nodes", true ).toBool() );
	aDrawHidden->setChecked( settings.value( "draw hidden", true ).toBool() );
	aRotate->setChecked( settings.value( "rotate", true ).toBool() );
	aAnimate->setChecked( settings.value( "enable animations", true ).toBool() );
	aAnimPlay->setChecked( settings.value( "play animation", true ).toBool() );
	checkActions();
	//settings.endGroup();
}

void GLView::checkActions()
{
	scene->texturing = aTexturing->isChecked();
	scene->blending = aBlending->isChecked();
	scene->highlight = aHighlight->isChecked();
	scene->drawNodes = aDrawNodes->isChecked();
	scene->drawHidden = aDrawHidden->isChecked();
	scene->animate = aAnimate->isChecked();
	lastTime = QTime::currentTime();
	if ( aRotate->isChecked() || ( aAnimate->isChecked() && aAnimPlay->isChecked() ) )
	{
		timer->start( 1000 / FPS );
	}
	else
	{
		timer->stop();
		updateGL();
	}
}

void GLView::advanceGears()
{
	QTime t = QTime::currentTime();
	
	if ( aAnimate->isChecked() && aAnimPlay->isChecked() )
	{
		time += fabs( t.msecsTo( lastTime ) / 1000.0 );

		if ( time > scene->timeMax )
			time = scene->timeMin;
		
		emit sigFrame( (int) ( time * FPS ), (int) ( scene->timeMin * FPS ), (int) ( scene->timeMax * FPS ) );
	}
	
	lastTime = t;
	
	if ( aRotate->isChecked() )
	{
		zRot += zInc;
		normalizeAngle( &zRot );
	}
	updateGL();
}

void GLView::dataChanged()
{
	updated = true;
	updateGL();
}

void GLView::normalizeAngle(int *angle)
{
	while (*angle < 0)
		*angle += 360 * 16;
	while (*angle > 360 * 16)
		*angle -= 360 * 16;
}

void GLView::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();
	pressPos = event->pos();
}

void GLView::mouseReleaseEvent( QMouseEvent *event )
{
	if ( ! ( isVisible() && height() && model && ( pressPos - event->pos() ).manhattanLength() <= 3 ) )
		return;
	
	int pick = pickGL( event->x(), event->y() );
	if ( pick >= 0 )
	{
		emit clicked( model->getBlock( pick ) );
		scene->currentNode = pick;
		updateGL();
	}
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton)
	{
		xRot += 8 * dy;
		yRot += 8 * dx;
		normalizeAngle( &xRot );
		normalizeAngle( &yRot );
		updateGL();
	}
	else if (event->buttons() & Qt::RightButton)
	{
		zRot += 8 * dx;
		normalizeAngle( &zRot );
		updateGL();
	}
	else if ( event->buttons() & Qt::MidButton)
	{
		xTrans += dx * 5;
		yTrans += dy * 5;
		updateGL();
	}
	lastPos = event->pos();
}

void GLView::mouseDoubleClickEvent( QMouseEvent * )
{
	compile( false );
}

void GLView::wheelEvent( QWheelEvent * event )
{
	zoom = zoom * ( event->delta() > 0 ? 0.9 : 1.1 );
	if ( zoom < 0.25 ) zoom = 0.25;
	if ( zoom > 1000 ) zoom = 1000;
	updateGL();
}

QList<QAction*> GLView::animActions() const
{
	QList<QAction*> actions;
	actions << aAnimPlay;
	return actions;
}

#endif
