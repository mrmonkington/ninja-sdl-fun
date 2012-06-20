#testsources = test1.cpp TMXLoader.cpp base64.cpp
ninjasources = ninja.cpp
CPPFLAGS=-std=c++0x -DDEBUG
OBJS=-LTmxParser -ltmxparser -lSDL -lSDL_image -lSDL_ttf -ltinyxml
#OBJS=-lSDL -lSDL_image -lSDL_ttf -ltinyxml
INCS="-ITmxParser"

#CPPFLAGS=""

ALL : ninja

#test1 : $(testsources)
#	g++ -g $(CPPFLAGS) -o test1 $(testsources) -lSDL -lSDL_image -lSDL_ttf -ltinyxml

ninja : $(ninjasources)
	g++ -g $(CPPFLAGS) -o ninja $(ninjasources) $(OBJS)

# vim:set noexpandtab:set nosmarttab:
