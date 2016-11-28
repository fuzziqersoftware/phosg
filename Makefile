ARCH?=64
OBJECTS=Concurrency.o Filesystem.o Image.o JSONPickle.o JSON.o Network.o Process.o Strings.o Time.o UnitTest.o
CXX=g++
CXXFLAGS=-I/usr/local/include -std=c++14 -g -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H -Wall -Werror -m$(ARCH) -DARCH=$(ARCH)
LDFLAGS=-L/usr/local/lib -std=c++14 -lstdc++ -m$(ARCH)

INSTALL_DIR=/usr/local
INCLUDE_INSTALL_DIR=/usr/local/include

all: libphosg.a test

install: libphosg.a
	mkdir -p $(INSTALL_DIR)/include/phosg
	cp libphosg.a $(INSTALL_DIR)/lib/
	cp -r *.hh $(INSTALL_DIR)/include/phosg/

libphosg.a: $(OBJECTS)
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
