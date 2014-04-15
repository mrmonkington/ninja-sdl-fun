#testsources = test1.cpp TMXLoader.cpp base64.cpp
CPPFLAGS=-std=c++0x -DDEBUG -I../tmx-parser-read-only -I/usr/local/include/
OBJS=-L../tmx-parser-read-only -L/usr/local/lib -ltmxparser -lSDL -lSDL_image -lSDL_ttf -ltinyxml -lz
#OBJS=-lSDL -lSDL_image -lSDL_ttf -ltinyxml
INCS="-ITmxParser"

#CPPFLAGS=""

ALL : ninjabox

#test1 : $(testsources)
#	g++ -g $(CPPFLAGS) -o test1 $(testsources) -lSDL -lSDL_image -lSDL_ttf -ltinyxml

ninja : ninja.cpp
	g++ -g $(CPPFLAGS) -o ninja ninja.cpp $(OBJS)

ninjabox : ninjabox.cpp
	g++ -g $(CPPFLAGS) -I/usr/local/include/Box2D/ -o ninjabox ninjabox.cpp $(OBJS) -lBox2D

gears: gears.c
	gcc $(CPPFLAGS) -c gears.c
	gcc $(CPPFLAGS) -o gears gears.o -lSDL -lGL -lGLU

# vim:set noexpandtab:set nosmarttab:
