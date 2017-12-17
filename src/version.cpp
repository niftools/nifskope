/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
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

#include "version.h"

#include "QDebug"


//! @file version.cpp %NifSkope version number management

// Number of version parts (Default = 3)
int NifSkopeVersion::numParts = 3;

NifSkopeVersion::NifSkopeVersion( const QString & ver )
	: rawVersion( ver ), displayVersion( rawToDisplay( ver, true ) )
{

}

NifSkopeVersion::NifSkopeVersion( const NifSkopeVersion & other )
	: rawVersion( other.rawVersion ), displayVersion( other.displayVersion )
{

}

NifSkopeVersion::NifSkopeVersion()
{

}

NifSkopeVersion::~NifSkopeVersion()
{

}

QList<int> NifSkopeVersion::parts() const
{
	return versionParts( rawVersion, numParts );
}

QString NifSkopeVersion::majMin() const
{
	return rawToMajMin( rawVersion );
}

int NifSkopeVersion::hex() const
{
	return hexVersion( versionParts( rawVersion ) );
}


void NifSkopeVersion::setNumParts( int num )
{
	if ( num < 8 && num > 1 )
		numParts = num;
}

int NifSkopeVersion::hexVersion( QString ver )
{
	return hexVersion( versionParts( ver ) );
}

int NifSkopeVersion::hexVersion( QList<int> majMinRev )
{
	// This can only work up to but not including 32-bit values
	// i.e. in this case a 4th value ( n << 24 ) is possible
	// but I've decided to leave it as 3 parts only, to reflect 
	// Qt's own QT_VERSION_CHECK macro.
	//
	// Granularity past MAJ.MIN.REV will require a NifSkopeVersion object

	return (majMinRev[0] << 16) | (majMinRev[1] << 8) | (majMinRev[2]);
}


int NifSkopeVersion::compare( const QString & ver1, const QString & ver2, int parts )
{
	int oldNumParts = numParts;

	// Set granularity
	NifSkopeVersion::setNumParts( parts );

	NifSkopeVersion v1( ver1 );
	NifSkopeVersion v2( ver2 );

	int ret = 0;

	if ( v1 < v2 )
		ret = -1;
	else if ( v1 == v2 )
		ret = 0;
	else
		ret = 1;

	// Reset granularity
	NifSkopeVersion::setNumParts( oldNumParts );

	return ret;
}

int NifSkopeVersion::compare( const QString & ver1, const QString & ver2 )
{
	return compare( ver1, ver2, numParts );
}


bool NifSkopeVersion::compareGreater( const QString & ver1, const QString & ver2, int parts )
{
	int oldNumParts = numParts;

	// Set granularity
	NifSkopeVersion::setNumParts( parts );

	NifSkopeVersion v1( ver1 );
	NifSkopeVersion v2( ver2 );

	bool result = v1 > v2;

	// Reset granularity
	NifSkopeVersion::setNumParts( oldNumParts );

	return result;
}

bool NifSkopeVersion::compareGreater( const QString & ver1, const QString & ver2 )
{
	return compareGreater( ver1, ver2, numParts );
}


bool NifSkopeVersion::compareLess( const QString & ver1, const QString & ver2, int parts )
{
	int oldNumParts = numParts;

	// Set granularity
	NifSkopeVersion::setNumParts( parts );

	NifSkopeVersion v1( ver1 );
	NifSkopeVersion v2( ver2 );

	bool result = v1 < v2;

	// Reset granularity
	NifSkopeVersion::setNumParts( oldNumParts );

	return result;
}

bool NifSkopeVersion::compareLess( const QString & ver1, const QString & ver2 )
{
	return compareLess( ver1, ver2, numParts );
}


QString NifSkopeVersion::rawToDisplay( const QString & ver, bool showStage /* = false */, bool showDev /* = false */ )
{
	// Explicitly call all parts (i.e. 7)
	QList<int> verList = versionParts( ver, 7 );

	if ( verList.isEmpty() )
		return QString();

	QString verString = QString( "%1.%2.%3" ).arg( verList[0] ).arg( verList[1] ).arg( verList[2] );
	QString stage, dev;

	QList<QString> stages { "Dev", "Alpha", "Beta", "RC", "" };
	QList<QString> devs { "Dev", "Post" };

	if ( showStage ) {
		stage = stages.value( verList.value( 3, 4 ), "" );
		int stageVer = verList.value( 4, -1 );

		if ( stage == "Dev" && verList[2] == 0 )
			verString = QString( "%1.%2" ).arg( verList[0] ).arg( verList[1] );

		if ( !stage.isEmpty() && stageVer > 0 ) {
			// Append Stage
			verString = QString( "%1 %2 %3" ).arg( verString ).arg( stage ).arg( stageVer );

			if ( showDev ) {
				dev = devs.value( verList.value( 5, -1 ), "" );
				int devVer = verList.value( 6, -1 );

				if ( !dev.isEmpty() && devVer > 0 ) {
					// Append Dev
					verString = QString( "%1 %2 %3" ).arg( verString ).arg( dev ).arg( devVer );
				}
			}
		}
	}

	return verString;
}


