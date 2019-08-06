#include "convert.h"

QModelIndex Converter::niSkinInstance(QModelIndex iBSTriShapeDst, QModelIndex iShaderPropertyDst, QModelIndex iSrc) {
    if (!iSrc.isValid()) {
        return QModelIndex();
    }

    QModelIndex iDst = nifDst->insertNiBlock("BSSkin::Instance");

    Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

    c.ignore("Data");
    c.ignore("Skin Partition");

    reLink(iDst, iSrc, "Skeleton Root");
    c.processed("Skeleton Root");

    c.copyValue("Num Bones");

    if (nifSrc->get<int>(iSrc, "Num Bones") > 0) {
        reLinkArray(iDst, iSrc, "Bones");
        c.processed(iSrc, "Bones");
    }

    setLink(iBSTriShapeDst, "Skin", iDst);

    BSVertexDesc vertexFlagsDst = nifDst->get<BSVertexDesc>(iBSTriShapeDst, "Vertex Desc");
    vertexFlagsDst.SetFlag(VertexFlags::VF_SKINNED);
    vertexFlagsDst.ResetAttributeOffsets(nifDst->getUserVersion2());

    bool ok;

    // Set shader and vertex flags.
    nifDst->set<BSVertexDesc>(iBSTriShapeDst, "Vertex Desc", vertexFlagsDst);
    nifDst->set<uint>(iShaderPropertyDst, "Shader Flags 1",
                      nifDst->get<uint>(iShaderPropertyDst, "Shader Flags 1") |
                      NifValue::enumOptionValue("Fallout4ShaderPropertyFlags1", "Skinned", &ok));

    if (!ok) {
        qDebug() << __FUNCTION__ << "Enum option not found";

        conversionResult = false;
    }

    ushort numVertices = nifDst->get<ushort>(iBSTriShapeDst, "Num Vertices");
    uint numTriangles = nifDst->get<uint>(iBSTriShapeDst, "Num Triangles");

    nifDst->updateArray(iBSTriShapeDst, "Vertex Data");
    nifDst->set<uint>(iBSTriShapeDst, "Data Size", vertexFlagsDst.GetVertexSize() * numVertices + numTriangles * 6);

    setLink(iDst, "Data", niSkinData(iBSTriShapeDst, getBlockSrc(iSrc, "Data")));
    // TODO: Set bone weights in vertex data

    // Only used for optimization?
    ignoreBlock(iSrc, "Skin Partition", false);

    if (nifSrc->getBlockName(iSrc) == "BSDismemberSkinInstance") {
        c.ignore("Num Partitions");

        if (nifSrc->get<int>(iSrc, "Num Partitions") > 0) {
            c.ignore(getIndexSrc(getIndexSrc(iSrc, "Partitions").child(0, 0), "Part Flag"));
            c.ignore(getIndexSrc(getIndexSrc(iSrc, "Partitions").child(0, 0), "Body Part"));
        }
    }

    setHandled(iDst, iSrc);

    return iDst;
}

void Converter::niSkinDataSkinTransform(QModelIndex iBoneDst, QModelIndex iSkinTransformSrc, QModelIndex iSkinTransformGlobalSrc) {
    Matrix skinRotation = nifSrc->get<Matrix>(iSkinTransformGlobalSrc, "Rotation");
    Vector3 skinTranslation = nifSrc->get<Vector3>(iSkinTransformGlobalSrc, "Translation");
    float skinScale = nifSrc->get<float>(iSkinTransformGlobalSrc, "Scale");

    Matrix rotation = nifSrc->get<Matrix>(iSkinTransformSrc, "Rotation");
    for (uint r = 0; r < 3; r++) {
        for (uint c = 0; c < 3; c++) {
            rotation(r, c) += skinRotation(r, c);
        }
    }

    nifDst->set<Matrix>(iBoneDst, "Rotation", rotation);
    nifDst->set<Vector3>(iBoneDst, "Translation", nifSrc->get<Vector3>(iSkinTransformSrc, "Translation") + skinTranslation);
    nifDst->set<float>(iBoneDst, "Scale", nifSrc->get<float>(iSkinTransformSrc, "Scale") * skinScale);
}

