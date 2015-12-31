dirs = -I../../include/ -I../../src/ -L../../libs/
libs = -lm -lX11 -lXxf86vm -lGL -lopenal -lfreetype -lpng -lzip -ljpeg -lmpg123 -lvorbis -lvorbisfile
sources = ../../src/*.c ../../src/*.cpp

all:
	mkdir -p ../../bin/linux
	gcc -shared -Wall $(dirs) $(sources) $(libs) -Wwrite-strings -DGK_PLATFORM_LINUX -o ../../bin/linux/libGK.so

install: ../../bin/linux/libGK.so
	cp ../../bin/linux/libGK.so /usr/local/lib/
	chmod 0775 /usr/local/lib/libGK.so
	cp ../../include/* /usr/local/include/
	ldconfig

test: copyassets
	gcc -Wall $(dirs) $(sources) $(libs) -Wwrite-strings -DGK_PLATFORM_LINUX -DGK_PLATFORM_TEST -o../../bin/linux/test

copyassets:
	mkdir -p ../../bin/linux/
	mkdir -p ../../bin/linux/assets/
	cp ../../build/emscripten/assets/* ../../bin/linux/assets/

clean:
	rm -rf ../../bin/linux
