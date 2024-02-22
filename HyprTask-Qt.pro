TARGET = HyprTask
CONFIG += c++17
QT += core gui widgets
SOURCES += src/main.cpp
#HEADERS += include/ClientWindow.h
# RESOURCES += resources/qrc/resources.qrc
# INCLUDEPATH += /path/to/additional/includes
# LIBS += -L/path/to/additional/libs -lnameOfLib
# Enable debug symbols in debug mode, and high optimization in release mode
CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -g
} else {
    QMAKE_CXXFLAGS += -O3
}
