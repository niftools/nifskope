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

#include "../config.h"

#define tr(x) QApplication::tr(x)

/**
 * TODO LIST:
 * - handle NiTriShapeData better way
 * - build regexp for xml id and names
 * - all "name" attributes with QRegExp("\\W")
 * - find models with other textures (than base + dark)
 * - sampler only once if same texture used multiple times for material
 * DONE:
 * + now skips "deleted" nodes correctly
 * + first "multitexture" testing (diffuse + dark)
 * + textureElement function for image handling and hooking
 * + added NiTextureProperty
 * + changed matrix order
 * + models and texture works, tested multiple dae supported software (except rotation)
 * + added NifLODNode as "NifNode"
 * + Now using matrix to translate/rotate/size, still rotate is not exactly in place in multi layer (node) models.
 * + Vertex Colors now added to export
 * + NiTriStrips to attachNiShape
 * SCHEMA TESTING:
 * xmllint --noout --schema http://www.khronos.org/files/collada_schema_1_4_1.xsd ~/file.dae
 *
 */

// "globals"
QDomDocument doc("");
QDomElement libraryImages;
QDomElement libraryMaterials;
QDomElement libraryEffects;
QDomElement libraryGeometries;


QVector<int> textureIds;
QVector<QString> textureNames;

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
 * create matrix element
 * @param trans Translation
 * @param rot  Rotation
 * @param scale Scale
 * @return QDomElement
 * NOTE: translation need to be in OpenGL way
 * <matrix>
 *   1.000000 0.000000 0.000000 20.290175
 *   0.000000 1.000000 0.000000 37.955944
 *   0.000000 0.000000 1.000000 -0.000000
 *   0.000000 0.000000 0.000000 1.000000
 * </matrix>
 */
QDomElement matrixElement(Vector3 trans,Matrix rot,float scale) {
	QDomElement element = doc.createElement("matrix");
	Vector3 scales;
	scales[0]=scale; // scale X
	scales[1]=scale; // scale Y
	scales[2]=scale; // scale Z
	Matrix4 m4;
	m4.compose(trans,rot, scales );
	const float *e = m4.data(); // array
	element.setAttribute("sid", "matrix");
	element.appendChild( doc.createTextNode(
		QString("\n%1 %2 %3 %4\n%5 %6 %7 %8\n%9 %10 %11 %12\n%13 %14 %15 %16\n")
			.arg(e[0] ,0,'f',6).arg(e[4] ,0,'f',6).arg(e[8] ,0,'f',6).arg(e[12],0,'f',6)
			.arg(e[1] ,0,'f',6).arg(e[5] ,0,'f',6).arg(e[9] ,0,'f',6).arg(e[13],0,'f',6)
			.arg(e[2] ,0,'f',6).arg(e[6] ,0,'f',6).arg(e[10],0,'f',6).arg(e[14],0,'f',6)
			.arg(e[3] ,0,'f',6).arg(e[7] ,0,'f',6).arg(e[11],0,'f',6).arg(e[15],0,'f',6)
		)
	);
	return element;
}

/**
 * create color map array with (nif)index
 * @param colors array of polygon colors
 * @param idx nif index
 * @return QDomElement
 */
