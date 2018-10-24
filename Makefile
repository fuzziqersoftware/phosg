OBJECTS=Concurrency.o ConsistentHashRing.o FileCache.o Filesystem.o Hash.o Image.o JSONPickle.o JSON.o Network.o Process.o Strings.o Time.o UnitTest.o
CXX=g++ -fPIC
CXXFLAGS=-std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-g -std=c++14 -lstdc++

ifeq ($(shell uname -s),Darwin)
	INSTALL_DIR=/opt/local
	CXXFLAGS +=  -DMACOSX -mmacosx-version-min=10.11
	LDFLAGS +=  -mmacosx-version-min=10.11
else
	INSTALL_DIR=/usr/local
	CXXFLAGS +=  -DLINUX
	LDFLAGS +=  -pthread
endif

all: libphosg.a jsonformat test

install: libphosg.a
	mkdir -p $(INSTALL_DIR)/include/phosg
	cp libphosg.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg/
	cp jsonformat $(INSTALL_DIR)/bin/

libphosg.a: $(OBJECTS)
	rm -f libphosg.a
	ar rcs libphosg.a $(OBJECTS)

jsonformat: JSON.o JSONPickle.o Strings.o Filesystem.o Process.o Time.o JSONFormat.o
	$(CXX) $(LDFLAGS) $^ -o $@

test: ConsistentHashRingTest EncodingTest FileCacheTest FilesystemTest HashTest JSONPickleTest JSONTest LRUSetTest ProcessTest StringsTest TimeTest UnitTestTest
	./ConsistentHashRingTest
	./EncodingTest
	./FileCacheTest
	./FilesystemTest
	./HashTest
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
	rm -rf *.dSYM *.o gmon.out libphosg.a jsonformat *Test

.PHONY: clean
