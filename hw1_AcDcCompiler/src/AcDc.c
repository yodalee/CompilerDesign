#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "header.h"


int main( int argc, char *argv[] )
{
    FILE *source, *target;
    Program program;
    SymbolTable symtab;

    if( argc == 3){
        source = fopen(argv[1], "r");
        target = fopen(argv[2], "w");
        if( !source ){
            printf("can't open the source file\n");
            exit(2);
        }
        else if( !target ){
            printf("can't open the target file\n");
            exit(2);
        }
        else{
            program = parser(source);
            fclose(source);
            symtab = build(program);
            check(&program, &symtab);
            gencode(program, &symtab, target);
        }
    }
    else{
        printf("Usage: %s source_file target_file\n", argv[0]);
    }


    return 0;
}


/********************************************* 
  Scanning 
 *********************************************/
Token getNumericToken( FILE *source, char c )
{
    Token token;
    int i = 0;

    while( isdigit(c) ) {
        token.tok[i++] = c;
        c = fgetc(source);
    }

    if( c != '.' ){
        ungetc(c, source);
        token.tok[i] = '\0';
        token.type = IntValue;
        return token;
    }

    token.tok[i++] = '.';

    c = fgetc(source);
    if( !isdigit(c) ){
        ungetc(c, source);
        printf("Expect a digit : %c\n", c);
        exit(1);
    }

    while( isdigit(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

    ungetc(c, source);
    token.tok[i] = '\0';
    token.type = FloatValue;
    return token;
}

Token getFullToken( FILE *source, char c)
{
	Token token;
    int i = 0;
	token.tok[i++] = c;
	c = fgetc(source);

    while( isalnum(c) ){
        token.tok[i++] = c;
        c = fgetc(source);
    }

	ungetc(c, source);
	token.tok[i] = '\0';

	if ( i > 256 ) {
		printf("Variable longer than 256 characters: %s\n", token.tok);
		exit(1);
	}
	if ( i > 1) {
		token.type = Alphabet;
		return token;
	} else {
		if( islower( token.tok[0] ) ) {
			switch(token.tok[0]) {
				case 'f':
					token.type = FloatDeclaration;
					break;
				case 'i':
					token.type = IntegerDeclaration;
					break;
				case 'p':
					token.type = PrintOp;
					break;
				default: 
					token.type = Alphabet;
					break;
			}
			return token;
		}

		switch(token.tok[0]){
			case '=':
				token.type = AssignmentOp;
				return token;
			case '+':
				token.type = PlusOp;
				return token;
			case '-':
				token.type = MinusOp;
				return token;
			case '*':
				token.type = MulOp;
				return token;
			case '/':
				token.type = DivOp;
				return token;
			case EOF:
				token.type = EOFsymbol;
				token.tok[0] = '\0';
				return token;
			default:
				printf("Invalid character : %c\n", c);
				exit(1);
		}
		return token;
	}
}

Token scanner( FILE *source )
{
    char c;
    Token token;

    while( !feof(source) ){
        c = fgetc(source);

        while( isspace(c) ) c = fgetc(source);

        if( isdigit(c) )
            return getNumericToken(source, c);

		return getFullToken(source, c);
    }

    token.tok[0] = '\0';
    token.type = EOFsymbol;
    return token;
}


/********************************************************
  Parsing
 *********************************************************/
Declaration parseDeclaration( FILE *source, Token token )
{
    Token token2;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            token2 = scanner(source);
            if (strncmp(token2.tok, "f", 1) == 0 ||
                    strncmp(token2.tok, "i", 1) == 0 ||
                    strncmp(token2.tok, "p", 1) == 0) {
                printf("Syntax Error: %s cannot be used as id\n", token2.tok);
                exit(1);
            }
            return makeDeclarationNode( token, token2 );
        default:
            printf("Syntax Error: Expect Declaration %s\n", token.tok);
            exit(1);
    }
}

Declarations *parseDeclarations( FILE *source )
{
    Token token = scanner(source);
    Declaration decl;
    Declarations *decls;
	int i;
    switch(token.type){
        case FloatDeclaration:
        case IntegerDeclaration:
            decl = parseDeclaration(source, token);
            decls = parseDeclarations(source);
            return makeDeclarationTree( decl, decls );
        case PrintOp:
        case Alphabet:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
            return NULL;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect declarations %s\n", token.tok);
            exit(1);
    }
}

Expression *parseValue( FILE *source )
{
    Token token = scanner(source);
    Expression *expr = (Expression *)malloc( sizeof(Expression) );
    expr->leftOperand = expr->rightOperand = NULL;

    switch(token.type){
        case Alphabet:
            (expr->v).type = Identifier;
			strncpy((expr->v).val.id, token.tok, 256);
            break;
        case IntValue:
            (expr->v).type = IntConst;
            (expr->v).val.ivalue = atoi(token.tok);
            break;
        case FloatValue:
            (expr->v).type = FloatConst;
            (expr->v).val.fvalue = atof(token.tok);
            break;
        default:
            printf("Syntax Error: Expect Identifier or a Number %s\n", token.tok);
            exit(1);
    }

    return expr;
}

Expression *parseTerm( FILE *source, Expression *lvalue ) 
{
    Token token = scanner(source);
    Expression *term;
	int i;

    switch(token.type){
        case MulOp:
            term = (Expression *)malloc( sizeof(Expression) );
            (term->v).type = MulNode;
            (term->v).val.op = Mul;
            term->leftOperand = lvalue;
            term->rightOperand = parseValue(source);
            return parseTermTail(source, term);
        case DivOp:
            term = (Expression *)malloc( sizeof(Expression) );
            (term->v).type = DivNode;
            (term->v).val.op = Div;
            term->leftOperand = lvalue;
            term->rightOperand = parseValue(source);
            return parseTermTail(source, term);
		case PlusOp:
		case MinusOp:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
			return lvalue;
        case Alphabet:
        case PrintOp:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseTermTail( FILE *source, Expression *lvalue ) 
{
    Token token = scanner(source);
    Expression *term;
	int i;

    switch(token.type){
        case MulOp:
            term = (Expression *)malloc( sizeof(Expression) );
            (term->v).type = MulNode;
            (term->v).val.op = Mul;
            term->leftOperand = lvalue;
            term->rightOperand = parseValue(source);
            return parseTermTail(source, term);
        case DivOp:
            term = (Expression *)malloc( sizeof(Expression) );
            (term->v).type = DivNode;
            (term->v).val.op = Div;
            term->leftOperand = lvalue;
            term->rightOperand = parseValue(source);
            return parseTermTail(source, term);
		case PlusOp:
		case MinusOp:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
			return lvalue;
        case Alphabet:
        case PrintOp:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
			return lvalue;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpressionTail( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr, *term;
	int i;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
			term = parseValue(source);
            expr->rightOperand = parseTerm(source, term);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
			term = parseValue(source);
            expr->rightOperand = parseTerm(source, term);
            return parseExpressionTail(source, expr);
        case Alphabet:
        case PrintOp:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
            return lvalue;
        case EOFsymbol:
            return lvalue;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Expression *parseExpression( FILE *source, Expression *lvalue )
{
    Token token = scanner(source);
    Expression *expr, *term;
	int i;

    switch(token.type){
        case PlusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = PlusNode;
            (expr->v).val.op = Plus;
            expr->leftOperand = lvalue;
			term = parseValue(source);
            expr->rightOperand = parseTerm(source, term);
            return parseExpressionTail(source, expr);
        case MinusOp:
            expr = (Expression *)malloc( sizeof(Expression) );
            (expr->v).type = MinusNode;
            (expr->v).val.op = Minus;
            expr->leftOperand = lvalue;
			term = parseValue(source);
            expr->rightOperand = parseTerm(source, term);
            return parseExpressionTail(source, expr);
        case Alphabet:
        case PrintOp:
			for (i = strlen(token.tok)-1; i >=0; --i) {
				ungetc(token.tok[i], source);
			}
            return lvalue;
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect a numeric value or an identifier %s\n", token.tok);
            exit(1);
    }
}

Statement parseStatement( FILE *source, Token token )
{
    Token next_token;
    Expression *expr, *value, *term;

    switch(token.type){
        case Alphabet:
            next_token = scanner(source);
            if(next_token.type == AssignmentOp){
				value = parseValue(source);
				term = parseTerm(source, value);
				expr = parseExpression(source, term);
                return makeAssignmentNode(token.tok, value, expr);
            }
            else{
                printf("Syntax Error: Expect an assignment op %s\n", next_token.tok);
                exit(1);
            }
        case PrintOp:
            next_token = scanner(source);
            if(next_token.type == Alphabet)
                return makePrintNode(next_token.tok);
            else{
                printf("Syntax Error: Expect an identifier %s\n", next_token.tok);
                exit(1);
            }
            break;
        default:
            printf("Syntax Error: Expect a statement %s\n", token.tok);
            exit(1);
    }
}

Statements *parseStatements( FILE * source )
{

    Token token = scanner(source);
    Statement stmt;
    Statements *stmts;

    switch(token.type){
        case Alphabet:
        case PrintOp:
            stmt = parseStatement(source, token);
            stmts = parseStatements(source);
            return makeStatementTree(stmt , stmts);
        case EOFsymbol:
            return NULL;
        default:
            printf("Syntax Error: Expect statements %s\n", token.tok);
            exit(1);
    }
}


/*********************************************************************
  Build AST
 **********************************************************************/
Declaration makeDeclarationNode( Token declare_type, Token identifier )
{
    Declaration tree_node;

    switch(declare_type.type){
        case FloatDeclaration:
            tree_node.type = Float;
            break;
        case IntegerDeclaration:
            tree_node.type = Int;
            break;
        default:
            break;
    }
	strncpy(tree_node.name, identifier.tok, sizeof(tree_node.name));

    return tree_node;
}

Declarations *makeDeclarationTree( Declaration decl, Declarations *decls )
{
    Declarations *new_tree = (Declarations *)malloc( sizeof(Declarations) );
    new_tree->first = decl;
    new_tree->rest = decls;

    return new_tree;
}


Statement makeAssignmentNode( char* id, Expression *v, Expression *expr_tail )
{
    Statement stmt;
    AssignmentStatement assign;

    stmt.type = Assignment;
	strncpy(assign.id, id, 256);
    if(expr_tail == NULL)
        assign.expr = v;
    else
        assign.expr = expr_tail;
    stmt.stmt.assign = assign;

    return stmt;
}

Statement makePrintNode( char* id )
{
    Statement stmt;
    stmt.type = Print;
	strncpy(stmt.stmt.variable, id, 256);

    return stmt;
}

Statements *makeStatementTree( Statement stmt, Statements *stmts )
{
    Statements *new_tree = (Statements *)malloc( sizeof(Statements) );
    new_tree->first = stmt;
    new_tree->rest = stmts;

    return new_tree;
}

/* parser */
Program parser( FILE *source )
{
    Program program;

    program.declarations = parseDeclarations(source);
    program.statements = parseStatements(source);

    return program;
}


/********************************************************
  Build symbol table
 *********************************************************/
void InitializeTable( SymbolTable *table )
{
    int i;

    for(i = 0 ; i < 26; i++) {
        table->table[i] = Notype;
		*table->name[i] = '\0';
	}
}

void add_table( SymbolTable *table, char* s, DataType t )
{
	int i;
	for (i = 0; i < 26; ++i) {
		if(table->table[i] != Notype) {
			if ( strncmp(table->name[i], s, 256) == 0) {
				printf("Error : id %s has been declared\n", s);//error
			}
		} else {
			table->table[i] = t;
			strncpy(table->name[i], s, 256);
			return;
		}
	}
}

SymbolTable build( Program program )
{
    SymbolTable table;
    Declarations *decls = program.declarations;
    Declaration current;

    InitializeTable(&table);

    while(decls !=NULL){
        current = decls->first;
        add_table(&table, current.name, current.type);
        decls = decls->rest;
    }

    return table;
}


/********************************************************************
  Type checking
 *********************************************************************/

void convertType( Expression * old, DataType type )
{
    if(old->type == Float && type == Int){
        printf("error : can't convert float to integer\n");
        return;
    }
    if(old->type == Int && type == Float){
        Expression *tmp = (Expression *)malloc( sizeof(Expression) );
        if(old->v.type == Identifier)
            printf("convert to float %s \n",old->v.val.id);
        else
            printf("convert to float %d \n", old->v.val.ivalue);
        tmp->v = old->v;
        tmp->leftOperand = old->leftOperand;
        tmp->rightOperand = old->rightOperand;
        tmp->type = old->type;

        Value v;
        v.type = IntToFloatConvertNode;
        v.val.op = IntToFloatConvert;
        old->v = v;
        old->type = Int;
        old->leftOperand = tmp;
        old->rightOperand = NULL;
    }
}

DataType generalize( Expression *left, Expression *right )
{
    if(left->type == Float || right->type == Float){
        printf("generalize : float\n");
        return Float;
    }
    printf("generalize : int\n");
    return Int;
}

float calConst(Operation op, float lval, float rval)
{
	switch(op){
		case Plus:
			return lval + rval;
		case Minus:
			return lval - rval;
		case Mul:
			return lval * rval;
		case Div:
			return lval / rval;
	}
}

void constFolding( Expression* expr )
{
	Value lval = expr->leftOperand->v;
	Value rval = expr->rightOperand->v;

	(expr->v).type = FloatConst;
	expr->type = FloatConst;
	if (lval.type == IntConst && rval.type == IntConst) {
		(expr->v).type = IntConst;
		expr->type = IntConst;
		(expr->v).val.ivalue = (int)calConst((expr->v).val.op, (float)lval.val.ivalue, rval.val.ivalue);
	} else if (lval.type == FloatConst && rval.type == IntConst) {
		(expr->v).val.fvalue = calConst((expr->v).val.op, lval.val.fvalue, (float)rval.val.ivalue);
	} else if (lval.type == IntConst && rval.type == FloatConst) {
		(expr->v).val.fvalue = calConst((expr->v).val.op, (float)lval.val.ivalue, rval.val.fvalue);
	} else {
		(expr->v).val.fvalue = calConst((expr->v).val.op, lval.val.fvalue, rval.val.fvalue);
	}
	free(expr->leftOperand);
	free(expr->rightOperand);
	expr->leftOperand = expr->rightOperand = NULL;
}

DataType lookup_table( SymbolTable *table, char* s )
{
	int i;
	int find = 1;
	for (i = 0; i < 26; ++i) {
		if ( table->table[i] != Notype) {
			if ( strncmp(table->name[i], s, 256) == 0 ) {
				return table->table[i];
			}
		} else {
			break;
		}
	}
	printf("Error : identifier %s is not declared\n", s);//error
}

char lookup_symbol( SymbolTable *table, char* s) 
{
	int i;
	for (i = 0; i < 26; ++i) {
		if ( strncmp(table->name[i], s, 256) == 0 ) {
			return 'a'+i;
		}
	}
}

void checkexpression( Expression * expr, SymbolTable * table )
{
    char s[257];
    if(expr->leftOperand == NULL && expr->rightOperand == NULL){
        switch(expr->v.type){
            case Identifier:
				strncpy(s, expr->v.val.id, 256);
                printf("identifier : %s\n",s);
                expr->type = lookup_table(table, s);
                break;
            case IntConst:
                printf("constant : int\n");
                expr->type = Int;
                break;
            case FloatConst:
                printf("constant : float\n");
                expr->type = Float;
                break;
                //case PlusNode: case MinusNode: case MulNode: case DivNode:
            default:
                break;
        }
    } else {
        Expression *left = expr->leftOperand;
        Expression *right = expr->rightOperand;

        checkexpression(left, table);
        checkexpression(right, table);

		if (expr->leftOperand->v.type != Identifier &&\
			expr->rightOperand->v.type != Identifier ) {
			constFolding( expr );
		} else {
			DataType type = generalize(left, right);
			convertType(left, type);//left->type = type;//converto
			convertType(right, type);//right->type = type;//converto
			expr->type = type;
		}
    }
}

void checkstmt( Statement *stmt, SymbolTable * table )
{
    if(stmt->type == Assignment){
        AssignmentStatement assign = stmt->stmt.assign;
        printf("assignment : %s \n",assign.id);
        checkexpression(assign.expr, table);
        stmt->stmt.assign.type = lookup_table(table, assign.id);
        if (assign.expr->type == Float && stmt->stmt.assign.type == Int) {
            printf("error : can't convert float to integer\n");
        } else {
            convertType(assign.expr, stmt->stmt.assign.type);
        }
    }
    else if (stmt->type == Print){
        printf("print : %s \n",stmt->stmt.variable);
        lookup_table(table, stmt->stmt.variable);
    }
    else printf("error : statement error\n");//error
}

void check( Program *program, SymbolTable * table )
{
    Statements *stmts = program->statements;
    while(stmts != NULL){
        checkstmt(&stmts->first,table);
        stmts = stmts->rest;
    }
}


/***********************************************************************
  Code generation
 ************************************************************************/
void fprint_op( FILE *target, ValueType op )
{
    switch(op){
        case MinusNode:
            fprintf(target,"-\n");
            break;
        case PlusNode:
            fprintf(target,"+\n");
            break;
        case MulNode:
            fprintf(target,"*\n");
            break;
        case DivNode:
            fprintf(target,"/\n");
            break;
        default:
            fprintf(target,"Error in fprintf_op ValueType = %d\n",op);
            break;
    }
}

void fprint_expr( FILE *target, SymbolTable *table, Expression *expr)
{
    if(expr->leftOperand == NULL){
        switch( (expr->v).type ){
            case Identifier:
                fprintf(target,"l%c\n",lookup_symbol(table, (expr->v).val.id));
                break;
            case IntConst:
                fprintf(target,"%d\n",(expr->v).val.ivalue);
                break;
            case FloatConst:
                fprintf(target,"%f\n", (expr->v).val.fvalue);
                break;
            default:
                fprintf(target,"Error In fprint_left_expr. (expr->v).type=%d\n",(expr->v).type);
                break;
        }
    } else {
        fprint_expr(target, table, expr->leftOperand);
        if(expr->rightOperand == NULL){
            fprintf(target,"5k\n");
        } else {
            //	fprint_right_expr(expr->rightOperand);
            fprint_expr(target, table, expr->rightOperand);
            fprint_op(target, (expr->v).type);
        }
    }
}

void gencode(Program prog, SymbolTable* table, FILE * target)
{
    Statements *stmts = prog.statements;
    Statement stmt;

    while(stmts != NULL){
        stmt = stmts->first;
        switch(stmt.type){
            case Print:
                fprintf(target,"l%c\n",lookup_symbol(table, stmt.stmt.variable));
                fprintf(target,"p\n");
                break;
            case Assignment:
				if ( stmt.stmt.assign.type == Float ){
					fprintf(target,"5 k\n");
				}
                fprint_expr(target, table, stmt.stmt.assign.expr);
                fprintf(target,"s%c\n",lookup_symbol(table, stmt.stmt.assign.id));
                fprintf(target,"0 k\n");
                break;
        }
        stmts=stmts->rest;
    }
}


/***************************************
  For our debug,
  you can omit them.
 ****************************************/
void print_expr(Expression *expr)
{
    if(expr == NULL)
        return;
    else{
        print_expr(expr->leftOperand);
        switch((expr->v).type){
            case Identifier:
                printf("%s ", (expr->v).val.id);
                break;
            case IntConst:
                printf("%d ", (expr->v).val.ivalue);
                break;
            case FloatConst:
                printf("%f ", (expr->v).val.fvalue);
                break;
            case PlusNode:
                printf("+ ");
                break;
            case MinusNode:
                printf("- ");
                break;
            case MulNode:
                printf("* ");
                break;
            case DivNode:
                printf("/ ");
                break;
            case IntToFloatConvertNode:
                printf("(float) ");
                break;
            default:
                printf("error ");
                break;
        }
        print_expr(expr->rightOperand);
    }
}

void test_parser( FILE *source )
{
    Declarations *decls;
    Statements *stmts;
    Declaration decl;
    Statement stmt;
    Program program = parser(source);

    decls = program.declarations;

    while(decls != NULL){
        decl = decls->first;
        if(decl.type == Int)
            printf("i ");
        if(decl.type == Float)
            printf("f ");
        printf("%s ",decl.name);
        decls = decls->rest;
    }

    stmts = program.statements;

    while(stmts != NULL){
        stmt = stmts->first;
        if(stmt.type == Print){
            printf("p %s ", stmt.stmt.variable);
        }

        if(stmt.type == Assignment){
            printf("%s = ", stmt.stmt.assign.id);
            print_expr(stmt.stmt.assign.expr);
        }
        stmts = stmts->rest;
    }

}