QDomElement colorMapElement(QVector<Color4> colors,int idx) {
	QDomElement source = doc.createElement("source");
	source.setAttribute("id",QString("nifid_%1-lib_colors").arg(idx));

	QDomElement float_array = doc.createElement("float_array");
	float_array.setAttribute("id",QString("nifid_%1-lib_colors-array").arg(idx));
	float_array.setAttribute("count", (colors.size()*4) );
	source.appendChild(float_array);

	QString text("");
	foreach ( Color4 v, colors )
		text.append(QString("%1 %2 %3 %4 ").arg(v[0]).arg(v[1]).arg(v[2]).arg(v[3]));
	float_array.appendChild( doc.createTextNode(text) );

	QDomElement technique_common = doc.createElement("technique_common");
	source.appendChild(technique_common);

	QDomElement accessor = doc.createElement("accessor");
	accessor.setAttribute("source",QString("#nifid_%1-lib_colors-array").arg(idx));
	accessor.setAttribute("count",colors.size());
	accessor.setAttribute("stride","4");
	technique_common.appendChild(accessor);
	QDomElement param;

	param = doc.createElement("param");
	param.setAttribute("name","R");
	param.setAttribute("type","float");
	accessor.appendChild(param);

	param = doc.createElement("param");
	param.setAttribute("name","G");
	param.setAttribute("type","float");
	accessor.appendChild(param);

	param = doc.createElement("param");
	param.setAttribute("name","B");
	param.setAttribute("type","float");
	accessor.appendChild(param);

	param = doc.createElement("param");
	param.setAttribute("name","A");
	param.setAttribute("type","float");
	accessor.appendChild(param);
	return source;
}

/**
 * create UV map array with (nif)index and row (multiple UV for one mesh)
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

	QString uvText("");
        // we have to flip the second UV coordinate because nif uses
        // different convention from collada
	foreach ( Vector2 v, uvMap )
		uvText.append(QString("%1 %2 ").arg(v[0]).arg(1.0 - v[1]));
	float_array.appendChild( doc.createTextNode(uvText) );

	QDomElement technique_common = doc.createElement("technique_common");
	source.appendChild(technique_common);

	QDomElement accessor = doc.createElement("accessor");
	accessor.setAttribute("source",QString("#nifid_%1-lib-UV%2-array").arg(idx).arg(row));
	accessor.setAttribute("count",uvMap.size());
	accessor.setAttribute("stride","2");
	technique_common.appendChild(accessor);
	QDomElement param;

	param = doc.createElement("param");
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
 * create positions array with (nif)index
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

	QString posText("");
	foreach ( Vector3 v, verts )
		posText.append(QString("%1 %2 %3 ").arg(v[0],0,'f',6).arg(v[1],0,'f',6).arg(v[2],0,'f',6));
	float_array.appendChild( doc.createTextNode(posText) );

	QDomElement technique_common = doc.createElement("technique_common");
	source.appendChild(technique_common);

	QDomElement accessor = doc.createElement("accessor");
	accessor.setAttribute("source",QString("#nifid_%1-lib-Position-array").arg(idx));
	accessor.setAttribute("count",verts.size());
	accessor.setAttribute("stride","3");
	technique_common.appendChild(accessor);
	QDomElement param;

	param = doc.createElement("param");
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
 * create normals array with (nif)index
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

		QString norText("");
		foreach ( Vector3 v, normals )
			norText.append(QString("%1 %2 %3 ").arg(v[0],0,'f',6).arg(v[1],0,'f',6).arg(v[2],0,'f',6));
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
 *
 * TODO: bit hack-ish
 */
QDomElement textureArrayElement(QString file,QDomElement effect,qint32 idx,QString type) {
	QDomElement ret;
	file.replace("\\","/");
	// surface
	QDomElement newparam = doc.createElement("newparam");
	newparam.setAttribute("sid",QString("nifid_%1_%2-surface").arg(idx).arg(type));
	effect.appendChild(newparam);
	QDomElement surface = doc.createElement("surface");
	surface.setAttribute("type","2D");
	newparam.appendChild(surface);
	QDomElement init_from = doc.createElement("init_from");
	surface.appendChild(init_from);
	init_from.appendChild( doc.createTextNode( QString("nifid_%1_%2_image").arg(idx).arg(type) ) );
	// sampler
	newparam = doc.createElement("newparam");
	newparam.setAttribute("sid",QString("nifid_%1_%2-sampler").arg(idx).arg(type));
	effect.appendChild(newparam);
	QDomElement sampler2D = doc.createElement("sampler2D");
	newparam.appendChild(sampler2D);
	QDomElement source = doc.createElement("source");
	sampler2D.appendChild(source);
	source.appendChild( doc.createTextNode( QString("nifid_%1_%2-surface").arg(idx).arg(type) ) );

	QString tIdName = QString("nifid_%1_%2_image").arg(idx).arg(type);
	if ( ! textureNames.contains(tIdName) ) {
		textureNames.append(tIdName);
		QDomElement image = doc.createElement("image");
		image.setAttribute("name",QString("Map_%1").arg( QFileInfo(file).baseName() ) );
		image.setAttribute("id",tIdName);
		QDomElement initFrom = doc.createElement("init_from");
		initFrom.appendChild( doc.createTextNode( file ) );
		image.appendChild(initFrom);
		libraryImages.appendChild(image);
	}
	// return "sampler"
	ret = colorTextureElement(QString("nifid_%1_%2-sampler").arg(idx).arg(type),QString("UVSET0"));
	return ret;
}

