# QMake project file to build jnotify native libraries
TEMPLATE = lib
CONFIG -= qt
TARGET = aerofsjn
#DESTDIR = $$PWD/build

#QMAKE_CXX_FLAGS += -D_FILE_OFFSET_BITS=64 -fmessage-length=0 -fno-rtti

macx {
	# separate SDKROOT optional
	#SDKROOT = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.6.sdk/
	QMAKE_CXXFLAGS += -mmacosx-version-min=10.5 -fPIC -arch x86_64 -O3 -Wall
	INCLUDEPATH += $$PWD/osx
	INCLUDEPATH += "$$(SDKROOT)/System/Library/Frameworks/JavaVM.framework/Headers"
	HEADERS += osx/net_contentobjects_jnotify_macosx_JNotify_macosx.h
	SOURCES += osx/net_contentobjects_jnotify_macosx_JNotifyAdapterMacOSX_JNFile.c \
	           osx/net_contentobjects_jnotify_macosx_JNotify_macosx.c
	LIBS += -framework CoreFoundation -framework CoreServices
}
win32 {
	# TODO: complete/test
	QMAKE_CXXFLAGS += -DUNICODE -D_UNICODE
	INCLUDEPATH += $$PWD/win32
	INCLUDEPATH += "$$(JAVA_HOME)\\include"
	INCLUDEPATH += "$$(JAVA_HOME)\\include\\win32"
	HEADERS += win32/net_contentobjects_jnotify_win32_JNotify_win32.h \
	           win32/Lock.h                                           \
	           win32/Logger.h                                         \
	           win32/WatchData.h                                      \
	           win32/Win32FSHook.h                                    \

	SOURCES += win32/net_contentobjects_jnotify_win32_JNotify_win32.cpp \
	           win32/Lock.cpp                                           \
	           win32/Logger.cpp                                         \
	           win32/WatchData.cpp                                      \
	           win32/Win32FSHook.cpp                                    \

}
linux-g++ {
	# TODO: complete/test
	#DEFINES += "_FILE_OFFSET_BITS=64"
	INCLUDEPATH += $$PWD/linux
	INCLUDEPATH += "$$(JAVA_HOME)/include"
	INCLUDEPATH += "$$(JAVA_HOME)/include/linux"
	HEADERS += linux/inotify-syscalls.h \
	           linux/net_contentobjects_jnotify_linux_JNotify_linux.h
	SOURCES += linux/net_contentobjects_jnotify_linux_JNotify_linux.c
}
