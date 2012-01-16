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

#include "niftypes.h"
#include "nifmodel.h"

#include <QStringList>

const float Quat::identity[4] = { 1.0, 0.0, 0.0, 0.0 };
const float Matrix::identity[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
const float Matrix4::identity[16] = { 1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0,  0.0, 0.0, 0.0, 1.0 };

QString NumOrMinMax( float val, char f, int prec )
{
	// minimum or maximum
	if( val == -FLT_MAX )
		return "<float_min>";
	else if( val == FLT_MAX )
		return "<float_max>";
	
	// display 0x80000000 as -0.0
	union flt_int {
		float f;
		quint32 i;
	};
	flt_int _val;
	_val.f = val;
	if( _val.i == 0x80000000 )
		return QString( "-%1" ).arg( QString::number( val, f, prec ) );

	// usual case
	return QString::number( val, f, prec );
}

void Vector2::fromString( QString str )
{
	QStringList parts = str.split( "," );
	if( parts.size() != 2 )
		return;
	bool xOk, yOk;
	float x_ = parts[0].toFloat( &xOk );
	float y_ = parts[1].toFloat( &yOk );
	if( xOk && yOk ) {
		xy[0] = x_;
		xy[1] = y_;
	}
}

void Vector3::fromString( QString str )
{
	QStringList parts = str.split( "," );
	if( parts.size() != 3 )
		return;
	bool xOk, yOk, zOk;
	float x_ = parts[0].toFloat( &xOk );
	float y_ = parts[1].toFloat( &yOk );
	float z_ = parts[2].toFloat( &zOk );
	if( xOk && yOk && zOk ) {
		xyz[0] = x_;
		xyz[1] = y_;
		xyz[2] = z_;
	}
}

void Vector4::fromString( QString str )
{
	QStringList parts = str.split( "," );
	if( parts.size() != 4 )
		return;
	bool xOk, yOk, zOk, wOk;
	float x_ = parts[0].toFloat( &xOk );
	float y_ = parts[1].toFloat( &yOk );
	float z_ = parts[2].toFloat( &zOk );
	float w_ = parts[3].toFloat( &wOk );
	if( xOk && yOk && zOk && wOk ) {
		xyzw[0] = x_;
		xyzw[1] = y_;
		xyzw[2] = z_;
		xyzw[3] = w_;
	}
}

void Quat::fromString( QString str )
{
	QStringList parts = str.split( "," );
	if( parts.size() != 4 )
		return;
	bool wOk, xOk, yOk, zOk;
	float w_ = parts[3].toFloat( &wOk );
	float x_ = parts[0].toFloat( &xOk );
	float y_ = parts[1].toFloat( &yOk );
	float z_ = parts[2].toFloat( &zOk );
	if( wOk && xOk&& yOk && zOk  ) {
		wxyz[0] = w_;
		wxyz[1] = x_;
		wxyz[2] = y_;
		wxyz[3] = z_;
	}
}

void Matrix::fromQuat( const Quat & q )
{
	float fTx  = ((float)2.0)*q[1];
	float fTy  = ((float)2.0)*q[2];
	float fTz  = ((float)2.0)*q[3];
	float fTwx = fTx*q[0];
	float fTwy = fTy*q[0];
	float fTwz = fTz*q[0];
	float fTxx = fTx*q[1];
	float fTxy = fTy*q[1];
	float fTxz = fTz*q[1];
	float fTyy = fTy*q[2];
	float fTyz = fTz*q[2];
	float fTzz = fTz*q[3];
	
	m[0][0] = (float)1.0-(fTyy+fTzz);
	m[0][1] = fTxy-fTwz;
	m[0][2] = fTxz+fTwy;
	m[1][0] = fTxy+fTwz;
	m[1][1] = (float)1.0-(fTxx+fTzz);
	m[1][2] = fTyz-fTwx;
	m[2][0] = fTxz-fTwy;
	m[2][1] = fTyz+fTwx;
	m[2][2] = (float)1.0-(fTxx+fTyy);
}

Quat Matrix::toQuat() const
{
	Quat q;
	
	float trace = m[0][0] + m[1][1] + m[2][2];
	float root;
	
	if (trace > 0.0)
	{
		root = sqrt( trace + 1.0 );
		q[0] = root / 2.0;
		root = 0.5 / root;
		q[1] = ( m[2][1] - m[1][2] ) * root;
		q[2] = ( m[0][2] - m[2][0] ) * root;
		q[3] = ( m[1][0] - m[0][1] ) * root;
	}
	else
	{
		int i = ( m[1][1] > m[0][0] ? 1 : 0 );
		if ( m[2][2] > m[i][i] )
			i = 2;
		const int next[3] = { 1, 2, 0 };
		int j = next[i];
		int k = next[j];

		root = sqrt( m[i][i] - m[j][j] - m[k][k] + 1.0 );
		q[i+1] = root / 2;
		root = 0.5 / root;
		q[0]   = ( m[k][j] - m[j][k] ) * root;
		q[j+1] = ( m[j][i] + m[i][j] ) * root;
		q[k+1] = ( m[k][i] + m[i][k] ) * root;
	}
	return q;
}

void Matrix::fromEuler( float x, float y, float z )
{
	float sinX = sin( x );
	float cosX = cos( x );
	float sinY = sin( y );
	float cosY = cos( y );
	float sinZ = sin( z );
	float cosZ = cos( z );
	
	m[0][0] = cosY * cosZ;
	m[0][1] = - cosY * sinZ;
	m[0][2] = sinY;
	m[1][0] = sinX * sinY * cosZ + sinZ * cosX;
	m[1][1] = cosX * cosZ - sinX * sinY * sinZ;
	m[1][2] = - sinX * cosY;
	m[2][0] = sinX * sinZ - cosX * sinY * cosZ;
	m[2][1] = cosX * sinY * sinZ + sinX * cosZ;
	m[2][2] = cosX * cosY;
}

bool Matrix::toEuler( float & x, float & y, float & z ) const
{
	if ( m[0][2] < 1.0 )
	{
		if ( m[0][2] > - 1.0 )
		{
			x = atan2( - m[1][2], m[2][2] );
			y = asin( m[0][2] );
			z = atan2( - m[0][1], m[0][0] );
			return true;
		}
		else
		{
			x = - atan2( - m[1][0], m[1][1] );
			y = - PI / 2;
			z = 0.0;
			return false;
		}
	}
	else
	{
		x = atan2( m[1][0], m[1][1] );
		y = PI / 2;
		z = 0.0;
		return false;
	}
}


Matrix Matrix::inverted () const
{
    Matrix i;

    i(0,0) = m[1][1]*m[2][2] - m[1][2]*m[2][1];
    i(0,1) = m[0][2]*m[2][1] - m[0][1]*m[2][2];
    i(0,2) = m[0][1]*m[1][2] - m[0][2]*m[1][1];
    i(1,0) = m[1][2]*m[2][0] - m[1][0]*m[2][2];
    i(1,1) = m[0][0]*m[2][2] - m[0][2]*m[2][0];
    i(1,2) = m[0][2]*m[1][0] - m[0][0]*m[1][2];
    i(2,0) = m[1][0]*m[2][1] - m[1][1]*m[2][0];
    i(2,1) = m[0][1]*m[2][0] - m[0][0]*m[2][1];
    i(2,2) = m[0][0]*m[1][1] - m[0][1]*m[1][0];

    float d = m[0][0]*i(0,0) + m[0][1]*i(1,0) + m[0][2]*i(2,0);

    if ( fabs( d ) <= 0.0 )
        return Matrix();

	for ( int x = 0; x < 3; x++ )
		for ( int y = 0; y < 3; y++ )
			i(x,y) /= d;
	
    return i;
}

QString Matrix::toHtml() const
{
	return QString( "<table>" )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[0][0], 0, 'f', 4 ).arg( m[0][1], 0, 'f', 4 ).arg( m[0][2], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[1][0], 0, 'f', 4 ).arg( m[1][1], 0, 'f', 4 ).arg( m[1][2], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td></tr>" ).arg( m[2][0], 0, 'f', 4 ).arg( m[2][1], 0, 'f', 4 ).arg( m[2][2], 0, 'f', 4 )
		+ QString( "</table>" );
}

QString Matrix4::toHtml() const
{
	return QString( "<table>" )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>" ).arg( m[0][0], 0, 'f', 4 ).arg( m[0][1], 0, 'f', 4 ).arg( m[0][2], 0, 'f', 4 ).arg( m[0][3], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>" ).arg( m[1][0], 0, 'f', 4 ).arg( m[1][1], 0, 'f', 4 ).arg( m[1][2], 0, 'f', 4 ).arg( m[1][3], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>" ).arg( m[2][0], 0, 'f', 4 ).arg( m[2][1], 0, 'f', 4 ).arg( m[2][2], 0, 'f', 4 ).arg( m[2][3], 0, 'f', 4 )
		+ QString( "<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>" ).arg( m[3][0], 0, 'f', 4 ).arg( m[3][1], 0, 'f', 4 ).arg( m[3][2], 0, 'f', 4 ).arg( m[3][3], 0, 'f', 4 )
		+ QString( "</table>" );
}

void Matrix4::decompose( Vector3 & trans, Matrix & rot, Vector3 & scale ) const
{
	trans = Vector3( m[ 3 ][ 0 ], m[ 3 ][ 1 ], m[ 3 ][ 2 ] );
	
	Matrix rotT;
	
	for ( int i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < 3; j++ )
		{
			rot( j, i ) = m[ i ][ j ];
			rotT( i, j ) = m[ i ][ j ];
		}
	}
	
	Matrix mtx = rot * rotT;
	
	scale = Vector3( sqrt( mtx( 0, 0 ) ), sqrt( mtx( 1, 1 ) ), sqrt( mtx( 2, 2 ) ) );
	
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			rot( i, j ) /= scale[ i ];
}