/**
 *
 * NOTE: NiTextureProperty don't have childNode for each textures, so node is itself as child
 */

QDomElement textureElement(const NifModel * nif,QDomElement effect,QModelIndex childNode,int idx) {
	QDomElement ret;
	qint32 texIdx = nif->getLink( childNode, "Source" );
	QModelIndex iTexture = nif->getBlock(texIdx, "NiSourceTexture" );
	int uvSet =  nif->get<int>(childNode, "UV Set");
	if ( ! iTexture.isValid() ) { // try to use NiTextureProperty attributes
		texIdx = nif->getLink( childNode, "Image" );
		iTexture = nif->getBlock( texIdx , "NiImage" );
		uvSet = 0; // NiTextureProperty only have one texture
	}
	if ( iTexture.isValid() ) { // we have texture
		QFileInfo textureFile = nif->get<QString>( iTexture, "File Name" );

		// surface
		QDomElement newparam = doc.createElement("newparam");
		newparam.setAttribute("sid",QString("nifid_%1-surface").arg(texIdx));
		effect.appendChild(newparam);
		QDomElement surface = doc.createElement("surface");
		surface.setAttribute("type","2D");
		newparam.appendChild(surface);
		QDomElement init_from = doc.createElement("init_from");
		surface.appendChild(init_from);
		init_from.appendChild( doc.createTextNode( QString("nifid_%1_image").arg(texIdx) ) );

		// sampler
		newparam = doc.createElement("newparam");
		newparam.setAttribute("sid",QString("nifid_%1-sampler").arg(texIdx));
		effect.appendChild(newparam);
		QDomElement sampler2D = doc.createElement("sampler2D");
		newparam.appendChild(sampler2D);
		QDomElement source = doc.createElement("source");
		sampler2D.appendChild(source);
		source.appendChild( doc.createTextNode( QString("nifid_%1-surface").arg(texIdx) ) );

		// add to ImageLibrary if id don't exists yet
		if ( ! textureIds.contains(texIdx) ) {
			textureIds.append(texIdx);
			QDomElement image = doc.createElement("image");
			image.setAttribute("name",QString("Map_%1").arg(QFileInfo(textureFile.baseName()).baseName()));
			image.setAttribute("id",QString("nifid_%1_image").arg(texIdx));
			QDomElement initFrom = doc.createElement("init_from");
			initFrom.appendChild( doc.createTextNode( QString("%1%2").arg( (textureFile.isAbsolute()?"":"./") ).arg(textureFile.filePath()) ) );
			image.appendChild(initFrom);
			libraryImages.appendChild(image);
		}

		// TODO: bind_vertex_input should also built here?

		// return "sampler"
		ret = colorTextureElement(QString("nifid_%1-sampler").arg(texIdx),QString("UVSET%1").arg(uvSet));
	}
	return ret;
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
	bool haveColors = false;
	bool haveMaterial = false;
	int haveUV = 0;
	QModelIndex iBlock = nif->getBlock( idx );
	QDomElement textureBaseTexture;
	QDomElement textureDarkTexture;
	QDomElement textureGlowTexture;
	QDomElement input;
	// effect
	QDomElement effect;
	// profile
	QDomElement profile;
	foreach ( qint32 link, nif->getChildLinks(idx) ) {
		QModelIndex iProp = nif->getBlock( link );
		if ( nif->inherits( iProp, "NiTexturingProperty" ) ) {
			if ( ! effect.isElement() ) {
				effect = doc.createElement("effect");
				effect.setAttribute("id",QString("nifid_%1-effect").arg(idx));
			}
			if ( ! profile.isElement() )
				profile = doc.createElement("profile_COMMON");
			// base texture = map diffuse
			textureBaseTexture = textureElement(nif,profile,nif->getIndex( iProp, "Base Texture" ),idx);
			// dark texture ("like reverse light map") = map specular (not good, but at least somewhere)
			textureDarkTexture = textureElement(nif,profile,nif->getIndex( iProp, "Dark Texture" ),idx);
			// glow texture = map emission
			textureGlowTexture = textureElement(nif,profile,nif->getIndex( iProp, "Glow Texture" ),idx);

		} else if ( nif->inherits( iProp, "NiTextureProperty" ) ) {
			if ( ! effect.isElement() ) {
				effect = doc.createElement("effect");
				effect.setAttribute("id",QString("nifid_%1-effect").arg(idx));
			}
			if ( ! profile.isElement() )
				profile = doc.createElement("profile_COMMON");
			textureBaseTexture = textureElement(nif,profile,iProp,idx);
		} else if ( nif->inherits( iProp, "NiMaterialProperty" ) || nif->inherits( iProp, "BSLightingShaderProperty" ) ) {
			if ( ! effect.isElement() ) {
				effect = doc.createElement("effect");
				effect.setAttribute("id",QString("nifid_%1-effect").arg(idx));
			}
			if ( ! profile.isElement() )
				profile = doc.createElement("profile_COMMON");
			// BSLightingShaderProperty inherits textures .. so it's bit ugly hack
			// 0 = diffuse, 1 = normal
			qint32 subIdx = nif->getLink( iProp, "Texture Set" );
			QModelIndex iTextures = nif->getBlock( subIdx );
			if ( iTextures.isValid() ) {
				int tCount = nif->get<int>( iTextures, "Num Textures" );
				QVector<QString> textures = nif->getArray<QString>( iTextures, "Textures" );
				if ( ! textures.at(0).isEmpty()  )
					textureBaseTexture = textureArrayElement(textures.at(0),profile,subIdx,"base");
				// TODO: add normal map
				/*	<extra>
						<technique profile="FCOLLADA">
							<bump>
								<texture texture="sid-of-some-param-sampler" texcoord="symbolic_name_to_bind_from_shader"/>
							</bump>
						</technique>
					</extra>
				*/
			}
			// Material parameters
			haveMaterial = true;
			QString name = nif->get<QString>( iProp, "Name" ).replace(" ","_");
			// library_materials -> material
			QDomElement material = doc.createElement("material");
			material.setAttribute("name",QString("Material_%1").arg(name));
			material.setAttribute("id",QString("nifid_%1-material").arg(idx));
			libraryMaterials.appendChild(material);
			// library_materials -> material -> instance_effect
			QDomElement instance = doc.createElement("instance_effect");
			instance.setAttribute("url",QString("#nifid_%1-effect").arg(idx));
			material.appendChild(instance);
			// library_effects -> effect
			if ( ! effect.isElement() ) {
				effect = doc.createElement("effect");
				effect.setAttribute("id",QString("nifid_%1-effect").arg(idx));
			}
			if ( ! profile.isElement() )
				profile = doc.createElement("profile_COMMON");
			effect.setAttribute("name",(name.isEmpty()?QString("nifid_%1-effect").arg(idx):QString("%1").arg(name) ) );

			// library_effects -> effect -> technique
			QDomElement technique = doc.createElement("technique");
			technique.setAttribute("sid","COMMON");
			profile.appendChild(technique);
			// library_effects -> effect -> technique ->phong
			QDomElement phong = doc.createElement("phong");
			technique.appendChild(phong);
			// library_effects -> effect -> technique ->phong -> emission
			if ( ! textureGlowTexture.isElement() )
				phong.appendChild( colorElement("emission", nif->get<Color3>( iProp, "Emissive Color" ) ));
			else {
				QDomElement emission = doc.createElement("emission");
				emission.appendChild(textureGlowTexture);
				phong.appendChild(emission);
			}
			// ambient
			phong.appendChild( colorElement("ambient", nif->get<Color3>( iProp, "Ambient Color" ) ));
			// diffuse with texture
			if ( ! textureBaseTexture.isElement() )
				phong.appendChild(colorElement("diffuse", nif->get<Color3>( iProp, "Diffuse Color" ) ) );
			else {
				QDomElement diffuse = doc.createElement("diffuse");
				diffuse.appendChild(textureBaseTexture);
				phong.appendChild(diffuse);
			}
			// specular
			if ( !textureDarkTexture.isElement() )
				phong.appendChild( colorElement("specular", nif->get<Color3>( iProp, "Specular Color" ) ));
			else {
				QDomElement specular = doc.createElement("specular");
				specular.appendChild(textureDarkTexture);
				phong.appendChild(specular);
			}
			// shininess
			phong.appendChild( effectElement("shininess" , nif->get<float>( iProp, "Glossiness" ) ) );
			// reflective
			phong.appendChild( colorElement("reflective", Color3(0.0f,0.0f,0.0f)));
			phong.appendChild( effectElement("reflectivity" ,1.0f ) );
			// transparency
			phong.appendChild( colorElement("transparent", Color3(1.0f,1.0f,1.0f)));
			phong.appendChild( effectElement("transparency" , nif->get<float>( iProp, "Alpha" ) ) );
		} else if ( nif->inherits( iProp, "NiTriBasedGeomData" ) ) {
			QDomElement geometry = doc.createElement("geometry");
			geometry.setAttribute("id",QString("nifid_%1-lib").arg(idx));
			geometry.setAttribute("name",QString("%1-lib").arg(nif->get<QString>( iBlock, "Name" ).replace(QRegExp("\\W"),"_") ) );
			libraryGeometries.appendChild(geometry);
			QDomElement mesh = doc.createElement("mesh");
			geometry.appendChild(mesh);

			// Position
			if ( nif->get<bool>( iProp, "Has Vertices") == true) {
				haveVertex = true;
				mesh.appendChild( positionsElement(nif->getArray<Vector3>( iProp, "Vertices" ),idx) );
			}

			// Normals
			if ( nif->get<bool>( iProp, "Has Normals") == true) {
				haveNormal = true;
				mesh.appendChild( normalsElement( nif->getArray<Vector3>( iProp, "Normals" ) , idx ) );
			}

			// UV maps
			int uvCount = (nif->get<int>( iProp, "Num UV Sets") & 63) | (nif->get<int>( iProp, "BS Num UV Sets") & 1);
			QModelIndex iUV = nif->getIndex( iProp, "UV Sets" );
			for(int row=0;row <  uvCount ; row++ ) {
				QVector<Vector2> uvMap = nif->getArray<Vector2>( iUV.child( row, 0 ) );
				mesh.appendChild(uvMapElement(uvMap,idx,row));
				if ( uvMap.size() > 0 )
					haveUV++;
			}

			// vertex color
			if ( nif->get<bool>( iProp, "Has Vertex Colors") == true) {
				mesh.appendChild(colorMapElement(nif->getArray<Color4>( iProp, "Vertex Colors" ),idx) );
				haveColors=true;
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
			if ( haveMaterial )
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
			for(int i=0;i<haveUV;i++) {
//			if ( haveUV > 0 ) {
				// TODO: add multiple UV
				input = doc.createElement("input");
				input.setAttribute("semantic","TEXCOORD");
				input.setAttribute("offset",x++);
				input.setAttribute("source",QString("#nifid_%1-lib-UV%2").arg(idx).arg(i));
				input.setAttribute("set",QString("%1").arg(i));
				triangles.appendChild(input);
			}
			if ( haveColors == true ) {
				input = doc.createElement("input");
				input.setAttribute("semantic","COLOR");
				input.setAttribute("offset",x++);
				input.setAttribute("source",QString("#nifid_%1-lib_colors").arg(idx));
				triangles.appendChild(input);
			}
			// Polygon structure array
			QVector<Triangle> tri;
			QModelIndex iPoints = nif->getIndex( iProp, "Points" );
			if ( iPoints.isValid() ) {
				QList< QVector<quint16> > strips;
				for ( int r = 0; r < nif->rowCount( iPoints ); r++ )
					strips.append( nif->getArray<quint16>( iPoints.child( r, 0 ) ) );
				tri = triangulate( strips );
			} else
				tri = nif->getArray<Triangle>( iProp, "Triangles" );

			QDomElement p = doc.createElement("p");
			QString triText;
			foreach ( Triangle v, tri ) {
				// TODO: add multiple UV
				if ( haveVertex == true )	triText.append( QString("%1 ").arg(v[0]) );
				if ( haveNormal == true )	triText.append( QString("%1 ").arg(v[0]) );
				for(int i=0;i<haveUV;i++)	triText.append( QString("%1 ").arg(v[0]) );
				if ( haveColors == true )	triText.append( QString("%1 ").arg(v[0]) );

				if ( haveVertex == true )	triText.append( QString("%1 ").arg(v[1]) );
				if ( haveNormal == true )	triText.append( QString("%1 ").arg(v[1]) );
				for(int i=0;i<haveUV;i++)	triText.append( QString("%1 ").arg(v[1]) );
				if ( haveColors == true )	triText.append( QString("%1 ").arg(v[1]) );

				if ( haveVertex == true )	triText.append( QString("%1 ").arg(v[2]) );
				if ( haveNormal == true )	triText.append( QString("%1 ").arg(v[2]) );
				for(int i=0;i<haveUV;i++)	triText.append( QString("%1 ").arg(v[2]) );
				if ( haveColors == true )	triText.append( QString("%1 ").arg(v[2]) );
			}
			p.appendChild( doc.createTextNode(triText) );
			triangles.appendChild(p);

			// extra node for model matrix move
			QDomElement node = doc.createElement("node");
			node.setAttribute("id", QString("nifid_%1-matrix").arg(idx) );
			parentNode.appendChild(node);

			// matrix
			node.appendChild(matrixElement(nif->get<Vector3>( iBlock, "Translation" ),nif->get<Matrix>( iBlock, "Rotation" ),nif->get<float>( iBlock, "Scale" )));

			// attach structure and material to node structure
			QDomElement instanceGeometry = doc.createElement("instance_geometry");
			instanceGeometry.setAttribute("url",QString("#nifid_%1-lib").arg(idx));
			node.appendChild(instanceGeometry);
			QDomElement bindMaterial = doc.createElement("bind_material");
			instanceGeometry.appendChild(bindMaterial);
			QDomElement techniqueCommon = doc.createElement("technique_common");
			bindMaterial.appendChild(techniqueCommon);
			QDomElement instanceMaterial = doc.createElement("instance_material");
			instanceMaterial.setAttribute("symbol",QString("material_nifid_%1").arg(idx));
			instanceMaterial.setAttribute("target",QString("#nifid_%1-material").arg(idx));
			techniqueCommon.appendChild(instanceMaterial);
			//	<bind_vertex_input semantic="CHANNEL1" input_semantic="TEXCOORD" input_set="0"/>
			// TODO: check if this is correct way!
			for(int i=0;i<haveUV;i++) {
				QDomElement bind_vertex_input = doc.createElement("bind_vertex_input");
				bind_vertex_input.setAttribute("semantic",QString("UVSET%1").arg(i));
				bind_vertex_input.setAttribute("input_semantic","TEXCOORD");
				bind_vertex_input.setAttribute("input_set",QString("%1").arg(i));
				instanceMaterial.appendChild(bind_vertex_input);
			}
		} else {
			qDebug() << "NOT_USED_PROPERTY:" << nif->getBlockName(iProp);
		}
		if ( effect.isElement() )
			effect.appendChild(profile);
		if ( libraryEffects.isElement() )
			libraryEffects.appendChild(effect);
	}
}

/**
 * Node "tree" looping
 */
void attachNiNode (const NifModel * nif,QDomElement parentNode,int idx) {
	QModelIndex iBlock = nif->getBlock( idx );
	QDomElement node = doc.createElement("node");
	QString nodeName = nif->get<QString>( iBlock, "Name" ).replace(" ","_");
	QString nodeID = QString("nifid_%1_node").arg(idx);
	node.setAttribute("name",  (nodeName.isEmpty()?nodeID:nodeName) );
	node.setAttribute("id", nodeID );

	// matrix
	node.appendChild(matrixElement(nif->get<Vector3>( iBlock, "Translation" ),nif->get<Matrix>( iBlock, "Rotation" ),nif->get<float>( iBlock, "Scale" )));

	// parent attach and new loop
	parentNode.appendChild(node);
	foreach ( int l,nif->getChildLinks(idx) ) {
		QModelIndex iChild = nif->getBlock( l );
		if ( iChild.isValid() ) {
			if ( nif->inherits( iChild, "NiNode" ) )
				attachNiNode(nif,node,l);
			else if ( nif->inherits( iChild, "NiTriBasedGeom") )
				attachNiShape(nif,node,l);
			else {
//				qDebug() << "NO_FUNC:" << type;
			}
		}
	}
}

void exportCol( const NifModel * nif,QFileInfo fileInfo ) {
	QList<int> roots = nif->getRootLinks();
	QString question;
	QSettings settings;
	settings.beginGroup( "import-export" );
	settings.beginGroup( "col" );

	QString fname = QFileDialog::getSaveFileName( 0, tr("Choose a .DAE file for export"), QString("%1%2.dae").arg(settings.value("Path").toString()).arg(fileInfo.baseName()) , "*.dae" );
	if ( fname.isEmpty() )
		return;
	while ( fname.endsWith( ".dae", Qt::CaseInsensitive ) )
		fname = fname.left( fname.length() - 4 );
	QFile fobj( fname + ".dae" );
	if ( ! fobj.open( QIODevice::WriteOnly ) ) {
		qWarning() << "could not open " << fobj.fileName() << " for write access";
		return;
	}
	// clear texture ID list
	textureIds.clear();
	// clear texture name list (if slot based)
	textureNames.clear();
	// clean dom and init global elemets
	doc.clear();
	libraryImages = doc.createElement("library_images");
	libraryMaterials = doc.createElement("library_materials");
	libraryEffects = doc.createElement("library_effects");
	libraryGeometries = doc.createElement("library_geometries");
	// root
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
	contributor.appendChild(textElement("authoring_tool",QString("NifSkope %1").arg(NIFSKOPE_VERSION) ));
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
		// get more if NiNode
		if ( nif->inherits( iBlock, "NiNode" )  )
			attachNiNode(nif,lv,idx);
	}
	QDomElement scene = doc.createElement("scene");
	root.appendChild(scene);
	QDomElement ivl = doc.createElement("instance_visual_scene");
	ivl.setAttribute("url","#NifRootScene");
	scene.appendChild(ivl);
	fobj.write(doc.toString().toAscii());
	settings.setValue( "Path", QString("%1/").arg(QFileInfo(fobj.fileName()).path()) );
	QTextStream sobj( &fobj ); // let's save xml
	fobj.close();
}

