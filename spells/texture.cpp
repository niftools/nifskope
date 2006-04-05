
#include "../spellbook.h"
#include "../gltex.h"

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
		QModelIndex index = nif->getBlock( idx );
		return ( nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiSourceTexture"
			&& ( index == idx.sibling( idx.row(), 0 ) || nif->itemName( idx ) == "File Name" ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & idx )
	{
		QModelIndex index = nif->getBlock( idx );
		QModelIndex iTex = nif->getIndex( index, "Texture Source" );
		if ( iTex.isValid() )
		{
			QString file = GLTex::findFile( nif->get<QString>( iTex, "File Name" ), nif->getFolder() );
			
			file = QFileDialog::getOpenFileName( 0, "Select a texture file", file );
			
			if ( !file.isEmpty() )
			{
				QStringList folders = GLTex::texfolders;
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
				nif->set<int>( iTex, "Use External", 1 );
				QModelIndex iFile = nif->getIndex( iTex, "File Name" );
				nif->setData( iFile.sibling( iFile.row(), NifModel::ValueCol ), file );
			}
		}
		return index;
	}
};

REGISTER_SPELL( spChooseTexture )

QModelIndex addTexture( NifModel * nif, const QModelIndex & index, const QString & name )
{
	QModelIndex iTexProp = nif->getBlock( index, "NiTexturingProperty" );
	if ( ! iTexProp.isValid() )	return index;
	if ( nif->get<int>( iTexProp, "Texture Count" ) < 7 )
		nif->set<int>( iTexProp, "Texture Count", 7 );
	QModelIndex iTex = nif->getIndex( iTexProp, name );
	if ( ! iTex.isValid() )	return index;
	nif->set<int>( iTex, "Is Used", 1 );
	QPersistentModelIndex iTexDesc = nif->getIndex( iTex, "Texture Data" );
	if ( ! iTexDesc.isValid() ) return index;
	
	nif->set<int>( iTexDesc, "Clamp Mode", 3 );
	nif->set<int>( iTexDesc, "Filter Mode", 3 );
	nif->set<int>( iTexDesc, "PS2 K", 65461 );
	nif->set<int>( iTexDesc, "Unknown1", 257 );
	
	QModelIndex iSrcTex = nif->insertNiBlock( "NiSourceTexture", nif->getBlockNumber( iTexProp ) + 1 );
	nif->setLink( iTexDesc, "Source", nif->getBlockNumber( iSrcTex ) );
	
	nif->set<int>( iSrcTex, "Pixel Layout", 5 );
	nif->set<int>( iSrcTex, "Use Mipmaps", 2 );
	nif->set<int>( iSrcTex, "Alpha Format", 3 );
	nif->set<int>( iSrcTex, "Unknown Byte", 1 );
	nif->set<int>( nif->getIndex( iSrcTex, "Texture Source" ), "Use External", 1 );
	
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
		return ( block.isValid() && nif->get<int>( nif->getIndex( block, "Base Texture" ), "Is Used" ) == 0 ); 
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
		return ( block.isValid() && nif->get<int>( nif->getIndex( block, "Dark Texture" ), "Is Used" ) == 0 ); 
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
		return ( block.isValid() && nif->get<int>( nif->getIndex( block, "Detail Texture" ), "Is Used" ) == 0 ); 
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
		return ( block.isValid() && nif->get<int>( nif->getIndex( block, "Glow Texture" ), "Is Used" ) == 0 ); 
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		return addTexture( nif, index, "Glow Texture" );
	}
};

REGISTER_SPELL( spAddGlowMap )

