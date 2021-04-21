#include "texture.h"

#include "spellbook.h"
#include "gl/gltex.h"
#include "spells/blocks.h"
#include "ui/widgets/fileselect.h"
#include "ui/widgets/nifeditors.h"
#include "ui/widgets/uvedit.h"

#include "lib/nvtristripwrapper.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSettings>
#include <QStringListModel>


// Brief description is deliberately not autolinked to class Spell
/*! \file texture.cpp
 * \brief Texturing spells
 *
 * All classes here inherit from the Spell class.
 */

/* XPM */
static char const * tex42_xpm[] = {
	"80 80 43 1",
	"   c None",
	".	c #29194A", "+	c #2D1D37", "@	c #2F2528", "#	c #333018",
	"$	c #3C2D66", "%	c #50521A", "&	c #5B4E3C", "*	c #5B4286",
	"=	c #544885", "-	c #584B70", ";	c #64589D", ">	c #67710F",
	",	c #7558A4", "'	c #6A619A", ")	c #68658B", "!	c #7264AA",
	"~	c #7E78A5", "{	c #8173B8", "]	c #8274B1", "^	c #8276AD",
	"/	c #8A72C0", "(	c #998F0F", "_	c #83819C", ":	c #908F97",
	"<	c #9F80D1", "[	c #9589BD", "}	c #9885CE", "|	c #9986C7",
	"1	c #9D9E9D", "2	c #9F9AB4", "3	c #A992DC", "4	c #B38FDD",
	"5	c #AC9AD0", "6	c #AF97DA", "7	c #C2BD04", "8	c #C1A1DC",
	"9	c #BEA9DD", "0	c #BABBBB", "a	c #C1B8D2", "b	c #CDB7DC",
	"c	c #D5CAE0", "d	c #D6D4D7",
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
	"                                                                                "
};

static QIconPtr tex42_xpm_icon = nullptr;

//! Find the reference to shape data from a model and index
QModelIndex getData( const NifModel * nif, const QModelIndex & index )
{
	if ( nif->isNiBlock( index, { "NiTriShape", "NiTriStrips", "BSLODTriShape" } ) )
		return nif->getBlock( nif->getLink( index, "Data" ) );
	
	if ( nif->isNiBlock( index, { "NiTriShapeData", "NiTriStripsData" } ) )
		return index;

	return QModelIndex();
}

//! Find the reference to UV data from a model and index
QModelIndex getUV( const NifModel * nif, const QModelIndex & index )
{
	QModelIndex iData = getData( nif, index );

	if ( iData.isValid() ) {
		QModelIndex iUVs = nif->getIndex( iData, "UV Sets" );
		return iUVs;
	}

	return QModelIndex();
}

