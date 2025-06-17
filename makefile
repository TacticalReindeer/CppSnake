all: Program.exe

Program.exe: Program.o d:/TestSite/C++Library/Vector2.o
	g++ d:/TestSite/C++Library/Vector2.o Program.o -o Snake.exe

Program.o: Program.cpp
	g++ -I d:/TestSite/C++Library -c Program.cpp

Vector2.o: d:/TestSite/C++Library/Vector2.cpp
	g++ -c d:/TestSite/C++Library/Vector2.cpp

clean:
	rm *.o

%.o: %.cpp %.h
	g++ -c $<