QString NifSkopeVersion::rawToMajMin( const QString & ver )
{
	QList<int> parts = versionParts( ver, 2 );

	return QString( "%1.%2" ).arg( parts[0] ).arg( parts[1] );
}


QList<int> NifSkopeVersion::versionParts( const QString & ver, int parts /* = 7 */ )
{
	QList<int> verList;
	
	if ( formatVersion( ver, verList, parts ) )
		return verList;

	return QList<int>();
}


bool NifSkopeVersion::formatVersion( const QString & ver, QList<int> & verNums, int parts )
{
	QString v = ver;

	if ( ver.isEmpty() )
		v = "0.0.0";

	if ( parts > 7 || parts < 2 )
		parts = 3;

	QStringList verParts = v.split( "." );
	QList<int> rawVersionNums;

	// The version string must at least have one period e.g. "1.2"
	if ( verParts.count() < 2 )
		verParts = QStringList( { "0", "0", "0" } );

	QString major = verParts[0];
	QString minor = verParts[1];
	// Deal with missing .0, e.g. "1.2" instead of "1.2.0"
	QString rev = verParts.value( 2, "0" );

	bool majIsNum;
	bool minIsNum;

	int majNum = major.toInt( &majIsNum );
	int minNum = minor.toInt( &minIsNum );

	if ( !majIsNum || !minIsNum )
		return false;

	// Combine Version parts
	rawVersionNums.append( { majNum, minNum } );

	// Confirm fourth part begins with "dev" or "post"
	// and not "a", "b", or "rc" (an invalid naming scheme)
	// dev = 0, post = 1
	QString dev = verParts.value( 3, "" );
	QString devVer = "-1";
	int devCode = -1;

	if ( dev.startsWith( "a" ) || dev.startsWith( "b" ) || dev.startsWith( "rc" ) ) {
		// Invalid version format
		//	Valid: 1.2.0a1.dev1
		//	Invalid: 1.2.0.a1.dev1
		// Remove dot here to compensate
		rev += dev;
		// Reassign dev to 4th value
		dev = verParts.value( 4, "" );
	}

	if ( dev.startsWith( "dev" ) || dev.startsWith( "post" ) ) {
		// Normal developmental suffix
		if ( dev.startsWith( "dev" ) ) {
			devCode = 0;
			devVer = dev.mid( 3 );
		} else {
			devCode = 1;
			devVer = dev.mid( 4 );
		}
	}

	int devNum = devVer.toInt();

	// Stage Code
	//	0 = dev (pre-alpha)
	//	1 = alpha
	//	2 = beta
	//	3 = rc
	//	4 = final
	int stageCode = 4;
	QString stageVer = "-1";

	// Check if Revision has an a, b, rc appended
	bool isFinal;
	rev.toInt( &isFinal );
	if ( !isFinal && rev >= 0 ) {
		// Pre-Release Build
		QStringList revParts; // "0a1" -> ("0", "1")
		QString revPart;      // "0a1" -> "0"
		QString stagePart;    // "0a1" -> "a"
		QString stageVerPart; // "0a1" -> "1"

		if ( rev.startsWith( "dev" ) ) {
			// Pre-Alpha Dev
			//	e.g. 1.2.dev1, not 1.2.0dev1, not 1.2.0.dev1
			stagePart = "dev";
			stageVer = rev.mid( 3 );
			stageCode = 0;
			// Unset rev
			rev = "0";
		} else if ( rev.length() > 1 ) {
			// Pre-Release
			if ( rev.contains( "a" ) ) {
				stagePart = "a";
				stageCode = 1;
			} else if ( rev.contains( "b" ) ) {
				stagePart = "b";
				stageCode = 2;
			} else if ( rev.contains( "rc" ) ) {
				stagePart = "rc";
				stageCode = 3;
			}

			// Splitting at stagePart gives you a list with the rev and stageVer
			//	e.g. "0a1" becomes ("0", "1")
			revParts = rev.split( stagePart, QString::SkipEmptyParts );
			// This is the revision number without e.g. "a1" appended
			revPart = revParts.value( 0, "0" );
			// This is the version of the Alpha/Beta/RC
			stageVerPart = revParts.value( 1, "1" );

			// Set Revision and Stage Version
			rev = revPart;
			stageVer = stageVerPart;
		}
	}

	int stageNum = stageVer.toInt();

	// Append Revision part
	bool revIsNum;
	int revNum = rev.toInt( &revIsNum );

	if ( !revIsNum )
		return false;

	rawVersionNums.append( { revNum, stageCode, stageNum, devCode, devNum } );
	verNums = rawVersionNums.mid( 0, parts );

	return true;
}