//! Selects a texture filename
class spChooseTexture final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Choose" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }
	bool instant() const override final { return true; }
	QIcon icon() const override final
	{
		if ( !tex42_xpm_icon )
			tex42_xpm_icon = QIconPtr( new QIcon(QPixmap( tex42_xpm )) );

		return *tex42_xpm_icon;
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & idx ) override final
	{
		QModelIndex iBlock = nif->getBlock( idx );

		if ( nif->isNiBlock( iBlock, { "NiSourceTexture", "NiImage" } )
		     && ( iBlock == idx.sibling( idx.row(), 0 ) || nif->itemName( idx ) == "File Name" ) )
			return true;
		else if ( nif->isNiBlock( iBlock, "BSShaderNoLightingProperty" ) && nif->itemName( idx ) == "File Name" )
			return true;
		else if ( nif->isNiBlock( iBlock, "BSShaderTextureSet" ) && nif->itemName( idx ) == "Textures" )
			return true;
		else if ( nif->isNiBlock( iBlock, "SkyShaderProperty" ) && nif->itemName( idx ) == "File Name" )
			return true;
		else if ( nif->isNiBlock( iBlock, "TileShaderProperty" ) && nif->itemName( idx ) == "File Name" )
			return true;

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & idx ) override final
	{
		QModelIndex iBlock = nif->getBlock( idx );
		QModelIndex iFile;
		bool setExternal = false;

		if ( nif->isNiBlock( iBlock, { "NiSourceTexture", "NiImage" } )
		     && ( iBlock == idx.sibling( idx.row(), 0 ) || nif->itemName( idx ) == "File Name" ) )
		{
			iFile = nif->getIndex( iBlock, "File Name" );
			setExternal = true;
		} else if ( nif->isNiBlock( iBlock, "BSShaderTextureSet" ) && nif->itemName( idx ) == "Textures" ) {
			iFile = idx;
		} else if ( nif->isNiBlock( iBlock, "BSShaderNoLightingProperty" ) && nif->itemName( idx ) == "File Name" ) {
			iFile = idx;
		} else if ( nif->isNiBlock( iBlock, "SkyShaderProperty" ) && nif->itemName( idx ) == "File Name" ) {
			iFile = idx;
		} else if ( nif->isNiBlock( iBlock, "TileShaderProperty" ) && nif->itemName( idx ) == "File Name" ) {
			iFile = idx;
		}

		if ( !iFile.isValid() )
			return idx;

		QString file = TexCache::find( nif->get<QString>( iFile ), nif->getFolder() );

		QSettings settings;
		QString key = QString( "%1/%2/%3/Last Texture Path" ).arg( "Spells", page(), name() );

		if ( !QFile::exists( file ) ) {
			// if file not found in cache, use last texture path


			QString defaulttexpath( settings.value( key, QVariant( QDir::homePath() ) ).toString() );
			//file = QDir(defaulttexpath).filePath(file);
			file = defaulttexpath;
		}


		// to avoid shortcut resolve bug take *.* as text filter
		file = QFileDialog::getOpenFileName( qApp->activeWindow(), "Select a texture file", file, "*.*" );

		if ( !file.isEmpty() ) {
			// save path for future
			settings.setValue( key, QVariant( QDir( file ).absolutePath() ) );

			file = TexCache::stripPath( file, nif->getFolder() );

			if ( setExternal ) {
				nif->set<int>( iBlock, "Use External", 1 );

				// update the "File Name" block reference, since it changes when we set Use External
				if ( nif->checkVersion( 0x0A010000, 0 ) && nif->isNiBlock( iBlock, "NiSourceTexture" ) ) {
					iFile = nif->getIndex( iBlock, "File Name" );
				}
			}

			nif->set<QString>( iFile, file.replace( "/", "\\" ) );
		}

		return idx;
	}
};

REGISTER_SPELL( spChooseTexture )

//! Opens a UVWidget to edit texture coordinates
class spEditTexCoords final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit UV" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		auto iUVs = getUV( nif, index );
		auto iTriData = nif->getIndex( index, "Num Triangles" );
		return (iUVs.isValid() || iTriData.isValid())
			&& (nif->inherits( index, "NiTriBasedGeom" ) || nif->inherits( index, "BSTriShape" ));
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		UVWidget::createEditor( nif, index );
		return index;
	}
};

REGISTER_SPELL( spEditTexCoords )

//! Add a texture to the specified texture slot
/*!
 * \param nif The model
 * \param index The index of the mesh
 * \param name The name of the texture slot
 */
QModelIndex addTexture( NifModel * nif, const QModelIndex & index, const QString & name )
{
	QModelIndex iTexProp = nif->getBlock( index, "NiTexturingProperty" );

	if ( !iTexProp.isValid() )
		return index;

	if ( nif->get<int>( iTexProp, "Texture Count" ) < 7 )
		nif->set<int>( iTexProp, "Texture Count", 7 );

	nif->set<int>( iTexProp, QString( "Has %1" ).arg( name ), 1 );
	QPersistentModelIndex iTex = nif->getIndex( iTexProp, name );

	if ( !iTex.isValid() )
		return index;

	nif->set<int>( iTex, "Clamp Mode", 3 );
	nif->set<int>( iTex, "Filter Mode", 3 );
	nif->set<int>( iTex, "PS2 K", -75 );
	nif->set<int>( iTex, "Unknown1", 257 );

	QModelIndex iSrcTex = nif->insertNiBlock( "NiSourceTexture", nif->getBlockNumber( iTexProp ) + 1 );
	nif->setLink( iTex, "Source", nif->getBlockNumber( iSrcTex ) );

	nif->set<int>( iSrcTex, "Pixel Layout", ( nif->getVersion() == "20.0.0.5" && name == "Base Texture" ? 6 : 5 ) );
	nif->set<int>( iSrcTex, "Use Mipmaps", 2 );
	nif->set<int>( iSrcTex, "Alpha Format", 3 );
	nif->set<int>( iSrcTex, "Unknown Byte", 1 );
	nif->set<int>( iSrcTex, "Is Static", 1 );
	nif->set<int>( iSrcTex, "Use External", 1 );

	spChooseTexture * chooser = new spChooseTexture();
	return chooser->cast( nif, iSrcTex );
}

//! Adds a Base texture
class spAddBaseMap final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Base Texture" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Base Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Base Texture" );
	}
};

