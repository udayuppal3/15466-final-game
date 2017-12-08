.PHONY : all clean

CPP=g++ -g -Wall -Werror -std=c++11 -I./kit-libs-linux/SDL2/include/ -I../kit-libs-linux/glm/include
SDL_LIBS=-L../kit-libs-linux/SDL2/lib/ -lGL -lpng -lSDL2 -lpthread -ldl -lm

all : dist/main

clean :
	rm -rf main objs

dist/main : objs/main.o objs/load_save_png.o
	$(CPP) -o $@ $^ $(SDL_LIBS)


objs/main.o : main.cpp load_save_png.hpp GL.hpp glcorearb.h gl_shims.hpp
	mkdir -p objs
	$(CPP) -c -o $@ $<

objs/load_save_png.o : load_save_png.cpp load_save_png.hpp GL.hpp glcorearb.h gl_shims.hpp
	mkdir -p objs
	$(CPP) -c -o $@ $<
