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

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>
#include <QApplication>
#include <QDomDocument>
#include <QDateTime>

#include "../nifmodel.h"

#include "../NvTriStrip/qtwrapper.h"

#include "../gl/gltex.h"

#define tr(x) QApplication::tr(x)

/**
 * TODO LIST:
 * - NiTriStrips to attachNiShape
 * - handle NiTriShapeData better way
 * - build own functions for translation/rotate/scale as used multiple times or try to use <matrix> instead
 * - multiple UV mapping to Collada Node (now only one added)
 * - build regexp for xml id and names
 * DONE:
 * + models and texture works, tested multiple dae supported software (except rotation)
 * + added NifLODNode as "NifNode"
 * + Now using matrix to translate/rotate/size, still rotate is not exactly in place in multi layer (node) models.
 * SCHEMA TESTING:
 * xmllint --noout --schema http://www.khronos.org/files/collada_schema_1_4_1.xsd ~/file.dae
 *
 */

// "globals"
QDomDocument doc("");
QDomElement libraryImages = doc.createElement("library_images");
QDomElement libraryMaterials = doc.createElement("library_materials");
QDomElement libraryEffects = doc.createElement("library_effects");
QDomElement libraryGeometries = doc.createElement("library_geometries");

QDomElement textElement(QString type,QString text) {
	QDomElement source = doc.createElement(type);
	source.appendChild(doc.createTextNode(text));
	return source;
}

QDomElement textSidElement(QString type,QString text) {
	QDomElement source = doc.createElement(type);
	source.setAttribute("sid",type);
	source.appendChild(doc.createTextNode(text));
	return source;
}

QDomElement dateElement(QString type,QDateTime time) {
	QDomElement source = doc.createElement(type);
	source.appendChild(doc.createTextNode(time.toString(Qt::ISODate)));
	return source;
}

/**
 *  create UV map array with (nif)index and row (multiple UV for one mesh)
 * @param verts array of polygon positions (X;Y;Z)
 * @param idx nif index
 * @param row UV row number
 * @return QDomElement
 */
QDomElement uvMapElement(QVector<Vector2> uvMap,int idx,int row) {
	QDomElement source = doc.createElement("source");
	source.setAttribute("id",QString("nifid_%1-lib-UV%2").arg(idx).arg(row));
	QDomElement float_array = doc.createElement("float_array");
	float_array.setAttribute("id",QString("nifid_%1-lib-UV%2-array").arg(idx).arg(row));
	float_array.setAttribute("count", (uvMap.size()*2) );
	source.appendChild(float_array);
	QString uvText("\n");
	foreach ( Vector2 v, uvMap )
		uvText.append(QString("%1 %2\n").arg(v[0]).arg(v[1]));
	float_array.appendChild( doc.createTextNode(uvText) );
	QDomElement technique_common = doc.createElement("technique_common");
	source.appendChild(technique_common);
	QDomElement accessor = doc.createElement("accessor");
	accessor.setAttribute("source",QString("#nifid_%1-lib-UV%2-array").arg(idx).arg(row));
	accessor.setAttribute("count",uvMap.size());
	accessor.setAttribute("stride","2");
	technique_common.appendChild(accessor);
	QDomElement param = doc.createElement("param");
	param.setAttribute("name","S");
	param.setAttribute("type","float");
	accessor.appendChild(param);
	param = doc.createElement("param");
	param.setAttribute("name","T");
	param.setAttribute("type","float");
	accessor.appendChild(param);
	return source;
}

/**
 *  create positions array with (nif)index
 * @param verts array of polygon positions (X;Y;Z)
 * @param idx nif index
 * @return QDomElement
 */