REGISTER_SPELL( spAddBaseMap )

//! Adds a Dark texture
class spAddDarkMap final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Dark Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Dark Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Dark Texture" );
	}
};

REGISTER_SPELL( spAddDarkMap )

//! Adds a Detail texture
class spAddDetailMap final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Detail Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Detail Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Detail Texture" );
	}
};

REGISTER_SPELL( spAddDetailMap )

//! Adds a Glow texture
class spAddGlowMap final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Glow Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Glow Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Glow Texture" );
	}
};

REGISTER_SPELL( spAddGlowMap )

//! Adds a Bump texture
class spAddBumpMap final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Bump Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Bump Map Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iSrcTex = addTexture( nif, index, "Bump Map Texture" );
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		nif->set<float>( block, "Bump Map Luma Scale", 1.0 );
		nif->set<float>( block, "Bump Map Luma Offset", 0.0 );
		QModelIndex iMatrix = nif->getIndex( block, "Bump Map Matrix" );
		nif->set<float>( iMatrix, "m11", 1.0 );
		nif->set<float>( iMatrix, "m12", 0.0 );
		nif->set<float>( iMatrix, "m21", 0.0 );
		nif->set<float>( iMatrix, "m22", 1.0 );
		return iSrcTex;
	}
};

REGISTER_SPELL( spAddBumpMap )

//! Adds a Decal 0 texture
class spAddDecal0Map final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Decal 0 Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Decal 0 Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Decal 0 Texture" );
	}
};

REGISTER_SPELL( spAddDecal0Map )

//! Adds a Decal 1 texture
class spAddDecal1Map final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Decal 1 Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Decal 1 Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Decal 1 Texture" );
	}
};

REGISTER_SPELL( spAddDecal1Map )

//! Adds a Decal 2 texture
class spAddDecal2Map final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Decal 2 Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Decal 2 Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Decal 2 Texture" );
	}
};

REGISTER_SPELL( spAddDecal2Map )

//! Adds a Decal 3 texture
class spAddDecal3Map final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Decal 3 Map" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return ( block.isValid() && nif->get<int>( block, "Has Decal 3 Texture" ) == 0 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		return addTexture( nif, index, "Decal 3 Texture" );
	}
};

REGISTER_SPELL( spAddDecal3Map )

//! Wrap a value between 0 and 1
#define wrap01f( X ) ( X > 1 ? X - floor( X ) : X < 0 ? X - floor( X ) : X )

//! Saves the UV layout as a TGA
class spTextureTemplate final : public Spell
{
	QString name() const override final { return Spell::tr( "Export Template" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iUVs = getUV( nif, index );
		auto iTriData = nif->getIndex( index, "Num Triangles" );
		bool bstri = nif->getUserVersion2() >= 100 && nif->inherits( index, "BSTriShape" ) && iTriData.isValid();
		return (iUVs.isValid() && nif->rowCount( iUVs ) >= 1) || bstri;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iUVs = getUV( nif, index );

		if ( nif->rowCount( iUVs ) <= 0 && nif->getUserVersion2() < 100 )
			return index;

		// fire up a dialog to set the user parameters
		QDialog dlg;
		QGridLayout * lay = new QGridLayout;
		dlg.setLayout( lay );

		FileSelector * file = new FileSelector( FileSelector::SaveFile, "File", QBoxLayout::RightToLeft );
		file->setFilter( { "", "PNG (*.png)", "BMP (*.bmp)" } );
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

		lay->addWidget( new QLabel( "Antialias" ), 4, 0 );
		QCheckBox * antialias = new QCheckBox;
		lay->addWidget( antialias, 4, 1 );

		lay->addWidget( new QLabel( "Wire Color" ), 5, 0 );
		QPushButton * wireColor = new QPushButton;
		lay->addWidget( wireColor, 5, 1 );

		QPushButton * ok = new QPushButton( "Ok" );
		QObject::connect( ok, &QPushButton::clicked, &dlg, &QDialog::accept );
		lay->addWidget( ok, 6, 0, 1, 2 );

		QSettings settings;
		QString keyGroup = QString( "%1/%2/%3/" ).arg( "Spells", page(), name() );

		// Key formatter, avoid lots of beginGroup() and endGroup() this way
		auto k = [&keyGroup]( const QString& key ) { return QString( "%1%2" ).arg( keyGroup, key ); };

		wrap->setCurrentIndex( settings.value( k( "Wrap Mode" ), 0 ).toInt() );
		size->setCurrentIndex( settings.value( k( "Image Size" ), 2 ).toInt() );
		file->setText( settings.value( k( "File Name" ), "" ).toString() );
		antialias->setChecked( settings.value( k( "Antialias" ), true ).toBool() );

		QString colorARGB = settings.value( k( "Wire Color" ), "#FF000000" ).toString();
		wireColor->setText( colorARGB );
		QString bc = "background-color: ";
		wireColor->setStyleSheet( bc + colorARGB );

		QColorDialog * colorDlg = new QColorDialog;
		QObject::connect( wireColor, &QPushButton::clicked, [&]()
			{
				QColor c = colorDlg->getColor( wireColor->text(), nullptr, "Wire Color", QColorDialog::ShowAlphaChannel );

				if ( c.isValid() ) {
					colorARGB = c.name( QColor::NameFormat::HexArgb );
					wireColor->setText( colorARGB );
					wireColor->setStyleSheet( bc + colorARGB );
				}
			}
		);

		if ( dlg.exec() != QDialog::Accepted )
			return index;

		settings.setValue( k( "Wrap Mode" ), wrap->currentIndex() );
		settings.setValue( k( "Image Size" ), size->currentIndex() );
		settings.setValue( k( "File Name" ), file->text() );
		settings.setValue( k( "Antialias" ), antialias->isChecked() );
		settings.setValue( k( "Wire Color" ), colorARGB );

		// get the selected coord set
		QModelIndex iSet = iUVs.child( set->currentIndex(), 0 );

		QVector<Vector2> uv;
		QVector<Triangle> tri;

		if ( nif->getUserVersion2() >= 100 ) {
			QModelIndex iVertData;
			auto vf = nif->get<BSVertexDesc>( index, "Vertex Desc" );
			if ( (vf & VertexFlags::VF_SKINNED) && nif->getUserVersion2() == 100 ) {
				// Skinned SSE
				auto skinID = nif->getLink( nif->getIndex( index, "Skin" ) );
				auto partID = nif->getLink( nif->getBlock( skinID, "NiSkinInstance" ), "Skin Partition" );
				auto iPartBlock = nif->getBlock( partID, "NiSkinPartition" );
				if ( !iPartBlock.isValid() )
					return index;

				iVertData = nif->getIndex( iPartBlock, "Vertex Data" );

				// Get triangles from all partitions
				auto numParts = nif->get<int>( iPartBlock, "Num Skin Partition Blocks" );
				auto iParts = nif->getIndex( iPartBlock, "Partition" );
				for ( int i = 0; i < numParts; i++ )
					tri << nif->getArray<Triangle>( iParts.child( i, 0 ), "Triangles" );

			} else {
				iVertData = nif->getIndex( index, "Vertex Data" );
				tri = nif->getArray<Triangle>( index, "Triangles" );
			}

			for ( int i = 0; i < nif->rowCount( iVertData ); i++ )
				uv << nif->get<Vector2>( nif->index( i, 0, iVertData ), "UV" );

		} else {
			uv = nif->getArray<Vector2>( iSet );
		}

		// get the triangles
		QModelIndex iData = getData( nif, index );
		QModelIndex iPoints = nif->getIndex( iData, "Points" );

		if ( iPoints.isValid() ) {
			QVector<QVector<quint16> > strips;

			for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
				strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );

			tri = triangulate( strips );
		} else if ( nif->getUserVersion2() < 100 ) {
			tri = nif->getArray<Triangle>( nif->getIndex( getData( nif, index ), "Triangles" ) );
		}

		// render the template image
		quint16 s = size->currentText().toInt();

		QImage img( s, s, QImage::Format_ARGB32 );
		QPainter pntr( &img );

		img.fill( Qt::transparent );

		pntr.setRenderHint( QPainter::Antialiasing, antialias->isChecked() );
		pntr.fillRect( img.rect(), QColor( 0xff, 0xff, 0xff, 0 ) );
		//pntr.scale( s, s ); // Seems to work differently in Qt 5
		pntr.setPen( QColor( colorARGB ) );

		bool wrp = wrap->currentIndex() == 0;

		for ( const Triangle& t : tri ) {

			QPointF points[3];

			for ( int i = 0; i < 3; i++ ) {

				float u, v;

				u = uv.value( t[i] )[0];
				v = uv.value( t[i] )[1];

				if ( wrp ) {
					u = wrap01f( u );
					v = wrap01f( v );
				}

				// Scale points here instead of using pntr.scale()
				QPointF point( u * s, v * s );

				points[i] = point;
			}

			pntr.drawPolygon( points, 3 );
		}

		// write the file
		QString filename = file->text();

		// TODO: Fix FileSelector class so that this isn't necessary.
		if ( !filename.endsWith( ".PNG", Qt::CaseInsensitive ) && !filename.endsWith( ".BMP", Qt::CaseInsensitive ) )
			filename.append( ".png" );

		if ( filename.endsWith( ".PNG", Qt::CaseInsensitive ) ) {
			// Transparent PNG
			img.save( filename );

		} else {
			// Paint transparency onto bitmap with white background (Qt defaults to black)
			QImage bmp( img.size(), QImage::Format_RGB32 );
			bmp.fill( Qt::white );
			QPainter bmpPntr( &bmp );
			bmpPntr.drawImage( 0, 0, img );

			bmp.save( filename );
		}

		return index;
	}
};

REGISTER_SPELL( spTextureTemplate )

//! Global search and replace of texturing apply modes
class spMultiApplyMode final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Multi Apply Mode" ); }
	QString page() const override final { return Spell::tr( "Batch" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		// Apply Mode field is defined in nifs up to version 20.0.0.5
		return nif->checkVersion( 0, 0x14000005 ) && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( index );
		QStringList modes{ "Replace", "Decal", "Modulate", "Hilight", "Hilight2" };

		QDialog dlg;
		dlg.resize( 300, 60 );
		QComboBox * cbRep = new QComboBox( &dlg );
		QComboBox * cbBy  = new QComboBox( &dlg );
		QPushButton * btnOk     = new QPushButton( "OK", &dlg );
		QPushButton * btnCancel = new QPushButton( "Cancel", &dlg );
		cbRep->addItems( modes );
		cbRep->setCurrentIndex( 2 );
		cbBy->addItems( modes );
		cbBy->setCurrentIndex( 2 );

		QGridLayout * layout;
		layout = new QGridLayout;
		layout->setSpacing( 20 );
		layout->addWidget( new QLabel( "Replace", &dlg ), 0, 0, Qt::AlignBottom );
		layout->addWidget( new QLabel( "By", &dlg ), 0, 1, Qt::AlignBottom );
		layout->addWidget( cbRep, 1, 0, Qt::AlignTop );
		layout->addWidget( cbBy, 1, 1, Qt::AlignTop );
		layout->addWidget( btnOk, 2, 0 );
		layout->addWidget( btnCancel, 2, 1 );
		dlg.setLayout( layout );

		QObject::connect( btnOk, &QPushButton::clicked, &dlg, &QDialog::accept );
		QObject::connect( btnCancel, &QPushButton::clicked, &dlg, &QDialog::reject );

		if ( dlg.exec() != QDialog::Accepted )
			return QModelIndex();

		QList<QPersistentModelIndex> indices;

		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex idx = nif->getBlock( n );
			indices << idx;
		}

		for ( const QModelIndex& idx : indices ) {
			replaceApplyMode( nif, idx, cbRep->currentIndex(), cbBy->currentIndex() );
		}

		return QModelIndex();
	}

	//! Recursively replace the Apply Mode
	void replaceApplyMode( NifModel * nif, const QModelIndex & index, int rep, int by )
	{
		if ( !index.isValid() )
			return;

		if ( nif->inherits( index, "NiTexturingProperty" )
		     && nif->get<int>( index, "Apply Mode" ) == rep )
			nif->set<int>( index, "Apply Mode", by );

		QModelIndex iChildren = nif->getIndex( index, "Children" );
		QList<qint32> lChildren = nif->getChildLinks( nif->getBlockNumber( index ) );

		if ( iChildren.isValid() ) {
			for ( int c = 0; c < nif->rowCount( iChildren ); c++ ) {
				qint32 link = nif->getLink( iChildren.child( c, 0 ) );

				if ( lChildren.contains( link ) ) {
					QModelIndex iChild = nif->getBlock( link );
					replaceApplyMode( nif, iChild, rep, by );
				}
			}
		}

		QModelIndex iProperties = nif->getIndex( index, "Properties" );

		if ( iProperties.isValid() ) {
			for ( int p = 0; p < nif->rowCount( iProperties ); p++ ) {
				QModelIndex iProp = nif->getBlock( nif->getLink( iProperties.child( p, 0 ) ) );
				replaceApplyMode( nif, iProp, rep, by );
			}
		}
	}
};

REGISTER_SPELL( spMultiApplyMode )

//! Debug function - display information about a texture
class spTexInfo final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Info" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iBlock = nif->getBlock( index );

		if ( nif->isNiBlock( iBlock, "NiSourceTexture" ) )
			return true;

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		TexCache * tex = new TexCache();
		tex->setNifFolder( nif->getFolder() );
		int isExternal = nif->get<int>( index, "Use External" );

		if ( isExternal ) {
			QString filename = nif->get<QString>( index, "File Name" );
			tex->bind( filename );
		} else {
			tex->bind( index );
		}

		qDebug() << tex->info( index );
		return QModelIndex();
	}
};

#ifndef QT_NO_DEBUG
REGISTER_SPELL( spTexInfo )
#endif

//! Export a packed NiPixelData texture
class spExportTexture final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Export" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iBlock = nif->getBlock( index );

		if ( nif->isNiBlock( iBlock, "NiSourceTexture" ) && nif->get<int>( iBlock, "Use External" ) == 0 ) {
			QModelIndex iData = nif->getBlock( nif->getLink( index, "Pixel Data" ) );

			if ( iData.isValid() ) {
				return true;
			}
		} else if ( nif->inherits( iBlock, "NiPixelFormat" ) ) {
			int thisBlockNumber = nif->getBlockNumber( index );
			QModelIndex iParent = nif->getBlock( nif->getParent( thisBlockNumber ) );

			if ( !iParent.isValid() ) {
				return true;
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		TexCache * tex = new TexCache();
		tex->setNifFolder( nif->getFolder() );
		QModelIndex iBlock = nif->getBlock( index );

		if ( nif->inherits( iBlock, "NiSourceTexture" ) ) {
			tex->bind( index );
			QString file = nif->getFolder();

			if ( nif->checkVersion( 0x0A010000, 0 ) ) {
				// Qt uses "/" regardless of platform
				file.append( "/" + nif->get<QString>( index, "File Name" ) );
			}

			QModelIndex iData = nif->getBlock( nif->getLink( index, "Pixel Data" ) );
			QString filename  = QFileDialog::getSaveFileName( qApp->activeWindow(), Spell::tr( "Export texture" ), file, "Textures (*.dds *.tga)" );

			if ( !filename.isEmpty() ) {
				if ( tex->exportFile( iData, filename ) ) {
					nif->set<int>( index, "Use External", 1 );
					filename = TexCache::stripPath( filename, nif->getFolder() );
					nif->set<QString>( index, "File Name", filename );
					tex->bind( filename );
				}
			}

			return index;
		} else if ( nif->inherits( iBlock, "NiPixelFormat" ) ) {
			TexCache * tex = new TexCache();
			tex->setNifFolder( nif->getFolder() );
			QString file = nif->getFolder();
			QString filename = QFileDialog::getSaveFileName( qApp->activeWindow(), Spell::tr( "Export texture" ), file, "Textures (*.dds *.tga)" );

			if ( !filename.isEmpty() ) {
				tex->exportFile( index, filename );
			}
		}

		return index;
	}
};

REGISTER_SPELL( spExportTexture )

//! Pack a texture to NiPixelData
class spEmbedTexture final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Embed" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		if ( !( nif->checkVersion( 0, 0x0A020000 ) || nif->checkVersion( 0x14000004, 0 ) ) ) {
			return false;
		}

		QModelIndex iBlock = nif->getBlock( index );

		if ( !( nif->isNiBlock( iBlock, "NiSourceTexture" ) && nif->get<int>( iBlock, "Use External" ) == 1 ) ) {
			return false;
		}

		TexCache * tex = new TexCache();
		tex->setNifFolder( nif->getFolder() );

		if ( tex->bind( nif->get<QString>( iBlock, "File Name" ) ) ) {
			return true;
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		TexCache * tex = new TexCache();
		tex->setNifFolder( nif->getFolder() );

		if ( tex->bind( index ) ) {
			//qDebug() << "spEmbedTexture: Embedding texture " << index;

			int blockNum = nif->getBlockNumber( index );
			nif->insertNiBlock( "NiPixelData", blockNum + 1 );
			QPersistentModelIndex iSourceTexture = nif->getBlock( blockNum, "NiSourceTexture" );
			QModelIndex iPixelData = nif->getBlock( blockNum + 1, "NiPixelData" );

			//qDebug() << "spEmbedTexture: Block number" << blockNum << "holds source" << iSourceTexture << "Pixel data will be stored in" << iPixelData;

			// finish writing this function
			if ( tex->importFile( nif, iSourceTexture, iPixelData ) ) {
				QString tempFileName = nif->get<QString>( iSourceTexture, "File Name" );
				tempFileName = TexCache::stripPath( tempFileName, nif->getFolder() );
				nif->set<int>( iSourceTexture, "Use External", 0 );
				nif->set<int>( iSourceTexture, "Unknown Byte", 1 );
				nif->setLink( iSourceTexture, "Pixel Data", blockNum + 1 );

				if ( nif->checkVersion( 0x0A010000, 0 ) ) {
					nif->set<QString>( iSourceTexture, "File Name", tempFileName );
				} else {
					nif->set<QString>( index, "Name", tempFileName );
				}
			} else {
				qCWarning( nsSpell ) << tr( "Could not save texture." );
				// delete block?
				/*
				nif->removeNiBlock( blockNum+1 );
				nif->set<int>( iSourceTexture, "Use External", 1 );
				*/
			}
		}

		return index;
	}
};

