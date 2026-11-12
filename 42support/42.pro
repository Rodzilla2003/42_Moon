TEMPLATE = app
TARGET = 42sh_moon
QT = core gui widgets
CONFIG += console c++11
CONFIG -= app_bundle
DEFINES += _USE_QTPLOT_ _ENABLE_GUI_ _USE_SHADERS_ _USE_GLUT_
INCLUDEPATH += $$PWD/../42/Include $$PWD/../42/Kit/Include $$PWD/../lunar
INCLUDEPATH += $$PWD/glew/include $$PWD/glew/include/GL $$PWD/freeglut/include $$PWD/freeglut/include/GL
include(sources.pri)
SOURCES += $$PWD/../lunar/qt_main.cpp $$PWD/../42/Source/42gl.c $$PWD/../lunar/moon_glut.c $$PWD/../42/Source/42gpgpu.c $$PWD/../42/Kit/Source/glkit.c
OBJECTS_DIR = $$OUT_PWD/obj/$$QT_ARCH
DESTDIR = $$OUT_PWD/bin/$$QT_ARCH
win32 {
    DEFINES += GLEW_STATIC
    SOURCES += $$PWD/glew/src/glew.c
    contains(QT_ARCH, x86_64) {
        LIBS += $$PWD/freeglut/lib/x64/libfreeglut.a
        FG_DLL = $$PWD/freeglut/bin/x64/freeglut.dll
    } else {
        LIBS += $$PWD/freeglut/lib/libfreeglut.a
        FG_DLL = $$PWD/freeglut/bin/freeglut.dll
    }
    LIBS += -lopengl32 -lglu32 -lws2_32
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$FG_DLL)) $$shell_quote($$shell_path($$DESTDIR)) $$escape_expand(\n\t)
}
macx { LIBS += -framework OpenGL -framework GLUT }
unix:!macx { DEFINES += GLEW_STATIC; SOURCES += $$PWD/glew/src/glew.c; LIBS += -lGL -lGLU -lglut }