QDomElement positionsElement(QVector<Vector3> verts,int idx) {
	QDomElement source = doc.createElement("source");
	source.setAttribute("id",QString("nifid_%1-lib-Position").arg(idx));
	QDomElement float_array = doc.createElement("float_array");
	float_array.setAttribute("id",QString("nifid_%1-lib-Position-array").arg(idx));
	float_array.setAttribute("count", (verts.size()*3) );
	source.appendChild(float_array);
	QString posText("\n");
	foreach ( Vector3 v, verts )
		posText.append(QString("%1 %2 %3\n").arg(v[0],0,'f',6).arg(v[1],0,'f',6).arg(v[2],0,'f',6));
	float_array.appendChild( doc.createTextNode(posText) );
	QDomElement technique_common = doc.createElement("technique_common");
	source.appendChild(technique_common);
	QDomElement accessor = doc.createElement("accessor");
	accessor.setAttribute("source",QString("#nifid_%1-lib-Position-array").arg(idx));
	accessor.setAttribute("count",verts.size());
	accessor.setAttribute("stride","3");
	technique_common.appendChild(accessor);
	QDomElement param = doc.createElement("param");
	param.setAttribute("name","X");
	param.setAttribute("type","float");
	accessor.appendChild(param);
	param = doc.createElement("param");
	param.setAttribute("name","Y");
	param.setAttribute("type","float");
	accessor.appendChild(param);
	param = doc.createElement("param");
	param.setAttribute("name","Z");
	param.setAttribute("type","float");
	accessor.appendChild(param);
	return source;
}

/**
 *  create normals array with (nif)index
 * @param normals array of normals
 * @param idx nif index
 * @return QDomElement
 */
QDomElement normalsElement(QVector<Vector3> normals,int idx) {
	QDomElement source;
	if ( normals.size() > 0 ) {
		source = doc.createElement("source");
		source.setAttribute("id",QString("nifid_%1-lib-Normal0").arg(idx));
		QDomElement float_array = doc.createElement("float_array");
		float_array.setAttribute("id",QString("nifid_%1-lib-Normal0-array").arg(idx));
		float_array.setAttribute("count", (normals.size()*3) );
		source.appendChild(float_array);

		QString norText("\n");
		foreach ( Vector3 v, normals )
			norText.append(QString("%1 %2 %3\n").arg(v[0],0,'f',6).arg(v[1],0,'f',6).arg(v[2],0,'f',6));
		float_array.appendChild( doc.createTextNode(norText) );
		QDomElement technique_common = doc.createElement("technique_common");
		source.appendChild(technique_common);

		QDomElement accessor = doc.createElement("accessor");
		accessor.setAttribute("source",QString("#nifid_%1-lib-Normal0-array").arg(idx));
		accessor.setAttribute("count",normals.size());
		accessor.setAttribute("stride","3");
		technique_common.appendChild(accessor);

		QDomElement param = doc.createElement("param");
		param.setAttribute("name","X");
		param.setAttribute("type","float");
		accessor.appendChild(param);

		param = doc.createElement("param");
		param.setAttribute("name","Y");
		param.setAttribute("type","float");
		accessor.appendChild(param);

		param = doc.createElement("param");
		param.setAttribute("name","Z");
		param.setAttribute("type","float");
		accessor.appendChild(param);
	}
	return source;
}

/**
 * create color element from color type and Color3
 * @param name sid name
 * @param color values<Color3>
 * @return QDomElement
 *
 * <{name}>
 *   <color sid="{name}">{color.red()}  {color.green()} {color.blue()} 1.000000</color>
 * </{name}>
 */
QDomElement colorElement(QString name,Color3 color) {
	QDomElement element = doc.createElement(name);
	QDomElement colorElement = doc.createElement("color");
	colorElement.setAttribute("sid",name);
	colorElement.appendChild( doc.createTextNode(QString("%1 %2 %3 1.000000").arg(color.red(),0,'f',6).arg(color.green(),0,'f',6).arg(color.blue(),0,'f',6)) );
	element.appendChild(colorElement);
	return element;
}

/**
 * create "effect" element (shininess/reflectivity/transparency) from type and float
 * @param name sid name
 * @param amount value
 * @return QDomElement
 *
 * <{name}>
 *   <float sid="{name}">{amount}</float>
 * </{name}>
 */
QDomElement effectElement(QString name,float amount) {
	QDomElement element = doc.createElement(name);
	QDomElement colorElement = doc.createElement("float");
	colorElement.setAttribute("sid",name);
	colorElement.appendChild( doc.createTextNode(QString("%1").arg(amount,0,'f',6) ) );
	element.appendChild(colorElement);
	return element;
}

