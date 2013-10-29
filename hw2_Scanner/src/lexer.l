%option noyywrap 
%{
#include <stdio.h>
#include "header.h" 
int linenumber;
symtab * lookup();
symtab * ptr;
void     insertID();
void 	 printSymTab();
/* You need to define for all tokens in C--, here are some examples */
#define	RETURN		10
#define	TYPEDEF		11
#define	IF			12
#define	ELSE		13
#define	INT			14
#define	FLOAT		15
#define	FOR			16
#define	STRUCT		17
#define	UNION		18
#define	VOID		19
#define	WHILE		20
/* Arithmetic operators */
#define	op_PLUS		30
#define	op_MINUS	31
#define	op_MUL  	32
#define	op_DIV  	33
/* Relational operators */
#define	OP_GE   	40
#define	OP_LE   	41
#define	OP_GT   	42
#define	OP_LT   	43
#define	OP_NE   	44
#define	OP_EQ   	45
/*Logical operators*/
#define	OP_or   	46
#define	OP_and  	47
#define	OP_not  	48
/*Assignment operators*/
#define OP_assign	49

/* delimiter */
#define	DL_lparen 	50	
#define DL_rparen 	51	
#define DL_lbrack 	52	
#define DL_rbrack 	53	
#define DL_lbrace 	54	
#define DL_rbrace 	55	
#define DL_comma  	56	
#define DL_semicol 	57	
#define DL_dot  	58	

#define ERROR 		100 

%}
 /* reserve word */
RETURN	"return"
TYPEDEF	"typedef"
IF		"if"
ELSE	"else"
INT		"int"
FLOAT	"float"
FOR		"for"
STRUCT	"struct"
UNION	"union"
VOID	"void"
WHILE	"while"

 /*token constant*/
letter   [A-Za-z]
digit    [0-9]
ID	 {letter}({letter}|{digit}|"_")*
WS	 [ \t]+
RWInt    {digit}+
RWFloat  {digit}+\.{digit}+
String_constant \"[^"]*\" 
comment "//(.)*\n"
lcomment "/*"((\*+[^/*])|([^*]))*\**"*/"


 /*token operators */
 /*Arithmetic operators*/ 
OP_plus "+"
OP_minus "-"
OP_mul  "*"
OP_div  "/"
 /* Relational operators */
OP_ge   ">="
OP_le   "<="
OP_gt   ">"
OP_lt   "<"
OP_ne   "!="
OP_eq   "=="
 /*Logical operators*/
OP_or   "||"
OP_and  "&&"
OP_not  "!"
 /*Assignment operators*/
OP_assign "="

newline  "\n"

DL_lparen "("
DL_rparen ")"
DL_lbrack "["
DL_rbrack "]"
DL_lbrace "{"
DL_rbrace "}"
DL_comma  ","
DL_semicol ";"
DL_dot  "."

 /* Other separators appear here */

error    .

%%
{RETURN}	{} /*return RETURN; }	*/
{TYPEDEF}	{} /*return TYPEDEF; }	*/
{IF}		{} /*return IF; }	*/
{ELSE}		{} /*return ELSE; }	*/
{INT}		{} /*return INT; }	*/
{FLOAT}		{} /*return FLOAT; }	*/
{FOR}		{} /*return FOR; }	*/
{STRUCT}	{} /*return STRUCT; }	*/
{UNION}		{} /*return UNION; }	*/
{VOID}		{} /*return VOID; }	*/
{WHILE}		{} /*return WHILE; }	*/

{WS}		{}
{RWInt}         {}
{RWFloat}       {}
{String_constant} {}
{ID}            { 
			ptr = lookup(yytext);
			if (ptr == NULL)
			     insertID(yytext);	
			else 
			     ptr->counter++;
			}
{comment}		{ printf("%s\n", yytext); }
{lcomment}		{ printf("%s\n", yytext); }
{OP_plus}	{} /* return {OP_PLUS}	;*/
{OP_minus}	{} /* return {OP_MINUS}	;*/
{OP_mul}	{} /* return {OP_MUL}	;*/
{OP_div}	{} /* return {OP_DIV}	;*/
{OP_ge}		{} /* return {OP_GE}	;*/
{OP_le}		{} /* return {OP_LE}	;*/
{OP_gt}		{} /* return {OP_GT}	;*/
{OP_lt}		{} /* return {OP_LT}	;*/
{OP_ne}		{} /* return {OP_NE}	;*/
{OP_eq}		{} /* return {OP_EQ}	;*/
{OP_or}		{} /* return {OP_OR}	;*/
{OP_and}	{} /* return {OP_AND}	;*/
{OP_not}	{} /* return {OP_NOT}	;*/
{OP_assign}     {} /* return OP_ASSIGN; */ 
{DL_lparen}     {} /* return MK_LPAREN; */
{DL_rparen}     {} /* return MK_RPAREN; */
{DL_lbrack}     {} /* return MK_LBRACK; */
{DL_rbrack}     {} /* return MK_RBRACK; */
{DL_lbrace}     {} /* return MK_LBRACE; */
{DL_rbrace}     {} /* return MK_RBRACE; */
{DL_comma}      {}
{DL_semicol}    {}
{DL_dot}		{}
{newline}       linenumber += 1;
{error}         printf("ERR %s\n", yytext);/* return ERROR; */

%%

main(int argc, char **argv)
{
 argc--; ++argv;
   if (argc > 0)
      yyin = fopen(argv[0], "r");
   else
      yyin = stdin;
   yylex();
   printSymTab();
}

