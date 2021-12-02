CCFLAGS=-DDEBUG -Wall -g

compiler: lex.yy.o yacc.tab.o utils.o
	g++ -Wno-register -O2 -lm -std=c++17 lex.yy.o yacc.tab.o utils.o -o compiler -Idirs ${CCFLAGS}

lex.yy.cpp: lex.l utils.hpp
	flex -o lex.yy.cpp lex.l

yacc.tab.hpp: yacc.y utils.hpp
	bison -d -o yacc.tab.cpp yacc.y

lex.yy.o: utils.hpp lex.yy.cpp yacc.tab.hpp
	g++ -Wno-register -O2 -lm -std=c++17 lex.yy.cpp -c -Idirs ${CCFLAGS}

yacc.tab.o: utils.hpp yacc.tab.hpp
	g++ -Wno-register -O2 -lm -std=c++17 yacc.tab.cpp -c -Idirs ${CCFLAGS}

utils.o: utils.hpp utils.cpp
	g++ -Wno-register -O2 -lm -std=c++17 utils.cpp -c -Idirs ${CCFLAGS}

clean:
	rm -rf compiler *.o lex.yy.cpp yacc.tab.cpp yacc.tab.hpp

.PHONY: clean