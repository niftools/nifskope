#include "convert.h"

QModelIndex Converter::niInterpolatorMultToColor(QModelIndex iInterpolatorMult, Vector3 colorVector) {
    QModelIndex iInterpolatorColor = nifDst->insertNiBlock("NiPoint3Interpolator");
        QModelIndex iInterpolatorMultData = getBlockDst(iInterpolatorMult, "Data");

        if (iInterpolatorMultData.isValid()) {
            QModelIndex iInterpolatorColorData = nifDst->insertNiBlock("NiPosData");

            QModelIndex iDataMult = getIndexDst(iInterpolatorMultData, "Data");
            QModelIndex iDataColor = getIndexDst(iInterpolatorColorData, "Data");

            nifDst->set<uint>(iDataColor, "Num Keys", nifDst->get<uint>(iDataMult, "Num Keys"));
            nifDst->set<uint>(iDataColor, "Interpolation", enumOptionValue("KeyType", "LINEAR_KEY"));

            QModelIndex iKeysArrayMult = getIndexDst(iDataMult, "Keys");
            QModelIndex iKeysArrayColor = getIndexDst(iDataColor, "Keys");

            nifDst->updateArray(iKeysArrayColor);

            for (int i = 0; i < nifDst->rowCount(iKeysArrayMult); i++) {
                nifDst->set<float>(iKeysArrayColor.child(i, 0), "Time", nifDst->get<float>(iKeysArrayMult.child(i, 0), "Time"));
                nifDst->set<Vector3>(iKeysArrayColor.child(i, 0), "Value", colorVector);
            }

            setLink(iInterpolatorColor, "Data", iInterpolatorColorData);
        }

        return iInterpolatorColor;
}

