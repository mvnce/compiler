# Setup some aliases to these can be easily altered in the future.
GCC = g++
CFLAGS = -O3 -Wall -std=c++11
YACC = bison
LEX = flex

# Link the object files together into the final executable.

tube6: tube6-lexer.o tube6-parser.tab.o ast.o tc.o type_info.o symbol_table.o
	$(GCC) $(CFLAGS) -O3 tube6-parser.tab.o tube6-lexer.o ast.o tc.o type_info.o symbol_table.o -o tube6 -ll -ly


# Use the lex and yacc templates to build the C++ code files.

tube6-lexer.o: tube6-lexer.cc tube6.lex symbol_table.h
	$(GCC) $(CFLAGS) -c tube6-lexer.cc

tube6-parser.tab.o: tube6-parser.tab.cc tube6.y symbol_table.h
	$(GCC) $(CFLAGS) -c tube6-parser.tab.cc


# Compile the individual code files into object files.

tube6-lexer.cc: tube6.lex tube6-parser.tab.cc symbol_table.h
	$(LEX) -otube6-lexer.cc tube6.lex

tube6-parser.tab.cc: tube6.y symbol_table.h
	$(YACC) -o tube6-parser.tab.cc -d tube6.y

ast.o: ast.cc ast.h symbol_table.h type_info.h
	$(GCC) $(CFLAGS) -c ast.cc

tc.o: tc.cc tc.h symbol_table.h type_info.h
	$(GCC) $(CFLAGS) -c tc.cc

type_info.o: type_info.h type_info.cc
	$(GCC) $(CFLAGS) -c type_info.cc

symbol_table.o: symbol_table.h symbol_table.cc type_info.h
	$(GCC) $(CFLAGS) -c symbol_table.cc


# Cleanup all auto-generated files

clean:
	rm -f tube6 *.o tube6-lexer.cc *.tab.cc *.tab.hh *~
