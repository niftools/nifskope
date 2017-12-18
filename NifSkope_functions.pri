###############################
## Functions
###############################

# Shortcut for ends of commands
nt = $$escape_expand(\\n\\t)

# Sanitize filepath for OS
defineReplace(syspath) {
	path = $$1
	path ~= s,/,$${QMAKE_DIR_SEP},g

	return($$path)
}


# Get command for sed
defineReplace(getSed) {
	sedbin = ""

	win32 {
		PROG = C:/Program Files (x86)
		!exists($$PROG) {
			PROG = C:/Program Files
		}

		GNUWIN32 = $${PROG}/GnuWin32/bin
		CYGWIN = C:/cygwin/bin
		CYGWIN64 = C:/cygwin64/bin
		SEDPATH = /sed.exe

		exists($${GNUWIN32}$${SEDPATH}) {
			sedbin = $${GNUWIN32}$${SEDPATH}
		} else:exists($${CYGWIN}$${SEDPATH}) {
			sedbin = $${CYGWIN}$${SEDPATH}
		} else:exists($${CYGWIN64}$${SEDPATH}) {
			sedbin = $${CYGWIN64}$${SEDPATH}
		} else {
			#message(Neither GnuWin32 or Cygwin were found)
			sedbin = $$system(where sed 2> NUL)
		}
		
		!isEmpty(sedbin) {
			sedbin = \"$${sedbin}\"
		}
	}

	unix {
		sedbin = sed
	}

	return($$sedbin)
}


