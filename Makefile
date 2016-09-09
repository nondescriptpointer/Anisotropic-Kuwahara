MAIN = kuwahara
CC = g++
INCDIRS = -I/home/ego/rnd/gliby/include/
CXXFLAGS = $(COMPILERFLAGS) -O3 -march=native -pipe -std=c++0x -Wall -g $(INCDIRS)
CFLAGS = -g $(INCDIRS)
LIBS = -lGL -lGLU -lGLEW -lglfw -lboost_system -lboost_filesystem -lpthread -lfreeimage

prog : $(MAIN)

$(MAIN).o : $(MAIN).cpp

build/%.o : %.cpp
	$(CC) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

build/gliby/%.o : /home/ego/rnd/gliby/src/%.cpp
	$(CC) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

$(MAIN) : build/$(MAIN).o
	$(CC) -o $(MAIN) $^ $(LIBS)

.PHONY: clean
clean:
	rm -f build/*.o
	rm -f build/gliby/*.o
