BIN = ../bin
MKDIR = mkdir -p
TARGET = parser
OBJECT = parser.tab.c parser.tab.o lex.yy.c alloc.o functions.o semanticAnalysis.o symbolTable.o codeGeneration.o genfunction.o
OUTPUT = parser.output parser.tab.h
CC = gcc -g
LEX = flex
YACC = bison -v
YACCFLAG = -d
LIBS = -lfl 

parser: mkdir parser.tab.o alloc.o functions.o symbolTable.o semanticAnalysis.o codeGeneration.o genfunction.o
	$(CC) -o $(BIN)/$(TARGET) parser.tab.o alloc.o functions.o symbolTable.o semanticAnalysis.o codeGeneration.o genfunction.o $(LIBS)

mkdir: 
	$(MKDIR) $(BIN)

codeGeneration.o: codeGeneration.c genfunction.o
	$(CC) -c codeGeneration.c

genfunction.o: genfunction.c
	$(CC) -c genfunction.c

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

clean:
	rm -f $(OBJECT) $(OUTPUT)
	rm -f $(BIN)/$(TARGET)
