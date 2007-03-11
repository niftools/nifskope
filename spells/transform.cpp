#include "transform.h"

#include "../widgets/nifeditors.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QMessageBox>
#include <QMimeData>

/* XPM */
static char * transform_xpm[] = {
"64 64 6 1",
" 	c None",
".	c #1800FF",
"+	c #FF0301",
"@	c #C46EBC",
"#	c #0DFF00",
"$	c #2BFFAC",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                           .....                                ",
"                            ....                                ",
"                            ....                                ",
"                           ......                               ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                    ##           ",
"     +                      ...                  ####           ",
"    ++++                    ...                 #####           ",
"     +++++                  ...                #######          ",
"      +++++                 ...              #######            ",
"        +++++               ...             #####               ",
"          +++++             ...           #####                 ",
"           +++++            ...          #####                  ",
"             +++++          ...        #####                    ",
"               +++++        ...       #####                     ",
"                +++++       ...      ####                       ",
"                  +++++     ...    #####                        ",
"                    +++++   ...   ####                          ",
"                     ++++++ ... #####                           ",
"                       +++++...#####                            ",
"                         +++...###                              ",
"                          ++...+#                               ",
"                           #...++                               ",
"                         ###...++++                             ",
"                        ####...++++++                           ",
"                      ##### ...  +++++                          ",
"                     ####   ...    +++++                        ",
"                   #####    ...     ++++++                      ",
"                  #####     ...       +++++                     ",
"                #####       ...         +++++                   ",
"               #####        ...          ++++++                 ",
"              ####          ...            +++++                ",
"            #####           ...              +++++              ",
"           ####             ...               ++++++            ",
"         #####              ...                 +++++  +        ",
"        #####               ...                   +++++++       ",
"      #####                 ...                    +++++++      ",
"     #####                  ...                      +++++      ",
"    ####                    ...                      ++++       ",
"    ###                     ...                        +        ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                            ...                                 ",
"                             ..                                 ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                ",
"                                                                "};

bool spApplyTransformation::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	return nif->itemType( index ) == "NiBlock" && ( nif->inherits( nif->itemName( index ), "NiNode" )
			|| nif->itemName( index ) == "NiTriShape" || nif->itemName( index ) == "NiTriStrips" );
}

QModelIndex spApplyTransformation::cast( NifModel * nif, const QModelIndex & index )
{
	if ( ( nif->getLink( index, "Controller" ) != -1 || nif->getLink( index, "Skin Instance" ) != -1 ) )
		if ( QMessageBox::question( 0, Spell::tr("Apply Transformation"), Spell::tr("On animated and or skinned nodes Apply Transformation most likely won't work the way you expected it."), Spell::tr("Try anyway"), Spell::tr("Cancel") ) != 0 )
			return index;
	
	if ( nif->inherits( nif->itemName( index ), "NiNode" ) )
	{
		Transform tp( nif, index );
		bool ok = false;
		foreach ( int l, nif->getChildLinks( nif->getBlockNumber( index ) ) )
		{
			QModelIndex iChild = nif->getBlock( l );
			if ( iChild.isValid() && nif->inherits( nif->itemName( iChild ), "NiAVObject" ) )
			{
				Transform tc( nif, iChild );
				tc = tp * tc;
				tc.writeBack( nif, iChild );
				ok = true;
			}
		}
		if ( ok )
		{
			tp = Transform();
			tp.writeBack( nif, index );
		}
	}
	else
	{
		QModelIndex iData;
		if ( nif->itemName( index ) == "NiTriShape") 
			iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriShapeData" );
		else if ( nif->itemName( index ) == "NiTriStrips" ) 
			iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriStripsData" );
		
		if ( iData.isValid() )
		{
			Transform t( nif, index );
			QModelIndex iVertices = nif->getIndex( iData, "Vertices" );
			if ( iVertices.isValid() )
			{
				QVector<Vector3> a = nif->getArray<Vector3>( iVertices );
				for ( int v = 0; v < nif->rowCount( iVertices ); v++ )
					a[v] = t * a[v];
				nif->setArray<Vector3>( iVertices, a );
				
				QModelIndex iNormals = nif->getIndex( iData, "Normals" );
				if ( iNormals.isValid() )
				{
					a = nif->getArray<Vector3>( iNormals );
					for ( int n = 0; n < nif->rowCount( iNormals ); n++ )
						a[n] = t.rotation * a[n];
					nif->setArray<Vector3>( iNormals, a );
				}
			}
			QModelIndex iCenter = nif->getIndex( iData, "Center" );
			if ( iCenter.isValid() )
				nif->set<Vector3>( iCenter, t * nif->get<Vector3>( iCenter ) );
			QModelIndex iRadius = nif->getIndex( iData, "Radius" );
			if ( iRadius.isValid() )
				nif->set<float>( iRadius, t.scale * nif->get<float>( iRadius ) );
			t = Transform();
			t.writeBack( nif, index );
		}
	}
	return index;
}

REGISTER_SPELL( spApplyTransformation )

class spClearTransformation : public Spell
{
public:
	QString name() const { return Spell::tr("Clear"); }
	QString page() const { return Spell::tr("Transform"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return Transform::canConstruct( nif, index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		Transform tp;
		tp.writeBack( nif, index );
		return index;
	}
};

REGISTER_SPELL( spClearTransformation )

class spCopyTransformation : public Spell
{
public:
	QString name() const { return Spell::tr("Copy"); }
	QString page() const { return Spell::tr("Transform"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return Transform::canConstruct( nif, index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) )
		{
			QDataStream ds( &buffer );
			ds << Transform( nif, index );
			
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/transform" ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
};

REGISTER_SPELL( spCopyTransformation )

class spPasteTransformation : public Spell
{
public:
	QString name() const { return Spell::tr("Paste"); }
	QString page() const { return Spell::tr("Transform"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( Transform::canConstruct( nif, index ) && mime )
			foreach ( QString form, mime->formats() )
				if ( form == "nifskope/transform" )
					return true;
		
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				if ( form == "nifskope/transform" )
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QDataStream ds( &buffer );
						Transform t;
						ds >> t;
						t.writeBack( nif, index );
						return index;
					}
				}
			}
		}
		return index;
	}
};

REGISTER_SPELL( spPasteTransformation )

QIcon * transform_xpm_icon = 0;

class spEditTransformation : public Spell
{
public:
	QString name() const { return Spell::tr("Edit"); }
	QString page() const { return Spell::tr("Transform"); }
	QIcon icon() const
	{
		if ( ! transform_xpm_icon )
			transform_xpm_icon = new QIcon( transform_xpm );
		return *transform_xpm_icon;
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return Transform::canConstruct( nif, index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		NifBlockEditor * edit = new NifBlockEditor( nif, nif->getBlock( index ) );
		edit->add( new NifVectorEdit( nif, nif->getIndex( index, "Translation" ) ) );
		edit->add( new NifRotationEdit( nif, nif->getIndex( index, "Rotation" ) ) );
		edit->add( new NifFloatEdit( nif, nif->getIndex( index, "Scale" ) ) );
		edit->show();
		return index;
	}
};

REGISTER_SPELL( spEditTransformation )

class spEditMatrix4 : public Spell
{
	QString name() const { return Spell::tr("Edit Matrix"); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->getValue( index ).type() == NifValue::tMatrix4;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		NifBlockEditor * edit = new NifBlockEditor( nif, nif->getBlock( index ) );
		edit->add( new NifMatrix4Edit( nif, index ) );
		edit->show();
		
		return index;
	}
};

REGISTER_SPELL( spEditMatrix4 )
