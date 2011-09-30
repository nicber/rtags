HEADERS += $$PWD/Shared.h
SOURCES += $$PWD/Shared.c
INCLUDEPATH += $$PWD
CLANG_PREFIX=$$(CLANG_PREFIX)
isEmpty(CLANG_PREFIX):CLANG_PREFIX=/usr/local
CLANG_EXECUTABLE=$$(CLANG_EXECUTABLE)
isEmpty(CLANG_EXECUTABLE):CLANG_EXECUTABLE=$${CLANG_PREFIX}/bin/clang
DEFINES += CLANG_PREFIX=$$quote(\"$${CLANG_PREFIX}\") CLANG_EXECUTABLE=\"$${CLANG_EXECUTABLE}\"
INCLUDEPATHS += $$(CLANG_PREFIX)/include
release {
    QMAKE_CXXFLAGS += -g
    QMAKE_CFLAGS += -g
}

