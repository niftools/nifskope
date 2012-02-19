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



#include "../nifmodel.h"

#include "../NvTriStrip/qtwrapper.h"

#include "../gl/gltex.h"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>
#include <QApplication>

#define tr(x) QApplication::tr(x)

/*
 *  .OBJ EXPORT
 */



static void writeData( const NifModel * nif, const QModelIndex & iData, QTextStream & obj, int ofs[1], Transform t )
{
	// copy vertices
	
	QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
	foreach ( Vector3 v, verts )
	{
		v = t * v;
		obj << "v " << qSetRealNumberPrecision(17) << v[0] << " " << v[1] << " " << v[2] << "\r\n";
	}
	
	// copy texcoords
	
	QModelIndex iUV = nif->getIndex( iData, "UV Sets" );
	if ( ! iUV.isValid() )
		iUV = nif->getIndex( iData, "UV Sets 2" );
	
	QVector<Vector2> texco = nif->getArray<Vector2>( iUV.child( 0, 0 ) );
	foreach( Vector2 t, texco )
		obj << "vt " << t[0] << " " << 1.0 - t[1] << "\r\n";
	
	// copy normals
	
	QVector<Vector3> norms = nif->getArray<Vector3>( iData, "Normals" );
	foreach ( Vector3 n, norms )
	{
		n = t.rotation * n;
		obj << "vn " << n[0] << " " << n[1] << " " << n[2] << "\r\n";
	}
	
	// get the triangles
	
	QVector<Triangle> tris;
	
	QModelIndex iPoints = nif->getIndex( iData, "Points" );
	if ( iPoints.isValid() )
	{
		QList< QVector<quint16> > strips;
		for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
			strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );
		tris = triangulate( strips );
	}
	else
	{
		tris = nif->getArray<Triangle>( iData, "Triangles" );
	}
	
	// write the triangles
	
	foreach ( Triangle t, tris )
	{
		obj << "f";
		for ( int p = 0; p < 3; p++ )
		{
			obj << " " << ofs[0] + t[p];
			if ( norms.count() )
				if ( texco.count() )
					obj << "/" << ofs[1] + t[p] << "/" << ofs[2] + t[p];
				else
					obj << "//" << ofs[2] + t[p];
			else
				if ( texco.count() )
					obj << "/" << ofs[1] + t[p];
		}
		obj << "\r\n";
	}
	
	ofs[0] += verts.count();
	ofs[1] += texco.count();
	ofs[2] += norms.count();
}

