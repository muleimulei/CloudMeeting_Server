TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    error.cpp \
    net.cpp \
    unpthread.cpp \
    wrapunix.cpp \
    userdeal.cpp

HEADERS += \
    unp.h \
    unpthread.h

LIBS += -pthread
