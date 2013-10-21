TARGET = scanner
OBJECT = lex.yy.o symboltable.o 
CC = cc -g
LEX = flex
LIBS = -lfl 

scanner:	lex.yy.o symboltable.o mkdir
	$(CC) -o ../bin/scanner lex.yy.o symboltable.o

symboltable.o: symboltable.c 
	$(CC) -c symboltable.c 

lex.yy.o: lex.yy.c 
	$(CC) -c lex.yy.c 

lex.yy.c: lexer.l
	$(LEX) $(LIB) lexer.l

mkdir:
	mkdir -p ../bin

clean:
	rm -f $(TARGET) $(OBJECT) 