class spPackTexture : public Spell
{
public:
	QString name() const { return "Pack"; }
	QString page() const { return "Texture"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & idx )
	{
		QModelIndex index = nif->getBlock( idx );
		if ( ! ( QGLPixelBuffer::hasOpenGLPbuffers() && nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiSourceTexture" ) )
			return false;
		QModelIndex iTex = nif->getIndex( index, "Texture Source" );
		return ( iTex.isValid() && nif->get<int>( iTex, "Use External" ) == 1 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & idx )
	{
		if ( nif->getVersionNumber() >= 0x14000000 && QMessageBox::question( 0, "Pack Texture", "Dunno how to pack textures for version 20.x.x.x", "Try Anyway", "Ok" ) != 0 )
			return idx;
		
		qWarning( "%x", nif->getVersionNumber() );
		
		QModelIndex index = nif->getBlock( idx );
		
		QGLPixelBuffer gl( QSize( 32, 32 ) );
		gl.makeCurrent();
		
		GLTex * tex = new GLTex( index );
		
		if ( tex->id )
		{
			glBindTexture( GL_TEXTURE_2D, tex->id );
			
			GLint w, h;
			
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h );
			
			quint32 s = w * h * 4;
			quint32 o = 0;
			
			QByteArray data;
			data.resize( s );
			glPixelStorei( GL_PACK_ALIGNMENT, 1 );
			glPixelStorei( GL_PACK_SWAP_BYTES, GL_FALSE );
			glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data() );
			
			QList<QSize> sizes;
			QList<quint32> offsets;
			
			sizes.append( QSize( w, h ) );
			offsets.append( o );
			
			while ( w != 1 || h != 1 )
			{
				o = s;
				w /= 2; if ( w == 0 ) w = 1;
				h /= 2; if ( h == 0 ) h = 1;
				s += w*h*4;
				sizes.append( QSize( w, h ) );
				offsets.append( o );
			}
			
			data.resize( s );
			
			for ( int i = 1; i < sizes.count(); i++ )
			{
				const quint8 * src = (const quint8 * ) data.data() + offsets[ i-1 ];
				quint8 * dst = (quint8 *) data.data() + offsets[ i ];
				
				w = sizes[ i ].width();
				h = sizes[ i ].height();
				
				quint32 xo = ( sizes[ i-1 ].width() > 1 ? 4 : 0 );
				quint32 yo = ( sizes[ i-1 ].height() > 1 ? sizes[ i-1 ].width() * 4 : 0 );
				
				for ( int y = 0; y < h; y++ )
				{
					for ( int x = 0; x < w; x++ )
					{
						for ( int b = 0; b < 4; b++ )
						{
							*dst++ = ( *(src+xo) + *(src+yo) + *(src+xo+yo) + *src++ ) / 4;
						}
						src += xo;
					}
					src += yo;
				}
			}
			
			int blockNum = nif->getBlockNumber( index );
			nif->insertNiBlock( "NiPixelData", blockNum+1 );
			
			QPersistentModelIndex iSourceTexture = nif->getBlock( blockNum, "NiSourceTexture" );
			QModelIndex iPixelData = nif->getBlock( blockNum+1, "NiPixelData" );
			if ( iSourceTexture.isValid() && iPixelData.isValid() )
			{
				nif->set<int>( iPixelData, "Pixel Format", 1 );
				nif->set<int>( iPixelData, "Red Mask", 0x000000ff );
				nif->set<int>( iPixelData, "Green Mask", 0x0000ff00 );
				nif->set<int>( iPixelData, "Blue Mask", 0x00ff0000 );
				nif->set<int>( iPixelData, "Alpha Mask", 0xff000000 );
				nif->set<int>( iPixelData, "Bits Per Pixel", 32 );
				nif->set<int>( iPixelData, "Bytes Per Pixel", 4 );
				nif->set<int>( iPixelData, "Num Mipmaps", sizes.count() );
				QModelIndex iMipMaps = nif->getIndex( iPixelData, "Mipmaps" );
				if ( iMipMaps.isValid() )
				{
					nif->updateArray( iMipMaps );
					for ( int m = 0; m < sizes.count() && m < nif->rowCount( iMipMaps ); m++ )
					{
						nif->set<int>( iMipMaps.child( m, 0 ), "Width", sizes[m].width() );
						nif->set<int>( iMipMaps.child( m, 0 ), "Height", sizes[m].height() );
						nif->set<int>( iMipMaps.child( m, 0 ), "Offset", offsets[m] );
					}
				}
				nif->set<QByteArray>( iPixelData, "Pixel Data", data );
				
				QModelIndex iUnknown = nif->getIndex( iPixelData, "Unknown 8 Bytes" );
				if ( iUnknown.isValid() )
				{
					static const int unknownPixeldataBytes[8] = { 129, 8, 130, 32, 0, 65, 12, 0 };
					for ( int r = 0; r < 8 && r < nif->rowCount( iUnknown ); r++ )
					{
						nif->set<int>( iUnknown.child( r, 0 ), unknownPixeldataBytes[r] );
					}
				}
				
				QModelIndex iTexSrc = nif->getIndex( iSourceTexture, "Texture Source" );
				if ( iTexSrc.isValid() )
				{
					nif->set<int>( iTexSrc, "Use External", 0 );
					nif->set<int>( iTexSrc, "Unknown Byte", 1 );
					nif->setLink( iTexSrc, "Pixel Data", blockNum+1 );
				}
			}
			delete tex;
			return iSourceTexture;
		}
		delete tex;
		return QModelIndex();
	}
};

REGISTER_SPELL( spPackTexture )

class spExportTexture : public Spell
{
public:
	QString name() const { return "Export"; }
	QString page() const { return "Texture"; }

