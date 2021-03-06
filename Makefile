platform=$(shell uname)

CFLAGS=-O2 -g -I./src/plog-1.0.0/include
ifeq ($(platform),Darwin)
  ALL=bin/dummy_client bin/dummy_server bin/gl_server
  GL_OPTS=-framework OpenGL -framework GLUT -Wno-deprecated-declarations
else ifeq ($(platform),Linux)
  ALL=bin/dummy_client bin/dummy_server bin/tcl_server bin/ws2801_server bin/lpd8806_server bin/gl_server
  GL_OPTS=-lGL -lglut -lGLU -lm
endif

all: $(ALL)

clean:
	rm -rf bin/*

bin/dummy_client: src/dummy_client.c src/opc_client.c
	mkdir -p bin
	gcc ${CFLAGS} -o $@ $^

bin/dummy_server: src/dummy_server.c src/opc_server.cpp
	mkdir -p bin
	g++ ${CFLAGS} -o $@ $^

bin/tcl_server: src/tcl_server.cpp src/opc_server.cpp src/spi.cpp
	mkdir -p bin
	g++ -std=c++0x ${CFLAGS} -o $@ $^

bin/ws2801_server: src/ws2801_server.c src/opc_server.c src/spi.c
	mkdir -p bin
	gcc ${CFLAGS} -o $@ $^

bin/lpd8806_server: src/lpd8806_server.c src/opc_server.c src/spi.c
	mkdir -p bin
	gcc ${CFLAGS} -o $@ $^

bin/gl_server: src/gl_server.cpp src/opc_server.cpp src/cJSON.c
	mkdir -p bin
	g++ ${CFLAGS} -o $@ $^ $(GL_OPTS)