/**
 * create color element from color type and Color3
 * TODO: correct blend_mode ?
 *
 * <texture texture="{name}" texcoord="{texcoord}">
 *   <extra>
 *     <technique profile="MAYA">
 *       <wrapU sid="wrapU0">TRUE</wrapU>
 *       <wrapV sid="wrapV0">TRUE</wrapV>
 *       <blend_mode>ADD</blend_mode>
 *     </technique>
 *   </extra>
 * </texture>
 */
QDomElement colorTextureElement(QString name,QString texcoord) {
	QDomElement textureBaseTexture = doc.createElement("texture");
	textureBaseTexture.setAttribute("texture",name);
	textureBaseTexture.setAttribute("texcoord",texcoord);
	QDomElement extra = doc.createElement("extra");
	textureBaseTexture.appendChild(extra);
	QDomElement technique = doc.createElement("technique");
	technique.setAttribute("profile","MAYA");
	extra.appendChild(technique);
	QDomElement wrapU = doc.createElement("wrapU");
	wrapU.setAttribute("sid","wrapU0");
	wrapU.appendChild( doc.createTextNode(QString("TRUE") ) );
	technique.appendChild(wrapU);
	QDomElement wrapV = doc.createElement("wrapV");
	wrapV.setAttribute("sid","wrapV0");
	wrapV.appendChild( doc.createTextNode(QString("TRUE") ) );
	technique.appendChild(wrapV);
	QDomElement blendMode = doc.createElement("blend_mode");
	blendMode.appendChild( doc.createTextNode(QString("ADD") ) );
	technique.appendChild(blendMode);
	return textureBaseTexture;
}

/**
 * extract shape to dom structures
 * TODO: WIP and do major cleanup and re-structuring .. tons of crap "definitions" and boilerplate
 * TODO: maybe use QList and QVertex "size" instead of "bool have*"
 * FIXME: handle multiple UV maps in <polygons> .. find example!
 */
