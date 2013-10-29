#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<math.h>
#include"header.h"

#define TABLE_SIZE	256

symtab * hash_table[TABLE_SIZE];
//define a structure symtab 
extern int linenumber;

int HASH(char * str){
	int idx=0;
	while(*str){
		idx = idx << 1;
		idx+=*str;
		str++;
	}	
	return (idx & (TABLE_SIZE-1));
}

/*returns the symbol table entry if found else NULL*/

symtab * lookup(char *name){
	int hash_key;
	symtab* symptr;
	if(!name)
		return NULL;
	hash_key=HASH(name);
	symptr=hash_table[hash_key];

	while(symptr){
		if(!(strcmp(name,symptr->lexeme)))
			return symptr;
		symptr=symptr->front;
	}
	return NULL;
}


void insertID(char *name){
	int hash_key;
	symtab* ptr;
	symtab* symptr=(symtab*)malloc(sizeof(symtab));	
	
	hash_key=HASH(name);
	ptr=hash_table[hash_key];
	
	if(ptr==NULL){
		/*first entry for this hash_key*/
		hash_table[hash_key]=symptr;
		symptr->front=NULL;
		symptr->back=symptr;
	}
	else{
		symptr->front=ptr;
		ptr->back=symptr;
		symptr->back=symptr;
		hash_table[hash_key]=symptr;	
	}
	
	strcpy(symptr->lexeme,name);
	symptr->line=linenumber;
	symptr->counter=1;
}

void printSym(symtab* ptr) 
{
	printf(" Name = %s \n", ptr->lexeme);
	printf(" References = %d \n", ptr->counter);
}

void printSymTab()
{
    int i;
	symtab* symptr;
    printf("----- Symbol Table ---------\n");
    for (i=0; i<TABLE_SIZE; i++)
    {
		symptr = hash_table[i];
		while (symptr != NULL)
		{
			//printf("====>  index = %d \n", i);
			printSym(symptr);
			symptr=symptr->front;
		}
    }
}

void sortPrint() {
	//first count the num of symbol in table
	int count = 0;
	int i,j = 0;
	symtab* symptr;
    for (i=0; i<TABLE_SIZE; i++)
    {
		symptr = hash_table[i];
		while (symptr != NULL)
		{
			count++;
			symptr=symptr->front;
		}
    }
	//get element, and 
	symtab** sortArray = (symtab**)malloc(count * sizeof(symtab*));
	int used = 0;
	int insert = 0;
	int move = 0;
    for (i=0; i<TABLE_SIZE; i++)
    {
		symptr = hash_table[i];
		while (symptr != NULL)
		{
			for (insert = 0; insert < used; ++insert) {
				if (strncmp(symptr->lexeme, sortArray[insert]->lexeme, 256) < 0) {
					for (move = used; move > insert; --move) {
						sortArray[move] = sortArray[move-1];
					}
					break;
				}
			}
			sortArray[insert] = symptr;
			used++;
			symptr=symptr->front;
		}
    }
	printf("------sort table-------\n");
	for (i = 0; i < used; ++i) {
		printSym(sortArray[i]);
	}
}
