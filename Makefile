## Haiku Generic Makefile (based on the makefile-engine template).
## Builds TarballOMatic outside of the Haiku source tree.

NAME = TarballOMatic
TYPE = APP
APP_MIME_SIG = application/x-vnd.haiku.tarball-o-matic

SRCS = GenericThread.cpp \
	TarballOMatic.cpp \
	TarballOMaticActivity.cpp \
	TarballOMaticWindow.cpp \
	TarballThread.cpp

RDEFS = TarballOMatic.rdef

LIBS = be tracker localestub $(STDCPPLIBS)
LIBPATHS =
SYSTEM_INCLUDE_PATHS =
LOCAL_INCLUDE_PATHS =
OPTIMIZE := FULL
LOCALES =
DEFINES =
WARNINGS = ALL
SYMBOLS :=
DEBUGGER :=
COMPILER_FLAGS =
LINKER_FLAGS =

## Include the makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
