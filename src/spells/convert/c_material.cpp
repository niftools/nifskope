#include "convert.h"

#include <QJsonDocument>
#include <QJsonObject>

// Whether to check if the Material JSON is complete
#define CHECK_COMPLETENESS false

/**
 * @brief The MaterialType enum describes the material type of a shader.
 * BGSM: Lighting material
 * BGEM: Effect material
 */
enum MaterialType
{
    Unknown,
    BGSM,
    BGEM,
};

bool isComplete(QJsonObject & json, MaterialType shaderType) {
    if (!CHECK_COMPLETENESS) {
        return true;
    }

    const QStringList bgemList = {
        // BGEM
        "bAlphaTest",
        "bDecal",
        "bDecalNoFade",
        "bDepthBias",
        "bEnvironmentMapping",
        "bGrayscaleToPaletteColor",
        "bNonOccluder",
        "bRefraction",
        "bScreenSpaceReflections",
        "bTileU",
        "bTileV",
        "bTwoSided",
        "bWetnessControl_ScreenSpaceReflections",
        "bWriteMaskAlbedo",
        "bWriteMaskAmbientOcclusion",
        "bWriteMaskEmissive",
        "bWriteMaskGloss",
        "bWriteMaskNormal",
        "bWriteMaskSpecular",
        "bZBufferTest",
        "bZBufferWrite",
        "eAlphaBlendMode",
        "fAlpha",
        "fAlphaTestRef",
        "fEnvironmentMappingMaskScale",
        "fRefractionFalloff",
        "fRefractionPower",
        "fUOffset",
        "fUScale",
        "fVOffset",
        "fVScale",
        "bBloodEnabled",
        "bEffectLightingEnabled",
        "bEffectPbrSpecular",
        "bFalloffColorEnabled",
        "bFalloffEnabled",
        "bGlowmap",
        "bGrayscaleToPaletteAlpha",
        "bSoftEnabled",
        "cBaseColor",
        "cEmittanceColor",
        "fAdaptativeEmissive_ExposureOffset",
        "fAdaptativeEmissive_FinalExposureMax",
        "fAdaptativeEmissive_FinalExposureMin",
        "fBaseColorScale",
        "fFalloffStartAngle",
        "fFalloffStartOpacity",
        "fFalloffStopAngle",
        "fFalloffStopOpacity",
        "fLightingInfluence",
        "fSoftDepth",
        "iEnvmapMinLOD",
        "sBaseTexture",
        "sEnvmapMaskTexture",
        "sEnvmapTexture",
        "sGlowTexture",
        "sGrayscaleTexture",
        "sLightingTexture",
        "sNormalTexture",
        "sSpecularTexture",
    };

    const QStringList bgsmList = {
        "bAlphaTest",
        "bDecal",
        "bDecalNoFade",
        "bDepthBias",
        "bEnvironmentMapping",
        "bGrayscaleToPaletteColor",
        "bNonOccluder",
        "bRefraction",
        "bScreenSpaceReflections",
        "bTileU",
        "bTileV",
        "bTwoSided",
        "bWetnessControl_ScreenSpaceReflections",
        "bWriteMaskAlbedo",
        "bWriteMaskAmbientOcclusion",
        "bWriteMaskEmissive",
        "bWriteMaskGloss",
        "bWriteMaskNormal",
        "bWriteMaskSpecular",
        "bZBufferTest",
        "bZBufferWrite",
        "eAlphaBlendMode",
        "fAlpha",
        "fAlphaTestRef",
        "fEnvironmentMappingMaskScale",
        "fRefractionFalloff",
        "fRefractionPower",
        "fUOffset",
        "fUScale",
        "fVOffset",
        "fVScale",
        "UnkInt1",
        "bAnisoLighting",
        "bAssumeShadowmask",
        "bBackLighting",
        "bCastShadows",
        "bCustomPorosity",
        "bDissolveFade",
        "bEmitEnabled",
        "bEnableEditorAlphaRef",
        "bEnvironmentMappingEye",
        "bEnvironmentMappingWindow",
        "bExternalEmittance",
        "bFacegen",
        "bGlowmap",
        "bHair",
        "bHideSecret",
        "bModelSpaceNormals",
        "bPBR",
        "bReceiveShadows",
        "bRimLighting",
        "bSkewSpecularAlpha",
        "bSkinTint",
        "bSpecularEnabled",
        "bSubsurfaceLighting",
        "bTerrain",
        "bTessellate",
        "bTranslucency",
        "bTranslucencyMixAlbedoWithSubsurfaceColor",
        "bTranslucencyThickObject",
        "bTree",
        "bUseAdaptativeEmissive",
        "cEmittanceColor",
        "cHairTintColor",
        "cSpecularColor",
        "cTranslucencySubsurfaceColor",
        "fAdaptativeEmissive_ExposureOffset",
        "fAdaptativeEmissive_FinalExposureMax",
        "fAdaptativeEmissive_FinalExposureMin",
        "fBackLightPower",
        "fDisplacementTextureBias",
        "fDisplacementTextureScale",
        "fEmittanceMult",
        "fFresnelPower",
        "fGrayscaleToPaletteScale",
        "fLumEmittance",
        "fPorosityValue",
        "fRimPower",
        "fSmoothness",
        "fSpecularMult",
        "fSubsurfaceLightingRolloff",
        "fTerrainRotationAngle",
        "fTerrainThresholdFalloff",
        "fTerrainTilingDistance",
        "fTessellationBaseFactor",
        "fTessellationFadeDistance",
        "fTessellationPnScale",
        "fTranslucencyTransmissiveScale",
        "fTranslucencyTurbulence",
        "fWetnessControl_EnvMapScale",
        "fWetnessControl_FresnelPower",
        "fWetnessControl_Metalness",
        "fWetnessControl_SpecMinvar",
        "fWetnessControl_SpecPowerScale",
        "fWetnessControl_SpecScale",
        "sDiffuseTexture",
        "sDisplacementTexture",
        "sDistanceFieldAlphaTexture",
        "sEnvmapTexture",
        "sFlowTexture",
        "sGlowTexture",
        "sGreyscaleTexture",
        "sInnerLayerTexture",
        "sLightingTexture",
        "sNormalTexture",
        "sRootMaterialPath",
        "sSmoothSpecTexture",
        "sSpecularTexture",
        "sWrinklesTexture",
    };

    bool result = true;

    auto lambda = [&](const QString & val) {
        if (!json.contains(val)) {
            qDebug() << "JSON missing value:" << val;

            result = false;
        }
    };

    if (shaderType == BGSM) {
        for (const QString & val : bgsmList) {
            lambda(val);
        }
    } else if (shaderType == BGEM) {
        for (const QString & val : bgemList) {
            lambda(val);
        }
    }

    const QStringList * list = nullptr;

    if (shaderType == BGSM) {
        list = &bgsmList;
    } else if (shaderType == BGEM) {
        list = &bgemList;
    }

    if (list) {
        for (const QString & key : json.keys()) {
            if (!list->contains(key)) {
                qDebug() << __FUNCTION__ << "Unnecessary key" << key << "in" << shaderType;
            }
        }
    }

    return result;
}

