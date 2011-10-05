/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2010, NIF File Format Library and Tools
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

// defines

#ifndef CONFIG_H
#define CONFIG_H

#include <QStringList>

/*! \file config.h
 * \brief Configuration info
 *
 * Include this if you want to access the current version or persistent QSettings.
 */

//! QSettings keys for older versions of nifskope
/*!
 * Add versions to this list (most recent first) whenever incrementing NIFSKOPE_VERSION
 */
const QStringList NIFSKOPE_OLDERVERSIONS = (QStringList()
		<< "NifSkope-2.0.0"
		<< "NifSkope-1.1.0"
		<< "NifSkope-1.0.22"
		<< "NifSkope-1.0.21"
		<< "NifSkope-1.0.20"
		<< "NifSkope-1.0.19"
		<< "NifSkope-1.0.18"
		<< "NifSkope-1.0.17"
		<< "NifSkope-1.0.16"
		<< "NifSkope-1.0.15"
		<< "NifSkope-1.0.14"
		<< "NifSkope-1.0.13"
		<< "NifSkope-1.0.12"
		<< "NifSkope-1.0.11"
		<< "NifSkope-1.0.10"
		<< "NifSkope-1.0.9"
		<< "NifSkope-1.0.8"
		<< "NifSkope-1.0.7"
		<< "NifSkope-1.0.6"
		<< "NifSkope-1.0.5"
		<< "NifSkope");

//! A string describing the version of nifskope
#define NIFSKOPE_VERSION "2.0.0"

//! The nifskope revision number;
#define NIFSKOPE_REVISION "42"

//! Create or use a QSettings variable for nifskope
#define NIFSKOPE_QSETTINGS(config) QSettings config( "NifTools", "NifSkope-"NIFSKOPE_VERSION )

#endif
