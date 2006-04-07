#ifndef NVTRISTRIP_WRAPPER_H
#define NVTRISTRIP_WRAPPER_H

#include "../niftypes.h"

#include <QList>
#include <QVector>

QList< QVector<quint16> > strippify( QVector<Triangle> triangles );

QVector<Triangle> triangulate( QVector<quint16> strip );
QVector<Triangle> triangulate( QList< QVector<quint16> > strips );

#endif
