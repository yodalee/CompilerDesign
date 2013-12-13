BIN = ../bin
TARGET = parser
OBJECT = parser.tab.c parser.tab.o lex.yy.c alloc.o functions.o semanticAnalysis.o symbolTable.o
OUTPUT = parser.output parser.tab.h
CC = gcc -g
LEX = flex
YACC = bison -v
YACCFLAG = -d
LIBS = -lfl 
PARAM = $(BIN)/test.c

parser: mkdir parser.tab.o alloc.o functions.o symbolTable.o semanticAnalysis.o 
	$(CC) -o $(BIN)/$(TARGET) parser.tab.o alloc.o functions.o symbolTable.o semanticAnalysis.o $(LIBS)

mkdir: 
	mkdir -p ../bin

parser.tab.o: parser.tab.c lex.yy.c alloc.o functions.c symbolTable.o semanticAnalysis.o
	$(CC) -c parser.tab.c
    
semanticAnalysis.o: semanticAnalysis.c symbolTable.o
	$(CC) -c semanticAnalysis.c

symbolTable.o: symbolTable.c
	$(CC) -c symbolTable.c

lex.yy.c: lexer3.l
	$(LEX) lexer3.l

parser.tab.c: parser.y 
	$(YACC) $(YACCFLAG) parser.y

alloc.o: alloc.c
	$(CC) -c alloc.c
	
functions.o: functions.c
	$(CC) -c functions.c

run:
	$(BIN)/$(TARGET) $(PARAM)
	$(shell dot -Tpng AST_Graph.gv -o ../bin/test.png)

#	$(shell eom ../bin/test.png)

clean:
	rm -f $(BIN)/$(TARGET) $(OBJECT) $(OUTPUT)