static void writeShape( const NifModel * nif, const QModelIndex & iShape, QTextStream & obj, QTextStream & mtl, int ofs[], Transform t )
{
	QString name = nif->get<QString>( iShape, "Name" );
	QString matn = name, map_Kd, map_Ks, map_Ns, map_d ,disp, decal, bump;
	
	Color3 mata, matd, mats;
	float matt = 1.0, matg = 33.0;
	
	foreach ( qint32 link, nif->getChildLinks( nif->getBlockNumber( iShape ) ) )
	{
		QModelIndex iProp = nif->getBlock( link );
		if ( nif->isNiBlock( iProp, "NiMaterialProperty" ) )
		{
			mata = nif->get<Color3>( iProp, "Ambient Color" );
			matd = nif->get<Color3>( iProp, "Diffuse Color" );
			mats = nif->get<Color3>( iProp, "Specular Color" );
			matt = nif->get<float>( iProp, "Alpha" );
			matg = nif->get<float>( iProp, "Glossiness" );
			//matn = nif->get<QString>( iProp, "Name" );
		}
		else if ( nif->isNiBlock( iProp, "NiTexturingProperty" ) )
		{
			QModelIndex iBase = nif->getBlock( nif->getLink( nif->getIndex( iProp, "Base Texture" ), "Source" ), "NiSourceTexture" );
			map_Kd = TexCache::find( nif->get<QString>( iBase, "File Name" ), nif->getFolder() );

			QModelIndex iDark = nif->getBlock( nif->getLink( nif->getIndex( iProp, "Decal Texture 1" ), "Source" ), "NiSourceTexture" );
			decal = TexCache::find( nif->get<QString>( iDark, "File Name" ), nif->getFolder() );

			QModelIndex iBump = nif->getBlock( nif->getLink( nif->getIndex( iProp, "Bump Map Texture" ), "Source" ), "NiSourceTexture" );
			bump = TexCache::find( nif->get<QString>( iBump, "File Name" ), nif->getFolder() );
		}
		else if ( nif->isNiBlock( iProp, "NiTextureProperty" ) )
		{
			QModelIndex iSource = nif->getBlock( nif->getLink( iProp, "Image" ), "NiImage" );
			map_Kd = TexCache::find( nif->get<QString>( iSource, "File Name" ), nif->getFolder() );
		}
		else if ( nif->isNiBlock( iProp, "NiSkinInstance" ) )
		{
			QMessageBox::warning(
				0,
				"OBJ Export Warning",
				QString("The shape ") + name + QString(" is skinned, but the "
				"obj format does not support skinning. This mesh will be "
				"exported statically in its bind pose, without skin weights.")
			);
		}
		else if ( nif->isNiBlock( iProp, "BSShaderNoLightingProperty" ) 
			    || nif->isNiBlock( iProp, "SkyShaderProperty" ) 
				 || nif->isNiBlock( iProp, "TileShaderProperty" ) 
			     )
		{
			map_Kd = TexCache::find( nif->get<QString>( iProp, "File Name" ), nif->getFolder() );
		}
		else if ( nif->isNiBlock( iProp, "BSShaderPPLightingProperty" ) 
			    || nif->isNiBlock( iProp, "Lighting30ShaderProperty" ) 
			     )
		{
			QModelIndex iArray = nif->getIndex( nif->getBlock( nif->getLink( iProp, "Texture Set" ) ) , "Textures");
			map_Kd = TexCache::find( nif->get<QString>( iArray.child( 0, 0 ) ), nif->getFolder() );
		}
	}
	
	//if ( ! texfn.isEmpty() )
	//	matn += ":" + texfn;
	
	matn = QString( "Material.%1" ).arg( ofs[0], 6, 16, QChar( '0' ) );
	
	mtl << "\r\n";
	mtl << "newmtl " << matn << "\r\n";
	mtl << "Ka " << mata[0] << " "  << mata[1] << " "  << mata[2] << "\r\n";
	mtl << "Kd " << matd[0] << " "  << matd[1] << " "  << matd[2] << "\r\n";
	mtl << "Ks " << mats[0] << " "  << mats[1] << " " << mats[2] << "\r\n";
	mtl << "d " << matt << "\r\n";
	mtl << "Ns " << matg << "\r\n";
	if ( ! map_Kd.isEmpty() )
		mtl << "map_Kd " << map_Kd << "\r\n\r\n";
	if ( ! decal.isEmpty() )
		mtl << "decal " << decal << "\r\n\r\n";
	if ( ! bump.isEmpty() )
		mtl << "bump " << decal << "\r\n\r\n";
	
	obj << "\r\n# " << name << "\r\n\r\ng " << name << "\r\n" << "usemtl " << matn << "\r\n\r\n";
	
	writeData( nif, nif->getBlock( nif->getLink( iShape, "Data" ) ), obj, ofs, t );
}

static void writeParent( const NifModel * nif, const QModelIndex & iNode, QTextStream & obj, QTextStream & mtl, int ofs[], Transform t )
{
	t = t * Transform( nif, iNode );
	foreach ( int l, nif->getChildLinks( nif->getBlockNumber( iNode ) ) )
	{
		QModelIndex iChild = nif->getBlock( l );
		if ( nif->inherits( iChild, "NiNode" ) )
			writeParent( nif, iChild, obj, mtl, ofs, t );
		else if ( nif->isNiBlock( iChild, "NiTriShape" ) || nif->isNiBlock( iChild, "NiTriStrips" ) )
			writeShape( nif, iChild, obj, mtl, ofs, t * Transform( nif, iChild ) );
		else if ( nif->inherits( iChild, "NiCollisionObject" ) )
		{
			QModelIndex iBody = nif->getBlock( nif->getLink( iChild, "Body" ) );
			if ( iBody.isValid() )
			{
				Transform bt;
				bt.scale = 7;
				if ( nif->isNiBlock( iBody, "bhkRigidBodyT" ) )
				{
					bt.rotation.fromQuat( nif->get<Quat>( iBody, "Rotation" ) );
					bt.translation = Vector3( nif->get<Vector4>( iBody, "Translation" ) * 7 );
				}
				QModelIndex iShape = nif->getBlock( nif->getLink( iBody, "Shape" ) );
				if ( nif->isNiBlock( iShape, "bhkMoppBvTreeShape" ) )
				{
					iShape = nif->getBlock( nif->getLink( iShape, "Shape" ) );
					if ( nif->isNiBlock( iShape, "bhkPackedNiTriStripsShape" ) )
					{
						QModelIndex iData = nif->getBlock( nif->getLink( iShape, "Data" ) );
						if ( nif->isNiBlock( iData, "hkPackedNiTriStripsData" ) )
						{
							bt = t * bt;
							obj << "\r\n# bhkPackedNiTriStripsShape\r\n\r\ng collision\r\n" << "usemtl collision\r\n\r\n";
							QVector<Vector3> verts = nif->getArray<Vector3>( iData, "Vertices" );
							foreach ( Vector3 v, verts )
							{
								v = bt * v;
								obj << "v " << v[0] << " " << v[1] << " " << v[2] << "\r\n";
							}
							
							QModelIndex iTris = nif->getIndex( iData, "Triangles" );
							for ( int t = 0; t < nif->rowCount( iTris ); t++ )
							{
								Triangle tri = nif->get<Triangle>( iTris.child( t, 0 ), "Triangle" );
								Vector3 n = nif->get<Vector3>( iTris.child( t, 0 ), "Normal" );
								
								Vector3 a = verts.value( tri[0] );
								Vector3 b = verts.value( tri[1] );
								Vector3 c = verts.value( tri[2] );
								
								Vector3 fn = Vector3::crossproduct( b - a, c - a );
								fn.normalize();
								
								bool flip = Vector3::dotproduct( n, fn ) < 0;
								
								obj << "f"
									<< " " << tri[0] + ofs[0]
									<< " " << tri[ flip ? 2 : 1 ] + ofs[0]
									<< " " << tri[ flip ? 1 : 2 ] + ofs[0]
									<< "\r\n";
							}
							ofs[0] += verts.count();
						}
					}
				}
				else if ( nif->isNiBlock( iShape, "bhkNiTriStripsShape" ) )
				{
					bt.scale = 1;
					obj << "\r\n# bhkNiTriStripsShape\r\n\r\ng collision\r\n" << "usemtl collision\r\n\r\n";
					QModelIndex iStrips = nif->getIndex( iShape, "Strips Data" );
					for ( int r = 0; r < nif->rowCount( iStrips ); r++ )
						writeData( nif, nif->getBlock( nif->getLink( iStrips.child( r, 0 ) ), "NiTriStripsData" ), obj, ofs, t * bt );
				}
			}
		}
	}
}

