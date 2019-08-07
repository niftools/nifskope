#include "convert.h"

#include <QJsonDocument>
#include <QJsonObject>

class FlagsChecker
{
public:
    FlagsChecker(uint flags1, uint flags2, Converter * c, QJsonObject & json)
        : flags1(flags1),
          flags2(flags2),
          c(c),
          json(json)
    { }

    bool isSet(const QString & optionName) {
        return c->bsShaderFlags1IsSet(flags1, optionName) || c->bsShaderFlags2IsSet(flags2, optionName);
    }

    void toJson(const QString & optionName, const QString & jsonKey) {
        json.insert(jsonKey, isSet(optionName));
    }
private:
    uint flags1;
    uint flags2;
    Converter * c;
    QJsonObject & json;
};

void insertMatTexture(QJsonObject & json, const QString & texture, const QString & jsonBKey, const QString & jsonTKey) {
    if (texture != "") {
        json.insert(jsonBKey, true);
        json.insert(jsonTKey, texture);
    } else {
        json.insert(jsonBKey, false);
    }
}

void Converter::matTextures(const QString & shaderType, QModelIndex iShader, QJsonObject & json) {
    if (shaderType == "BSLightingShaderProperty") {
        QModelIndex iTextureSet = getBlockDst(iShader, "Texture Set");

        if (!iTextureSet.isValid() || nifDst->rowCount(iTextureSet) < 10) {
            json.insert("bDiffuseEnable",          false);
            json.insert("bEnvmapEnable",           false);
            json.insert("bGlowmap",                false);
            json.insert("bGlowEnable",             false);
            json.insert("bNormalEnable",           false);
            json.insert("bWrinklesEnable",         false);
            json.insert("bSmoothSpecEnable",       false);
            json.insert("bGrayscaleTextureEnable", false);
        } else {
            QVector<QString> textures = nifDst->getArray<QString>(iTextureSet, "Textures");

            // TODO: Find remaining textures
            insertMatTexture(json, textures[0], "bDiffuseEnable", "sDiffuseTexture");
            insertMatTexture(json, textures[1], "bNormalEnable", "sNormalTexture");
            insertMatTexture(json, textures[4], "bEnvmapEnable", "sEnvmapTexture");
            json.insert("bGlowmap",                false);
            json.insert("bGlowEnable",             false);
            json.insert("bWrinklesEnable",         false);
            json.insert("bSmoothSpecEnable",       false);
            json.insert("bGrayscaleTextureEnable", false);
        }
    } else if (shaderType == "BSEffectShaderProperty") {
        insertMatTexture(json, nifDst->get<QString>(iShader, "Source Texture"), "bDiffuseEnable", "sDiffuseTexture");
        insertMatTexture(json, nifDst->get<QString>(iShader, "Env Map Texture"), "bEnvmapEnable", "sEnvmapTexture");
        insertMatTexture(json, nifDst->get<QString>(iShader, "Normal Texture"), "bNormalEnable", "sNormalTexture");
        insertMatTexture(
                json, nifDst->get<QString>(iShader, "Greyscale Texture"),
                "bGrayscaleTextureEnable",
                "sGreyscaleTexture");
        json.insert("bGlowmap",                false);
        json.insert("bGlowEnable",             false);
        json.insert("bWrinklesEnable",         false);
        json.insert("bSmoothSpecEnable",       false);
    }
}