	bool isApplicable( const NifModel * nif, const QModelIndex & idx )
	{
		QModelIndex index = nif->getBlock( idx );
		if ( ! ( QGLPixelBuffer::hasOpenGLPbuffers() && nif->itemType( index ) == "NiBlock" && nif->itemName( index ) == "NiSourceTexture" ) )
			return false;
		return true;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & idx )
	{
		QModelIndex index = nif->getBlock( idx );
		
		QGLPixelBuffer gl( QSize( 32, 32 ) );
		gl.makeCurrent();
		
		GLTex * tex = new GLTex( index );
		
		if ( tex->id )
		{
			QString filename = QFileDialog::getSaveFileName( 0, "Export texture", QString(), "*.TGA" );
			if ( ! filename.isEmpty() )
				tex->exportFile( filename );
		}
		
		delete tex;
		return index;
	}
};

REGISTER_SPELL( spExportTexture )

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
		GLTex::texfolders = selectMultipleDirs( "select texture folders", GLTex::texfolders );
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
		return nif->itemType( index ) == "texcoord";
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

#define clamp01f( X ) ( X > 1 ? 1 : X < 0 ? 0 : X )
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
		
		lay->addWidget( new QLabel( "Coord Set" ), 0, 0 );
		QComboBox * set = new QComboBox;
		lay->addWidget( set, 0, 1 );
		for ( int i = 0; i < nif->rowCount( iUVs ); i++ )
			set->addItem( QString( "set %1" ).arg( i ) );
		
		lay->addWidget( new QLabel( "Wrap Mode" ), 1, 0 );
		QComboBox * wrap = new QComboBox;
		lay->addWidget( wrap, 1, 1 );
		wrap->addItem( "wrap" );
		wrap->addItem( "clamp" );
		
		lay->addWidget( new QLabel( "Size" ), 2, 0 );
		QComboBox * size = new QComboBox;
		lay->addWidget( size, 2, 1 );
		for ( int i = 6; i < 12; i++ )
			size->addItem( QString::number( 2 << i ) );
		
		lay->addWidget( new QLabel( "File" ), 3, 0 );
		QLineEdit * file = new QLineEdit;
		lay->addWidget( file, 3, 1 );
		
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
			for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
			{
				QVector<quint16> p = nif->getArray<quint16>( iPoints.child( r, 0 ) );
				quint16 a = p.value( 0 );
				quint16 b = p.value( 1 );
				bool flip = false;
				for ( int x = 2; x < p.count(); x++ )
				{
					quint16 c = p[x];
					if ( a != b && b != c && c != a )
						if ( flip )
							tri.append( Triangle( a, c, b ) );
						else
							tri.append( Triangle( a, b, c ) );
					a = b;
					b = c;
					flip = ! flip;
				}
			}
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
		
		bool clamp = wrap->currentIndex() == 1;
		
		foreach ( Triangle t, tri )
		{
			Vector2 v2[3];
			for ( int i = 0; i < 3; i++ )
			{
				v2[i] = uv.value( t[i] );
				if ( clamp )
				{
					v2[i][0] = clamp01f( v2[i][0] );
					v2[i][1] = clamp01f( v2[i][1] );
				}
				else
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
		if ( ! filename.toLower().endsWith( ".tga" ) )
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