void exportObj( const NifModel * nif, const QModelIndex & index )
{
	//--Determine how the file will export, and be sure the user wants to continue--//
	QList<int> roots;
	QModelIndex iBlock = nif->getBlock( index );

	QString question;
	if ( iBlock.isValid() )
	{
		roots.append( nif->getBlockNumber(index) );
		if ( nif->itemName(index) == "NiNode" )
		{
			question = tr("NiNode selected.  All children of selected node will be exported.");
		} else if ( nif->itemName(index) == "NiTriShape" || nif->itemName(index) == "NiTriStrips" )
		{
			question = nif->itemName(index) + tr(" selected.  Selected mesh will be exported.");
		}
	}
	
	if ( question.size() == 0 )
	{
		question = tr("No NiNode, NiTriShape,or NiTriStrips is selected.  Entire scene will be exported.");
		roots = nif->getRootLinks();
	}

	int result = QMessageBox::question( 0, tr("Export OBJ"), question, QMessageBox::Ok, QMessageBox::Cancel );
	if ( result == QMessageBox::Cancel ) {
		return;
	}

	//--Allow the user to select the file--//

	QSettings settings;
	settings.beginGroup( "import-export" );
	settings.beginGroup( "obj" );

	QString fname = QFileDialog::getSaveFileName( 0, tr("Choose a .OBJ file for export"), settings.value( "File Name" ).toString(), "*.obj" );
	if ( fname.isEmpty() )
		return;
	
	while ( fname.endsWith( ".obj", Qt::CaseInsensitive ) )
		fname = fname.left( fname.length() - 4 );
	
	QFile fobj( fname + ".obj" );
	if ( ! fobj.open( QIODevice::WriteOnly ) )
	{
		qWarning() << "could not open " << fobj.fileName() << " for write access";
		return;
	}
	
	QFile fmtl( fname + ".mtl" );
	if ( ! fmtl.open( QIODevice::WriteOnly ) )
	{
		qWarning() << "could not open " << fmtl.fileName() << " for write access";
		return;
	}
	
	fname = fmtl.fileName();
	int i = fname.lastIndexOf( "/" );
	if ( i >= 0 )
		fname = fname.remove( 0, i+1 );
	
	QTextStream sobj( &fobj );
	QTextStream smtl( &fmtl );
	
	sobj << "# exported with NifSkope\r\n\r\n" << "mtllib " << fname << "\r\n";

	//--Translate NIF structure into file structure --//

	int ofs[3] = { 1, 1, 1 };
	foreach ( int l, roots )
	{
		QModelIndex iBlock = nif->getBlock( l );
		if ( nif->inherits( iBlock, "NiNode" ) )
			writeParent( nif, iBlock, sobj, smtl, ofs, Transform() );
		else if ( nif->isNiBlock( iBlock, "NiTriShape" ) || nif->isNiBlock( iBlock, "NiTriStrips" ) )
			writeShape( nif, iBlock, sobj, smtl, ofs, Transform() );
	}
	
	settings.setValue( "File Name", fobj.fileName() );
}



