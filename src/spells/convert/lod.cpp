#include "convert.h"

void Converter::lodLandscapeTranslationZero(QModelIndex iTranslation) {
    Vector3 translation = nifDst->get<Vector3>(iTranslation);
        if (4096 * fileProps.getLodXCoord() - translation[0] != 0.0f || 4096 * fileProps.getLodYCoord() - translation[1] != 0.0f) {
            qDebug() << __FUNCTION__ << "File name translation does not match file data translation";

            conversionResult = false;
        }

        translation[0] = 0;
        translation[1] = 0;

        nifDst->set<Vector3>(iTranslation, translation);
}

void Converter::lODLandscapeShape(const qint32 link, const QModelIndex iLink, const QModelIndex iLinkBlock, QModelIndex &iWater) {
    QModelIndex iShaderProperty = getBlockDst(iLinkBlock, "Shader Property");

        if (!iShaderProperty.isValid()) {
            qDebug() << __FUNCTION__ << "Shader Property not found";

            conversionResult = false;

            return;
        }

        Vector3 translation = nifDst->get<Vector3>(iLinkBlock, "Translation");

        if (translation[0] != 0.0f || translation[1] != 0.0f) {
            if (link != 1) {
                translation[0] = 0.0f;
                translation[1] = 0.0f;

                nifDst->set<Vector3>(iLinkBlock, "Translation", translation);
            } else {
                qDebug() << __FUNCTION__ << "Unknown case of first trishape in LOD to be translated";

                conversionResult = false;
            }
        }

        if (link != 1) {
            if (iWater.isValid()) {
                nifDst->setLink(iLink, -1);

                lodLandscapeWaterShader(iLinkBlock);
                nifDst->set<int>(iWater, "Num Children", nifDst->get<int>(iWater, "Num Children") + 1);

                QModelIndex iWaterChildren = getIndexDst(iWater, "Children");

                nifDst->updateArray(iWaterChildren);
                nifDst->setLink(iWaterChildren.child(nifDst->rowCount(iWaterChildren) - 1, 0), link);

                return;
            }

            iWater = lodLandscapeWater(iLinkBlock);

            // Make water branch
            nifDst->setLink(iLink, nifDst->getBlockNumber(iWater));

            return;
        }

        bool ok = false;

        nifDst->set<uint>(iShaderProperty, "Skyrim Shader Type", NifValue::enumOptionValue("BSLightingShaderPropertyShaderType", "LOD Landscape Noise", & ok));

        uint flags = 0;

        bsShaderFlags1Add(flags, "Model_Space_Normals");
        bsShaderFlags1Add(flags, "Own_Emit");
        bsShaderFlags1Add(flags, "ZBuffer_Test");

        nifDst->set<uint>(iShaderProperty, "Shader Flags 1", flags);

        flags = 0;

        bsShaderFlags2Add(flags, "ZBuffer_Write");
        bsShaderFlags2Add(flags, "LOD_Landscape");

        nifDst->set<uint>(iShaderProperty, "Shader Flags 2", flags);

        nifDst->set<QString>(iLinkBlock, "Name", "Land");

        // TODO: Set shader type
}

void Converter::lODLandscape() {
    if (!bLODLandscape) {
            return;
        }

        QModelIndex iRoot = nifDst->getBlock(0);

        if (nifDst->getBlockName(iRoot) != "BSMultiBoundNode") {
            qDebug() << __FUNCTION__ << "Invalid root" << nifDst->getBlockName(iRoot);

            conversionResult = false;

            return;
        }

        nifDst->set<QString>(iRoot, "Name", "chunk");
        nifDst->set<uint>(iRoot, "Culling Mode", 1);

        lodLandscapeTranslationZero(getIndexDst(iRoot, "Translation"));
        lodLandscapeMultiBound(iRoot);

        QModelIndex iChildren = getIndexDst(iRoot, "Children");
        QVector<qint32> links = nifDst->getLinkArray(iRoot, "Children");
        QModelIndex iWater;
        QList<QModelIndex> shapeList;

        for (qint32 i = 0; i < nifDst->rowCount(iChildren); i++) {
            QModelIndex iLink = iChildren.child(i, 0);
            qint32 link = nifDst->getLink(iLink);
            QModelIndex iLinkBlock = nifDst->getBlock(link);
            QString type = nifDst->getBlockName(iLinkBlock);

            if (type == "BSTriShape" || type == "BSSubIndexTriShape") {
                lODLandscapeShape(link, iLink, iLinkBlock, iWater);

                shapeList.append(iLinkBlock);
            } else if (type == "BSMultiBoundNode") {
                nifDst->set<QString>(iLinkBlock, "Name", "WATER");
            } else {
                qDebug() << __FUNCTION__ << "Unknown LOD Structure with block:" << type;

                conversionResult = false;

                return;
            }
        }

        int numChildren = nifDst->get<int>(iRoot, "Num Children");

        for (int i = numChildren - 1; i >= 0; i--) {
            if (nifDst->getLink(iChildren.child(i, 0)) == -1) {
                numChildren--;
                nifDst->removeRow(i, iChildren);
            }
        }

        nifDst->set<int>(iRoot, "Num Children", numChildren);

        if (iWater.isValid()) {
            lodLandscapeWaterMultiBound(iWater);
            //            lodLandscapeWaterRemoveEdgeFaces(iWater, lodLandscapeMinVert(shapeList));
        }
}

float Converter::lodLandscapeMinVert(QList<QModelIndex> shapeList) {
    float min = HUGE_VALF;

        for (QModelIndex iBlock : shapeList) {
            QModelIndex iVertices = getIndexDst(iBlock, "Vertices");

            if (!iVertices.isValid()) {
                continue;
            }

            lodLandscapeMinVert(iVertices, min);
        }

        return min;
}

void Converter::lodLandscapeMinVert(QModelIndex iShape, float &min) {
    for (int i = 0; i < nifDst->rowCount(iShape); i++) {
            HalfVector3 v = nifDst->get<Vector3>(iShape.child(i, 0), "Vertex");

            if (v[2] < min) {
                min = v[2];
            }
        }
}

bool Converter::lodLandscapeIsEdgeCoord(float f) {
    return f == 0.0f || f == 4096.0f;
}

void Converter::lodLandscapeWaterRemoveEdgeFaces(QModelIndex iWater, float min) {
    QList<QPair<QModelIndex, QModelIndex>> vertexDataArrayList;
        QList<QPair<QModelIndex, QModelIndex>> triangleArrayList;

        for (qint32 link : nifDst->getLinkArray(iWater, "Children")) {
            QModelIndex iBlock = nifDst->getBlock(link);

            QModelIndex iVertexDataArray = getIndexDst(iBlock, "Vertex Data");
            QModelIndex iNumVertices = getIndexDst(iBlock, "Num Vertices");
            QModelIndex iTriangleArray = getIndexDst(iBlock, "Triangles");
            QModelIndex iNUmTriangles = getIndexDst(iBlock, "Num Triangles");

            if (
                    !iVertexDataArray.isValid() ||
                    nifDst->rowCount(iVertexDataArray) == 0 ||
                    !getIndexDst(iVertexDataArray.child(0, 0), "Vertex").isValid() ||
                    !iTriangleArray.isValid() ||
                    !iNumVertices.isValid() ||
                    !iNUmTriangles.isValid()) {
                continue;
            }

            vertexDataArrayList.append(QPair<QModelIndex, QModelIndex>(iNumVertices, iVertexDataArray));
            triangleArrayList.append(QPair<QModelIndex, QModelIndex>(iNUmTriangles, iTriangleArray));
        }

        if (vertexDataArrayList.count() == 0) {
            return;
        }

        //        float min = nifDst->get<float>(vertexDataArrayList[0].second.child(0, 0), "Vertex");

        // Set min to the lowest vertex.
        for (QPair pair : vertexDataArrayList) {
            lodLandscapeMinVert(pair.second, min);
        }

        for (int j = 0; j < vertexDataArrayList.count(); j++) {
            QPair vertPair = vertexDataArrayList[j];
            QPair triPair = triangleArrayList[j];

            QModelIndex iNumTriangles = triPair.first;
            QModelIndex iTriangleArray = triPair.second;
            QModelIndex iNumVertices = vertPair.first;
            QModelIndex iVertexDataArray = vertPair.second;
            QList<int> vertRemoveList;
            int numTriangles = nifDst->get<int>(iNumTriangles);
            int numVertices = nifDst->get<int>(iNumVertices);

            for (int i = nifDst->rowCount(iTriangleArray) - 1; i >= 0; i--) {
                Triangle tri = nifDst->get<Triangle>(iTriangleArray.child(i, 0));

                int vertIndex1 = tri.v1();
                int vertIndex2 = tri.v2();
                int vertIndex3 = tri.v3();

                HalfVector3 v1 = nifDst->get<HalfVector3>(iVertexDataArray.child(vertIndex1, 0), "Vertex");
                HalfVector3 v2 = nifDst->get<HalfVector3>(iVertexDataArray.child(vertIndex2, 0), "Vertex");
                HalfVector3 v3 = nifDst->get<HalfVector3>(iVertexDataArray.child(vertIndex3, 0), "Vertex");

                if (removeTri(v1, v2, v3, min)) {
                    vertRemoveListAppend(v1, vertIndex1, min, vertRemoveList);
                    vertRemoveListAppend(v2, vertIndex2, min, vertRemoveList);
                    vertRemoveListAppend(v3, vertIndex3, min, vertRemoveList);

                    nifDst->removeRow(i, iTriangleArray);
                    numTriangles--;
                }
            }

            nifDst->set<int>(iNumTriangles, numTriangles);

            // NOTE: Removal of vertices not required. There can still be detached vertices.

            std::sort(vertRemoveList.begin(), vertRemoveList.end(), std::greater<int>());

            for (int vertIndex : vertRemoveList) {
                // Remove verts and update triangle vert indices
                if (removeVertex(iVertexDataArray, iTriangleArray, vertIndex)) {
                    numVertices--;
                }
            }

            nifDst->set<int>(iNumVertices, numVertices);
        }
}

bool Converter::removeVertex(QModelIndex iVertices, QModelIndex iTriangles, int index) {
    for (int i = 0; i < nifDst->rowCount(iTriangles); i++) {
            Triangle tri = nifDst->get<Triangle>(iTriangles.child(i, 0));

            if (tri[0] == index || tri[1] == index || tri[2] == index) {
                qDebug() << __FUNCTION__ << "Deleting referenced vertex" << nifDst->get<Vector3>(iVertices.child(index, 0), "Vertex");

                conversionResult = false;

                return false;
            }

            if (tri[0] > index) {
                tri[0]--;
            } else if (tri[1] > index) {
                tri[1]--;
            } else if (tri[2] > index) {
                tri[2]--;
            }
        }

        nifDst->removeRow(index, iVertices);

        return true;
}

void Converter::vertRemoveListAppend(HalfVector3 v, int vertIndex, float min, QList<int> &vertRemoveList) {
    if (isLowVert(v, min)) {
            if (!vertRemoveList.contains(vertIndex)) {
                vertRemoveList.append(vertIndex);
            }
        }
}

bool Converter::isSameEdgeCoord(float f1, float f2, float f3) {
    return lodLandscapeIsEdgeCoord(f1) && (f1 - f2 == 0.0f && f2 - f3 == 0.0f);
}

bool Converter::isEdgeVert(HalfVector3 v) {
    return lodLandscapeIsEdgeCoord(v[0]) || lodLandscapeIsEdgeCoord(v[1]);
}

bool Converter::isLowVert(HalfVector3 v, float min) {
    return v[2] - min == 0.0f;
}

bool Converter::removeTri(HalfVector3 v1, HalfVector3 v2, HalfVector3 v3, float min) {
    return isLowVert(v1, min) || isLowVert(v2, min) || isLowVert(v3, min);
}

