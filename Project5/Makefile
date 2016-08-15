exe = tube5

$(exe): tube clean-except-exe
	mv tube $(exe)

tube: tube.tab.o tube.yy.o ast.o
	g++ -o tube tube.tab.o tube.yy.o ast.o -ll -ly

ast.o: ast.h ast.cc
	g++ -c ast.cc

tube.tab.o: tube.tab.cc
	g++ -c tube.tab.cc

tube.yy.o: tube.yy.cc
	g++ -c tube.yy.cc

tube.tab.cc: tube.y symbol_table.h ast.h
	bison -o tube.tab.cc -b tube -d tube.y

tube.yy.cc: tube.lex symbol_table.h ast.h
	flex -o tube.yy.cc tube.lex

exam: $(exe)
	$(exe) example.tube example.tic
	cat example.tic
	../TubeCode/TubeIC example.tic

clean-except-exe:
	rm tube.tab.o tube.yy.o tube.tab.cc tube.yy.cc tube.tab.hh example.out *.tic *.out *.o 2>/dev/null || true

clean: clean-except-exe
	rm tube $(exe) 2>/dev/null || true