void matFlags(
        NifModel * nifDst,
        Converter * c,
        QModelIndex iAlpha,
        QJsonObject & json,
        FlagsChecker & flags,
        uint flags2) {
    json.insert(
            "bAlphaTest",
            (iAlpha.isValid() &&
            nifDst->get<int>(iAlpha, "Flags") | 1 << 9) ||
            c->bsShaderFlags2IsSet(flags2, "Alpha_Test"));

    flags.toJson("Anisotropic_Lighting",     "bAnisoLighting");
    flags.toJson("Cast_Shadows",             "bCastShadows");
    flags.toJson("Decal",                    "bDecal");
    flags.toJson("No_Fade",                  "bDecalNoFade");
    flags.toJson("Environment_Mapping",      "bEnvironmentMapping");
    flags.toJson("Eye_Environment_Mapping",  "bEnvironmentMappingEye");
    flags.toJson("External_Emittance",       "bExternalEmittance");
    flags.toJson("GreyscaleToPalette_Color", "bGrayscaleToPaletteColor");
    flags.toJson("Hair",                     "bHair");
    flags.toJson("Localmap_Hide_Secret",     "bHideSecret");
    flags.toJson("Model_Space_Normals",      "bModelSpaceNormals");
    flags.toJson("Refraction",               "bRefraction");
    flags.toJson("Tint",                     "bSkinTint");
    flags.toJson("Tree_Anim",                "bTree");
    flags.toJson("Double_Sided",             "bTwoSided");
    flags.toJson("ZBuffer_Test",             "bZBufferTest");
    flags.toJson("ZBuffer_Write",            "bZBufferWrite");

    // TODO:
    json.insert("bSubsurfaceLighting",          false);
    json.insert("bTileU",                       false);
    json.insert("bTileV",                       false);
    json.insert("bRefractionFalloff",           false);
    json.insert("bScreenSpaceReflections",      false);
    json.insert("bNonOccluder",                 false);
    json.insert("bEnvironmentMappingLightFade", false);
    json.insert("bEnvironmentMappingWindow",    false);
    json.insert("bDissolveFade",                false);
    json.insert("bEnableEditorAlphaRef",        false);
    json.insert("bEmitEnabled",                 false);
    json.insert("bSpecularEnabled",             false);
}

QString colorFloatToHex(float f) {
    return QString::number(double(f) * 0xff);
}

QString color3ToString(Color3 color) {
    return
            "#" +
            colorFloatToHex(color.red()) +
            colorFloatToHex(color.blue()) +
            colorFloatToHex(color.green());
}

QString color4ToString(Color4 color) {
    return
            "#" +
            colorFloatToHex(color.red()) +
            colorFloatToHex(color.blue()) +
            colorFloatToHex(color.green());
}

void matColors(NifModel * nifDst, const QString & shaderType, QModelIndex iShader, QJsonObject & json) {
    if (shaderType == "BSLightingShaderProperty") {
        json.insert("cEmittanceColor", color3ToString(nifDst->get<Color3>(iShader, "Emissive Color")));
        json.insert("cSpecularColor", color3ToString(nifDst->get<Color3>(iShader, "Specular Color")));
    } else if (shaderType == "BSEffectShaderProperty") {
        json.insert("cEmittanceColor", color4ToString(nifDst->get<Color4>(iShader, "Emissive Color")));
        json.insert("cSpecularColor", "#ffffff");
    }

    json.insert("cHairTintColor", "#808080");
}

void Converter::makeMaterialFile(QModelIndex iShader, QModelIndex iAlpha) {
    if (!iShader.isValid() || bLODLandscape || bLODBuilding) {
        return;
    }

    const QString shaderType = nifDst->getBlockName(iShader);

    if (
            shaderType != "BSEffectShaderProperty" &&
            shaderType != "BSLightingShaderProperty" &&
            shaderType != "BSSkyShaderProperty" &&
            shaderType != "BSWaterShaderProperty") {
        qDebug() << __FUNCTION__ << "Unknown shader type" << shaderType;

        conversionResult = false;

        return;
    }

    if (shaderType == "BSSkyShaderProperty" || shaderType == "BSWaterShaderProperty") {
        return;
    }

    uint flags1 = nifDst->get<uint>(iShader, "Shader Flags 1");
    uint flags2 = nifDst->get<uint>(iShader, "Shader Flags 2");
    QJsonObject json;
    FlagsChecker flags(flags1, flags2, this, json);

    matTextures(shaderType, iShader, json);
    matFlags(nifDst, this, iAlpha, json, flags, flags2);
}