/*
 *  .OBJ IMPORT
 */


struct ObjPoint
{
	int v, t, n;
	
	bool operator==( const ObjPoint & other ) const
	{
		return v == other.v && t == other.t && n == other.n;
	}
};

struct ObjFace
{
	ObjPoint p[3];
};

struct ObjMaterial
{
	Color3 Ka, Kd, Ks;
	float d, Ns;
	QString map_Kd;
	
	ObjMaterial() : d( 1.0 ), Ns( 31.0 ) {}
};

static void readMtlLib( const QString & fname, QMap< QString, ObjMaterial > & omaterials )
{
	QFile file( fname );
	if ( ! file.open( QIODevice::ReadOnly ) )
	{
		qWarning() << "failed to open" << fname;
		return;
	}
	
	QTextStream smtl( &file );
	
	QString mtlid;
	ObjMaterial mtl;
	
	while ( ! smtl.atEnd() )
	{
		QString line = smtl.readLine();
		
		QStringList t = line.split( " ", QString::SkipEmptyParts );
		
		if ( t.value( 0 ) == "newmtl" )
		{
			if ( ! mtlid.isEmpty() )
				omaterials.insert( mtlid, mtl );
			mtlid = t.value( 1 );
			mtl = ObjMaterial();
		}
		else if ( t.value( 0 ) == "Ka" )
		{
			mtl.Ka = Color3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() );
		}
		else if ( t.value( 0 ) == "Kd" )
		{
			mtl.Kd = Color3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() );
		}
		else if ( t.value( 0 ) == "Ks" )
		{
			mtl.Ks = Color3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() );
		}
		else if ( t.value( 0 ) == "d" )
		{
			mtl.d = t.value( 1 ).toDouble();
		}
		else if ( t.value( 0 ) == "Ns" )
		{
			mtl.Ns = t.value( 1 ).toDouble();
		}
		else if ( t.value( 0 ) == "map_Kd" )
		{
			// handle spaces in filenames
			mtl.map_Kd = t.value( 1 );
			for (int i = 2; i < t.size (); i++)
				mtl.map_Kd += " " + t.value( i );
		}
	}
	if ( ! mtlid.isEmpty() )
		omaterials.insert( mtlid, mtl );
}

static void addLink( NifModel * nif, QModelIndex iBlock, QString name, qint32 link )
{
	QModelIndex iArray = nif->getIndex( iBlock, name );
	QModelIndex iSize = nif->getIndex( iBlock, QString( "Num %1" ).arg( name ) );
	int numIndices = nif->get<int>( iSize );
	nif->set<int>( iSize, numIndices + 1 );
	nif->updateArray( iArray );
	nif->setLink( iArray.child( numIndices, 0 ), link );
}

