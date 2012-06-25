#testsources = test1.cpp TMXLoader.cpp base64.cpp
CPPFLAGS=-std=c++0x -DDEBUG
OBJS=-L../tmx-parser-read-only -I../tmx-parser-read-only -L/usr/local/lib -I/usr/local/include/ -ltmxparser -lSDL -lSDL_image -lSDL_ttf -ltinyxml
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

# vim:set noexpandtab:set nosmarttab:
