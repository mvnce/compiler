CXX	= g++

.cc.o:
	$(CXX) -O3 -c $< -o $*.o

default: all

all: tube4

tube4.tab.cc: tube4.y
	bison -o tube4.tab.cc -b tube4 -d tube4.y

tube4.yy.cc: tube4.lex
	flex -o tube4.yy.cc tube4.lex

tube4: tube4.tab.o tube4.yy.o ast.o
	$(CXX) -o tube4 tube4.tab.o tube4.yy.o ast.o -ll -ly

clean:
	rm -f tube4.tab.cc tube4.tab.hh tube4.yy.cc *~ *.o tube4