class FlagsChecker
{
public:
    FlagsChecker(uint flags1, uint flags2, Converter * c, QJsonObject & json)
        : flags1(flags1),
          flags2(flags2),
          c(c),
          json(json)
    { }

    bool isSet1(const QString & optionName) {
        return c->bsShaderFlagsIsSet(flags1, "Fallout4ShaderPropertyFlags1", optionName);
    }

    bool isSet2(const QString & optionName) {
        return c->bsShaderFlagsIsSet(flags2, "Fallout4ShaderPropertyFlags2", optionName);
    }

    void toJson1(const QString & optionName, const QString & jsonKey) {
        json.insert(jsonKey, isSet1(optionName));
    }

    void toJson2(const QString & optionName, const QString & jsonKey) {
        json.insert(jsonKey, isSet2(optionName));
    }
private:
    uint flags1;
    uint flags2;
    Converter * c;
    QJsonObject & json;
};

QString removeTextureRoot(const QString & sTexture) {
    if (sTexture.left(QString("textures\\").length()).compare("textures\\", Qt::CaseInsensitive) == 0) {
        return sTexture.right(sTexture.length() - QString("Textures\\").length());
    }

    return sTexture;
}

void insertMatTexture(QJsonObject & json, const QString & texture, const QString & jsonBKey, const QString & jsonTKey) {
    if (texture != "") {
        json.insert(jsonBKey, true);
        json.insert(jsonTKey, removeTextureRoot(texture));
    } else {
        json.insert(jsonBKey, false);
        json.insert(jsonTKey, "");
    }
}