# Get command for 7z
defineReplace(get7z) {
	_7zbin = ""

	win32 {
		_7zbin = C:/Program Files/7-Zip/7z.exe

		!exists($$_7zbin) {
			_7zbin = C:/Program Files (x86)/7-Zip/7z.exe
		}
		!exists($$_7zbin) {
			return()
		}
	}

	unix {
		# TODO: Untested
		_7zbin = $$system(which unzip 2>/dev/null)
	}

	return(\"$$_7zbin\")
}


_VERSION =
_REVISION =

# Retrieve NifSkope version
defineReplace(getVersion) {
	# I turned this into a function because I didn't want
	# the Version/Revision macros to have to straddle the
	# includes. (VERSION needed to come before, REVISION after)
	!isEmpty(_VERSION):return($$_VERSION)

	_VERSION = $$cat(build/VERSION)
	export(_VERSION)
	return($$_VERSION)
}

# Retrieve NifSkope revision
defineReplace(getRevision) {

	!isEmpty(_REVISION):return($$_REVISION)

	GIT_HEAD = $$cat(.git/HEAD)
	# At this point GIT_HEAD either contains commit hash, or symbolic ref:
	#	GIT_HEAD = 303c05416ecceb3368997c86676a6e63e968bc9b
	#	GIT_HEAD = ref: refs/head/feature/blabla
	contains(GIT_HEAD, "ref:") {
		# Resolve symbolic ref
		GIT_HEAD = .git/$$member(GIT_HEAD, 1)
		# GIT_HEAD now points to the file containing hash,
		#	e.g. .git/refs/head/feature/blabla
		exists($$GIT_HEAD) {
			GIT_HEAD = $$cat($$GIT_HEAD)
		} else {
			clear(GIT_HEAD)
		}
	}
	count(GIT_HEAD, 1) {
		# Single component, hopefully the commit hash
		# Fetch first seven characters (abbreviated hash)
		GIT_HEAD ~= s/^(.......).*/\\1/
		_REVISION = $$GIT_HEAD
		export(_REVISION)
		return($$_REVISION)
	}
	return()
}

# Format Qt Version
defineReplace(QtHex) {

	maj = $$QT_MAJOR_VERSION
	min = $$QT_MINOR_VERSION
	pat = $$QT_PATCH_VERSION

	greaterThan(min, 9) {
		equals(min, 10):min=a
		equals(min, 11):min=b
		equals(min, 12):min=c
		equals(min, 13):min=d
		equals(min, 14):min=e
		equals(min, 15):min=f
		greaterThan(min, 15):min=f
		# Stop. They won't go this high.
	}

	greaterThan(pat, 9) {
		equals(pat, 10):pat=a
		equals(pat, 11):pat=b
		equals(pat, 12):pat=c
		equals(pat, 13):pat=d
		equals(pat, 14):pat=e
		equals(pat, 15):pat=f
		greaterThan(pat, 15):pat=f
		# Stop. They won't go this high.
	}

	return(0x0$${maj}0$${min}0$${pat})
}

# Format string for Qt DLL

DLLSTRING = $$quote(Qt5%1)
CONFIG(debug, debug|release) {
	DLLEXT = $$quote(d.dll)
} else {
	DLLEXT = $$quote(.dll)
}

# Returns list of absolute paths to Qt DLLs required by project
defineReplace(QtBins) {
	modules = $$eval(QT)
	list =

	for(m, modules) {
		list += $$sprintf($$[QT_INSTALL_BINS]/$${DLLSTRING}, $$m)$${DLLEXT}
	}

	*-g++ {
        # Copies libgcc*-*, libstdc++-*, libwinpthread-*
        #   Note: As of Qt 5.5, changed `lib*` to `lib*-*` in order to avoid unneeded libs.
		list += \
            $$[QT_INSTALL_BINS]/lib*-*.dll
	}

	return($$list)
}

# Copies the given files to the destination directory
defineTest(copyFiles) {
	files = $$1
	subdir = $$2
	abs = $$3
	rename = $$4 # for renaming ONE file, no wildcards
	regexp = $$5 # for file extension renaming, no wildcards

	!isEmpty(rename):!isEmpty(regexp):message(WARNING: Cannot use both 4th and 5th param of copyFiles)

	unset(oldext)
	unset(newext)
	unset(newfile)

	# If `abs` wasn't passed in, make it false
	# (For whatever reason true/false don't pass in as true/false
	# even after using $$eval())
	isEmpty(abs) {
		abs = false
	} else {
		abs = true
	}

	ddir = $$syspath($${DESTDIR}$${QMAKE_DIR_SEP}$${subdir})
	QMAKE_POST_LINK += $$sprintf($$QMAKE_MKDIR_CMD, $${ddir}) $$nt

	for(FILE, files) {
		fileabs = $${PWD}$${QMAKE_DIR_SEP}$${FILE}
		$$abs {
			fileabs = $${FILE}
		}

		!isEmpty(regexp) {
			oldext = $$section(regexp, :, 0, 0)
			newext = $$section(regexp, :, 1, 1)
			newfile = $$section(FILE, /, -1)
			newfile ~= s,.$${oldext},.$${newext},g
		}

		fileabs = $$syspath($${fileabs})

		QMAKE_POST_LINK += $$QMAKE_COPY $${fileabs} $${ddir}$${rename}$${newfile} $$nt
	}

	export(QMAKE_POST_LINK)
	unset(ddir)
}

# Copies the given dirs to the destination directory
defineTest(copyDirs) {
	dirs = $$1
	subdir = $$2
	abs = $$3

	# If `abs` wasn't passed in, make it false
	# (For whatever reason true/false don't pass in as true/false
	# even after using $$eval())
	isEmpty(abs) {
		abs = false
	} else {
		abs = true
	}

	ddir = $$syspath($${DESTDIR}$${QMAKE_DIR_SEP}$${subdir})
	QMAKE_POST_LINK += $$sprintf($$QMAKE_MKDIR_CMD, $${ddir}) $$nt

	for(DIR, dirs) {
		dirabs = $${PWD}$${QMAKE_DIR_SEP}$${DIR}
		$$abs {
			dirabs = $${FILE}
		}

		dirabs = $$syspath($${dirabs})

		# Fix copy for subdir on unix, also assure clean subdirs (no extra files)
		!isEmpty(subdir) {
			win32:*msvc*:QMAKE_POST_LINK += rd /s /q $${ddir} $$nt
			else:!unix:QMAKE_POST_LINK += rm -rf $${ddir} $$nt
			unix:QMAKE_POST_LINK += rm -rf $${ddir} $$nt
		}

		QMAKE_POST_LINK += $$QMAKE_COPY_DIR $${dirabs} $${ddir} $$nt
	}

	export(QMAKE_POST_LINK)
	unset(ddir)
}
