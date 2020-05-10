OBJECTS=Concurrency.o ConsistentHashRing.o Encoding.o FileCache.o Filesystem.o Hash.o Image.o JSONPickle.o JSON.o Network.o Process.o Random.o Strings.o Time.o UnitTest.o
CXX=g++ -fPIC
CXXFLAGS=-std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-g -std=c++14 -lstdc++
LDLIBS=

ifeq ($(OS),Windows_NT)
	# INSTALL_DIR not set because we don't support installing on windows. also,
	# we expect gcc and whatnot to be on PATH; probably the user has to set this
	# up manually but w/e
	CXXFLAGS +=  -DWINDOWS -D__USE_MINGW_ANSI_STDIO=1 -D_WIN32_WINNT=0x0601 -DNTDDI_VERSION=NTDDI_WIN7
	LDLIBS += -lws2_32
	EXE_EXTENSION=.exe
else
	EXE_EXTENSION=

	ifeq ($(shell uname -s),Darwin)
		INSTALL_DIR=/opt/local
		CXXFLAGS +=  -DMACOSX -mmacosx-version-min=10.11
		LDFLAGS +=  -mmacosx-version-min=10.11
	else
		INSTALL_DIR=/usr/local
		CXXFLAGS +=  -DLINUX
		LDFLAGS +=  -pthread
	endif
endif

all: libphosg.a jsonformat$(EXE_EXTENSION) test

install: libphosg.a
	mkdir -p $(INSTALL_DIR)/include/phosg
	cp libphosg.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg/
	cp jsonformat $(INSTALL_DIR)/bin/

libphosg.a: $(OBJECTS)
	rm -f libphosg.a
	ar rcs libphosg.a $(OBJECTS)

jsonformat$(EXE_EXTENSION): JSON.o JSONPickle.o Strings.o Filesystem.o Process.o Time.o JSONFormat.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

test: ConsistentHashRingTest$(EXE_EXTENSION) EncodingTest$(EXE_EXTENSION) FileCacheTest$(EXE_EXTENSION) FilesystemTest$(EXE_EXTENSION) HashTest$(EXE_EXTENSION) ImageTest$(EXE_EXTENSION) JSONPickleTest$(EXE_EXTENSION) JSONTest$(EXE_EXTENSION) KDTreeTest$(EXE_EXTENSION) LRUSetTest$(EXE_EXTENSION) ProcessTest$(EXE_EXTENSION) StringsTest$(EXE_EXTENSION) TimeTest$(EXE_EXTENSION) UnitTestTest$(EXE_EXTENSION)
	./ConsistentHashRingTest$(EXE_EXTENSION)
	./EncodingTest$(EXE_EXTENSION)
	./FileCacheTest$(EXE_EXTENSION)
	./FilesystemTest$(EXE_EXTENSION)
	./HashTest$(EXE_EXTENSION)
	./ImageTest$(EXE_EXTENSION)
	./JSONPickleTest$(EXE_EXTENSION)
	./JSONTest$(EXE_EXTENSION)
	./KDTreeTest$(EXE_EXTENSION)
	./LRUSetTest$(EXE_EXTENSION)
	./ProcessTest$(EXE_EXTENSION)
	./StringsTest$(EXE_EXTENSION)
	./TimeTest$(EXE_EXTENSION)
	./UnitTestTest$(EXE_EXTENSION)

%Test$(EXE_EXTENSION): %Test.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -rf *.dSYM *.o gmon.out libphosg.a jsonformat$(EXE_EXTENSION) *Test$(EXE_EXTENSION)

.PHONY: clean