void matTextures(Converter * c, NifModel * nifDst, MaterialType shaderType, QModelIndex iShader, QJsonObject & json) {
    QString sDiffuse = "", sEnvMap = "", sNormal = "";

    // Determine texture paths

    if (shaderType == BGSM) {
        QModelIndex iTextureSet = c->getIndexDst(c->getBlockDst(iShader, "Texture Set"), "Textures");

        if (iTextureSet.isValid() && nifDst->rowCount(iTextureSet) >= 10) {
            QVector<QString> textures = nifDst->getArray<QString>(iTextureSet);

            sDiffuse = textures[0];
            sNormal = textures[1];
            sEnvMap = textures[4];
        }

        json.insert("sGreyscaleTexture",          "");

        json.insert("sDiffuseTexture",            sDiffuse);

        json.insert("bSpecularEnabled",                          false);

        // TODO:
        json.insert("sFlowTexture",               "");
        json.insert("sInnerLayerTexture",         "");
        json.insert("sDistanceFieldAlphaTexture", "");
        json.insert("sDisplacementTexture",       "");
        json.insert("sRootMaterialPath",          "");
        json.insert("sSmoothSpecTexture",         "");
        json.insert("sWrinklesTexture",           "");
    } else if (shaderType == BGEM) {
        sEnvMap = nifDst->get<QString>(iShader, "Env Map Texture");
        sNormal = nifDst->get<QString>(iShader, "Normal Texture");

        json.insert("sBaseTexture",       nifDst->get<QString>(iShader, "Source Texture"));
        json.insert("sEnvmapMaskTexture", nifDst->get<QString>(iShader, "Env Mask Texture"));
        json.insert("sGrayscaleTexture",  nifDst->get<QString>(iShader, "Greyscale Texture"));
    }

    // Insert Shared JSON values

    insertMatTexture(json, "",         "bGlowmap",                "sGlowTexture");

    json.insert("sEnvmapTexture",    sEnvMap);
    json.insert("sNormalTexture",    sNormal);

    // TODO:

    json.insert("sLightingTexture",           "");
    json.insert("sSpecularTexture",           "");
}

