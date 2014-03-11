#ifndef NVTRISTRIP_WRAPPER_H
#define NVTRISTRIP_WRAPPER_H

#include "niftypes.h"

#include <QList>
#include <QVector>

QList< QVector<quint16> > stripify( QVector<Triangle> triangles, bool stitch = true );

QVector<Triangle> triangulate( QVector<quint16> strips );
QVector<Triangle> triangulate( QList< QVector<quint16> > strips );

#endif
