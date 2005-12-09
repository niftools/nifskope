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


#ifdef QT_OPENGL_LIB

#include "glview.h"

#include <QtOpenGL>

#include <math.h>

#include "nifmodel.h"

#include "glscene.h"


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

GLView::GLView()
	: QGLWidget()
{
	xRot = yRot = zRot = 0;
	zoom = 1000;
	zInc = 1;
	
	updated = false;
	doCompile = doCenter = false;
	
	lightsOn = true;
	drawaxis = true;
	
	model = 0;
	
	scene = new Scene( context() );

	click_tex = 0;
	
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(advanceGears()));
}

GLView::~GLView()
{
	makeCurrent();
	delete scene;
}

void GLView::initializeGL()
{
	QString extensions( (const char *) glGetString(GL_EXTENSIONS) );
	//qDebug( (const char *) glGetString(GL_EXTENSIONS) );

	qglClearColor( palette().color( QPalette::Active, QPalette::Background ) );

	static const GLfloat L0position[4] = { 5.0f, 5.0f, 10.0f, 1.0f };
	static const GLfloat L0ambient[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	static const GLfloat L0diffuse[4] = { 0.8f, 0.8f, 0.8f, 1.0f };
	static const GLfloat L0specular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat L0emissive[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv( GL_LIGHT0, GL_POSITION, L0position );
	glLightfv( GL_LIGHT0, GL_AMBIENT, L0ambient );
	glLightfv( GL_LIGHT0, GL_DIFFUSE, L0diffuse );
	glLightfv( GL_LIGHT0, GL_SPECULAR, L0specular );
	glLightfv( GL_LIGHT0, GL_EMISSION, L0emissive );
	glEnable( GL_LIGHT0 );
	
	glShadeModel( GL_SMOOTH );

	click_tex = bindTexture( QImage( click_xpm ) );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
}

void GLView::paintGL()
{
	if ( ! isVisible() )	return;
	
	makeCurrent();
	
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// setup projection mode

	int x = qMin( width(), height() );
	
	float fx = (float) width() / x;
	float fy = (float) height() / x;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-fx, +fx, -fy, fy, 5.0, 100000.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// update the model
	
	if ( doCompile )
	{
		doCompile = false;
		updated = false;
		
		scene->make( model );
	}
	
	// center the model

	if ( doCenter )
	{
		scene->boundaries( boundMin, boundMax );
		
		double max = 0;
		for ( int c = 0; c < 3; c++ )
			max = qMax( max, qMax( fabs( boundMin[ c ] ), fabs( boundMax[ c ] ) ) );
		zoom = (int) ( max * 40 );
		xTrans = 0;
		yTrans = (int) ( ( boundMin[2] + 0.5 * ( boundMax[2] - boundMin[2] ) ) * 20 );
		xRot = - 90*16;
		yRot = 0;
		
		doCenter = false;
	}
	
	// calculate the view transform matrix
	
	Matrix viewTrans = Matrix::trans( xTrans / 20.0, - yTrans / 20.0, - zoom / 10.0 ) * Matrix::rotX( xRot / 16.0 / 180.0 * 3.14 )
				* Matrix::rotY( yRot / 16.0 / 180 * 3.14 ) * Matrix::rotZ( zRot / 16.0 / 180.0 * 3.14 );
	
	// draw the axis
	
	if ( drawaxis )
	{
		glDisable( GL_BLEND );
		glDisable( GL_LIGHTING );
		glDisable( GL_COLOR_MATERIAL );
		glEnable( GL_DEPTH_TEST );
		glDepthMask( GL_TRUE );
		glDisable( GL_TEXTURE_2D );
		glDisable( GL_NORMALIZE );
		
		viewTrans.glLoadMatrix();
		
		glBegin( GL_LINES );
		glColor3f( 1.0, 0.0, 0.0 );
		glVertex3f( - 1000.0, 0, 0 );
		glVertex3f( + 1000.0, 0, 0 );
		glVertex3f( + 1000.0,    0,    0 );
		glVertex3f( +  950.0, + 20,    0 );
		glVertex3f( + 1000.0,    0,    0 );
		glVertex3f( +  950.0, - 20,    0 );
		glVertex3f( + 1000.0,    0,    0 );
		glVertex3f( +  950.0,    0, + 20 );
		glVertex3f( + 1000.0,    0,    0 );
		glVertex3f( +  950.0,    0, - 20 );
		glColor3f( 0.0, 1.0, 0.0 );
		glVertex3f( 0, - 1000.0, 0 );
		glVertex3f( 0, + 1000.0, 0 );
		glVertex3f(    0, + 1000.0,    0 );
		glVertex3f( + 20, +  950.0,    0 );
		glVertex3f(    0, + 1000.0,    0 );
		glVertex3f( - 20, +  950.0,    0 );
		glVertex3f(    0, + 1000.0,    0 );
		glVertex3f(    0, +  950.0, + 20 );
		glVertex3f(    0, + 1000.0,    0 );
		glVertex3f(    0, +  950.0, - 20 );
		glColor3f( 0.0, 0.0, 1.0 );
		glVertex3f( 0, 0, - 1000.0 );
		glVertex3f( 0, 0, + 1000.0 );
		glVertex3f(    0,    0, + 1000.0 );
		glVertex3f(    0, + 20, +  950.0 );
		glVertex3f(    0,    0, + 1000.0 );
		glVertex3f(    0, - 20, +  950.0 );
		glVertex3f(    0,    0, + 1000.0 );
		glVertex3f( + 20,    0, +  950.0 );
		glVertex3f(    0,    0, + 1000.0 );
		glVertex3f( - 20,    0, +  950.0 );
		glEnd();
		
		glColor3f( 1.0, 0.0, 1.0 );
		glBegin( GL_LINE_STRIP );
		glVertex3f( boundMin[0], boundMin[1], boundMin[2] );
		glVertex3f( boundMin[0], boundMax[1], boundMin[2] );
		glVertex3f( boundMin[0], boundMax[1], boundMax[2] );
		glVertex3f( boundMin[0], boundMin[1], boundMax[2] );
		glVertex3f( boundMin[0], boundMin[1], boundMin[2] );
		glEnd();
		glBegin( GL_LINE_STRIP );
		glVertex3f( boundMax[0], boundMin[1], boundMin[2] );
		glVertex3f( boundMax[0], boundMax[1], boundMin[2] );
		glVertex3f( boundMax[0], boundMax[1], boundMax[2] );
		glVertex3f( boundMax[0], boundMin[1], boundMax[2] );
		glVertex3f( boundMax[0], boundMin[1], boundMin[2] );
		glEnd();
		glBegin( GL_LINES );
		glVertex3f( boundMin[0], boundMin[1], boundMin[2] );
		glVertex3f( boundMax[0], boundMin[1], boundMin[2] );
		glVertex3f( boundMin[0], boundMax[1], boundMin[2] );
		glVertex3f( boundMax[0], boundMax[1], boundMin[2] );
		glVertex3f( boundMin[0], boundMax[1], boundMax[2] );
		glVertex3f( boundMax[0], boundMax[1], boundMax[2] );
		glVertex3f( boundMin[0], boundMin[1], boundMax[2] );
		glVertex3f( boundMax[0], boundMin[1], boundMax[2] );
		glEnd();
	}
	
	// draw the model

	if ( lightsOn )
		glEnable( GL_LIGHTING );
	else
		glDisable( GL_LIGHTING );
	
	scene->draw( viewTrans );
	
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
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho (0.0, width(), 0.0, height(), -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
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
		qDebug() << "GL ERROR : " << (const char *) gluErrorString( err );
}

void GLView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
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

void GLView::setTextureFolder( const QString & tf )
{
	scene->texfolder = tf;
	updated = true;
}

QString GLView::textureFolder() const
{
	return scene->texfolder;
}

void GLView::setTexturing( bool t )
{
	scene->texturing = t;
	updateGL();
}

bool GLView::texturing() const
{
	return scene->texturing;
}

void GLView::setBlending( bool b )
{
	scene->blending = b;
	updateGL();
}

bool GLView::blending() const
{
	return scene->blending;
}

void GLView::setLighting( bool l )
{
	lightsOn = l;
	updateGL();
}

void GLView::setDrawAxis( bool a )
{
	drawaxis = a;
	updateGL();
}


void GLView::setRotate( bool r )
{
	if ( r )
		timer->start(25);
	else
		timer->stop();
}

void GLView::setXRotation(int angle)
{
	normalizeAngle(&angle);
	if (angle != xRot) {
		xRot = angle;
		emit xRotationChanged(angle);
		updateGL();
	}
}

void GLView::setYRotation(int angle)
{
	normalizeAngle(&angle);
	if (angle != yRot) {
		yRot = angle;
		emit yRotationChanged(angle);
		updateGL();
	}
}

void GLView::setZRotation(int angle)
{
	normalizeAngle(&angle);
	if (angle != zRot) {
		zRot = angle;
		emit zRotationChanged(angle);
		updateGL();
	}
}

void GLView::setXTrans( int x )
{
	if ( x != xTrans )
	{
		xTrans = x;
		updateGL();
	}
}

void GLView::setYTrans( int y )
{
	if ( y != yTrans )
	{
		yTrans = y;
		updateGL();
	}
}

void GLView::setZoom( int z )
{
	if ( z <= 10 || z >= 100000 )
		return;
	if ( z != zoom )
	{
		zoom = z;
		emit zoomChanged( zoom );
		updateGL();
	}
}

void GLView::advanceGears()
{
	setZRotation( zRot + zInc );
	updateGL();
}

void GLView::dataChanged()
{
	updated = true;
	updateGL();
}

void GLView::compile( bool center )
{
	doCompile = true;
	doCenter = center;
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
	QPoint relPos = event->pos();
	if ( ! ( model && ( pressPos - relPos ).manhattanLength() <= 3 ) )
		return;
	
	makeCurrent();

	GLuint	buffer[512];
	GLint	hits;
	
	GLint	viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	
	glSelectBuffer( 512, buffer );

	glRenderMode( GL_SELECT );	
	glInitNames();
	glPushName( 0 );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPickMatrix( (GLdouble) pressPos.x(), (GLdouble) (viewport[3]-pressPos.y()), 1.0f, 1.0f, viewport);

	int x = qMin( width(), height() );	
	float fx = (float) width() / x;
	float fy = (float) height() / x;
	glFrustum(-fx, +fx, -fy, fy, 5.0, 100000.0);
	
	glMatrixMode(GL_MODELVIEW);

	scene->drawAgain();
	
	hits = glRenderMode( GL_RENDER );
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
		emit clicked( model->getBlock( choose ) );
	}
}

void GLView::mouseMoveEvent(QMouseEvent *event)
{
	int dx = event->x() - lastPos.x();
	int dy = event->y() - lastPos.y();

	if (event->buttons() & Qt::LeftButton)
	{
		setXRotation(xRot + 8 * dy);
		setYRotation(yRot + 8 * dx);
	}
	else if (event->buttons() & Qt::RightButton)
	{
		setZoom( zoom + 8 * dy );
		setZRotation(zRot + 8 * dx);
	}
	else if ( event->buttons() & Qt::MidButton)
	{
		setXTrans( xTrans + dx * 5 );
		setYTrans( yTrans + dy * 5 );
	}
	lastPos = event->pos();
}

void GLView::mouseDoubleClickEvent( QMouseEvent * )
{
	compile( false );
}

void GLView::wheelEvent( QWheelEvent * event )
{
	if ( event->delta() > 0 )
	{
		setZoom( zoom + zoom / 8 );
	}
	else if ( event->delta() < 0 )
	{
		setZoom( zoom - zoom / 8 );
	}
}



#endif
