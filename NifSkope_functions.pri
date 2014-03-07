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
		win32:!exists($$PROG) {
			PROG = C:/Program Files
		}

		GNUWIN32 = $${PROG}/GnuWin32/bin
		CYGWIN = C:/cygwin/bin
		SEDPATH = /sed.exe

		exists($${GNUWIN32}$${SEDPATH}) {
			sedbin = \"$${GNUWIN32}$${SEDPATH}\"
		} else:exists($${CYGWIN}$${SEDPATH}) {
			sedbin = \"$${CYGWIN}$${SEDPATH}\"
		} else {
			#message(Neither GnuWin32 or Cygwin were found)
			sedbin = $$system(where sed 2> NUL)
		}
	}

	unix {
		sedbin = sed
	}

	return($$sedbin)
}


# Format string for Qt DLL
DLLEXT = $$quote(.dll)
greaterThan(QT_MAJOR_VERSION, 4) {
	CONFIG(debug, debug|release) {
		DLLSTRING = $$quote(Qt5%1d)
	} else {
		DLLSTRING = $$quote(Qt5%1)
	}
} else {
	CONFIG(debug, debug|release) {
		DLLSTRING = $$quote(Qt%1d)
	} else {
		DLLSTRING = $$quote(Qt%1)
	}
	# Easiest way to deal with the %1 > %14 issue
	DLLEXT = $$quote(4.dll)
}

# Returns list of absolute paths to Qt DLLs required by project
defineReplace(QtBins) {
	modules = $$eval(QT)
	list =

	for(m, modules) {
		list += $$sprintf($$[QT_INSTALL_BINS]/$${DLLSTRING}, $$m)$${DLLEXT}
	}
	return($$list)
}

# Copies the given files to the destination directory
defineTest(copyFiles) {
	files = $$1
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
	!isEmpty(subdir) {
		subdir = $${subdir}$${QMAKE_DIR_SEP}
	}

	for(FILE, files) {
		fileabs = $${PWD}$${QMAKE_DIR_SEP}$${FILE}
		$$abs {
			fileabs = $${FILE}
		}

		ddir = $${DESTDIR}$${QMAKE_DIR_SEP}$${subdir}

		# Replace slashes in paths with backslashes for Windows
		fileabs = $$syspath($${fileabs})
		ddir = $$syspath($${ddir})

		QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $${ddir} $$QMAKE_MKDIR $${ddir} $$nt
		QMAKE_POST_LINK += $$QMAKE_COPY $${fileabs} $${ddir} $$nt
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
	!isEmpty(subdir) {
		subdir = $${subdir}$${QMAKE_DIR_SEP}
	}

	for(DIR, dirs) {
		dirabs = $${PWD}$${QMAKE_DIR_SEP}$${DIR}
		$$abs {
			dirabs = $${FILE}
		}

		ddir = $${DESTDIR}$${QMAKE_DIR_SEP}$${subdir}

		# Replace slashes in paths with backslashes for Windows
		dirabs = $$syspath($${dirabs})
		ddir = $$syspath($${ddir})

		QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $${ddir} $$QMAKE_MKDIR $${ddir} $$nt
		QMAKE_POST_LINK += $$QMAKE_COPY_DIR $${dirabs} $${ddir} $$nt
	}

	export(QMAKE_POST_LINK)
	unset(ddir)
}
