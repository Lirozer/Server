CXX = g++
CXX_FLAGS = -std=c++17 -Wall -Wextra -g -O2
TARGET = server

SOURCES = $(wildcard src/*.cpp)
HEADERS = $(wildcard src/*.h)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXX_FLAGS) -o $(TARGET) $(SOURCES)

package: $(TARGET)
	mkdir -p package/DEBIAN
	mkdir -p package/usr/local/bin
	mkdir -p package/lib/systemd/system

	cp $(TARGET) package/usr/local/bin/
	cp my-server.service package/lib/systemd/system
	cp debian/control package/DEBIAN
	dpkg-deb --build package server.deb

	rm -rf package

clean:
	rm -f $(TARGET)
	rm -rf package
	rm -f server.deb