REGISTER_SPELL( spEmbedTexture )

TexFlipDialog::TexFlipDialog( NifModel * n, QModelIndex & index, QWidget * parent ) : QDialog( parent )
{
	nif = n;
	baseIndex = index;

	grid = new QGridLayout;
	setLayout( grid );

	listmodel = new QStringListModel;
	listview  = new QListView;
	listview->setModel( listmodel );

	// texture action group; see options.cpp
	QButtonGroup * actgrp = new QButtonGroup( this );
	connect( actgrp, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &TexFlipDialog::textureAction );
	int btnid = 0;
	for ( const QString& tfaname : QStringList{
			Spell::tr( "Add Textures" ), Spell::tr( "Remove Texture" ),
			Spell::tr( "Move Up" ), Spell::tr( "Move Down" )
		} )
	{
		QPushButton * bt = new QPushButton( tfaname );
		textureButtons[btnid] = bt;
		actgrp->addButton( bt, btnid++ );
		grid->addWidget( bt, 0, btnid, 1, 1 );
	}

	// add the list view and set up members
	grid->addWidget( listview, 1, 0, 1, 0 );
	listFromNif();

	connect( listview->selectionModel(), &QItemSelectionModel::currentChanged, this, &TexFlipDialog::texIndex );
	texIndex( listview->currentIndex() );

	QHBoxLayout * hbox1 = new QHBoxLayout();
	hbox1->addWidget( startTime = new NifFloatEdit( nif, nif->getIndex( baseIndex, "Start Time" ), 0 ) );
	hbox1->addWidget( stopTime = new NifFloatEdit( nif, nif->getIndex( baseIndex, "Stop Time" ), 0 ) );
	grid->addLayout( hbox1, 2, 0, 1, 0 );

	startTime->updateData( nif );
	stopTime->updateData( nif );

	QHBoxLayout * hbox2 = new QHBoxLayout();
	QPushButton * ok = new QPushButton( Spell::tr( "OK" ), this );
	hbox2->addWidget( ok );
	connect( ok, &QPushButton::clicked, this, &TexFlipDialog::accept );
	QPushButton * cancel = new QPushButton( Spell::tr( "Cancel" ), this );
	hbox2->addWidget( cancel );
	connect( cancel, &QPushButton::clicked, this, &TexFlipDialog::reject );
	grid->addLayout( hbox2, 3, 0, 1, 0 );
}

void TexFlipDialog::textureAction( int i )
{
	QModelIndex idx = listview->currentIndex();

	switch ( i ) {
	case 0:     // add
		flipnames = QFileDialog::getOpenFileNames( this, Spell::tr( "Choose texture file(s)" ), nif->getFolder(), "Textures (*.dds *.tga *.bmp)" );
		listmodel->setStringList( listmodel->stringList() << flipnames );
		break;
	case 1:     // remove

		if ( idx.isValid() ) {
			listmodel->removeRow( idx.row(), QModelIndex() );
		}

	case 2:

		if ( idx.isValid() && idx.row() > 0 ) {
			// move up
			QModelIndex xdi = idx.sibling( idx.row() - 1, 0 );
			QVariant v = listmodel->data( idx, Qt::EditRole );
			listmodel->setData( idx, listmodel->data( xdi, Qt::EditRole ), Qt::EditRole );
			listmodel->setData( xdi, v, Qt::EditRole );
			listview->setCurrentIndex( xdi );
		}
		break;
	case 3:

		if ( idx.isValid() && idx.row() < listmodel->rowCount() - 1 ) {
			// move down
			QModelIndex xdi = idx.sibling( idx.row() + 1, 0 );
			QVariant v = listmodel->data( idx, Qt::EditRole );
			listmodel->setData( idx, listmodel->data( xdi, Qt::EditRole ), Qt::EditRole );
			listmodel->setData( xdi, v, Qt::EditRole );
			listview->setCurrentIndex( xdi );
		}
		break;
	}
}