void Converter::lodLandscapeWaterMultiBound(QModelIndex iWater) {
    QVector<qint32> links = nifDst->getLinkArray(iWater, "Children");
        float vert_max = nifDst->get<Vector3>(getIndexDst(nifDst->getBlock(links[0]), "Vertex Data").child(0, 0), "Vertex")[2];
        float vert_min = vert_max;

        for (qint32 link : links) {
            QModelIndex iVertices = getIndexDst(nifDst->getBlock(link), "Vertex Data");

            for (int i = 0; i < nifDst->rowCount(iVertices); i++) {
                float z = nifDst->get<Vector3>(iVertices.child(i, 0), "Vertex")[2];

                if (z < vert_min) {
                    vert_min = z;
                }

                if (z > vert_max) {
                    vert_max = z;
                }
            }
        }

        float extent = (vert_max - vert_min) * fileProps.getLodLevel() / 2;
        float position = vert_max * fileProps.getLodLevel() - extent;

        QModelIndex iAABB = getBlockDst(getBlockDst(iWater, "Multi Bound"), "Data");
        QModelIndex iPosition = getIndexDst(iAABB, "Position");
        QModelIndex iExtent = getIndexDst(iAABB, "Extent");

        Vector3 vPosition = nifDst->get<Vector3>(iPosition);
        Vector3 vExtent = nifDst->get<Vector3>(iExtent);

        vPosition[2] = position;
        vExtent[2] = extent;

        nifDst->set<Vector3>(iPosition, vPosition);
        nifDst->set<Vector3>(iExtent, vExtent);
}

QModelIndex Converter::lodLandscapeWater(QModelIndex iWater) {
    QModelIndex iMultiBoundNode = nifDst->insertNiBlock("BSMultiBoundNode");

        nifDst->set<QString>(iMultiBoundNode, "Name", "WATER");
        nifDst->set<uint>(iMultiBoundNode, "Culling Mode", 1);
        nifDst->set<uint>(iMultiBoundNode, "Num Children", 1);
        nifDst->updateArray(iMultiBoundNode, "Children");
        nifDst->setLink(getIndexDst(iMultiBoundNode, "Children").child(0, 0), nifDst->getBlockNumber(iWater));

        QModelIndex iMultiBound = nifDst->insertNiBlock("BSMultiBound");

        setLink(iMultiBoundNode, "Multi Bound", iMultiBound);

        QModelIndex iMultiBoundAABB = nifDst->insertNiBlock("BSMultiBoundAABB");
        int level = fileProps.getLodLevel();

        setLink(iMultiBound, "Data", iMultiBoundAABB);
        nifDst->set<Vector3>(iMultiBoundAABB, "Position", Vector3(4096 * level / 2, 4096 * level / 2, 0));
        nifDst->set<Vector3>(iMultiBoundAABB, "Extent", Vector3(4096 * level / 2, 4096 * level/ 2, 0));

        lodLandscapeWaterShader(iWater);

        return iMultiBoundNode;
}

void Converter::lodLandscapeWaterShader(QModelIndex iWater) {
    QModelIndex iShaderProperty = getBlockDst(iWater, "Shader Property");

        nifDst->set<uint>(iShaderProperty, "Shader Flags 1", bsShaderFlagsGet("Fallout4ShaderPropertyFlags1", "ZBuffer_Test"));
        nifDst->set<uint>(iShaderProperty, "Shader Flags 2", bsShaderFlagsGet("Fallout4ShaderPropertyFlags2", "ZBuffer_Write"));
        nifDst->set<int>(iShaderProperty, "Texture Clamp Mode", 3);
        nifDst->set<int>(iShaderProperty, "Lighting influence", 255);
        nifDst->set<float>(iShaderProperty, "Falloff Start Angle", 0);
        nifDst->set<float>(iShaderProperty, "Falloff Stop Angle", 0);
        nifDst->set<float>(iShaderProperty, "Emissive Multiple", 1.0f);
        nifDst->set<float>(iShaderProperty, "Soft Falloff Depth", 100.0f);
        nifDst->set<float>(iShaderProperty, "Environment Map scale", 1.0f);
}