void attachNiShape (const NifModel * nif,QDomElement parentNode,int idx) {
	bool haveVertex = false;
	bool haveNormal = false;
	bool haveUV = false;
	QModelIndex iBlock = nif->getBlock( idx );
	QDomElement textureBaseTexture;
	QDomElement input;
	// effect
	QDomElement effect;
	// profile
	QDomElement profile;
	foreach ( qint32 link, nif->getChildLinks(idx) ) {
		QModelIndex iProp = nif->getBlock( link );
		if ( nif->isNiBlock( iProp, "NiTexturingProperty" ) ) {
			if ( ! effect.isElement() ) {
				effect = doc.createElement("effect");
				effect.setAttribute("id",QString("nifid_%1-effect").arg(idx));
			}
			if ( ! profile.isElement() )
				profile = doc.createElement("profile_COMMON");
			QModelIndex iBase = nif->getBlock( nif->getLink( nif->getIndex( iProp, "Base Texture" ), "Source" ), "NiSourceTexture" );
			if ( iBase.isValid() ) {
				QFileInfo textureFile = TexCache::find( nif->get<QString>( iBase, "File Name" ), nif->getFolder() );
				// ImageLibrary
				QDomElement image = doc.createElement("image");
				image.setAttribute("name",QString("Map_%1").arg(QFileInfo(textureFile.baseName()).baseName()));
				image.setAttribute("id",QString("nifid_%1_image").arg(idx));
				QDomElement initFrom = doc.createElement("init_from");
				initFrom.appendChild( doc.createTextNode( QString("%1%2").arg( (textureFile.isAbsolute()?"":"./") ).arg(textureFile.filePath()) ) );
				image.appendChild(initFrom);
				libraryImages.appendChild(image);
				// LibraryMaterials
				QDomElement material = doc.createElement("material");
				material.setAttribute("name",QString("Material_%1").arg(textureFile.baseName()));
				material.setAttribute("id",QString("nifid_%1-material").arg(idx));
				libraryMaterials.appendChild(material);
				QDomElement instance = doc.createElement("instance_effect");
				instance.setAttribute("url",QString("#nifid_%1-effect").arg(idx));
				material.appendChild(instance);
				// surface
				QDomElement newparam = doc.createElement("newparam");
				newparam.setAttribute("sid",QString("nifid_%1-surface").arg(idx));
				profile.appendChild(newparam);
				QDomElement surface = doc.createElement("surface");
				surface.setAttribute("type","2D");
				newparam.appendChild(surface);
				QDomElement init_from = doc.createElement("init_from");
				surface.appendChild(init_from);
				init_from.appendChild( doc.createTextNode( QString("nifid_%1_image").arg(idx) ) );

				// sampler
				newparam = doc.createElement("newparam");
				newparam.setAttribute("sid",QString("nifid_%1-sampler").arg(idx));
				profile.appendChild(newparam);
				QDomElement sampler2D = doc.createElement("sampler2D");
				newparam.appendChild(sampler2D);
				QDomElement source = doc.createElement("source");
				sampler2D.appendChild(source);
				source.appendChild( doc.createTextNode( QString("nifid_%1-surface").arg(idx) ) );

				// attach texture to "diffuse"
				textureBaseTexture = colorTextureElement(QString("nifid_%1-sampler").arg(idx),"CHANNEL0");
			}
		} else if ( nif->isNiBlock( iProp, "NiMaterialProperty" ) ) {
			if ( ! effect.isElement() ) {
				effect = doc.createElement("effect");
				effect.setAttribute("id",QString("nifid_%1-effect").arg(idx));
			}
			if ( ! profile.isElement() )
				profile = doc.createElement("profile_COMMON");
			// effect
			QString name = nif->get<QString>( iProp, "Name" ).replace(" ","_");
			effect.setAttribute("name",QString("Material_%1-effect").arg( name ));
			// technique
			QDomElement technique = doc.createElement("technique");
			technique.setAttribute("sid","standard");
			profile.appendChild(technique);
			// phong
			QDomElement blinn = doc.createElement("blinn");
			technique.appendChild(blinn);
			// emission
			blinn.appendChild( colorElement("emission", nif->get<Color3>( iProp, "Emissive Color" ) ));
			// ambient
			blinn.appendChild( colorElement("ambient", nif->get<Color3>( iProp, "Ambient Color" ) ));
			// diffuse with texture
			if ( ! textureBaseTexture.isElement() )
				blinn.appendChild(colorElement("diffuse", nif->get<Color3>( iProp, "Diffuse Color" ) ) );
			else {
				QDomElement diffuse = doc.createElement("diffuse");
				diffuse.appendChild(textureBaseTexture);
				blinn.appendChild(diffuse);
			}
//			<texture texture="a_cliff_dirt_02_dds-sampler" texcoord="CHANNEL1"/>
//			diffuse.appendChild(textureBaseTexture);
//			blinn.appendChild(diffuse);
			// specular
			blinn.appendChild( colorElement("specular", nif->get<Color3>( iProp, "Specular Color" ) ));
			// shininess
			blinn.appendChild( effectElement("shininess" , nif->get<float>( iProp, "Glossiness" ) ) );
			// reflective
			blinn.appendChild( colorElement("reflective", Color3(0.0f,0.0f,0.0f)));
			blinn.appendChild( effectElement("reflectivity" ,1.0f ) );
			// transparency
			blinn.appendChild( colorElement("transparent", Color3(1.0f,1.0f,1.0f)));
			blinn.appendChild( effectElement("transparency" , nif->get<float>( iProp, "Alpha" ) ) );
		} else if ( nif->isNiBlock( iProp, "NiTriShapeData" ) ) {
			QDomElement geometry = doc.createElement("geometry");
			geometry.setAttribute("id",QString("nifid_%1-lib").arg(idx));
			geometry.setAttribute("name",QString("%1-lib").arg(nif->get<QString>( iBlock, "Name" ).replace(" ","_") ) );
			libraryGeometries.appendChild(geometry);
			QDomElement mesh = doc.createElement("mesh");
			geometry.appendChild(mesh);

			// Position
			QVector<Vector3> verts = nif->getArray<Vector3>( iProp, "Vertices" );
			mesh.appendChild( positionsElement(verts,idx) );
			if ( verts.size() > 0 )
				haveVertex = true;

			// Normals
			QVector<Vector3> normals = nif->getArray<Vector3>( iProp, "Normals" );
			mesh.appendChild( normalsElement( normals , idx ) );
			if ( normals.size() >  0 )
				haveNormal = true;

			// UV maps
			QModelIndex iUV = nif->getIndex( iProp, "UV Sets" );
			if ( ! iUV.isValid() )
				iUV = nif->getIndex( iProp, "UV Sets 2" );
			for(int row=0;row <  nif->get<int>( iProp, "Num UV Sets") ; row++ ) {
				QVector<Vector2> uvMap = nif->getArray<Vector2>( iUV.child( row, 0 ) );
				mesh.appendChild(uvMapElement(uvMap,idx,row));
				if ( uvMap.size() > 0 )
					haveUV = true;
			}

			// vertices
			QDomElement vertices = doc.createElement("vertices");
			vertices.setAttribute("id",QString("nifid_%1-lib-Vertex").arg(idx));
			mesh.appendChild(vertices);
			input = doc.createElement("input");
			input.setAttribute("semantic","POSITION");
			input.setAttribute("source",QString("#nifid_%1-lib-Position").arg(idx));
			vertices.appendChild(input);

			// polygons (mapping)
			QDomElement triangles = doc.createElement("triangles");
			triangles.setAttribute("material",QString("material_nifid_%1").arg(idx));
			triangles.setAttribute("count",nif->get<ushort>( iProp, "Num Triangles"));
			mesh.appendChild(triangles);
			int x=0;
			if ( haveVertex == true ) {
				input = doc.createElement("input");
				input.setAttribute("semantic","VERTEX");
				input.setAttribute("offset",x++);
				input.setAttribute("source",QString("#nifid_%1-lib-Vertex").arg(idx));
				triangles.appendChild(input);
			}
			if ( haveNormal == true ) {
				input = doc.createElement("input");
				input.setAttribute("semantic","NORMAL");
				input.setAttribute("offset",x++);
				input.setAttribute("source",QString("#nifid_%1-lib-Normal0").arg(idx));
				triangles.appendChild(input);
			}
			if ( haveUV == true ) {
				// TODO: add multiple UV
				input = doc.createElement("input");
				input.setAttribute("semantic","TEXCOORD");
				input.setAttribute("offset",x++);
				input.setAttribute("source",QString("#nifid_%1-lib-UV0").arg(idx));
				triangles.appendChild(input);
			}
			QVector<Triangle> tri = nif->getArray<Triangle>( iProp, "Triangles" );
			int c=0;
			QDomElement p = doc.createElement("p");
			QString triText;
			foreach ( Triangle v, tri ) {
				// TODO: add multiple UV
				if ( haveVertex == true )	triText.append( QString("%1 ").arg(v[0]) );
				if ( haveNormal == true )	triText.append( QString("%1 ").arg(v[0]) );
				if ( haveUV == true )		triText.append( QString("%1 ").arg(v[0]) );

				if ( haveVertex == true )	triText.append( QString("%1 ").arg(v[1]) );
				if ( haveNormal == true )	triText.append( QString("%1 ").arg(v[1]) );
				if ( haveUV == true )		triText.append( QString("%1 ").arg(v[1]) );

				if ( haveVertex == true )	triText.append( QString("%1 ").arg(v[2]) );
				if ( haveNormal == true )	triText.append( QString("%1 ").arg(v[2]) );
				if ( haveUV == true )		triText.append( QString("%1 ").arg(v[2]) );
			}
			p.appendChild( doc.createTextNode(triText) );
			triangles.appendChild(p);

			// extra node for model matrix move
			QDomElement node = doc.createElement("node");
			node.setAttribute("id", QString("nifid_%1-matrix").arg(idx) );
			parentNode.appendChild(node);
			// matrix
			// FIXME use matrix if possible
			float scale = nif->get<float>( iBlock, "Scale" );
			Vector3 scales;
			scales[0]=scale;
			scales[1]=scale;
			scales[2]=scale;
			Matrix4 m4;
			m4.compose(nif->get<Vector3>( iBlock, "Translation" ),nif->get<Matrix>( iBlock, "Rotation" ), scales );
			const float *e = m4.data(); // vector array
			QDomElement matrix = doc.createElement("matrix");
			matrix.setAttribute("sid", "matrix");
			matrix.appendChild( doc.createTextNode(
				QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16")
					.arg(e[0],0,'f',6).arg(e[1],0,'f',6).arg(e[2],0,'f',6).arg(e[12],0,'f',6)
					.arg(e[4],0,'f',6).arg(e[5],0,'f',6).arg(e[6],0,'f',6).arg(e[13],0,'f',6)
					.arg(e[8],0,'f',6).arg(e[9],0,'f',6).arg(e[10],0,'f',6).arg(e[14],0,'f',6)
					.arg(e[3],0,'f',6).arg(e[7],0,'f',6).arg(e[11],0,'f',6).arg(e[15],0,'f',6)
				)
			);
			node.appendChild(matrix);

			// attach structure and material to node structure
			QDomElement instanceGeometry = doc.createElement("instance_geometry");
			instanceGeometry.setAttribute("url",QString("#nifid_%1-lib").arg(idx));
			node.appendChild(instanceGeometry);
			QDomElement bindMaterial = doc.createElement("bind_material");
			instanceGeometry.appendChild(bindMaterial);
			QDomElement techniqueCommon = doc.createElement("technique_common");
			bindMaterial.appendChild(techniqueCommon);
			QDomElement instanceMaterial = doc.createElement("instance_material");
			instanceMaterial.setAttribute("symbol",QString("nifid_%1-material").arg(idx));
			instanceMaterial.setAttribute("target",QString("#nifid_%1-material").arg(idx));
			techniqueCommon.appendChild(instanceMaterial);
			//	<bind_vertex_input semantic="CHANNEL1" input_semantic="TEXCOORD" input_set="0"/>
			// TODO: now hard coded to have only one .. make multiple UV maps
			if ( haveUV == true ) {
				QDomElement bind_vertex_input = doc.createElement("bind_vertex_input");
				bind_vertex_input.setAttribute("semantic","CHANNEL0");
				bind_vertex_input.setAttribute("input_semantic","TEXCOORD");
				bind_vertex_input.setAttribute("input_set","0");
				instanceMaterial.appendChild(bind_vertex_input);
			}
		}
		if ( effect.isElement() )
			effect.appendChild(profile);
		if ( libraryEffects.isElement() )
			libraryEffects.appendChild(effect);
	}
}