void Matrix4::compose( const Vector3 & trans, const Matrix & rot, const Vector3 & scale )
{
	m[0][3] = 0.0;
	m[1][3] = 0.0;
	m[2][3] = 0.0;
	m[3][3] = 1.0;

	m[3][0] = trans[0];
	m[3][1] = trans[1];
	m[3][2] = trans[2];
	
	for ( int i = 0; i < 3; i++ )
		for ( int j = 0; j < 3; j++ )
			m[ i ][ j ] = rot( j, i ) * scale[ j ];
}

void Quat::fromAxisAngle( Vector3 axis, float angle )
{
	axis.normalize();
    float s = sin( angle / 2 );
    wxyz[0] = cos( angle / 2 );
    wxyz[1] = s * axis[0];
    wxyz[2] = s * axis[1];
    wxyz[3] = s * axis[2];
}

void Quat::toAxisAngle( Vector3 & axis, float & angle ) const
{
    float squaredLength = wxyz[1]*wxyz[1] + wxyz[2]*wxyz[2] + wxyz[3]*wxyz[3];

    if ( squaredLength > 0.0 )
    {
        angle = acos( wxyz[0] ) * 2.0;
        axis[0] = wxyz[1];
        axis[1] = wxyz[2];
        axis[2] = wxyz[3];
		axis /= sqrt( squaredLength );
    }
    else
    {
		axis = Vector3( 1, 0, 0 );
		angle = 0;
    }
}