void matFlags(
        NifModel * nifDst,
        MaterialType matType,
        QModelIndex iAlpha,
        QJsonObject & json,
        FlagsChecker & flags) {
    json.insert(
            "bAlphaTest",
            (iAlpha.isValid() &&
            nifDst->get<int>(iAlpha, "Flags") & 1 << 9) ||
            flags.isSet2("Alpha_Test"));

    if (matType == BGSM) {
        flags.toJson2("Anisotropic_Lighting",     "bAnisoLighting");
        flags.toJson1("Cast_Shadows",             "bCastShadows");
        flags.toJson1("Eye_Environment_Mapping",  "bEnvironmentMappingEye");
        flags.toJson1("External_Emittance",       "bExternalEmittance");
        flags.toJson1("Hair",                     "bHair");
        flags.toJson1("Localmap_Hide_Secret",     "bHideSecret");
        flags.toJson1("Model_Space_Normals",      "bModelSpaceNormals");
        flags.toJson2("Tint",                     "bSkinTint");
        flags.toJson2("Tree_Anim",                "bTree");

        json.insert("bAssumeShadowmask",                         false);
        json.insert("bBackLighting",                             false);
        json.insert("bCustomPorosity",                           false);
        json.insert("bDissolveFade",                             false);
        json.insert("bEmitEnabled",                              false);
        json.insert("bEnableEditorAlphaRef",                     false);
        json.insert("bEnvironmentMappingWindow",                 false);
        json.insert("bFacegen",                                  false);
        json.insert("bPBR",                                      false);
        json.insert("bReceiveShadows",                           false);
        json.insert("bRimLighting",                              false);
        json.insert("bSkewSpecularAlpha",                        false);
        json.insert("bSubsurfaceLighting",                       false);
        json.insert("bTerrain",                                  false);
        json.insert("bTessellate",                               false);
        json.insert("bTranslucency",                             false);
        json.insert("bTranslucencyMixAlbedoWithSubsurfaceColor", false);
        json.insert("bTranslucencyThickObject",                  false);
        json.insert("bUseAdaptativeEmissive",                    false);
    } else if (matType == BGEM) {
        flags.toJson1("Use_Falloff",              "bFalloffEnabled");
        flags.toJson2("Weapon_Blood",             "bBloodEnabled");
        flags.toJson2("Effect_Lighting",          "bEffectLightingEnabled");
        flags.toJson1("GreyscaleToPalette_Alpha", "bGrayscaleToPaletteAlpha");
        flags.toJson1("Soft_Effect",              "bSoftEnabled");

        json.insert("bFalloffColorEnabled",                      false);
        json.insert("bEffectPbrSpecular",                        false);
    }

    flags.toJson1("Decal",                    "bDecal");
    flags.toJson2("No_Fade",                  "bDecalNoFade");
    flags.toJson1("Environment_Mapping",      "bEnvironmentMapping");
    flags.toJson1("GreyscaleToPalette_Color", "bGrayscaleToPaletteColor");
    flags.toJson1("Refraction",               "bRefraction");
    flags.toJson2("Double_Sided",             "bTwoSided");
    flags.toJson1("ZBuffer_Test",             "bZBufferTest");
    flags.toJson2("ZBuffer_Write",            "bZBufferWrite");


    // TODO:
    json.insert("bTileU",                                    true);
    json.insert("bTileV",                                    true);
    json.insert("bScreenSpaceReflections",                   false);
    json.insert("bNonOccluder",                              false);
    json.insert("bDepthBias",                                false);
    json.insert("bWetnessControl_ScreenSpaceReflections",    false);
    json.insert("bWriteMaskAlbedo",                          false);
    json.insert("bWriteMaskAmbientOcclusion",                false);
    json.insert("bWriteMaskEmissive",                        false);
    json.insert("bWriteMaskGloss",                           false);
    json.insert("bWriteMaskNormal",                          false);
    json.insert("bWriteMaskSpecular",                        false);
}