void importObj( NifModel * nif, const QModelIndex & index )
{
	//--Determine how the file will import, and be sure the user wants to continue--//

	// If no existing node is selected, create a group node.  Otherwise use selected node
	QPersistentModelIndex iNode, iShape, iMaterial, iData, iTexProp, iTexSource;
	QModelIndex iBlock = nif->getBlock( index );
	bool cBSShaderPPLightingProperty = false;
	//Be sure the user hasn't clicked on a NiTriStrips object
	if ( iBlock.isValid() && nif->itemName(iBlock) == "NiTriStrips" )
	{
                QMessageBox::information( 0, tr("Import OBJ"), tr("You cannot import an OBJ file over a NiTriStrips object.  Please convert it to a NiTriShape object first by right-clicking and choosing Mesh > Triangulate") );
		return;
	}

	if ( iBlock.isValid() && nif->itemName(iBlock) == "NiNode" )
	{
		iNode = iBlock;
	}
	else if ( iBlock.isValid() && nif->itemName( iBlock ) == "NiTriShape" )
	{
		iShape = iBlock;
		//Find parent of NiTriShape
		int par_num = nif->getParent( nif->getBlockNumber( iBlock ) );
		if ( par_num != -1 )
		{
			iNode = nif->getBlock( par_num );
		}

		//Find material, texture, and data objects
		QList<int> children = nif->getChildLinks( nif->getBlockNumber(iShape) );
		for( QList<int>::iterator it = children.begin(); it != children.end(); ++it )
		{
			if ( *it != -1 )
			{
				QModelIndex temp = nif->getBlock( *it );
				QString type = nif->itemName( temp );
				if ( type == "BSShaderPPLightingProperty" )
				{
					cBSShaderPPLightingProperty = true;
				}
				if ( type == "NiMaterialProperty" )
				{
					iMaterial = temp;
				}
				else if ( type == "NiTriShapeData" )
				{
					iData = temp;
				}
				else if ( (type == "NiTexturingProperty") || (type == "NiTextureProperty") )
				{
					iTexProp = temp;

					//Search children of texture property for texture sources/images
					QList<int> children = nif->getChildLinks( nif->getBlockNumber(iTexProp) );
					for( QList<int>::iterator it = children.begin(); it != children.end(); ++it )
					{
						QModelIndex temp = nif->getBlock( *it );
						QString type = nif->itemName( temp );
						if ( (type == "NiSourceTexture") || (type == "NiImage") )
						{
							iTexSource = temp;
						}
					}
				}
			}
		}
	}

	QString question;
	if ( iNode.isValid() == true )
	{
		if ( iShape.isValid() == true )
		{
			question = tr("NiTriShape selected.  The first imported mesh will replace the selected one.");
		}
		else
		{
			question = tr("NiNode selected.  Meshes will be attached to the selected node.");
		}
	}
	else
	{
		question = tr("No NiNode or NiTriShape selected.  Meshes will be imported to the root of the file.");
	}

	int result = QMessageBox::question( 0, tr("Import OBJ"), question, QMessageBox::Ok, QMessageBox::Cancel );
	if ( result == QMessageBox::Cancel ) {
		return;
	}

	//--Read the file--//

	QSettings settings;
	settings.beginGroup( "import-export" );
	settings.beginGroup( "obj" );
	
	QString fname = QFileDialog::getOpenFileName( 0, tr("Choose a .OBJ file to import"), settings.value( "File Name" ).toString(), "*.obj" );
	if ( fname.isEmpty() )
		return;
	
	QFile fobj( fname );
	if ( ! fobj.open( QIODevice::ReadOnly ) )
	{
		qWarning() << tr("could not open ") << fobj.fileName() << tr(" for read access");
		return;
	}
	
	QTextStream sobj( & fobj );
	
	QVector<Vector3> overts;
	QVector<Vector3> onorms;
	QVector<Vector2> otexco;
	QMap< QString, QVector<ObjFace> * > ofaces;
	QMap< QString, ObjMaterial > omaterials;
	
	QVector<ObjFace> * mfaces = new QVector<ObjFace>();
	
	QString usemtl = "None";
	ofaces.insert( usemtl, mfaces );
	
	while ( ! sobj.atEnd() )
	{	// parse each line of the file
		QString line = sobj.readLine();
		
		QStringList t = line.split( " ", QString::SkipEmptyParts );
		
		if ( t.value( 0 ) == "mtllib" )
		{
			readMtlLib( fname.left( qMax( fname.lastIndexOf( "/" ), fname.lastIndexOf( "\\" ) ) + 1 ) + t.value( 1 ), omaterials );
		}
		else if ( t.value( 0 ) == "usemtl" )
		{
			usemtl = t.value( 1 );
			//if ( usemtl.contains( "_" ) )
			//	usemtl = usemtl.left( usemtl.indexOf( "_" ) );
			
			mfaces = ofaces.value( usemtl );
			if ( ! mfaces )
			{
				mfaces = new QVector<ObjFace>();
				ofaces.insert( usemtl, mfaces );
			}
		}
		else if ( t.value( 0 ) == "v" )
		{
			overts.append( Vector3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() ) );
		}
		else if ( t.value( 0 ) == "vt" )
		{
			otexco.append( Vector2( t.value( 1 ).toDouble(), 1.0 - t.value( 2 ).toDouble() ) );
		}
		else if ( t.value( 0 ) == "vn" )
		{
			onorms.append( Vector3( t.value( 1 ).toDouble(), t.value( 2 ).toDouble(), t.value( 3 ).toDouble() ) );
		}
		else if ( t.value( 0 ) == "f" )
		{
			if ( t.count() > 5 )
			{
				qWarning() << "please triangulate your mesh before import";
				return;
			}
			
			for ( int j = 1; j < t.count() - 2; j++ )
			{
				ObjFace face;
				for ( int i = 0; i < 3; i++ )
				{
					QStringList lst = t.value( i == 0 ? 1 : j+i ).split( "/" );
					
					int v = lst.value( 0 ).toInt();
					if ( v < 0 ) v += overts.count();	else v--;
					
					int t = lst.value( 1 ).toInt();
					if ( t < 0 ) v += otexco.count(); else t--;
					
					int n = lst.value( 2 ).toInt();
					if ( n < 0 ) n += onorms.count(); else n--;
					
					face.p[i].v = v;
					face.p[i].t = t;
					face.p[i].n = n;
				}
				mfaces->append( face );
			}
		}
	}

	//--Translate file structures into NIF ones--//

	if ( iNode.isValid() == false )
	{
		iNode = nif->insertNiBlock( "NiNode" );
		nif->set<QString>( iNode, "Name", "Scene Root" );
	}

	// create a NiTriShape foreach material in the object
	int shapecount = 0;
	bool first_tri_shape = true;
	QMapIterator< QString, QVector<ObjFace> * > it( ofaces );
	while ( it.hasNext() )
	{
		it.next();
		
		if ( ! it.value()->count() )
			continue;
		
		if ( it.key() != "collision" )
		{
			//If we are on the first shape, and one was selected in the 3D view, use the existing one
			bool newiShape = false;
			if ( iShape.isValid() == false || first_tri_shape == false )
			{
				iShape = nif->insertNiBlock( "NiTriShape" );
				newiShape = true;
			}

			if (newiShape)// don't change a name what already exists; // don't add duplicates
			{
				nif->set<QString>( iShape, "Name", QString( "%1:%2" ).arg( nif->get<QString>( iNode, "Name" ) ).arg( shapecount++ ) );
				addLink( nif, iNode, "Children", nif->getBlockNumber( iShape ) );
			}
			
			if ( !omaterials.contains( it.key() ) )
				qWarning() << "material" << it.key() << "not found in mtllib";
			
			ObjMaterial mtl = omaterials.value( it.key() );
			
			bool newiMaterial = false;
			if ( iMaterial.isValid() == false || first_tri_shape == false )
			{
				iMaterial = nif->insertNiBlock( "NiMaterialProperty" );
				newiMaterial = true;
			}
			if (newiMaterial)// don't affect a property  that is already there - that name is generated above on export and it has nothign to do with the stored name
				nif->set<QString>( iMaterial, "Name", it.key() );
			nif->set<Color3>( iMaterial, "Ambient Color", mtl.Ka );
			nif->set<Color3>( iMaterial, "Diffuse Color", mtl.Kd );
			nif->set<Color3>( iMaterial, "Specular Color", mtl.Ks );
			if (newiMaterial)// don't affect a property  that is already there
				nif->set<Color3>( iMaterial, "Emissive Color", Color3( 0, 0, 0 ) );
			nif->set<float>( iMaterial, "Alpha", mtl.d );
			nif->set<float>( iMaterial, "Glossiness", mtl.Ns );

			if (newiMaterial)// don't add property that is already there
				addLink( nif, iShape, "Properties", nif->getBlockNumber( iMaterial ) );
			
			if ( ! mtl.map_Kd.isEmpty() )
			{
				if ( nif->getVersionNumber() >= 0x0303000D )
				{
					//Newer versions use NiTexturingProperty and NiSourceTexture
					if ( iTexProp.isValid() == false || first_tri_shape == false || nif->itemType(iTexProp) != "NiTexturingProperty" )
					{
						if (!cBSShaderPPLightingProperty) // no need of NiTexturingProperty when BSShaderPPLightingProperty is present
							iTexProp = nif->insertNiBlock( "NiTexturingProperty" );
					}
					QModelIndex iBaseMap;
					if (!cBSShaderPPLightingProperty)
					{// no need of NiTexturingProperty when BSShaderPPLightingProperty is present
						addLink( nif, iShape, "Properties", nif->getBlockNumber( iTexProp ) );

						nif->set<int>( iTexProp, "Has Base Texture", 1 );
						iBaseMap = nif->getIndex( iTexProp, "Base Texture" );
						nif->set<int>( iBaseMap, "Clamp Mode", 3 );
						nif->set<int>( iBaseMap, "Filter Mode", 2 );
					}
					
					if ( iTexSource.isValid() == false || first_tri_shape == false || nif->itemType(iTexSource) != "NiSourceTexture" )
					{
						if (!cBSShaderPPLightingProperty)
							iTexSource = nif->insertNiBlock( "NiSourceTexture" );
					}
					if (!cBSShaderPPLightingProperty)// no need of NiTexturingProperty when BSShaderPPLightingProperty is present
						nif->setLink( iBaseMap, "Source", nif->getBlockNumber( iTexSource ) );
					
					if (!cBSShaderPPLightingProperty)
					{// no need of NiTexturingProperty when BSShaderPPLightingProperty is present
						nif->set<int>( iTexSource, "Pixel Layout", nif->getVersion() == "20.0.0.5" ? 6 : 5 );
						nif->set<int>( iTexSource, "Use Mipmaps", 2 );
						nif->set<int>( iTexSource, "Alpha Format", 3 );
						nif->set<int>( iTexSource, "Unknown Byte", 1 );
						nif->set<int>( iTexSource, "Unknown Byte 2", 1 );
					
						nif->set<int>( iTexSource, "Use External", 1 );
						nif->set<QString>( iTexSource, "File Name", TexCache::stripPath( mtl.map_Kd, nif->getFolder() ) );
					}
				} else {
					//Older versions use NiTextureProperty and NiImage
					if ( iTexProp.isValid() == false || first_tri_shape == false || nif->itemType(iTexProp) != "NiTextureProperty" )
					{
						iTexProp = nif->insertNiBlock( "NiTextureProperty" );
					}
					addLink( nif, iShape, "Properties", nif->getBlockNumber( iTexProp ) );
					
					if ( iTexSource.isValid() == false || first_tri_shape == false || nif->itemType(iTexSource) != "NiImage" )
					{
						iTexSource = nif->insertNiBlock( "NiImage" );
					}

					nif->setLink( iTexProp, "Image", nif->getBlockNumber( iTexSource ) );
					
					nif->set<int>( iTexSource, "External", 1 );
					nif->set<QString>( iTexSource, "File Name", TexCache::stripPath( mtl.map_Kd, nif->getFolder() ) );
				}
			}
			
			if ( iData.isValid() == false || first_tri_shape == false )
			{
				iData = nif->insertNiBlock( "NiTriShapeData" );
			}
			nif->setLink( iShape, "Data", nif->getBlockNumber( iData ) );
			
			QVector<Vector3> verts;
			QVector<Vector3> norms;
			QVector<Vector2> texco;
			QVector<Triangle> triangles;
			
			QVector<ObjPoint> points;
			
			foreach ( ObjFace oface, *(it.value()) )
			{
				Triangle tri;
				
				for ( int t = 0; t < 3; t++ )
				{
					ObjPoint p = oface.p[t];
					int ix;
					for ( ix = 0; ix < points.count(); ix++ )
					{
						if ( points[ix] == p )
							break;
					}
					if ( ix == points.count() )
					{
						points.append( p );
						verts.append( overts.value( p.v ) );
						norms.append( onorms.value( p.n ) );
						texco.append( otexco.value( p.t ) );
					}
					tri[t] = ix;
				}
				
				triangles.append( tri );
			}
			
			nif->set<int>( iData, "Num Vertices", verts.count() );
			nif->set<int>( iData, "Has Vertices", 1 );
			nif->updateArray( iData, "Vertices" );
			nif->setArray<Vector3>( iData, "Vertices", verts );
			nif->set<int>( iData, "Has Normals", 1 );
			nif->updateArray( iData, "Normals" );
			nif->setArray<Vector3>( iData, "Normals", norms );
			nif->set<int>( iData, "Has UV", 1 );
			int cNumUVSets = nif->get<int>( iData, "Num UV Sets");// keep things the way they are
			nif->set<int>( iData, "Num UV Sets", 1 | cNumUVSets );// keep things the way they are
			nif->set<int>( iData, "Num UV Sets 2", 1 | cNumUVSets );// keep things the way they are
			QModelIndex iTexCo = nif->getIndex( iData, "UV Sets" );
			if ( ! iTexCo.isValid() )
				iTexCo = nif->getIndex( iData, "UV Sets 2" );
			nif->updateArray( iTexCo );
			nif->updateArray( iTexCo.child( 0, 0 ) );
			nif->setArray<Vector2>( iTexCo.child( 0, 0 ), texco );
			
			nif->set<int>( iData, "Has Triangles", 1 );
			nif->set<int>( iData, "Num Triangles", triangles.count() );
			nif->set<int>( iData, "Num Triangle Points", triangles.count() * 3 );
			nif->updateArray( iData, "Triangles" );
			nif->setArray<Triangle>( iData, "Triangles", triangles );
			
			// "find me a center": see nif.xml for details
			// TODO: extract to a method somewhere...
			Vector3 center;
			if ( verts.count() > 0 )
			{
				Vector3 min, max;
				min[0] = verts[0][0];
				min[1] = verts[0][1];
				min[2] = verts[0][2];
				max[0] = min[0];
				max[1] = min[1];
				max[2] = min[2];
				foreach ( Vector3 v, verts )
				{
					if (v[0] < min[0]) min[0] = v[0];
					if (v[1] < min[1]) min[1] = v[1];
					if (v[2] < min[2]) min[2] = v[2];
					if (v[0] > max[0]) max[0] = v[0];
					if (v[1] > max[1]) max[1] = v[1];
					if (v[2] > max[2]) max[2] = v[2];
				}
				center[0] = min[0] + ((max[0] - min[0])/2);
				center[1] = min[1] + ((max[1] - min[1])/2);
				center[2] = min[2] + ((max[2] - min[2])/2);
			}
			nif->set<Vector3>( iData, "Center", center );
			float radius = 0;
			foreach ( Vector3 v, verts )
			{
				float d = ( center - v ).length();
				if ( d > radius ) radius = d;
			}
			nif->set<float>( iData, "Radius", radius );
			
			nif->set<int>( iData, "Unknown Short 2", 0x4000 );
		}
		else if ( nif->getVersionNumber() == 0x14000005 )
		{
			// create experimental havok collision mesh
			QVector<Vector3> verts;
			QVector<Vector3> norms;
			QVector<Triangle> triangles;
			
			QVector<ObjPoint> points;
			
			foreach ( ObjFace oface, *(it.value()) )
			{
				Triangle tri;
				
				for ( int t = 0; t < 3; t++ )
				{
					ObjPoint p = oface.p[t];
					int ix;
					for ( ix = 0; ix < points.count(); ix++ )
					{
						if ( points[ix] == p )
							break;
					}
					if ( ix == points.count() )
					{
						points.append( p );
						verts.append( overts.value( p.v ) );
						norms.append( onorms.value( p.n ) );
					}
					tri[t] = ix;
				}
				
				triangles.append( tri );
			}
			
			QPersistentModelIndex iData = nif->insertNiBlock( "NiTriStripsData" );
			
			nif->set<int>( iData, "Num Vertices", verts.count() );
			nif->set<int>( iData, "Has Vertices", 1 );
			nif->updateArray( iData, "Vertices" );
			nif->setArray<Vector3>( iData, "Vertices", verts );
			nif->set<int>( iData, "Has Normals", 1 );
			nif->updateArray( iData, "Normals" );
			nif->setArray<Vector3>( iData, "Normals", norms );
			
			Vector3 center;
			foreach ( Vector3 v, verts )
				center += v;
			if ( verts.count() > 0 ) center /= verts.count();
			nif->set<Vector3>( iData, "Center", center );
			float radius = 0;
			foreach ( Vector3 v, verts )
			{
				float d = ( center - v ).length();
				if ( d > radius ) radius = d;
			}
			nif->set<float>( iData, "Radius", radius );
			
			// do not stitch, because it looks better in the cs
			QList< QVector< quint16 > > strips = stripify( triangles, false );
			
			nif->set<int>( iData, "Num Strips", strips.count() );
			nif->set<int>( iData, "Has Points", 1 );
			
			QModelIndex iLengths = nif->getIndex( iData, "Strip Lengths" );
			QModelIndex iPoints = nif->getIndex( iData, "Points" );
			
			if ( iLengths.isValid() && iPoints.isValid() )
			{
				nif->updateArray( iLengths );
				nif->updateArray( iPoints );
				int x = 0;
				int z = 0;
				foreach ( QVector<quint16> strip, strips )
				{
					nif->set<int>( iLengths.child( x, 0 ), strip.count() );
					QModelIndex iStrip = iPoints.child( x, 0 );
					nif->updateArray( iStrip );
					nif->setArray<quint16>( iStrip, strip );
					x++;
					z += strip.count() - 2;
				}
				nif->set<int>( iData, "Num Triangles", z );
			}
			
			QPersistentModelIndex iShape = nif->insertNiBlock( "bhkNiTriStripsShape" );
			
			nif->setArray<float>( iShape, "Unknown Floats 1", QVector<float>() << 0.1f << 0.0f );
			nif->setArray<int>( iShape, "Unknown Ints 1",  QVector<int>() << 0 << 0 << 0 << 0 << 1 );
			nif->set<Vector3>( iShape, "Scale", Vector3( 1.0, 1.0, 1.0 ) );
			addLink( nif, iShape, "Strips Data", nif->getBlockNumber( iData ) );
			nif->set<int>( iShape, "Num Data Layers", 1 );
			nif->updateArray( iShape, "Data Layers" );
			nif->setArray<int>( iShape, "Data Layers", QVector<int>() << 1 );
			
			QPersistentModelIndex iBody = nif->insertNiBlock( "bhkRigidBody" );
			nif->setLink( iBody, "Shape", nif->getBlockNumber( iShape ) );
			
			QPersistentModelIndex iObject = nif->insertNiBlock( "bhkCollisionObject" );
			nif->setLink( iObject, "Parent", nif->getBlockNumber( iNode ) );
			nif->set<int>( iObject, "Unknown Short", 1 );
			nif->setLink( iObject, "Body", nif->getBlockNumber( iBody ) );
			
			nif->setLink( iNode, "Collision Object", nif->getBlockNumber( iObject ) );
		}

		//Finished with the first shape which is the only one that can import over the top of existing data
		first_tri_shape = false;
	}
	
	qDeleteAll( ofaces );
	
	settings.setValue( "File Name", fname );
	
	nif->reset();
}

