
#include "../spellbook.h"
#include "../gl/gltex.h"

#include "../widgets/fileselect.h"
#include "../NvTriStrip/qtwrapper.h"

#include <QGLPixelBuffer>

#include <QComboBox>
#include <QDialog>
#include <QDirModel>
#include <QImage>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTreeView>

/* XPM */
static char * tex42_xpm[] = {
"80 80 43 1",
" 	c None",
".	c #29194A","+	c #2D1D37","@	c #2F2528","#	c #333018",
"$	c #3C2D66","%	c #50521A","&	c #5B4E3C","*	c #5B4286",
"=	c #544885","-	c #584B70",";	c #64589D",">	c #67710F",
",	c #7558A4","'	c #6A619A",")	c #68658B","!	c #7264AA",
"~	c #7E78A5","{	c #8173B8","]	c #8274B1","^	c #8276AD",
"/	c #8A72C0","(	c #998F0F","_	c #83819C",":	c #908F97",
"<	c #9F80D1","[	c #9589BD","}	c #9885CE","|	c #9986C7",
"1	c #9D9E9D","2	c #9F9AB4","3	c #A992DC","4	c #B38FDD",
"5	c #AC9AD0","6	c #AF97DA","7	c #C2BD04","8	c #C1A1DC",
"9	c #BEA9DD","0	c #BABBBB","a	c #C1B8D2","b	c #CDB7DC",
"c	c #D5CAE0","d	c #D6D4D7",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                                                                ",
"                                             d   2                              ",
"                                             aaa ~c d                           ",
"                            dd              b_~2a!)~0                           ",
"                           cdcc            a6{'~[]'')0                          ",
"                          cdada9c         dc5^'^]];''~0                         ",
"                         53bba05[1        dc5{]|{|'']|20                        ",
"                        ca5}aaab}~0d     cdc5]{|||''|6~10                       ",
"                        bb2<50ab|{_0     cdc}}|]|['{[}[10                       ",
"                        b5b|<a0b|]':0    cda3|{{5{{}53|:1d                      ",
"                         [b[<5ca5]'~1d  cccb66{^6{{[3}|_10                      ",
"                         |55/|aa5/'':0  bdca65{|9{{5}||_:0                      ",
"                         555|<509|!;)1dcbccb93]55{|3}}5_11                      ",
"                         b}5|<]ab5{!':0[bccb9}]99{35}}|_11                      ",
"                          <5|</5a5/]!)1]ccbb9{]c3}3}}[^:11                      ",
"                          5<5|/,b5}!!')'bcbb9{5b|}|}}|{:11                      ",
"                          a<5|}/[5|}!]''bbb96{a933|}[{~110                      ",
"                           <|8//!56{;!''9bb8|[c5}5}}]{_1:0                      ",
"                           9}64/{/8}!!;=59b3}bc3|3{|^^:110                      ",
"            [{/}333369b    c}}6|!,|}!;]=]883{ab353]{|~1:0d       cad            ",
"        [2 !!{}<<433}}<}5   96<<]!!6,;!=]68}|c633|{^^:110    c9['{;'~           ",
"          ^;;{/}}<43<<//{/2 d6||<',]{;'==63]5b536|]]_111d cb|||{{{''-0d         ",
"           2!!{{}}<<<</{/{{{c9|||'=,];='=|<]99}6|{[~2110c963}{}}{]{')11a        ",
"           ]';;!{{{}}<//!,!{'_[]/]'=,'**-]]]8653[|^:12566333}}]{{''''^{~        ",
"            {{!!!!{{/{{/!,;;!'^[]'====$*$!'/6|3||~_:[966333}}{{]]]{'{'~0d       ",
"             ~{'!!!!{!{!{,;=='=]]]=-=*$=-=*]<6|3|^29b83444<}}{{{'{{{{{]_20      ",
"             2'{'{!!!!!;!!;'=$==]'=*.*$$$-*'/|66|bcb68643}}{]{{{}}{}{}}}_0      ",
"              ~''';;!,;!='';==-$='-=...$$+$,<45bcc99883<3}|{{{}3}333}}3|:10     ",
"              2'''!;!!!;;;='==$$$=$$.+..++*]/8bbbbcb8433}{}3333333333}~:110     ",
"               '''!{{!!!;;=====$..-$..+$@&*,<888cdc846}/}<33333365965~:111d     ",
"               '''{{}{;!';;==$$$....+@@+%&$,<<4bbb88<464333333396995_1110d      ",
"                ^{{}{]{;;;;===-...+++###>#-**<44884488434<<<333699[11110        ",
"               96}{||{]'';===$$$.+@##%>%>&@**,,4bb88444}}}}366352:11110         ",
"            [|33363}{{{!'====$$..+#>>>>>>((@-,<4b888444<<3436|_211110           ",
"         [{{/}34464}}{!;'===$$$$.+#%>>>(((7-*,4888888643}}]!')_1110d            ",
"      a]{{}}}3368633{/''==$$$...+#%>>((7(%%@'<8884/||<]!*====-=):d              ",
"     [}}/}/}366633<//!;===$...++#%>>>7777(&&&--]]]=*====;!;;;==='_d             ",
"     |3}333336334<<{!!;=$$$$....+@#%(777((&---$-*-==;*;=*;{;;;';;;)2            ",
"    63633333433<}{/!;;'=='==$$...+@%>(77(7&*/4]**,**=*=;!///{!!;';;;)c          ",
"    6966333<3<<}{{/{{!!!=====*$$.+@##&((&&@-,<48/,]/!!;;,/{/{{{{{{{]!'2         ",
"    93|}}}3}}//}<{}{/!'!;';;;*$$..++@@((-***,<484<<}<<//,,!!{}{{{}}}|{!2d       ",
"    5]{}3//{/{}/|//'!]{'!;;;,*$$.....@%&-****488444<////{!!;,;{{{|||66|^2d      ",
"    2~!!!{/{]{]{{]]]{]!!!//,***$....$+@@*,,</<884444<////{!!!;!;'!{{|||~)1d     ",
"     2;{'']''!]]]]!]!!!!//,,,*$.$$$*$$+$,/</464894446</!!!/!;,;;!'!'']^2:11d    ",
"      )'''''''''!]!]!!/////!,*$**$*$$*$$,]4<8b84888444</!!!!!;;;';'''''_~10d    ",
"        _)-)-)''~''']<<///,;********$***/<6b4cc848863<<</,;;;;;;=';=='-:::ad    ",
"          11::1:111[63<<<{,==;,;=;=*****{6<bbbcb6<888<<<</;;;=;;=;'''')::0dd    ",
"                  23}/}}/!;;!!;;;;=;!;;,/8|5c8bc9448844<///'='==';='=):110      ",
"                 9|3<}}/{;;///!!]=={;;!|<b|]db8bb84/8634<{{{='=)=))):1:11d      ",
"                933/{}{{;!/<}/{{;=}/!!,|498/)cb4b864}863}/}{^))__:111110d       ",
"                |}}}{{{'{}}}{{}'='3/{{!8458^'2c888834<663<3}|[11111000          ",
"               ]]|{{{{'{}33<}}''=36{/!'9689,-_28888866}66333|6:1d               ",
"              ~{{]{]]'||}3}}}{')]63}//]9689{=_:b868864}}666653910               ",
"             a]]]]''{|333}}3{]_)58|}}{]886b!--1:8638863}}999955a1d              ",
"             ~'^'']]3363336|^):[966}}{]9668{'-11a8|}8666}399bb9a10              ",
"            a')''']|666636]]_1:9b6}|3{|b469/-)11dc6||666}|69bcbc01d             ",
"            )__)']}636686|^_112c84|63{]b845/=)11d c[|||93|3699ccc10             ",
"              c']|6999966~:1112c86}8|}|b638{--11d  d[||69|}5898bb11d            ",
"              ~']5599b95_11110bc84}84}|c935{')11d    a||65|]3665[11d            ",
"             a~^5959992:1:10d bb83356}|b969!-)11d     c[|6||_2521:10            ",
"               c53652:11100   a96}3866|c969{')11d       a[[_:1:2200             ",
"                 a2:1111d     c53|66636cb9b{'-11d        d01110d2               ",
"                  d0100       d6||58666cbbb{')11d          ddd                  ",
"                               [||666b6cc9c|')11d                               ",
"                               [||653c8ccbd5'_11d                               ",
"                               a||663bbcdbc9')11d                               ",
"                               c]]|53bbcdcc9^_11d                               ",
"                                []|3599cccc5~:10d                               ",
"                                d^]|[^69ccc9_:11                                ",
"                                 a~||)56bcd[:11d                                ",
"                                   a[)26bbc:111                                 ",
"                                    a11[ac2111d                                 ",
"                                      d[a11110                                  ",
"                                       a d000                                   ",
"                                       d  d                                     ",
"                                                                                "};