QString colorFloatToHex(float f) {
    int val = int(double(f) * 0xff);

    QString result = QString::number(val, 16);

    if (result.length() == 1) {
        result = "0" + result;
    }

    return result;
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

void matColors(NifModel * nifDst, MaterialType shaderType, QModelIndex iShader, QJsonObject & json) {
    if (shaderType == BGSM) {
        json.insert("cEmittanceColor", color3ToString(nifDst->get<Color3>(iShader, "Emissive Color")));
        json.insert("cSpecularColor", color3ToString(nifDst->get<Color3>(iShader, "Specular Color")));
        json.insert("cHairTintColor",               "#808080");
        json.insert("cTranslucencySubsurfaceColor", "#000000");
        json.insert("cSpecularColor",               "#ffffff");
    } else if (shaderType == BGEM) {
        json.insert("cEmittanceColor", color4ToString(nifDst->get<Color4>(iShader, "Emissive Color")));
    }
}

class ShaderValues
{
public:
    ShaderValues(QJsonObject & json, NifModel * nifDst, QModelIndex iShader) : json(json), nifDst(nifDst), iShader(iShader) {}

    double getFloat(const QString & name) {
        return double(nifDst->get<float>(iShader, name));
    }

    void addFloat(const QString & key, const QString & valName) {
        json.insert(key, getFloat(valName));
    }
private:
    QJsonObject & json;
    NifModel * nifDst;
    QModelIndex iShader;
};

void matFloats(
        QJsonObject & json,
        NifModel * nifDst,
        QModelIndex iShader,
        MaterialType shaderType,
        QModelIndex iAlpha) {
    double fAlpha = 1.0;
    double fAlphaTestRef = 128.0;
    double fFresnelPower = 0.0;
    double fEnvironmentMappingMaskScale = 1.0;
    double fBackLightPower = 0.0;
    double fGrayscaleToPaletteScale = 1.0;
    double fRefractionPower = 0.0;
    double fRimPower = 0.0;
    double fSmoothness = 0.0;
    double fSpecularMult = 1.0;
    double fSubsurfaceLightingRolloff = 0.0;
    double fWetnessControl_EnvMapScale = -1.0;
    double fWetnessControl_FresnelPower = -1.0;
    double fWetnessControl_Metalness = -1.0;
    double fWetnessControl_SpecMinvar = -1.0;
    double fWetnessControl_SpecPowerScale = -1.0;
    double fWetnessControl_SpecScale = -1.0;

    ShaderValues sv(json, nifDst, iShader);

    if (shaderType == BGSM) {
        json.insert("UnkInt1", 0);

        fAlpha = double(nifDst->get<float>(iShader, "Alpha"));
        fFresnelPower = double(nifDst->get<float>(iShader, "Fresnel Power"));
        fBackLightPower = double(nifDst->get<float>(iShader, "Backlight Power"));
        fGrayscaleToPaletteScale = double(nifDst->get<float>(iShader, "Grayscale to Palette Scale"));
        fRefractionPower = double(nifDst->get<float>(iShader, "Refraction Strength"));

        if (!isinf(nifDst->get<float>(iShader, "Rimlight Power"))) {
            fRimPower = double(nifDst->get<float>(iShader, "Rimlight Power"));
        }

        fSmoothness = double(nifDst->get<float>(iShader, "Smoothness"));
        fSpecularMult = double(nifDst->get<float>(iShader, "Specular Strength"));
        fSubsurfaceLightingRolloff = double(nifDst->get<float>(iShader, "Subsurface Rolloff"));
        fWetnessControl_EnvMapScale = double(nifDst->get<float>(iShader, "Wetness Env Map Scale"));
        fWetnessControl_FresnelPower = double(nifDst->get<float>(iShader, "Wetness Fresnel Power"));
        fWetnessControl_Metalness = double(nifDst->get<float>(iShader, "Wetness Metalness"));
        fWetnessControl_SpecMinvar = double(nifDst->get<float>(iShader, "Wetness Min Var"));
        fWetnessControl_SpecPowerScale = double(nifDst->get<float>(iShader, "Wetness Spec Power"));
        fWetnessControl_SpecScale = double(nifDst->get<float>(iShader, "Wetness Spec Scale"));

        json.insert("fBackLightPower",                      fBackLightPower);
        json.insert("fDisplacementTextureBias",             0.0);
        json.insert("fDisplacementTextureScale",            1.0);
        json.insert("fEmittanceMult",                       double(nifDst->get<float>(iShader, "Emissive Multiple")));
        json.insert("fFresnelPower",                        fFresnelPower);
        json.insert("fGrayscaleToPaletteScale",             fGrayscaleToPaletteScale);
        json.insert("fLumEmittance",                        0.0);
        json.insert("fPorosityValue",                       0.0);
        json.insert("fRimPower",                            fRimPower);
        json.insert("fSmoothness",                          fSmoothness);
        json.insert("fSpecularMult",                        fSpecularMult);
        json.insert("fSubsurfaceLightingRolloff",           fSubsurfaceLightingRolloff);
        json.insert("fWetnessControl_EnvMapScale",          fWetnessControl_EnvMapScale);
        json.insert("fWetnessControl_FresnelPower",         fWetnessControl_FresnelPower);
        json.insert("fWetnessControl_Metalness",            fWetnessControl_Metalness);
        json.insert("fWetnessControl_SpecMinvar",           fWetnessControl_SpecMinvar);
        json.insert("fWetnessControl_SpecPowerScale",       fWetnessControl_SpecPowerScale);
        json.insert("fWetnessControl_SpecScale",            fWetnessControl_SpecScale);
        json.insert("fTerrainRotationAngle",                0.0);
        json.insert("fTerrainThresholdFalloff",             0.0);
        json.insert("fTerrainTilingDistance",               0.0);
        json.insert("fTessellationBaseFactor",              0.0);
        json.insert("fTessellationFadeDistance",            0.0);
        json.insert("fTessellationPnScale",                 0.0);
        json.insert("fTranslucencyTransmissiveScale",       0.0);
        json.insert("fTranslucencyTurbulence",              0.0);
    } else if (shaderType == BGEM) {
        fEnvironmentMappingMaskScale = double(nifDst->get<float>(iShader, "Environment Map Scale"));

        sv.addFloat("fFalloffStartAngle",   "Falloff Start Angle");
        sv.addFloat("fFalloffStopAngle",    "Falloff Stop Angle");
        sv.addFloat("fFalloffStartOpacity", "Falloff Start Opacity");
        sv.addFloat("fFalloffStopOpacity",  "Falloff Stop Opacity");
        sv.addFloat("fSoftDepth",           "Soft Falloff Depth");

        json.insert("fLightingInfluence", double(nifDst->get<int>(iShader, "Lighting Influence")));
        json.insert("iEnvmapMinLOD",      nifDst->get<int>(iShader, "Env Map Min LOD"));

        json.insert("cBaseColor",      "#ffffff");
        json.insert("fBaseColorScale", 1.0);
    }

    if (iAlpha.isValid()) {
        fAlphaTestRef = double(nifDst->get<int>(iAlpha, "Threshold"));
    }

    json.insert("fAdaptativeEmissive_ExposureOffset",   0.0);
    json.insert("fAdaptativeEmissive_FinalExposureMax", 0.0);
    json.insert("fAdaptativeEmissive_FinalExposureMin", 0.0);
    json.insert("fRefractionFalloff",                   0.0);
    json.insert("fAlpha",                               fAlpha);
    json.insert("fEnvironmentMappingMaskScale",         fEnvironmentMappingMaskScale);
    json.insert("fRefractionPower",                     fRefractionPower);
    json.insert("fUOffset",                             double(nifDst->get<Vector2>(iShader, "UV Offset")[0]));
    json.insert("fVOffset",                             double(nifDst->get<Vector2>(iShader, "UV Offset")[1]));
    json.insert("fUScale",                              double(nifDst->get<Vector2>(iShader, "UV Scale")[0]));
    json.insert("fVScale",                              double(nifDst->get<Vector2>(iShader, "UV Scale")[1]));

    json.insert("fAlphaTestRef",                        fAlphaTestRef);
}

void matBlending(QJsonObject & json, NifModel * nifDst, QModelIndex iAlpha) {
    if (nifDst->get<int>(iAlpha, "Flags") & 1) {
        json.insert("eAlphaBlendMode", "Standard");
    } else {
        json.insert("eAlphaBlendMode", "None");
    }
}

QJsonObject Converter::makeMaterialFile(QModelIndex iShader, QModelIndex iAlpha) {
    if (!iShader.isValid() || bLODLandscape || bLODBuilding) {
        return QJsonObject();
    }

    const QString shaderTypeName = nifDst->getBlockName(iShader);

    if (
            shaderTypeName != "BSEffectShaderProperty" &&
            shaderTypeName != "BSLightingShaderProperty" &&
            shaderTypeName != "BSSkyShaderProperty" &&
            shaderTypeName != "BSWaterShaderProperty") {
        qDebug() << __FUNCTION__ << "Unknown shader type" << shaderTypeName;

        conversionResult = false;

        return QJsonObject();
    }

    if (shaderTypeName == "BSSkyShaderProperty" || shaderTypeName == "BSWaterShaderProperty") {
        return QJsonObject();
    }

    MaterialType matType = Unknown;

    if (shaderTypeName == "BSEffectShaderProperty") {
        matType = BGEM;
    } else if (shaderTypeName == "BSLightingShaderProperty") {
        matType = BGSM;
    }

    if (matType == Unknown) {
        conversionResult = false;

        qDebug() << __FUNCTION__ << "Unknown mat type";

        return QJsonObject();
    }

    uint flags1 = nifDst->get<uint>(iShader, "Shader Flags 1");
    uint flags2 = nifDst->get<uint>(iShader, "Shader Flags 2");
    QJsonObject json;
    FlagsChecker flags(flags1, flags2, this, json);

    matTextures(this, nifDst, matType, iShader, json);
    matFlags(nifDst, matType, iAlpha, json, flags);
    matColors(nifDst, matType, iShader, json);
    matFloats(json, nifDst, iShader, matType, iAlpha);
    matBlending(json, nifDst, iAlpha);

    isComplete(json, matType);

    return json;
}