static inline float ISqrt_approx_in_neighborhood(float s) {
   static const float NEIGHBORHOOD = 0.959066f;
   static const float SCALE = 1.000311f;
   static const float ADDITIVE_CONSTANT = SCALE / (float)sqrt(NEIGHBORHOOD);
   static const float FACTOR = SCALE * (-0.5f / (NEIGHBORHOOD * (float)sqrt(NEIGHBORHOOD)));
   return ADDITIVE_CONSTANT + (s - NEIGHBORHOOD) * FACTOR;
}

static inline void fast_normalize(Quat &q) 
{
   float s = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
   float k = ISqrt_approx_in_neighborhood(s);
   if (s <= 0.91521198f) {
      k *= ISqrt_approx_in_neighborhood(k * k * s);
      if (s <= 0.65211970f) {
         k *= ISqrt_approx_in_neighborhood(k * k * s);
      }
   }
   q[0] *= k; q[1] *= k; q[2] *= k; q[3] *= k;
}

static inline float lerp(float v0, float v1, float perc)
{
   return v0 + perc * (v1 - v0);
}

static inline float correction(float t, float fCos) 
{
   const float s = 0.8228677f;
   const float kc = 0.5855064f;
   float factor = 1.0f - s * fCos;
   float k = kc * factor * factor;
   return t * ( k * t * ( 2.0f * t - 3.0f ) + 1.0f + k );
}