void TexFlipDialog::texIndex( const QModelIndex & idx )
{
	textureButtons[0]->setEnabled( true );
	textureButtons[1]->setEnabled( idx.isValid() );
	textureButtons[2]->setEnabled( idx.isValid() && ( idx.row() > 0 ) );
	textureButtons[3]->setEnabled( idx.isValid() && ( idx.row() < listmodel->rowCount() - 1 ) );
}

QStringList TexFlipDialog::flipList()
{
	return listmodel->stringList();
}

void TexFlipDialog::listFromNif()
{
	// update string list
	int numSources = nif->get<int>( baseIndex, "Num Sources" );
	QModelIndex sources = nif->getIndex( baseIndex, "Sources" );

	if ( nif->rowCount( sources ) != numSources ) {
		qCWarning( nsSpell ) << tr( "'Num Sources' does not match!" );
		return;
	}

	QStringList sourceFiles;

	for ( int i = 0; i < numSources; i++ ) {
		QModelIndex source = nif->getBlock( nif->getLink( sources.child( i, 0 ) ) );
		sourceFiles << nif->get<QString>( source, "File Name" );
	}

	listmodel->setStringList( sourceFiles );
}

//! Edit TexFlipController
/*!
 * TODO: update for version conditions, preserve properties on existing textures
 */
class spEditFlipper final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit Flip Controller" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return ( nif->getBlockName( index ) == "NiFlipController" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex flipController = index;
		TexFlipDialog * texFlip = new TexFlipDialog( nif, flipController );

		if ( texFlip->exec() != QDialog::Accepted ) {
			return QModelIndex();
		}

		QStringList flipNames = texFlip->flipList();

		if ( flipNames.size() == 0 ) {
			//nif->removeNiBlock( nif->getBlockNumber( flipController ) );
			return index;
		}

		// TODO: use a map here to delete missing textures and preserve existing properties

		QModelIndex sources = nif->getIndex( flipController, "Sources" );
		int size = flipNames.size();

		if ( nif->get<int>( flipController, "Num Sources" ) > size ) {
			// delete blocks
			int num = nif->get<int>( flipController, "Num Sources" );
			for ( int i = size; i < num; i++ ) {
				Message::append( tr( "Found %1 textures, have %2" ).arg( size ).arg( num ),
					tr( "Deleting %1" ).arg( nif->getLink( sources.child( i, 0 ) ) ), QMessageBox::Information
				);
				nif->removeNiBlock( nif->getLink( sources.child( i, 0 ) ) );
			}
		}

		nif->set<int>( flipController, "Num Sources", flipNames.size() );
		nif->updateArray( sources );

		for ( int i = 0; i < flipNames.size(); i++ ) {
			QString name = TexCache::stripPath( flipNames.at( i ), nif->getFolder() );
			QModelIndex sourceTex;

			if ( nif->getLink( sources.child( i, 0 ) ) == -1 ) {
				sourceTex = nif->insertNiBlock( "NiSourceTexture", nif->getBlockNumber( flipController ) + i + 1 );
				nif->setLink( sources.child( i, 0 ), nif->getBlockNumber( sourceTex ) );
			} else {
				sourceTex = nif->getBlock( nif->getLink( sources.child( i, 0 ) ) );
			}

			nif->set<QString>( sourceTex, "File Name", name );
		}

		nif->set<float>( flipController, "Frequency", 1 );
		nif->set<quint16>( flipController, "Flags", 8 );
		nif->set<float>( flipController, "Delta", ( nif->get<float>( flipController, "Stop Time" ) - nif->get<float>( flipController, "Start Time" ) ) / flipNames.size() );

		return index;
	}
};

REGISTER_SPELL( spEditFlipper )

//! Insert and manage TexFlipController
class spTextureFlipper final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Flip Controller" ); }
	QString page() const override final { return Spell::tr( "Texture" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		// also check NiTextureProperty?
		QModelIndex block = nif->getBlock( index, "NiTexturingProperty" );
		return block.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		// attach a NiFlipController
		QModelIndex flipController = nif->insertNiBlock( "NiFlipController", nif->getBlockNumber( index ) + 1 );
		blockLink( nif, index, flipController );

		spEditFlipper flipEdit;

		return flipEdit.cast( nif, flipController );
	}
};

REGISTER_SPELL( spTextureFlipper )