// Using std::list<int> here removes the need for the for loop as it
// has its own operator overloads already. Yet best not rely on it.

bool NifSkopeVersion::operator<(const NifSkopeVersion & other) const
{
	QList<int> verParts1, verParts2;

	verParts1 = versionParts( rawVersion, numParts );
	verParts2 = versionParts( other.rawVersion, numParts );

	for ( int i = 0; i < numParts; i++ ) {
		if ( verParts1[i] == verParts2[i] )
			continue;

		return verParts1[i] < verParts2[i];
	}

	return false;
}

bool NifSkopeVersion::operator<=(const NifSkopeVersion & other) const
{
	QList<int> verParts1, verParts2;

	verParts1 = versionParts( rawVersion, numParts );
	verParts2 = versionParts( other.rawVersion, numParts );

	for ( int i = 0; i < numParts; i++ ) {
		if ( verParts1[i] == verParts2[i] )
			continue;

		return verParts1[i] < verParts2[i];
	}

	return true;
}

bool NifSkopeVersion::operator==(const NifSkopeVersion & other) const
{
	QList<int> verParts1, verParts2;

	verParts1 = versionParts( rawVersion, numParts );
	verParts2 = versionParts( other.rawVersion, numParts );

	for ( int i = 0; i < numParts; i++ ) {
		if ( verParts1[i] != verParts2[i] )
			return false;
	}

	return true;
}

bool NifSkopeVersion::operator!=(const NifSkopeVersion & other) const
{
	QList<int> verParts1, verParts2;

	verParts1 = versionParts( rawVersion, numParts );
	verParts2 = versionParts( other.rawVersion, numParts );

	for ( int i = 0; i < numParts; i++ ) {
		if ( verParts1[i] != verParts2[i] )
			return true;
	}

	return false;
}

bool NifSkopeVersion::operator>( const NifSkopeVersion & other ) const
{
	QList<int> verParts1, verParts2;

	verParts1 = versionParts( rawVersion, numParts );
	verParts2 = versionParts( other.rawVersion, numParts );

	for ( int i = 0; i < numParts; i++ ) {
		if ( verParts1[i] == verParts2[i] )
			continue;

		return verParts1[i] > verParts2[i];
	}

	return false;
}

bool NifSkopeVersion::operator>=(const NifSkopeVersion & other) const
{
	QList<int> verParts1, verParts2;

	verParts1 = versionParts( rawVersion, numParts );
	verParts2 = versionParts( other.rawVersion, numParts );

	for ( int i = 0; i < numParts; i++ ) {
		if ( verParts1[i] == verParts2[i] )
			continue;

		return verParts1[i] > verParts2[i];
	}

	return true;
}

bool NifSkopeVersion::operator<(const QString & other) const
{
	return operator<( NifSkopeVersion( other ) );
}

bool NifSkopeVersion::operator<=(const QString & other) const
{
	return operator<=( NifSkopeVersion( other ) );
}

bool NifSkopeVersion::operator==(const QString & other) const
{
	return operator==( NifSkopeVersion( other ) );
}

bool NifSkopeVersion::operator!=(const QString & other) const
{
	return operator!=( NifSkopeVersion( other ) );
}

bool NifSkopeVersion::operator>( const QString & other ) const
{
	return operator>( NifSkopeVersion( other ) );
}

bool NifSkopeVersion::operator>=(const QString & other) const
{
	return operator>=( NifSkopeVersion( other ) );
}


QDebug operator<<( QDebug dbg, const NifSkopeVersion & ver )
{
	ver.setNumParts( 7 );
	return dbg.space() << ver.rawVersion << ver.displayVersion << ver.parts();
}