void Converter::lodLandscapeMultiBound(QModelIndex iRoot) {
    QModelIndex iMultiBoundAABB = getBlockDst(getBlockDst(iRoot, "Multi Bound"), "Data");

        if (!iMultiBoundAABB.isValid()) {
            qDebug() << "Invalid Multi Bound in LOD";

            conversionResult = false;

            return;
        }

        Vector3 position = nifDst->get<Vector3>(iMultiBoundAABB, "Position");

        if (
                4096 * fileProps.getLodXCoord() + 4096 * fileProps.getLodLevel() / 2 - position[0] != 0.0f ||
                4096 * fileProps.getLodYCoord() + 4096 * fileProps.getLodLevel() / 2 - position[1] != 0.0f) {
            qDebug() << "Unknown MulitboundAABB positioning" << position;

            conversionResult = false;
        }

        position[0] = 4096 * fileProps.getLodLevel() / 2;
        position[1] = 4096 * fileProps.getLodLevel() / 2;

        nifDst->set<Vector3>(iMultiBoundAABB, "Position", position);
}

void Converter::lODObjects() {
    if (!bLODBuilding) {
            return;
        }

        QModelIndex iRoot = nifDst->getBlock(0);

        // Check if the destination nif already has a processed LOD.
        if (nifDst->getBlockName(iRoot) == "NiNode" && nifDst->get<QString>(iRoot, "Name") == "obj") {
            QModelIndex iRootOld = iRoot;
            iRoot = QModelIndex();

            // Find the root of the newly created LOD.
            for (int i = 1; i < nifDst->getBlockCount(); i++) {
                if (nifDst->getParent(i) == -1) {
                    iRoot = nifDst->getBlock(i);

                    break;
                }
            }

            if (!iRoot.isValid()) {
                qDebug() << __FUNCTION__ << "Failed to find new root";

                conversionResult = false;

                return;
            }

            uint numChildren = nifDst->get<uint>(iRootOld, "Num Children");
            QModelIndex iChildren = getIndexDst(iRootOld, "Children");

            nifDst->set<uint>(iRootOld, "Num Children", numChildren + 1);
            nifDst->updateArray(iChildren);
            nifDst->setLink(iChildren.child(int(numChildren), 0), nifDst->getBlockNumber(iRoot));
        } else {
            QModelIndex iNewRoot = nifDst->insertNiBlock("NiNode", 0);

            nifDst->set<QString>(iNewRoot, "Name", "obj");
            nifDst->set<uint>(iNewRoot, "Num Children", 1);
            nifDst->updateArray(iNewRoot, "Children");
            nifDst->setLink(getIndexDst(iNewRoot, "Children").child(0, 0), 1);
        }

        if (nifDst->getBlockName(iRoot) != "BSMultiBoundNode") {
            qDebug() << __FUNCTION__ << "Invalid root" << nifDst->getBlockName(iRoot);

            conversionResult = false;

            return;
        }

        nifDst->set<QString>(iRoot, "Name", "");

        QVector<qint32> links = nifDst->getLinkArray(iRoot, "Children");

        for (qint32 link : links) {
            QModelIndex iLinkBlock = nifDst->getBlock(link);

            if (nifDst->getBlockName(iLinkBlock) == "BSSubIndexTriShape") {
                nifDst->set<QString>(iLinkBlock, "Name", "obj");

                QModelIndex iShaderProperty = getBlockDst(iLinkBlock, "Shader Property");

                if (!iShaderProperty.isValid()) {
                    qDebug() << __FUNCTION__ << "Shader Property not found";

                    conversionResult = false;

                    return;
                }

                QModelIndex iNiAlphaProperty = nifDst->insertNiBlock("NiAlphaProperty");

                nifDst->set<uint>(iNiAlphaProperty, "Flags", 4844);
                nifDst->set<int>(iNiAlphaProperty, "Threshold", 128);

                setLink(iLinkBlock, "Alpha Property", iNiAlphaProperty);

                // TODO: Set shader type
            } else {
                qDebug() << __FUNCTION__ << "Unknown LOD Structure";

                conversionResult = false;

                return;
            }
        }
}

bool Converter::getBLODLandscape() const
{
return bLODLandscape;
}

bool Converter::getBLODBuilding() const
{
return bLODBuilding;
}