void Converter::niMaterialEmittanceFinalize(const QModelIndex iBlock) {
    if (nifDst->getBlockName(iBlock) != "BSEffectShaderProperty") {
            return;
        }

        QModelIndex iController = getBlockDst(iBlock, "Controller");
        QModelIndex iControllerMult;
        QModelIndex iControllerColor;
        QModelIndex iLastController;

        QModelIndex iMult = getIndexDst(iBlock, "Emissive Multiple");
        QModelIndex iColor = getIndexDst(iBlock, "Emissive Color");
        float mult = nifDst->get<float>(iMult);
        Color4 color = nifDst->get<Color4>(iColor);

        while (iController.isValid()) {
            iLastController = iController;

            const QString controllerType = nifDst->getBlockName(iController);

            if (controllerType == "BSEffectShaderPropertyFloatController") {
                if (
                        nifDst->get<uint>(iController, "Type of Controlled Variable") ==
                        enumOptionValue("EffectShaderControlledVariable", "EmissiveMultiple")) {
                    if (!iControllerMult.isValid()) {
                        iControllerMult = iController;
                    } else {
                        qDebug() << __FUNCTION__ << "Multiple Mult Controllers";

                        conversionResult = false;
                    }
                }
            } else if (controllerType == "BSEffectShaderPropertyColorController") {
                if (
                        nifDst->get<uint>(iController, "Type of Controlled Color") ==
                        enumOptionValue("EffectShaderControlledColor", "Emissive Color")) {
                    if (!iControllerColor.isValid()) {
                        iControllerColor = iController;
                    } else {
                        qDebug() << __FUNCTION__ << "Multiple Color Controllers";

                        conversionResult = false;
                    }
                }
            }

            iController = getBlockDst(iController, "Next Controller");
        }

        if (!iControllerMult.isValid() && !iControllerColor.isValid()) {
            if (mult == 0.0f) {
                nifDst->set<float>(iMult, 1);
                nifDst->set<Color4>(iColor, Color4(1, 1, 1, 1));
            }

            return;
        }

        if (iControllerMult.isValid() && !iControllerColor.isValid()) {
            QModelIndex iInterpolatorMult = getBlockDst(iControllerMult, "Interpolator");

            if (!iInterpolatorMult.isValid()) {
                return;
            }

            Vector3 colorVector = Vector3(color[0], color[1], color[2]);

            iControllerColor = nifDst->insertNiBlock("BSEffectShaderPropertyColorController");
            nifDst->set<uint>(iControllerColor, "Type of Controlled Color", enumOptionValue("EffectShaderControlledColor", "Emissive Color"));

            int lNextController = nifDst->getLink(iControllerMult, "Next Controller");

            nifDst->setLink(iControllerMult, "Next Controller", nifDst->getBlockNumber(iControllerColor));
            nifDst->setLink(iControllerColor, "Next Controller", lNextController);

            const QString interpolatorType = nifDst->getBlockName(iInterpolatorMult);

            if (interpolatorType == "NiBlendFloatInterpolator") {
                QModelIndex iInterpolatorColor = nifDst->insertNiBlock("NiBlendPoint3Interpolator");

                Copier c = Copier(iInterpolatorColor, iInterpolatorMult, nifDst, nifDst);

                c.copyValue("Flags");
                c.copyValue("Array Size");
                c.copyValue("Weight Threshold");
                c.ignore("Value");

                for (QModelIndex iSequence : niControllerSequenceList) {
                    QModelIndex iControlledBlocksArray = getIndexDst(iSequence, "Controlled Blocks");
                    QModelIndex iNumControlledBlocks = getIndexDst(iSequence, "Num Controlled Blocks");

                    const int numControlledBlocksOrginal = nifDst->get<int>(iNumControlledBlocks);
                    int numControlledBlocks = numControlledBlocksOrginal;

                    for (int i = 0; i < numControlledBlocksOrginal; i++) {
                        QModelIndex iControlledBlockMult = iControlledBlocksArray.child(i, 0);

                        if (nifDst->getLink(iControlledBlockMult, "Controller") == nifDst->getBlockNumber(iControllerMult)) {
                            numControlledBlocks++;
                            nifDst->set<int>(iNumControlledBlocks, numControlledBlocks);
                            nifDst->updateArray(iControlledBlocksArray);

                            QModelIndex iControlledBlockColor = iControlledBlocksArray.child(numControlledBlocks - 1, 0);

                            Copier c = Copier(iControlledBlockColor, iControlledBlockMult, nifDst, nifDst);

                            c.tree();

                            QModelIndex iInterpolatorMult = getBlockDst(iControlledBlockMult, "Interpolator");
                            QModelIndex iInterpolatorColor = niInterpolatorMultToColor(iInterpolatorMult, colorVector);

                            nifDst->setLink(iControlledBlockColor, "Interpolator", nifDst->getBlockNumber(iInterpolatorColor));
                            nifDst->setLink(iControlledBlockColor, "Controller", nifDst->getBlockNumber(iControllerColor));
                        }
                    }
                }

                setLink(iControllerColor, "Interpolator", iInterpolatorColor);
            } else if (interpolatorType == "NiFloatInterpolator") {
                QModelIndex iInterpolatorColor = niInterpolatorMultToColor(iInterpolatorMult, colorVector);

                setLink(iControllerColor, "Interpolator", iInterpolatorColor);
            } else {
                qDebug() << __FUNCTION__ << __LINE__ << "Unknown interpolator type" << interpolatorType;

                conversionResult = false;

                return;
            }

            Copier c = Copier(iControllerColor, iControllerMult, nifDst, nifDst);

            c.processed("Next Controller");
            c.copyValue("Flags");
            c.copyValue("Frequency");
            c.copyValue("Phase");
            c.copyValue("Start Time");
            c.copyValue("Stop Time");
            c.copyValue("Target");
            c.processed("Interpolator");
            c.processed("Type of Controlled Variable");
        }

        if (!iControllerMult.isValid() && iControllerColor.isValid()) {
            if (mult == 0.0f) {
                nifDst->set<float>(iMult, 1);
                nifDst->set<Color4>(iColor, Color4(1, 1, 1, 1));

                for (QModelIndex iSequence : niControllerSequenceList) {
                    QModelIndex iControlledBlocksArray = getIndexDst(iSequence, "Controlled Blocks");

                    for (int i = 0; i < nifDst->rowCount(iControlledBlocksArray); i++) {
                        QModelIndex iControlledBlock = iControlledBlocksArray.child(i, 0);

                        if (nifDst->getLink(iControlledBlock, "Controller") == nifDst->getBlockNumber(iControllerMult)) {
                            QModelIndex iInterpolator = getBlockDst(getBlockDst(iControlledBlock, "Interpolator"), "Data");

                            if (!iInterpolator.isValid()) {
                                continue;
                            }

                            QModelIndex iData = getIndexDst(iInterpolator, "Data");
                            QModelIndex iKeys = getIndexDst(iInterpolator, "Keys");

                            if (!iInterpolator.isValid() || !iData.isValid() || !iKeys.isValid()) {
                                qDebug() << __FUNCTION__ << __LINE__ << "No Interpolator";

                                conversionResult = false;

                                return;
                            }

                            nifDst->set<uint>(iData, "Num Keys", 1);
                            nifDst->updateArray(iKeys);
                            nifDst->set<float>(iKeys.child(0, 0), "Time", 0);
                            nifDst->set<Vector3>(iKeys.child(0, 0), "Value", Vector3(1, 1, 1));
                        }
                    }
                }
            }

            return;
        }

        QModelIndex iInterpolatorMult = getBlockDst(iControllerMult, "Interpolator");
        QModelIndex iFlagsMult = getIndexDst(iInterpolatorMult, "Flags");

        if (iFlagsMult.isValid() && nifDst->get<int>(iFlagsMult) == int(enumOptionValue("InterpBlendFlags", "MANAGER_CONTROLLED"))) {
            iInterpolatorMult = QModelIndex();
        }

        QModelIndex iInterpolatorColor = getBlockDst(iControllerColor, "Interpolator");
        QModelIndex iFlagsColor = getIndexDst(iInterpolatorColor, "Flags");

        if (iFlagsColor.isValid() && nifDst->get<int>(iFlagsColor) == int(enumOptionValue("InterpBlendFlags", "MANAGER_CONTROLLED"))) {
            iInterpolatorColor = QModelIndex();
        }

        QList<QPair<QModelIndex, QModelIndex>> interpolatorList;
        int linkMult = nifDst->getBlockNumber(iControllerMult);
        int linkColor = nifDst->getBlockNumber(iControllerColor);

        if (iInterpolatorMult.isValid() && iInterpolatorColor.isValid()) {
            interpolatorList.append(QPair(iInterpolatorMult, iInterpolatorColor));
        } else {
            for (QModelIndex iSequence : niControllerSequenceList) {
                QModelIndex iControlledBlocksArray = getIndexDst(iSequence, "Controlled Blocks");
                QPair interpolatorPair = QPair(QModelIndex(), QModelIndex());

                if (iInterpolatorMult.isValid()) {
                    interpolatorPair.first = iInterpolatorMult;
                } else if (iInterpolatorColor.isValid()) {
                    interpolatorPair.second = iInterpolatorColor;
                }

                for (int i = 0; i < nifDst->rowCount(iControlledBlocksArray); i++) {
                    QModelIndex iControlledBlock = iControlledBlocksArray.child(i, 0);

                    if (!iInterpolatorMult.isValid() && nifDst->getLink(iControlledBlock, "Controller") == linkMult) {
                        if (interpolatorPair.first.isValid()) {
                            qDebug() << __FUNCTION__ << "Mult interpolator already set";

                            conversionResult = false;
                        }

                        interpolatorPair.first = getBlockDst(iControlledBlock, "Interpolator");
                    } else if (!iInterpolatorColor.isValid() && nifDst->getLink(iControlledBlock, "Controller") == linkColor) {
                        if (interpolatorPair.second.isValid()) {
                            qDebug() << __FUNCTION__ << "Color interpolator already set";

                            conversionResult = false;
                        }

                        interpolatorPair.second = getBlockDst(iControlledBlock, "Interpolator");
                    }
                }

                if (interpolatorPair.first.isValid() ^ interpolatorPair.second.isValid()) {
                    qDebug() << __FUNCTION__ << "No Interpolator Pair";

                    conversionResult = false;

                    return;
                }

                if (interpolatorPair.first.isValid() && interpolatorPair.second.isValid()) {
                    interpolatorList.append(interpolatorPair);
                }
            }
        }

        for (QPair<QModelIndex, QModelIndex> pair : interpolatorList) {
            QModelIndex iInterpolatorDataMult = getIndexDst(getBlockDst(pair.first, "Data"), "Data");
            QModelIndex iInterpolatorDataColor = getIndexDst(getBlockDst(pair.second, "Data"), "Data");

            // TODO: Process case where color interpolator is not valid
            if (!iInterpolatorDataMult.isValid() || !iInterpolatorDataColor.isValid()) {
                continue;
            }

            QModelIndex iNumKeysMult = getIndexDst(iInterpolatorDataMult, "Num Keys");
            QModelIndex iNumKeysColor = getIndexDst(iInterpolatorDataColor, "Num Keys");

            QModelIndex iKeysArrayMult = getIndexDst(iInterpolatorDataMult, "Keys");
            QModelIndex iKeysArrayColor = getIndexDst(iInterpolatorDataColor, "Keys");

            for (uint i = 0; i < uint(nifDst->rowCount(iKeysArrayMult) - 1); i++) {
                QModelIndex iKey1 = iKeysArrayMult.child(int(i), 0);
                QModelIndex iKey2 = iKeysArrayMult.child(int(i + 1), 0);

                QModelIndex iValue1 = getIndexDst(iKey1, "Value");
                QModelIndex iValue2 = getIndexDst(iKey2, "Value");

                float t1 = nifDst->get<float>(iKey1, "Time");
                float t2 = nifDst->get<float>(iKey2, "Time");

                if (nifDst->get<float>(iValue1) == 0.0f) {
                    if (nifDst->get<float>(iValue2) == 0.0f) {
                        if (i > 0) {
                            iKey1 = interpolatorDataInsertKey(iKeysArrayMult, iNumKeysMult, iKey1.row() + 1, RowToCopy::Before);
                            iValue1 = getIndexDst(iKey1, "Value");

                            // Update key 2
                            iKey2 = iKeysArrayMult.child(iKey2.row() + 1, 0);
                        }

                        iKey2 = interpolatorDataInsertKey(iKeysArrayMult, iNumKeysMult, iKey2.row(), RowToCopy::After);
                        iValue2 = getIndexDst(iKey2, "Value");

                        nifDst->set<float>(iValue1, 1);
                        nifDst->set<float>(iValue2, 1);

                        QModelIndex iColorKey1;
                        QModelIndex iColorKey2;

                        for (int j = 0; j < nifDst->rowCount(iKeysArrayColor); j++) {
                            QModelIndex iColorKey = iKeysArrayColor.child(j, 0);
                            float t = nifDst->get<float>(iColorKey, "Time");

                            if (t - t1 <= 0) {
                                // Get the last t1 key
                                iColorKey1 = iColorKey;
                            } else if (t - t1 > 0 && t - t2 < 0) {
                                // inbetweeners
                                nifDst->set<Vector3>(iColorKey, "Value", Vector3(1, 1, 1));
                            } else if (t - t2 >= 0) {
                                // Get the first t2 key
                                iColorKey2 = iColorKey;

                                break;
                            }
                        }

                        if (iColorKey1.isValid() && iColorKey2.isValid()) {
                            if (i > 0) {
                                iColorKey1 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey1.row() + 1, RowToCopy::Before);

                                // Update Color key 2
                                iColorKey2 = iKeysArrayColor.child(iColorKey2.row() + 1, 0);
                            }

                            if (nifDst->get<float>(iColorKey1, "Time") - t1 < 0) {
                                float tBegin = nifDst->get<float>(iColorKey1, "Time");
                                float tEnd = nifDst->get<float>(iColorKey2, "Time");
                                float tRange = tEnd - tBegin;
                                float changeFactor = (t1 - tBegin) / tRange;
                                Vector3 colorBefore = nifDst->get<Vector3>(iColorKey1, "Value");
                                Vector3 colorAfter = nifDst->get<Vector3>(iColorKey2, "Value");
                                Vector3 colorChange = (colorAfter - colorBefore) * changeFactor;
                                Vector3 color = colorBefore + colorChange;

                                // Set color to white
                                nifDst->set<float>(iColorKey1, "Time", t1);
                                nifDst->set<Vector3>(iColorKey1, "Value", color);

                                iColorKey1 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey1.row() + 1, RowToCopy::Before);
                            }

                            // Set color before to the color right before changing to white and color after to the color at t2
                            Vector3 colorBefore = nifDst->get<Vector3>(iColorKey1, "Value");
                            Vector3 colorAfter = nifDst->get<Vector3>(iColorKey2, "Value");

                            // Set color to white
                            nifDst->set<float>(iColorKey1, "Time", t1);
                            nifDst->set<Vector3>(iColorKey1, "Value", Vector3(1, 1, 1));

                            // Insert the color key at the end of the white color phase
                            iColorKey2 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey2.row(), RowToCopy::After);

                            // Set the color key's color to white and it's time to t2
                            nifDst->set<float>(iColorKey2, "Time", t2);
                            nifDst->set<Vector3>(iColorKey2, "Value", Vector3(1, 1, 1));

                            // If the color end key is after the mult end key, create a new color with its value being the color at that time in the timeline without white phases.
                            if (nifDst->get<float>(iColorKey2, "Time") - t2 > 0) {
                                // t > t2
                                float t = nifDst->get<float>(iColorKey2, "Time");

                                float keyRange = t2 - t1;
                                float keyInRange = t - t1;
                                float changeFactor = keyRange / keyInRange;
                                Vector3 colorChange = (colorAfter - colorBefore) * changeFactor;
                                Vector3 color = colorBefore + colorChange;

                                iColorKey2 = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, iColorKey2.row(), RowToCopy::After);

                                nifDst->set<Vector3>(iColorKey2, "Value", color);
                            }
                        } else {
                            qDebug() << __FUNCTION__ << "Missing color key" << iColorKey1 << iColorKey2;

                            conversionResult = false;

                            return;
                        }

                        i += 2;
                    }
                }
            }

            // Set the final key

            QModelIndex iKey = iKeysArrayMult.child(nifDst->rowCount(iKeysArrayMult) - 1, 0);

            if (nifDst->get<float>(iKey, "Value") == 0.0f) {
                float t = nifDst->get<float>(iKey, "Time");

                QModelIndex iColorKeyLast;

                for (int i = 0; i < nifDst->rowCount(iKeysArrayColor); i++) {
                    QModelIndex iColorKey = iKeysArrayColor.child(i, 0);

                    if (nifDst->get<float>(iColorKey, "Time") >= t) {
                        if (!iColorKeyLast.isValid()) {
                            iColorKeyLast = iColorKey;
                        } else {
                            nifDst->set<Vector3>(iColorKey, "Value", Vector3(1, 1, 1));
                        }
                    }
                }

                if (!iColorKeyLast.isValid()) {
                    // Add a key with the last color
                    interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, nifDst->rowCount(iKeysArrayColor), RowToCopy::Before);
                } else if (nifDst->get<float>(iColorKeyLast, "Time") > t) {
                    // Add a key with the color at the timeline point.

                    QModelIndex iColorKeyBeforeLast = iKeysArrayColor.child(iColorKeyLast.row() - 1, 0);

                    float tBegin = nifDst->get<float>(iColorKeyBeforeLast, "Time");
                    float tEnd = nifDst->get<float>(iColorKeyBeforeLast, "Time");
                    float tRange = tEnd - tBegin;
                    float tInRange = tEnd - t;
                    float changeFactor = tInRange / tRange;

                    Vector3 iColorBefore = nifDst->get<Vector3>(iColorKeyBeforeLast, "Value");
                    Vector3 iColorAfter = nifDst->get<Vector3>(iColorKeyLast, "Value");
                    Vector3 colorChange = (iColorAfter - iColorBefore) * changeFactor;
                    Vector3 color = iColorBefore + colorChange;

                    iColorKeyLast = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, nifDst->rowCount(iKeysArrayColor), RowToCopy::Before);

                    nifDst->set<Vector3>(iColorKeyLast, "Value", color);
                }

                iColorKeyLast = interpolatorDataInsertKey(iKeysArrayColor, iNumKeysColor, nifDst->rowCount(iKeysArrayColor), RowToCopy::Before);

                nifDst->set<Vector3>(iColorKeyLast, "Value", Vector3(1, 1, 1));
            }
        }

        if (mult == 0.0f) {
            nifDst->set<float>(iMult, 1);
            nifDst->set<Color4>(iColor, Color4(1, 1, 1, 1));
        }
}

QModelIndex Converter::interpolatorDataInsertKey(QModelIndex iKeys, QModelIndex iNumKeys, int row, Converter::RowToCopy rowToCopy) {
    uint numKeys = nifDst->get<uint>(iNumKeys) + 1;
        nifDst->set<uint>(iNumKeys, numKeys);
        nifDst->updateArray(iKeys);

        if (rowToCopy == RowToCopy::Before) {
            if (row == 0) {
                qDebug() << __FUNCTION__ << "Cannot insert at 0";

                conversionResult = false;

                return QModelIndex();
            }

            for (int i = int(numKeys) - 1; i >= row; i--) {
                Copier c = Copier(iKeys.child(i, 0), iKeys.child(i - 1, 0), nifDst, nifDst);

                c.tree();
            }
        } else if (rowToCopy == RowToCopy::After) {
            for (int i = int(numKeys) - 1; i > row; i--) {
                Copier c = Copier(iKeys.child(i, 0), iKeys.child(i - 1, 0), nifDst, nifDst);

                c.tree();
            }
        }

        return iKeys.child(row, 0);
}
