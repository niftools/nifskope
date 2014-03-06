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
		SEDPATH = /sed.exe

		exists($${GNUWIN32}$${SEDPATH}) {
			sedbin = \"$${GNUWIN32}$${SEDPATH}\"
		}
	}

	unix {
		sedbin = sed
	}

	return($$sedbin)
}


# Format string for Qt DLL
greaterThan(QT_MAJOR_VERSION, 4) {
	DLLEXT = $$quote(.dll)
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
		# Easiest way to deal with the %1 > %14 issue
		DLLEXT = $$quote(4.dll)

	}
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
		FILEABS = $${PWD}$${QMAKE_DIR_SEP}$${FILE}
		$$abs {
			FILEABS = $${FILE}
		}

		DDIR = $${DESTDIR}$${QMAKE_DIR_SEP}$${subdir}

		# Replace slashes in paths with backslashes for Windows
		win32:FILEABS ~= s,/,$${QMAKE_DIR_SEP},g
		win32:DDIR ~= s,/,$${QMAKE_DIR_SEP},g

		QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $${DDIR} $$QMAKE_MKDIR $${DDIR} $$nt
		QMAKE_POST_LINK += $$QMAKE_COPY $${FILEABS} $${DDIR} $$nt
	}

	export(QMAKE_POST_LINK)
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
		DIRABS = $${PWD}$${QMAKE_DIR_SEP}$${DIR}
		$$abs {
			DIRABS = $${FILE}
		}

		DDIR = $${DESTDIR}$${QMAKE_DIR_SEP}$${subdir}

		# Replace slashes in paths with backslashes for Windows
		win32:DIRABS ~= s,/,$${QMAKE_DIR_SEP},g
		win32:DDIR ~= s,/,$${QMAKE_DIR_SEP},g

		QMAKE_POST_LINK += $$QMAKE_CHK_DIR_EXISTS $${DDIR} $$QMAKE_MKDIR $${DDIR} $$nt
		QMAKE_POST_LINK += $$QMAKE_COPY_DIR $${DIRABS} $${DDIR} $$nt
	}

	export(QMAKE_POST_LINK)
}