QIcon * tex42_xpm_icon = 0;

class spChooseTexture : public Spell
{
public:
	QString name() const { return "Choose"; }
	QString page() const { return "Texture"; }
	bool instant() const { return true; }
	QIcon icon() const
	{
		if ( ! tex42_xpm_icon ) tex42_xpm_icon = new QIcon( tex42_xpm );
		return *tex42_xpm_icon;
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & idx )
	{
		QModelIndex iBlock = nif->getBlock( idx );
		return ( nif->isNiBlock( iBlock, "NiSourceTexture" ) && ( iBlock == idx.sibling( idx.row(), 0 ) || nif->itemName( idx ) == "File Name" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & idx )
	{
		QModelIndex iSource = nif->getBlock( idx );
		QString file = TexCache::find( nif->get<QString>( iSource, "File Name" ), nif->getFolder() );
		
		file = QFileDialog::getOpenFileName( 0, "Select a texture file", file );
		
		if ( !file.isEmpty() )
		{
			QStringList folders = TexCache::texfolders;
			if ( ! nif->getFolder().isEmpty() )
				folders.append( nif->getFolder() );
			foreach ( QString base, folders )
			{
				if ( file.toLower().replace( "/", "\\" ).startsWith( base.toLower().replace( "/", "\\" ) ) )
				{
					int pos = 0;
					if ( nif->getVersion() == "4.0.0.2" && ( pos = base.toLower().indexOf( "data files" ) ) >= 0 )
						base = base.left( pos + 10 );
					file.remove( 0, base.length() );
					break;
				}
			}
			if ( file.startsWith( "/" ) || file.startsWith( "\\" ) )
				file.remove( 0, 1 );
		}
		
		if ( ! file.isEmpty() )
		{
			nif->set<int>( iSource, "Use External", 1 );
			QModelIndex iFile = nif->getIndex( iSource, "File Name" );
			nif->setData( iFile.sibling( iFile.row(), NifModel::ValueCol ), file );
		}
		return idx;
	}
};

REGISTER_SPELL( spChooseTexture )

QModelIndex addTexture( NifModel * nif, const QModelIndex & index, const QString & name )
{
	QModelIndex iTexProp = nif->getBlock( index, "NiTexturingProperty" );
	if ( ! iTexProp.isValid() )	return index;
	if ( nif->get<int>( iTexProp, "Texture Count" ) < 7 )
		nif->set<int>( iTexProp, "Texture Count", 7 );
		
	nif->set<int>( iTexProp, QString( "Has %1" ).arg( name ), 1 );
	QPersistentModelIndex iTex = nif->getIndex( iTexProp, name );
	if ( ! iTex.isValid() ) return index;
	
	nif->set<int>( iTex, "Clamp Mode", 3 );
	nif->set<int>( iTex, "Filter Mode", 3 );
	nif->set<int>( iTex, "PS2 K", 65461 );
	nif->set<int>( iTex, "Unknown1", 257 );
	
	QModelIndex iSrcTex = nif->insertNiBlock( "NiSourceTexture", nif->getBlockNumber( iTexProp ) + 1 );
	nif->setLink( iTex, "Source", nif->getBlockNumber( iSrcTex ) );
	
	nif->set<int>( iSrcTex, "Pixel Layout", ( nif->getVersion() == "20.0.0.5" && name == "Base Texture" ? 6 : 5 ) );
	nif->set<int>( iSrcTex, "Use Mipmaps", 2 );
	nif->set<int>( iSrcTex, "Alpha Format", 3 );
	nif->set<int>( iSrcTex, "Unknown Byte", 1 );
	nif->set<int>( iSrcTex, "Unknown Byte 2", 1 );
	nif->set<int>( iSrcTex, "Use External", 1 );
	
	return iSrcTex;
}

class spAddBaseMap : public Spell
{
public:
	QString name() const { return "Add Base Texture"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Base Texture" ) == 0 ); 
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		return addTexture( nif, index, "Base Texture" );
	}
};

REGISTER_SPELL( spAddBaseMap )

class spAddDarkMap : public Spell
{
public:
	QString name() const { return "Add Dark Map"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Dark Texture" ) == 0 ); 
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		return addTexture( nif, index, "Dark Texture" );
	}
};

REGISTER_SPELL( spAddDarkMap )

class spAddDetailMap : public Spell
{
public:
	QString name() const { return "Add Detail Map"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Detail Texture" ) == 0 ); 
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		return addTexture( nif, index, "Detail Texture" );
	}
};

REGISTER_SPELL( spAddDetailMap )

class spAddGlowMap : public Spell
{
public:
	QString name() const { return "Add Glow Map"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Glow Texture" ) == 0 ); 
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		return addTexture( nif, index, "Glow Texture" );
	}
};

REGISTER_SPELL( spAddGlowMap )

class spTextureFolders : public Spell
{
public:
	QString name() const { return "Folders"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel *, const QModelIndex & index )
	{
		return ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		TexCache::texfolders = selectMultipleDirs( "Select texture folders", TexCache::texfolders );
		nif->reset();
		return index;
	}
	
	QStringList selectMultipleDirs( const QString & title, const QStringList & def, QWidget * parent = 0 )
	{
		QDialog dlg( parent );
		dlg.setWindowTitle( title );
		
		QGridLayout * grid = new QGridLayout;
		dlg.setLayout( grid );
		
		QDirModel * model = new QDirModel( QStringList(), QDir::Dirs, QDir::Name, &dlg );
		QTreeView * view = new QTreeView;
		view->setModel( model );
		view->setSelectionMode( QAbstractItemView::MultiSelection );
		view->setColumnHidden( 1, true );
		view->setColumnHidden( 2, true );
		view->setColumnHidden( 3, true );
		
		foreach ( QString d, def )
		{
			QModelIndex idx = model->index( d );
			if ( idx.isValid() )
			{
				view->selectionModel()->select( idx, QItemSelectionModel::Select );
				while ( idx.parent().isValid() )
				{
					idx = idx.parent();
					view->expand( idx );
				}
			}
		}
		
		grid->addWidget( view, 0, 0, 1, 2 );
		
		QPushButton * ok = new QPushButton( "ok" );
		grid->addWidget( ok, 1, 0 );
		QPushButton * cancel = new QPushButton( "cancel" );
		grid->addWidget( cancel, 1, 1 );
		QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
		QObject::connect( cancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
		
		if ( dlg.exec() == QDialog::Accepted )
		{
			QStringList lst;
			foreach ( QModelIndex idx, view->selectionModel()->selectedIndexes() )
			{
				if ( idx.column() == 0 )
				{
					lst << model->filePath( idx );
				}
			}
			return lst;
		}
		else
			return def;
	}
};

REGISTER_SPELL( spTextureFolders )

class spFlipTexCoords : public Spell
{
public:
	QString name() const { return "Flip UV"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ).toLower() == "texcoord";
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		static const char * const flipCmds[3] = { "S = 1.0 - S", "T = 1.0 - T", "S <=> T" };
		for ( int c = 0; c < 3; c++ )
			menu.addAction( flipCmds[c] );
		QAction * act = menu.exec( QCursor::pos() );
		for ( int c = 0; c < 3; c++ )
			if ( act->text() == flipCmds[c] )
				flip( nif, index, c );
		return index;
	}

	void flip( NifModel * nif, const QModelIndex & index, int f )
	{
		if ( nif->isArray( index ) )
		{
			QModelIndex idx = index.child( 0, 0 );
			if ( idx.isValid() )
			{
				if ( nif->isArray( idx ) )
					flip( nif, idx, f );
				else
				{
					QVector<Vector2> tc = nif->getArray<Vector2>( index );
					for ( int c = 0; c < tc.count(); c++ )
						flip( tc[c], f );
					nif->setArray<Vector2>( index, tc );
				}
			}
		}
		else
		{
			Vector2 v = nif->get<Vector2>( index );
			flip( v, f );
			nif->set<Vector2>( index, v );
		}
	}

	void flip( Vector2 & v, int f )
	{
		switch ( f )
		{
			case 0:
				v[0] = 1.0 - v[0];
				break;
			case 1:
				v[1] = 1.0 - v[1];
				break;
			default:
				{
					float x = v[0];
					v[0] = v[1];
					v[1] = x;
				}	break;
		}
	}
	
};

REGISTER_SPELL( spFlipTexCoords )

#define wrap01f( X ) ( X > 1 ? X - floor( X ) : X < 0 ? X - floor( X ) : X )

class spTextureTemplate : public Spell
{
	QString name() const { return "Export Template"; }
	QString page() const { return "Texture"; }
	
	QModelIndex getData( const NifModel * nif, const QModelIndex & index ) const
	{
		if ( nif->isNiBlock( index, "NiTriShape" ) || nif->isNiBlock( index, "NiTriStrips" ) )
			return nif->getBlock( nif->getLink( index, "Data" ) );
		else if ( nif->isNiBlock( index, "NiTriShapeData" ) || nif->isNiBlock( index, "NiTriStripsData" ) )
			return index;
		return QModelIndex();
	}
	
	QModelIndex getUV( const NifModel * nif, const QModelIndex & index ) const
	{
		QModelIndex iData = getData( nif, index );
		
		if ( iData.isValid() )
		{
			QModelIndex iUVs = nif->getIndex( iData, "UV Sets" );
			if ( ! iUVs.isValid() )
				iUVs = nif->getIndex( iData, "UV Sets 2" );
			return iUVs;
		}
		return QModelIndex();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iUVs = getUV( nif, index );
		return iUVs.isValid() && nif->rowCount( iUVs ) >= 1;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iUVs = getUV( nif, index );
		if ( nif->rowCount( iUVs ) <= 0 )
			return index;
		
		// fire up a dialog to set the user parameters
		QDialog dlg;
		QGridLayout * lay = new QGridLayout;
		dlg.setLayout( lay );
		
		FileSelector * file = new FileSelector( FileSelector::SaveFile, "File", QBoxLayout::RightToLeft );
		file->setFilter( "*.tga" );
		lay->addWidget( file, 0, 0, 1, 2 );
		
		lay->addWidget( new QLabel( "Size" ), 1, 0 );
		QComboBox * size = new QComboBox;
		lay->addWidget( size, 1, 1 );
		for ( int i = 6; i < 12; i++ )
			size->addItem( QString::number( 2 << i ) );
		
		lay->addWidget( new QLabel( "Coord Set" ), 2, 0 );
		QComboBox * set = new QComboBox;
		lay->addWidget( set, 2, 1 );
		for ( int i = 0; i < nif->rowCount( iUVs ); i++ )
			set->addItem( QString( "set %1" ).arg( i ) );
		
		lay->addWidget( new QLabel( "Wrap Mode" ), 3, 0 );
		QComboBox * wrap = new QComboBox;
		lay->addWidget( wrap, 3, 1 );
		wrap->addItem( "wrap" );
		wrap->addItem( "clamp" );
		
		QPushButton * ok = new QPushButton( "Ok" );
		QObject::connect( ok, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
		lay->addWidget( ok, 4, 0, 1, 2 );
		
		QSettings settings( "NifTools", "NifSkope" );
		settings.beginGroup( "spells" );
		settings.beginGroup( page() );
		settings.beginGroup( name() );
		
		wrap->setCurrentIndex( settings.value( "Wrap Mode", 0 ).toInt() );
		size->setCurrentIndex( settings.value( "Image Size", 2 ).toInt() );
		file->setText( settings.value( "File Name", "" ).toString() );
		
		if ( dlg.exec() != QDialog::Accepted )
			return index;
		
		settings.setValue( "Wrap Mode", wrap->currentIndex() );
		settings.setValue( "Image Size", size->currentIndex() );
		settings.setValue( "File Name", file->text() );
		
		// get the selected coord set
		QModelIndex iSet = iUVs.child( set->currentIndex(), 0 );
		
		QVector<Vector2> uv = nif->getArray<Vector2>( iSet );
		QVector<Triangle> tri;
		
		// get the triangles
		QModelIndex iData = getData( nif, index );
		QModelIndex iPoints = nif->getIndex( iData, "Points" );
		if ( iPoints.isValid() )
		{
			QList< QVector< quint16 > > strips;
			for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
				strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );
			tri = triangulate( strips );
		}
		else
		{
			tri = nif->getArray<Triangle>( nif->getIndex( getData( nif, index ), "Triangles" ) );
		}
		
		// render the template image
		quint16 s = size->currentText().toInt();
		
		QImage img( s, s, QImage::Format_RGB32 );
		QPainter pntr( &img );
		
		pntr.fillRect( img.rect(), QColor( 0xff, 0xff, 0xff ) );
		pntr.scale( s, s );
		pntr.setPen( QColor( 0x10, 0x20, 0x30 ) );
		
		bool wrp = wrap->currentIndex() == 0;
		
		foreach ( Triangle t, tri )
		{
			Vector2 v2[3];
			for ( int i = 0; i < 3; i++ )
			{
				v2[i] = uv.value( t[i] );
				if ( wrp )
				{
					v2[i][0] = wrap01f( v2[i][0] );
					v2[i][1] = wrap01f( v2[i][1] );
				}
			}
			
			pntr.drawLine( QPointF( v2[0][0], v2[0][1] ), QPointF( v2[1][0], v2[1][1] ) );
			pntr.drawLine( QPointF( v2[1][0], v2[1][1] ), QPointF( v2[2][0], v2[2][1] ) );
			pntr.drawLine( QPointF( v2[2][0], v2[2][1] ), QPointF( v2[0][0], v2[0][1] ) );
		}
		
		// write the file
		QString filename = file->text();
		if ( ! filename.endsWith( ".tga", Qt::CaseInsensitive ) )
			filename.append( ".tga" );
		
		quint8 hdr[18];
		for ( int o = 0; o < 18; o++ ) hdr[o] = 0;
		hdr[02] = 2;
		hdr[12] = s % 256;
		hdr[13] = s / 256;
		hdr[14] = s % 256;
		hdr[15] = s / 256;
		hdr[16] = 32;
		hdr[17] = 32; // flipV
		
		QFile f( filename );
		if ( ! f.open( QIODevice::WriteOnly ) || f.write( (char *) hdr, 18 ) != 18 || f.write( (char *) img.bits(), s * s * 4 ) != s * s * 4 )
			qWarning() << "exportTemplate(" << filename << ") : could not write file";
		
		return index;
	}
};

REGISTER_SPELL( spTextureTemplate )

class spMultiApplyMode : public Spell
{
	
public:

	QString name() const { return "Multi Apply Mode"; }
	QString page() const { return "Texture"; } 

  	bool isApplicable( const NifModel * nif, const QModelIndex &index )
  	{
     	return nif->checkVersion( 0x14000005, 0x14000005 ) && index.isValid();
	}

  	QModelIndex cast( NifModel *nif, const QModelIndex &index )
  	{
  		QStringList modes;
  		modes << "Replace" <<  "Decal" << "Modulate" << "Hilight" << "Hilight2";

		QDialog dlg;
		dlg.resize( 300, 60 );
		QComboBox *cbRep = new QComboBox( &dlg );
		QComboBox *cbBy = new QComboBox( &dlg );
		QPushButton *btnOk = new QPushButton( "OK", &dlg );
		QPushButton *btnCancel = new QPushButton( "Cancel", &dlg );
		cbRep->addItems( modes );
		cbRep->setCurrentIndex( 2 );
		cbBy->addItems( modes );
		cbBy->setCurrentIndex( 2 );
		
		QGridLayout *layout;
		layout = new QGridLayout;
		layout->setSpacing( 20 );
		layout->addWidget( new QLabel( "Replace", &dlg ), 0, 0, Qt::AlignBottom );
		layout->addWidget( new QLabel( "By", &dlg ), 0, 1, Qt::AlignBottom );
		layout->addWidget( cbRep, 1, 0, Qt::AlignTop );
		layout->addWidget( cbBy, 1, 1, Qt::AlignTop );
		layout->addWidget( btnOk, 2, 0 );
		layout->addWidget( btnCancel, 2, 1 );
		dlg.setLayout( layout );		
		
		QObject::connect( btnOk, SIGNAL( clicked() ), &dlg, SLOT( accept() ) );
		QObject::connect( btnCancel, SIGNAL( clicked() ), &dlg, SLOT( reject() ) );
		
		if ( dlg.exec() != QDialog::Accepted )
			return QModelIndex();
		
		replaceApplyMode( nif, index, cbRep->currentIndex(), cbBy->currentIndex() );

    		return QModelIndex();
	}
	
	void 	replaceApplyMode( NifModel *nif, const QModelIndex &index, int rep, int by )
	{	
		if ( !index.isValid() )
			return;
					
    		if ( nif->inherits( index, "NiTexturingProperty" ) &&
     		nif->get<int>( index, "Apply Mode" ) == rep )
       		nif->set<int>( index, "Apply Mode", by );
		
		QModelIndex iChildren = nif->getIndex( index, "Children" );
		QList<qint32> lChildren = nif->getChildLinks( nif->getBlockNumber( index ) );
		if ( iChildren.isValid() )
		{
			for ( int c = 0; c < nif->rowCount( iChildren ); c++ )
			{
				qint32 link = nif->getLink( iChildren.child( c, 0 ) );
				if ( lChildren.contains( link ) )
				{
					QModelIndex iChild = nif->getBlock( link );
					replaceApplyMode( nif, iChild, rep, by );
				}
			}
		}

		QModelIndex iProperties = nif->getIndex( index, "Properties" );
		if ( iProperties.isValid() )
		{
			for ( int p = 0; p < nif->rowCount( iProperties ); p++ )
			{
				QModelIndex iProp = nif->getBlock( nif->getLink( iProperties.child( p, 0 ) ) );
				replaceApplyMode( nif, iProp, rep, by );
			}
		}
	}
};

REGISTER_SPELL( spMultiApplyMode )
