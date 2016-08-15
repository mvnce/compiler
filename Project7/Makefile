# Setup some aliases to these can be easily altered in the future.
GCC = g++
CFLAGS = -O3 -Wall -std=c++11
YACC = bison
LEX = flex

# Link the object files together into the final executable.

tube7: tube-lexer.o tube-parser.tab.o ast.o ic.o type_info.o symbol_table.o
	$(GCC) $(CFLAGS) -O3 tube-parser.tab.o tube-lexer.o ast.o ic.o type_info.o symbol_table.o -o tube7 -ll -ly
	strip tube7


# Use the lex and yacc templates to build the C++ code files.

tube-lexer.o: tube-lexer.cc tube.lex symbol_table.h
	$(GCC) $(CFLAGS) -c tube-lexer.cc

tube-parser.tab.o: tube-parser.tab.cc tube.y symbol_table.h
	$(GCC) $(CFLAGS) -c tube-parser.tab.cc


# Compile the individual code files into object files.

tube-lexer.cc: tube.lex tube-parser.tab.cc symbol_table.h
	$(LEX) -otube-lexer.cc tube.lex

tube-parser.tab.cc: tube.y symbol_table.h
	$(YACC) -o tube-parser.tab.cc -d tube.y

ast.o: ast.cc ast.h symbol_table.h type_info.h
	$(GCC) $(CFLAGS) -c ast.cc

ic.o: ic.cc ic.h symbol_table.h type_info.h
	$(GCC) $(CFLAGS) -c ic.cc

type_info.o: type_info.h type_info.cc
	$(GCC) $(CFLAGS) -c type_info.cc

symbol_table.o: symbol_table.h symbol_table.cc type_info.h
	$(GCC) $(CFLAGS) -c symbol_table.cc


# Cleanup all auto-generated files

clean:
	rm -f tube7 *.o tube-lexer.cc *.tab.cc *.tab.hh *~
