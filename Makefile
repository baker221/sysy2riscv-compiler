BUILD_DIR ?= $(shell pwd)
CCFLAGS=-DDEBUG -Wall -g
#CCFLAGS=-DDEBUG -w -g

$(BUILD_DIR)/compiler: lex.yy.o yacc.tab.o tiggerutils.o utils.o
	g++ -Wno-register -O2 -lm -std=c++17 lex.yy.o yacc.tab.o tiggerutils.o utils.o -o $(BUILD_DIR)/compiler -Idirs ${CCFLAGS}

lex.yy.cpp: lex.l utils.hpp
	flex -o lex.yy.cpp lex.l

yacc.tab.hpp: yacc.y utils.hpp
	bison -d -o yacc.tab.cpp yacc.y

lex.yy.o: utils.hpp lex.yy.cpp yacc.tab.hpp
	g++ -Wno-register -O2 -lm -std=c++17 lex.yy.cpp -c -Idirs ${CCFLAGS}

yacc.tab.o: utils.hpp yacc.tab.hpp
	g++ -Wno-register -O2 -lm -std=c++17 yacc.tab.cpp -c -Idirs ${CCFLAGS}

tiggerutils.o: tiggerutils.hpp
	g++ -Wno-register -O2 -lm -std=c++17 tiggerutils.cpp -c -Idirs ${CCFLAGS}

utils.o: utils.hpp tiggerutils.hpp utils.cpp
	g++ -Wno-register -O2 -lm -std=c++17 utils.cpp -c -Idirs ${CCFLAGS}

clean:
	rm -rf compiler *.o lex.yy.cpp yacc.tab.cpp yacc.tab.hpp

.PHONY: clean