QModelIndex Converter::niSkinData(QModelIndex iBSTriShapeDst, QModelIndex iSrc) {
    QModelIndex iDst = nifDst->insertNiBlock("BSSkin::BoneData");

    Copier c = Copier(iDst, iSrc, nifDst, nifSrc);

    c.copyValue("Num Bones");

    QModelIndex iBoneListSrc = getIndexSrc(iSrc, "Bone List");
    QModelIndex iBoneListDst = getIndexDst(iDst, "Bone List");

    // Processed by niSkinDataSkinTransform()
    QModelIndex iSkinTransformGlobalSrc = getIndexSrc(iSrc, "Skin Transform");
    c.ignore(iSkinTransformGlobalSrc, "Rotation");
    c.ignore(iSkinTransformGlobalSrc, "Translation");
    c.ignore(iSkinTransformGlobalSrc, "Scale");


    nifDst->updateArray(iBoneListDst);

    for (int i = 0; i < nifSrc->rowCount(iBoneListSrc); i++) {
        QModelIndex iBoneDst = iBoneListDst.child(i, 0);
        QModelIndex iBoneSrc = iBoneListSrc.child(i, 0);
        QModelIndex iSkinTransformSrc = getIndexSrc(iBoneSrc, "Skin Transform");
        QModelIndex iBoundingSphereDst = getIndexDst(iBoneDst, "Bounding Sphere");

        c.copyValue(iBoneDst, iSkinTransformSrc, "Rotation");
        c.copyValue(iBoneDst, iSkinTransformSrc, "Translation");
        c.copyValue(iBoneDst, iSkinTransformSrc, "Scale");
        c.copyValue(iBoundingSphereDst, iBoneSrc, "Center", "Bounding Sphere Offset");
        c.copyValue(iBoundingSphereDst, iBoneSrc, "Radius", "Bounding Sphere Radius");
    }

    if (nifSrc->get<bool>(iSrc, "Has Vertex Weights")) {
        QModelIndex iVertexDataArrayDst = getIndexDst(iBSTriShapeDst, "Vertex Data");
        QMap<ushort, int> weightCounts;

        QModelIndex iBoneListArraySrc = getIndexSrc(iSrc, "Bone List");

        for (int i = 0; i < nifSrc->rowCount(iBoneListArraySrc); i++) {
            QModelIndex iBoneSrc = iBoneListArraySrc.child(i, 0);
            QModelIndex iVertexWeightsArraySrc = getIndexSrc(iBoneSrc, "Vertex Weights");

            for (int j = 0; j < nifSrc->rowCount(iVertexWeightsArraySrc); j++) {
                QModelIndex iVertexWeightSrc = iVertexWeightsArraySrc.child(j, 0);
                ushort vertexIndex = nifSrc->get<ushort>(iVertexWeightSrc, "Index");
                float vertexWeight = nifSrc->get<float>(iVertexWeightSrc, "Weight");

                if (weightCounts[vertexIndex] >= 4) {
                    qDebug() << "Too many boneweights for one vertex. Blocknr.:"
                             << nifSrc->getBlockNumber(iSrc)
                             << "Vertex"
                             << vertexIndex;

//                    conversionResult = false;

                    continue;
                }

                QModelIndex iVertexDataDst = iVertexDataArrayDst.child(vertexIndex, 0);
                QModelIndex iBoneWeightsDst = getIndexDst(iVertexDataDst, "Bone Weights");
                QModelIndex iBoneIndicesDst = getIndexDst(iVertexDataDst, "Bone Indices");

                nifDst->updateArray(iBoneWeightsDst);
                nifDst->updateArray(iBoneIndicesDst);

                int weightIndex = weightCounts[vertexIndex];

                if (!iBoneWeightsDst.child(weightIndex, 0).isValid()) {
                    qDebug() << __FUNCTION__ << "Bone Weights not found";

                    conversionResult = false;

                    break;
                }

                if (!iBoneIndicesDst.child(weightIndex, 0).isValid()) {
                    qDebug() << __FUNCTION__ << "Bone Indices not found";

                    conversionResult = false;

                    break;
                }

                nifDst->set<ushort>(iBoneIndicesDst.child(weightIndex, 0), ushort(i));
                nifDst->set<float>(iBoneWeightsDst.child(weightIndex, 0), vertexWeight);

                weightCounts[vertexIndex]++;
            }
        }

        if (nifSrc->rowCount(iBoneListArraySrc) > 0) {
            c.processed(iBoneListArraySrc.child(0, 0), "Num Vertices");

            for (int i = 0; i < nifSrc->rowCount(iBoneListArraySrc); i++) {
                QModelIndex iVertexWeightsArraySrc = getIndexSrc(iBoneListArraySrc.child(i, 0), "Vertex Weights");

                if (nifSrc->rowCount(iVertexWeightsArraySrc) > 0) {
                    c.processed(iVertexWeightsArraySrc.child(0, 0), "Index");
                    c.processed(iVertexWeightsArraySrc.child(0, 0), "Weight");

                    break;
                }
            }
        }
    }

    c. processed("Has Vertex Weights");

    setHandled(iDst, iSrc);

    return iDst;
}
