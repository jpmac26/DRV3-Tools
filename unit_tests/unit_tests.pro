#-------------------------------------------------
#
# Project created by QtCreator 2018-03-02T11:18:55
#
#-------------------------------------------------

QT       += testlib

QT       -= gui

TARGET = unit_tests
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    unit_tests.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../utils/release/ -lutils
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../utils/debug/ -lutils
else:unix: LIBS += -L$$OUT_PWD/../utils/ -lutils

INCLUDEPATH += $$PWD/../utils
DEPENDPATH += $$PWD/../utils
EPS += $$OUT_PWD/../utils/debug/utils.lib

#QMAKE_POST_LINK += $$quote($(MKDIR) $$OUT_PWD/test_data)$$escape_expand(\n\t)
QMAKE_POST_LINK += $$quote($(COPY_DIR) $$shell_path($$PWD/test_data) $$shell_path($$OUT_PWD/test_data))
