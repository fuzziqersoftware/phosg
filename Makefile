OBJECTS=Concurrency.o ConsistentHashRing.o FileCache.o Filesystem.o Image.o JSONPickle.o JSON.o Network.o Process.o Strings.o Time.o UnitTest.o
CXX=g++ -fPIC
CXXFLAGS=-std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-g -std=c++14 -lstdc++

ifeq ($(shell uname -s),Darwin)
	INSTALL_DIR=/opt/local
	CXXFLAGS +=  -arch i386 -arch x86_64 -DMACOSX
	LDFLAGS +=  -arch i386 -arch x86_64
else
	INSTALL_DIR=/usr/local
	CXXFLAGS +=  -DLINUX
	LDFLAGS +=  -pthread
endif

all: libphosg.a test

install: libphosg.a
	mkdir -p $(INSTALL_DIR)/include/phosg
	cp libphosg.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg/

libphosg.a: $(OBJECTS)
	rm -f libphosg.a
	ar rcs libphosg.a $(OBJECTS)

test: ConsistentHashRingTest EncodingTest FilesystemTest JSONPickleTest JSONTest LRUSetTest ProcessTest StringsTest TimeTest UnitTestTest
	./ConsistentHashRingTest
	./EncodingTest
	./FilesystemTest
	./JSONPickleTest
	./JSONTest
	./LRUSetTest
	./ProcessTest
	./StringsTest
	./TimeTest
	./UnitTestTest

%Test: %Test.o $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.dSYM *.o gmon.out libphosg.a *Test

.PHONY: clean
