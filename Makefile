all: lab

parser.tab.c parser.tab.h: parser.y
	bison -d -v parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

lab: lex.yy.c parser.tab.c ast.h
	gcc -o lab lex.yy.c parser.tab.c

run: lab
	./lab test.c
	node out.js

clean:
	rm -f lab lex.yy.c parser.tab.c parser.tab.h parser.output out.js ast_visualizer.html

.PHONY: all run clean
