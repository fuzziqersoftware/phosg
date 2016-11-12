OBJECTS=Concurrency.o Filesystem.o Image.o JSONPickle.o JSON.o Network.o Process.o Strings.o Time.o
CXX=g++
CXXFLAGS=-I/usr/local/include -std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror
LDFLAGS=-L/usr/local/lib -std=c++14 -lstdc++

INSTALL_DIR=/usr/local
INCLUDE_INSTALL_DIR=/usr/local/include

all: libphosg.a test

install: libphosg.a
	mkdir -p $(INSTALL_DIR)/include/phosg
	cp libphosg.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg/

libphosg.a: $(OBJECTS)
	ar rcs libphosg.a $(OBJECTS)

test: EncodingTest FilesystemTest JSONPickleTest JSONTest LRUSetTest ProcessTest StringsTest TimeTest
	./EncodingTest
	./FilesystemTest
	./JSONPickleTest
	./JSONTest
	./LRUSetTest
	./ProcessTest
	./StringsTest
	./TimeTest

EncodingTest: EncodingTest.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

FilesystemTest: FilesystemTest.o Filesystem.o Strings.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

JSONPickleTest: JSONPickleTest.o JSON.o Strings.o Filesystem.o JSONPickle.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

JSONTest: JSONTest.o JSON.o Strings.o Filesystem.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

LRUSetTest: LRUSetTest.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

ProcessTest: ProcessTest.o Process.o Filesystem.o Strings.o Time.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

StringsTest: StringsTest.o Strings.o Filesystem.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

TimeTest: TimeTest.o Time.o
	$(CXX) -std=c++14 -lstdc++ $^ -o $@

clean:
	rm -rf *.dSYM *.o gmon.out libphosg.a *Test

.PHONY: clean