/**
 *
 */
void attachNiNode (const NifModel * nif,QDomElement parentNode,int idx) {
	QModelIndex iBlock = nif->getBlock( idx );
	QDomElement node = doc.createElement("node");
	node.setAttribute("name",  nif->get<QString>( iBlock, "Name" ).replace(" ","_") );
	node.setAttribute("id", QString("nifid_%1_node").arg(idx) );

	// matrix
	float scale = nif->get<float>( iBlock, "Scale" );
	Vector3 scales;
	scales[0]=scale;
	scales[1]=scale;
	scales[2]=scale;
	Matrix4 m4;
	m4.compose(nif->get<Vector3>( iBlock, "Translation" ),nif->get<Matrix>( iBlock, "Rotation" ), scales );
	const float *e = m4.data(); // vector array
	QDomElement matrix = doc.createElement("matrix");
	matrix.setAttribute("sid", "matrix");
	matrix.appendChild( doc.createTextNode(
		QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14 %15 %16")
			.arg(e[0],0,'f',6).arg(e[1],0,'f',6).arg(e[2],0,'f',6).arg(e[12],0,'f',6)
			.arg(e[4],0,'f',6).arg(e[5],0,'f',6).arg(e[6],0,'f',6).arg(e[13],0,'f',6)
			.arg(e[8],0,'f',6).arg(e[9],0,'f',6).arg(e[10],0,'f',6).arg(e[14],0,'f',6)
			.arg(e[3],0,'f',6).arg(e[7],0,'f',6).arg(e[11],0,'f',6).arg(e[15],0,'f',6)
		)
	);
	node.appendChild(matrix);

	// parent attach and new loop
	parentNode.appendChild(node);
	foreach ( int l,nif->getChildLinks(idx) ) {
		QModelIndex iChild = nif->getBlock( l );
		QString type = nif->getBlockName(iChild);
//		qDebug() << "TYPE:" << type;
		if ( type == QString("NiNode") )
			attachNiNode(nif,node,l);
		if ( type == QString("NiLODNode") )
			attachNiNode(nif,node,l);
		if ( type == QString("NiTriShape") )
			attachNiShape(nif,node,l);
	}
}

