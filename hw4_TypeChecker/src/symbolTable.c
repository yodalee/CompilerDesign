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
  SymbolTable *symbolTable = symbolTableStack.table;
  SymbolTableEntry *ptr = symbolTable->hashTable[hashIndex];
  if (ptr == NULL) {
    symbolTable->hashTable[hashIndex] = entry;
    entry->prevInHashChain = NULL;
    entry->nextInHashChain = entry;
  } else {
    entry->prevInHashChain = ptr;
    ptr->nextInHashChain = entry;
    entry->nextInHashChain = entry;
    symbolTable->hashTable[hashIndex] = entry;
  }
}

void removeFromHashTrain(int hashIndex, SymbolTableEntry *entry, SymbolTable *symbolTable)
{
  //directly pass the entry pointer in hashTable
  if (symbolTable->hashTable[hashIndex] == entry) {
    symbolTable->hashTable[hashIndex] = entry->prevInHashChain;
    if (entry->prevInHashChain) {
      entry->prevInHashChain->nextInHashChain = entry->prevInHashChain;
    }
  } else {
    entry->nextInHashChain->prevInHashChain = entry->prevInHashChain;
    if (entry->prevInHashChain) {
      entry->prevInHashChain->nextInHashChain = entry->nextInHashChain;
    }
  }
  //free inside out
  free(entry->name);
  if (entry->attribute->attributeKind == FUNCTION_SIGNATURE) {
    Parameter *param = entry->attribute->attr.functionSignature->parameterList;
    while (param) {
      free(param->parameterName);
      free(param->type);
      param = param->next;
    }
  } else {
    free(entry->attribute->attr.typeDescriptor);
  }
  free(entry);
}

//search symbol table stack from first until end
SymbolTableEntry*
searchTable(SymbolTable* tablePtr, char* symbolName)
{
  int hashIndex = HASH(symbolName);
  int found = 0;
  SymbolTableEntry *ptr = tablePtr->hashTable[hashIndex];
  SymbolTableEntry *rtnEntry = NULL;
  while (ptr) {
    if ((strcmp(symbolName, ptr->name) == 0)){
      rtnEntry = ptr;
      found = 1;
      break;
    }
    ptr = ptr->prevInHashChain;
  }
  return rtnEntry;
}

//search symbol table stack from first until end
SymbolTableEntry*
retrieveSymbol(char* symbolName)
{
  SymbolTable *tablePtr = symbolTableStack.table;
  SymbolTableEntry *rtnEntry = NULL;
  while (tablePtr) {
    rtnEntry = searchTable(tablePtr, symbolName);
    if (rtnEntry) {
      break;
    }
    tablePtr = tablePtr->nextTable;
  }
  return rtnEntry;
}

//used in detect local symbol when declare local var
SymbolTableEntry*
detectSymbol(char *symbolName)
{
  return searchTable(symbolTableStack.table, symbolName);
}

DATA_TYPE
retrieveType(char *typeName)
{
  SymbolTableEntry *entry = retrieveSymbol(typeName);
  if (entry) {
    return entry->attribute->attr.typeDescriptor->properties.dataType;
  } else { 
    if (strcmp(typeName, SYMBOL_TABLE_INT_NAME) == 0) {
        return INT_TYPE;
    } else if (strcmp(typeName, SYMBOL_TABLE_FLOAT_NAME) == 0) {
      return FLOAT_TYPE;
    } else if (strcmp(typeName, SYMBOL_TABLE_VOID_NAME) == 0) {
      return VOID_TYPE;
    } else {
      return ERROR_TYPE;
    }
  }
}

void
initializeStack()
{

  symbolTableStack.table = NULL;
  symbolTableStack.numberOfStack = 0;
  openScope();
}

void 
stackEnd() 
{
  while (symbolTableStack.numberOfStack > 0) {
    closeScope();
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
  if(symbolTableStack.numberOfStack > 0) {
    SymbolTable *deleteTable = symbolTableStack.table;
    symbolTableStack.table = deleteTable->nextTable;
    symbolTableEnd(deleteTable);
    --symbolTableStack.numberOfStack;
  }
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
      nextptr = ptr->prevInHashChain;
      removeFromHashTrain(i, ptr, symbolTable);
      ptr = nextptr;
    }
  }
  free(symbolTable);
}

SymbolTableEntry* enterSymbol(char* symbolName, SymbolAttribute* attribute)
{
  int hashIndex = HASH(symbolName);
  SymbolTableEntry* entry = newSymbolTableEntry();
  entry->attribute = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
  entry->name = (char*)malloc(strlen(symbolName+1));
  memcpy(entry->attribute, attribute, sizeof(SymbolAttribute));
  memcpy(entry->name, symbolName, strlen(symbolName));
  enterIntoHashTrain(hashIndex, entry);
}

//remove the symbol from the current scope
void removeSymbol(char* symbolName)
{
  int hashIndex = HASH(symbolName);
  SymbolTableEntry* entry;
  if ((entry = retrieveSymbol(symbolName)) != NULL) {
    removeFromHashTrain(hashIndex, entry, symbolTableStack.table);
  }
}
