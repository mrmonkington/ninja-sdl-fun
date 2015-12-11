#testsources = test1.cpp TMXLoader.cpp base64.cpp
CPPFLAGS=-std=c++0x -DDEBUG -I../tmx-parser-read-only -I/usr/local/include/
OBJS=-L../tmx-parser-read-only -L/usr/local/lib -ltmxparser -lSDL -lSDL_image -lSDL_ttf -ltinyxml -lz -lBox2D
#OBJS=-lSDL -lSDL_image -lSDL_ttf -ltinyxml
INCS="-ITmxParser"

#CPPFLAGS=""

ALL : ninjabox

#test1 : $(testsources)
#	g++ -g $(CPPFLAGS) -o test1 $(testsources) -lSDL -lSDL_image -lSDL_ttf -ltinyxml

ninja : ninja.cpp
	g++ -g $(CPPFLAGS) -o ninja ninja.cpp $(OBJS)

ninjabox : ninjabox.cpp
	g++ -DDEBUG -g $(CPPFLAGS) -o ninjabox ninjabox.cpp $(OBJS)

# vim:set noexpandtab:set nosmarttab:
