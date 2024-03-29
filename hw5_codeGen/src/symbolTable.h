#ifndef __SYMBOL_TABLE_H__
#define __SYMBOL_TABLE_H__

#include "header.h"
// This file is for reference only, you are not required to follow the implementation. //


//SYMBOL_TABLE_PREINSERT_NAME
#define SYMBOL_TABLE_INT_NAME "int"
#define SYMBOL_TABLE_FLOAT_NAME "float"
#define SYMBOL_TABLE_VOID_NAME "void"
#define SYMBOL_TABLE_SYS_LIB_READ "read"
#define SYMBOL_TABLE_SYS_LIB_FREAD "fread"
#define HASH_TABLE_SIZE 256


typedef enum SymbolAttributeKind
{
  VARIABLE_ATTRIBUTE,
  TYPE_ATTRIBUTE,
  FUNCTION_SIGNATURE
} SymbolAttributeKind;

typedef enum TypeDescriptorKind
{
  SCALAR_TYPE_DESCRIPTOR,
  ARRAY_TYPE_DESCRIPTOR,
} TypeDescriptorKind;

typedef struct ArrayProperties
{
  int dimension;
  int sizeInEachDimension[MAX_ARRAY_DIMENSION];
  //point to a TypeDescriptor in the symbol table;
  DATA_TYPE elementType;
} ArrayProperties;

typedef struct TypeDescriptor
{
  TypeDescriptorKind kind;
  union
  {
    DATA_TYPE dataType;//kind: SCALAR_TYPE_DESCRIPTOR
    ArrayProperties arrayProperties;//kind: ARRAY_TYPE_DESCRIPTOR
  } properties;
} TypeDescriptor;

typedef struct Parameter
{
  //point to a TypeDescriptor in the symbol table;
  struct Parameter* next;
  TypeDescriptor* type;
  char* parameterName;
} Parameter;

typedef struct FunctionSignature
{
  int parametersCount;
  Parameter* parameterList;
  DATA_TYPE returnType;
} FunctionSignature;

typedef struct SymbolAttribute
{
  SymbolAttributeKind attributeKind;

  union
  {
    TypeDescriptor* typeDescriptor;
    FunctionSignature* functionSignature;
  } attr;
} SymbolAttribute;

typedef struct SymbolTableEntry
{
  struct SymbolTableEntry* nextInHashChain;
  struct SymbolTableEntry* prevInHashChain;
  char* name;
  SymbolAttribute* attribute;
} SymbolTableEntry;

typedef struct SymbolTable
{
  SymbolTableEntry* hashTable[HASH_TABLE_SIZE];
  SymbolTableEntry** scopeDisplay;
  int offset;
  struct SymbolTable *nextTable;
} SymbolTable;

typedef struct SymbolTableStack
{
  SymbolTable *table;
  int numberOfStack;
} SymbolTableStack;

//table function
SymbolTable* initializeSymbolTable();
void symbolTableEnd(SymbolTable *symbolTable);
SymbolTableEntry *enterSymbol(char *symbolName, SymbolAttribute* attribute);
void removeSymbol(char *symbolName);
SymbolTableEntry* retrieveSymbol(char *symbolName);
SymbolTableEntry* detectSymbol(char *symbolName);
DATA_TYPE retrieveType(char *typeName);

int declaredLocally(char* symbolName);

//stack function
void initializeStack();
void stackEnd();
void openScope();
void closeScope();

#endif
