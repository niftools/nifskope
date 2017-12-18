#include "transform.h"

#include "ui/widgets/nifeditors.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCheckBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QSettings>


/* XPM */
static char const * transform_xpm[] = {
	"64 64 6 1",
	"   c None",
	".	c #1800FF",
	"+	c #FF0301",
	"@	c #C46EBC",
	"#	c #0DFF00",
	"$	c #2BFFAC",
	"                                                                ",
	"                                                                ",
	"                                                                ",
	"                             .                                  ",
	"                            ...                                 ",
	"                           .....                                ",
	"                          .......                               ",
	"                            ...                                 ",
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
	"                            ...                                 ",
	"                                                                ",
	"                                                                ",
	"                                                                ",
	"                                                                ",
	"                                                                ",
	"                                                                "
};

bool spApplyTransformation::isApplicable( const NifModel * nif, const QModelIndex & index )
{
	return nif->itemType( index ) == "NiBlock" &&
		( nif->inherits( nif->itemName( index ), "NiNode" )
	      || nif->inherits( nif->itemName( index ), "NiTriBasedGeom" )
		  || nif->inherits( nif->itemName( index ), "BSTriShape" ));
}

QModelIndex spApplyTransformation::cast( NifModel * nif, const QModelIndex & index )
{
	if ( nif->getLink( index, "Controller" ) != -1
		 || nif->getLink( index, "Skin Instance" ) != -1
		 || nif->getLink( index, "Skin" ) != -1 )
		if ( QMessageBox::question( 0, Spell::tr( "Apply Transformation" ),
			Spell::tr( "On animated and or skinned nodes Apply Transformation most likely won't work the way you expected it." ),
			Spell::tr( "Try anyway" ),
			Spell::tr( "Cancel" ) ) != 0 )
		{
			return index;
		}




	if ( nif->inherits( nif->itemName( index ), "NiNode" ) ) {
		Transform tp( nif, index );
		bool ok = false;
		for ( const auto l : nif->getChildLinks( nif->getBlockNumber( index ) ) ) {
			QModelIndex iChild = nif->getBlock( l );

			if ( iChild.isValid() && nif->inherits( nif->itemName( iChild ), "NiAVObject" ) ) {
				Transform tc( nif, iChild );
				tc = tp * tc;
				tc.writeBack( nif, iChild );
				ok = true;
			}
		}

		if ( ok ) {
			tp = Transform();
			tp.writeBack( nif, index );
		}
	} else if ( nif->inherits( nif->itemName( index ), "NiTriBasedGeom" ) ) {
		QModelIndex iData;

		if ( nif->itemName( index ) == "NiTriShape" || nif->itemName( index ) == "BSLODTriShape" )
			iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriShapeData" );
		else if ( nif->itemName( index ) == "NiTriStrips" )
			iData = nif->getBlock( nif->getLink( index, "Data" ), "NiTriStripsData" );

		if ( iData.isValid() ) {
			Transform t( nif, index );
			QModelIndex iVertices = nif->getIndex( iData, "Vertices" );

			if ( iVertices.isValid() ) {
				QVector<Vector3> a = nif->getArray<Vector3>( iVertices );

				for ( int v = 0; v < nif->rowCount( iVertices ); v++ )
					a[v] = t * a[v];

				nif->setArray<Vector3>( iVertices, a );

				auto transformBTN = [nif, iData, &t]( QString str )
				{
					QModelIndex idx = nif->getIndex( iData, str );
					if ( idx.isValid() ) {
						auto a = nif->getArray<Vector3>( idx );
						for ( int n = 0; n < a.size(); n++ )
							a[n] = t.rotation * a[n];

						nif->setArray<Vector3>( idx, a );
					}
				};

				if ( !(t.rotation == Matrix()) ) {
					transformBTN( "Normals" );
					transformBTN( "Tangents" );
					transformBTN( "Bitangents" );
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
	} else if ( nif->inherits( nif->itemName( index ), "BSTriShape" ) ) {
		// Should not be used on skinned anyway so do not bother with SSE skinned geometry support
		QModelIndex iVertData = nif->getIndex( index, "Vertex Data" );
		if ( !iVertData.isValid() )
			return index;

		Transform t( nif, index );

		// Update Bounding Sphere
		auto iBound = nif->getIndex( index, "Bounding Sphere" );
		QModelIndex iCenter = nif->getIndex( iBound, "Center" );
		if ( iCenter.isValid() )
			nif->set<Vector3>( iCenter, t * nif->get<Vector3>( iCenter ) );
		
		QModelIndex iRadius = nif->getIndex( iBound, "Radius" );
		if ( iRadius.isValid() )
			nif->set<float>( iRadius, t.scale * nif->get<float>( iRadius ) );

		nif->setState( BaseModel::Processing );
		for ( int i = 0; i < nif->rowCount( iVertData ); i++ ) {
			auto iVert = iVertData.child( i, 0 );

			auto vertex = t * nif->get<Vector3>( iVert, "Vertex" );
			if ( !nif->set<HalfVector3>( iVert, "Vertex", vertex ) )
				nif->set<Vector3>( iVert, "Vertex", vertex );

			// Transform BTN if applicable
			if ( !(t.rotation == Matrix()) ) {
				auto normal = nif->get<ByteVector3>( iVert, "Normal" );
				nif->set<ByteVector3>( iVert, "Normal", t.rotation * normal );

				auto tangent = nif->get<ByteVector3>( iVert, "Tangent" );
				nif->set<ByteVector3>( iVert, "Tangent", t.rotation * tangent );

				// Unpack, Transform, Pack Bitangent
				auto bitX = nif->getValue( nif->getIndex( iVert, "Bitangent X" ) ).toFloat();
				auto bitYi = nif->getValue( nif->getIndex( iVert, "Bitangent Y" ) ).toCount();
				auto bitZi = nif->getValue( nif->getIndex( iVert, "Bitangent Z" ) ).toCount();

				auto bit = t.rotation * Vector3( bitX, (bitYi / 255.0) * 2.0 - 1.0, (bitZi / 255.0) * 2.0 - 1.0 );

				nif->set<float>( iVert, "Bitangent X", bit[0] );
				nif->set<quint8>( iVert, "Bitangent Y", round( ((bit[1] + 1.0) / 2.0) * 255.0 ) );
				nif->set<quint8>( iVert, "Bitangent Z", round( ((bit[2] + 1.0) / 2.0) * 255.0 ) );
			}
		}

		nif->resetState();

		t = Transform();
		t.writeBack( nif, index );
	}

	return index;
}

REGISTER_SPELL( spApplyTransformation )

class spClearTransformation final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Clear" ); }
	QString page() const override final { return Spell::tr( "Transform" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return Transform::canConstruct( nif, index );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		Transform tp;
		tp.writeBack( nif, index );
		return index;
	}
};

REGISTER_SPELL( spClearTransformation )

class spCopyTransformation final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Copy" ); }
	QString page() const override final { return Spell::tr( "Transform" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return Transform::canConstruct( nif, index );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QByteArray data;
		QBuffer buffer( &data );

		if ( buffer.open( QIODevice::WriteOnly ) ) {
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

class spPasteTransformation final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Paste" ); }
	QString page() const override final { return Spell::tr( "Transform" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( Transform::canConstruct( nif, index ) && mime ) {
			for ( const QString& form : mime->formats() ) {
				if ( form == "nifskope/transform" )
					return true;
			}
		}

		return false;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();

		if ( mime ) {
			for ( const QString& form : mime->formats() ) {
				if ( form == "nifskope/transform" ) {
					QByteArray data = mime->data( form );
					QBuffer buffer( &data );

					if ( buffer.open( QIODevice::ReadOnly ) ) {
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

static QIconPtr transform_xpm_icon = nullptr;

class spEditTransformation final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Edit" ); }
	QString page() const override final { return Spell::tr( "Transform" ); }
	bool instant() const override final { return true; }
	QIcon icon() const override final
	{
		if ( !transform_xpm_icon )
			transform_xpm_icon = QIconPtr( new QIcon(QPixmap( transform_xpm )) );

		return *transform_xpm_icon;
	}

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		if ( Transform::canConstruct( nif, index ) )
			return true;

		QModelIndex iTransform = nif->getIndex( index, "Transform" );

		if ( !iTransform.isValid() )
			iTransform = index;

		return ( nif->getValue( iTransform ).type() == NifValue::tMatrix4 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		NifBlockEditor * edit = new NifBlockEditor( nif, nif->getBlock( index ) );

		if ( Transform::canConstruct( nif, index ) ) {
			edit->add( new NifVectorEdit( nif, nif->getIndex( index, "Translation" ) ) );
			edit->add( new NifRotationEdit( nif, nif->getIndex( index, "Rotation" ) ) );
			edit->add( new NifFloatEdit( nif, nif->getIndex( index, "Scale" ) ) );
		} else {
			QModelIndex iTransform = nif->getIndex( index, "Transform" );

			if ( !iTransform.isValid() )
				iTransform = index;

			edit->add( new NifMatrix4Edit( nif, iTransform ) );
		}

		edit->show();
		return index;
	}
};

REGISTER_SPELL( spEditTransformation )


class spScaleVertices final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Scale Vertices" ); }
	QString page() const override final { return Spell::tr( "Transform" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif->inherits( index, "NiGeometry" );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QDialog dlg;

		QGridLayout * grid = new QGridLayout( &dlg );

		QList<QDoubleSpinBox *> scale;

		for ( int a = 0; a < 3; a++ ) {
			QDoubleSpinBox * spn = new QDoubleSpinBox;
			scale << spn;
			spn->setValue( 1.0 );
			spn->setDecimals( 4 );
			spn->setRange( -10e+4, 10e+4 );
			grid->addWidget( new QLabel( (QStringList{ "X", "Y", "Z" }).value( a ) ), a, 0 );
			grid->addWidget( spn, a, 1 );
		}

		QSettings settings;
		QString key = QString( "%1/%2/%3/Scale Normals" ).arg( "Spells", page(), name() );

		QCheckBox * chkNormals = new QCheckBox( Spell::tr( "Scale Normals" ) );

		chkNormals->setChecked( settings.value( key, true ).toBool() );
		grid->addWidget( chkNormals, 3, 1 );

		QPushButton * btScale = new QPushButton( Spell::tr( "Scale" ) );
		grid->addWidget( btScale, 4, 0, 1, 2 );
		QObject::connect( btScale, &QPushButton::clicked, &dlg, &QDialog::accept );

		if ( dlg.exec() != QDialog::Accepted )
			return QModelIndex();

		settings.setValue( key, chkNormals->isChecked() );

		QModelIndex iData = nif->getBlock( nif->getLink( nif->getBlock( index ), "Data" ), "NiGeometryData" );

		QVector<Vector3> vertices = nif->getArray<Vector3>( iData, "Vertices" );
		QMutableVectorIterator<Vector3> it( vertices );

		while ( it.hasNext() ) {
			Vector3 & v = it.next();

			for ( int a = 0; a < 3; a++ )
				v[a] *= scale[a]->value();
		}

		nif->setArray<Vector3>( iData, "Vertices", vertices );

		if ( chkNormals->isChecked() ) {
			QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );
			QMutableVectorIterator<Vector3> it( norms );

			while ( it.hasNext() ) {
				Vector3 & v = it.next();

				for ( int a = 0; a < 3; a++ )
					v[a] *= scale[a]->value();
			}

			nif->setArray<Vector3>( iData, "Normals", norms );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spScaleVertices )