void exportCol( const NifModel * nif ) {
	QList<int> roots = nif->getRootLinks();
	QString question;
	QSettings settings;
	settings.beginGroup( "import-export" );
	settings.beginGroup( "col" );
	QString fname = QFileDialog::getSaveFileName( 0, tr("Choose a .DAE file for export"), settings.value( "File Name" ).toString(), "*.dae" );
	if ( fname.isEmpty() )
		return;
	while ( fname.endsWith( ".dae", Qt::CaseInsensitive ) )
		fname = fname.left( fname.length() - 4 );
	QFile fobj( fname + ".dae" );
	if ( ! fobj.open( QIODevice::WriteOnly ) ) {
		qWarning() << "could not open " << fobj.fileName() << " for write access";
		return;
	}
	int i = fname.lastIndexOf( "/" );
	if ( i >= 0 )
		fname = fname.remove( 0, i+1 );
	doc.clear();
	QDomElement root = doc.createElement("COLLADA");
	root.setAttribute("xmlns","http://www.collada.org/2005/11/COLLADASchema");
	root.setAttribute("version","1.4.0");
	doc.appendChild(root);
	// asset tag
	QDomElement asset = doc.createElement("asset");
	root.appendChild(asset);
	QDomElement contributor = doc.createElement("contributor");
	asset.appendChild(contributor);
	contributor.appendChild(doc.createElement("author"));
	contributor.appendChild(textElement("authoring_tool","NifSkope"));
	contributor.appendChild(doc.createElement("comments"));
	asset.appendChild(dateElement("created", QDateTime::currentDateTime() ) );
	asset.appendChild(dateElement("modified", QDateTime::currentDateTime() ) );
	asset.appendChild(doc.createElement("revision"));
	asset.appendChild(doc.createElement("title"));
	QDomElement unit = doc.createElement("unit");
	unit.setAttribute("name","inch");
	unit.setAttribute("meter","0.0254");
	asset.appendChild(unit);
	asset.appendChild(textElement("up_axis","Z_UP")); // TODO: check!
	root.appendChild(libraryImages);
	root.appendChild(libraryMaterials);
	root.appendChild(libraryEffects);
	root.appendChild(libraryGeometries);
	QDomElement lvs = doc.createElement("library_visual_scenes");
	root.appendChild(lvs);
	QDomElement lv = doc.createElement("visual_scene");
	lv.setAttribute("id","NifRootScene");
	lvs.appendChild(lv);
	QDomElement parent = root;
	while ( ! roots.empty() ) {
		int idx = roots.takeFirst();
		QModelIndex iBlock = nif->getBlock( idx );
//		qDebug() << nif->getBlockName(iBlock);
		// get more if NiNode
		if ( nif->isNiBlock( iBlock, "NiNode" )  )
			attachNiNode(nif,lv,idx);
	}
	QDomElement scene = doc.createElement("scene");
	root.appendChild(scene);
	QDomElement ivl = doc.createElement("instance_visual_scene");
	ivl.setAttribute("url","#NifRootScene");
	scene.appendChild(ivl);
	fobj.write(doc.toString().toAscii());
	settings.setValue( "File Name", fobj.fileName() );
	QTextStream sobj( &fobj ); // let's save xml
}

