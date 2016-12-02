OBJECTS=Concurrency.o Filesystem.o Image.o JSONPickle.o JSON.o Network.o Process.o Strings.o Time.o UnitTest.o
CXX=g++ -fPIC
CXXFLAGS=-I/usr/local/include -std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-L/usr/local/lib -std=c++14 -lstdc++

INSTALL_DIR=/usr/local

ifeq ($(shell uname -s),Darwin)
	CXXFLAGS +=  -arch i386 -arch x86_64
	LDFLAGS +=  -arch i386 -arch x86_64
endif

all: libphosg.a test

install: libphosg.a
	mkdir -p $(INSTALL_DIR)/include/phosg
	cp libphosg.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg/

libphosg.a: $(OBJECTS)
	rm -f libphosg.a
	ar rcs libphosg.a $(OBJECTS)

test: EncodingTest FilesystemTest JSONPickleTest JSONTest LRUSetTest ProcessTest StringsTest TimeTest UnitTestTest
	./EncodingTest
	./FilesystemTest
	./JSONPickleTest
	./JSONTest
	./LRUSetTest
	./ProcessTest
	./StringsTest
	./TimeTest
	./UnitTestTest

%Test: %Test.o libphosg.a
	$(CXX) $(LDFLAGS) $^ -o $@

clean:
	rm -rf *.dSYM *.o gmon.out libphosg.a *Test

.PHONY: clean