Quat Quat::slerp(float t, const Quat& p, const Quat& q)
{
   // Copyright (c) 2002 Jonathan Blow
   //  "Hacking Quaternions", The Inner Product, March 2002
   //   http://number-none.com/product/Hacking%20Quaternions/index.html

   float tprime;
   if (t <= 0.5f) {
      tprime = correction(t, dotproduct(p, q) );
   } else {
      tprime = 1.0f - correction(1.0f - t, dotproduct(p, q) );
   }
   Quat result( lerp(p[0], q[0], tprime), lerp(p[1], q[1], tprime),
                lerp(p[2], q[2], tprime), lerp(p[3], q[3], tprime));
   fast_normalize(result);
   return result;
}

/*
 *  Transform
 */

QDataStream & operator<<( QDataStream & ds, const Transform & t )
{
	for ( int x = 0; x < 3; x++ )
	{
		for ( int y = 0; y < 3; y++ )
			ds << t.rotation( x, y );
		ds << t.translation[ x ];
	}
	ds << t.scale;
	return ds;
}

QDataStream & operator>>( QDataStream & ds, Transform & t )
{
	for ( int x = 0; x < 3; x++ )
	{
		for ( int y = 0; y < 3; y++ )
			ds >> t.rotation( x, y );
		ds >> t.translation[ x ];
	}
	ds >> t.scale;
	return ds;
}

Transform operator*( const Transform & t1, const Transform & t2 )
{
	Transform t;
	t.rotation = t1.rotation * t2.rotation;
	t.translation = t1.translation + t1.rotation * t2.translation * t1.scale;
	t.scale = t1.scale * t2.scale;
	return t;
}

bool Transform::canConstruct( const NifModel * nif, const QModelIndex & parent )
{
	QModelIndex skinTransform = nif->getIndex( parent, "Skin Transform" );
	return nif && (
		(
			parent.isValid() && nif->getIndex( parent, "Rotation" ).isValid()
			&& nif->getIndex( parent, "Translation" ).isValid()
			&& nif->getIndex( parent, "Scale" ).isValid()
		) || (
			skinTransform.isValid() && nif->getIndex( skinTransform, "Rotation" ).isValid()
			&& nif->getIndex( skinTransform, "Translation" ).isValid()
			&& nif->getIndex( skinTransform, "Scale" ).isValid()
		)
	);
}


Transform::Transform( const NifModel * nif, const QModelIndex & transform )
{
	QModelIndex skinTransform = nif->getIndex( transform, "Skin Transform" );
	if( !skinTransform.isValid() )
	{
		skinTransform = transform;
	}
	rotation = nif->get<Matrix>( skinTransform, "Rotation" );
	translation = nif->get<Vector3>( skinTransform, "Translation" );
	scale = nif->get<float>( skinTransform, "Scale" );
}

void Transform::writeBack( NifModel * nif, const QModelIndex & transform ) const
{
	QModelIndex skinTransform = nif->getIndex( transform, "Skin Transform" );
	if( !skinTransform.isValid() )
	{
		skinTransform = transform;
	}
	nif->set<Matrix>( skinTransform, "Rotation", rotation );
	nif->set<Vector3>( skinTransform, "Translation", translation );
	nif->set<float>( skinTransform, "Scale", scale );
}

QString Transform::toString() const
{
	float x, y, z;
	rotation.toEuler( x, y, z );
	return QString( "TRANS( %1, %2, %3 ) ROT( %4, %5, %6 ) SCALE( %7 )" ).arg( translation[0] ).arg( translation[1] ).arg( translation[2] ).arg( x ).arg( y ).arg( z ).arg( scale );
}

Matrix4 Transform::toMatrix4() const
{
	Matrix4 m;
	
	for ( int c = 0; c < 3; c++ )
	{
		for ( int d = 0; d < 3; d++ )
			m( c, d ) = rotation( d, c ) * scale;
		m( 3, c ) = translation[ c ];
	}
	m( 0, 3 ) = 0.0;
	m( 1, 3 ) = 0.0;
	m( 2, 3 ) = 0.0;
	m( 3, 3 ) = 1.0;
	return m;
}

