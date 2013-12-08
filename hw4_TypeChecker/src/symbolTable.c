#include "symbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// This file is for reference only, you are not required to follow the implementation. //

int HASH(char * str) {
  int idx=0;
  while (*str){
    idx = idx << 1;
    idx+=*str;
    str++;
  }
  return (idx & (HASH_TABLE_SIZE-1));
}

SymbolTableStack symbolTableStack;

SymbolTableEntry* newSymbolTableEntry()
{
  SymbolTableEntry* symbolTableEntry = (SymbolTableEntry*)malloc(sizeof(SymbolTableEntry));
  symbolTableEntry->nextInHashChain = NULL;
  symbolTableEntry->prevInHashChain = NULL;
  symbolTableEntry->attribute = NULL;
  symbolTableEntry->name = NULL;
  return symbolTableEntry;
}

// push entry into topest hashtable by hash
void enterIntoHashTrain(int hashIndex, SymbolTableEntry* entry)
{
  SymbolTable symbolTable = *symbolTableStack.table;
  SymbolTableEntry* ptr = symbolTable.hashTable[hashIndex];
  if (ptr == NULL) {
    symbolTable.hashTable[hashIndex] = entry;
    entry->prevInHashChain = NULL;
    entry->nextInHashChain = entry;
  } else {
    entry->prevInHashChain = ptr;
    ptr->nextInHashChain = entry;
    entry->nextInHashChain = entry;
    symbolTable.hashTable[hashIndex] = entry;
  }
}

void removeFromHashTrain(int hashIndex, SymbolTableEntry *entry)
{
  SymbolTable symbolTable = *symbolTableStack.table;
  //directly pass the entry pointer in hashTable
  if (symbolTable.hashTable[hashIndex] == entry) {
    symbolTable.hashTable[hashIndex] = entry->prevInHashChain;
    if (entry->prevInHashChain) {
      entry->prevInHashChain->nextInHashChain = entry->prevInHashChain;
    }
  } else {
    entry->nextInHashChain->prevInHashChain = entry->prevInHashChain;
    if (entry->prevInHashChain) {
      entry->prevInHashChain->nextInHashChain = entry->nextInHashChain;
    }
  }
  free(entry);
}

//search symbol table stack from first until end
SymbolTableEntry* retrieveSymbol(char* symbolName)
{
  int hashIndex = HASH(symbolName);
  int found = 0;
  SymbolTable *tablePtr = symbolTableStack.table;
  SymbolTableEntry *rtnEntry = NULL;
  while (tablePtr) {
    SymbolTableEntry* ptr = tablePtr->hashTable[hashIndex];
    while (ptr) {
      if ((strcmp(symbolName, ptr->name) == 0)){
        rtnEntry = ptr;
        found = 1;
        break;
      }
      ptr = ptr->prevInHashChain;
    }
    if (found) {
      break;
    } else {
      tablePtr = tablePtr->nextTable;
    }
  }
  return rtnEntry;
}


void
initializeStack()
{
  symbolTableStack.table = NULL;
  symbolTableStack.numberOfStack = -1;
  openScope();
}

void 
stackEnd() 
{
  SymbolTable *ptr = symbolTableStack.table;
  while (ptr) {
    symbolTableEnd(ptr);
    ptr = ptr->nextTable;
  }
}

//create new symbol table instance and insert into stack
void openScope()
{
  SymbolTable *newSymbolTable = initializeSymbolTable();
  newSymbolTable->nextTable = symbolTableStack.table;
  symbolTableStack.table = newSymbolTable;
  ++symbolTableStack.numberOfStack;
}

//pop out the stack, remove first table in stack
void closeScope()
{
  SymbolTable *deleteTable = symbolTableStack.table;
  symbolTableStack.table = deleteTable->nextTable;
  symbolTableEnd(deleteTable);
  --symbolTableStack.numberOfStack;
}

SymbolTable* initializeSymbolTable()
{
  SymbolTable* symbolTable = (SymbolTable*)malloc(sizeof(SymbolTable));
  int i;
  for (i = 0; i < HASH_TABLE_SIZE; ++i) {
    symbolTable->hashTable[i] = NULL;
  }
  return symbolTable;
}

//wipe out a symbolTable
void symbolTableEnd(SymbolTable *symbolTable)
{
  int i;
  SymbolTableEntry *ptr;
  SymbolTableEntry *nextptr;
  //clear hashTable using free
  for (i = 0; i < HASH_TABLE_SIZE; ++i) {
    ptr = symbolTable->hashTable[i];
    while (ptr) {
      nextptr = ptr->nextInHashChain;
      removeFromHashTrain(i, ptr);
      ptr = nextptr;
    }
  }
}

SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
  SymbolTable *symbolTable = symbolTableStack.table;
  int hashIndex = HASH(symbolName);
  SymbolTableEntry* entry = newSymbolTableEntry();
  memcpy(entry->attribute, attribute, sizeof(SymbolAttribute));
  enterIntoHashTrain(hashIndex, entry);
}

//remove the symbol from the current scope
void removeSymbol(char* symbolName)
{
  int hashIndex = HASH(symbolName);
  SymbolTableEntry* entry;
  SymbolTableEntry* retrieveSymbol(char* symbolName);
  if ((entry = retrieveSymbol(symbolName)) != NULL) {
    removeFromHashTrain(hashIndex, entry);
  }
}

//currently don't understand the meaning
int declaredLocally(char* symbolName)
{
}
