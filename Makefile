.PHONY : all clean

CPP=g++ -g -Wall -Werror -std=c++11
SDL_LIBS=`sdl2-config --libs` -lGL -lpng

all : dist/main

clean :
	rm -rf main objs

dist/main : objs/main.o objs/load_save_png.o
	$(CPP) -o $@ $^ $(SDL_LIBS)


objs/main.o : main.cpp load_save_png.hpp GL.hpp glcorearb.h gl_shims.hpp
	mkdir -p objs
	$(CPP) -c -o $@ $< `sdl2-config --cflags`

objs/load_save_png.o : load_save_png.cpp load_save_png.hpp GL.hpp glcorearb.h gl_shims.hpp
	mkdir -p objs
	$(CPP) -c -o $@ $< `sdl2-config --cflags`
