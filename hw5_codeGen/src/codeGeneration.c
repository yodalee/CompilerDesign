#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include <linux/limits.h>
#include "header.h"
#include "symbolTable.h"
#include "genfunction.h"

DATA_TYPE getBiggerType(DATA_TYPE dataType1, DATA_TYPE dataType2);
void gen_processProgramNode(AST_NODE *programNode);
void gen_processDeclarationNode(AST_NODE* declarationNode);
void gen_processTypeNode(AST_NODE* typeNode);
void gen_declareIdList(AST_NODE* typeNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize);
void gen_declareFunction(AST_NODE* returnTypeNode);
void gen_processDeclDimList(AST_NODE* variableDeclDimList, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize);
void gen_processBlockNode(AST_NODE* blockNode);
void gen_processStmtNode(AST_NODE* stmtNode);
void gen_processGeneralNode(AST_NODE *node);
void gen_checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode);
void gen_checkWhileStmt(AST_NODE* whileNode);
void gen_checkForStmt(AST_NODE* forNode);
void gen_checkAssignmentStmt(AST_NODE* assignmentNode);
void gen_checkIfStmt(AST_NODE* ifNode);
void gen_checkWriteFunction(AST_NODE* functionCallNode);
void gen_checkFunctionCall(AST_NODE* functionCallNode);
void gen_checkIDNode(AST_NODE *IDnode);
int cgen_heckSubscript(AST_NODE *IDNode);
void gen_processExprRelatedNode(AST_NODE* exprRelatedNode);
void gen_checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter);
void gen_checkParameterIdentifier(Parameter *formalParameter, AST_NODE *actualParameter);
void gen_checkReturnStmt(AST_NODE* returnNode);
void gen_processExprNode(AST_NODE* exprNode);
void gen_processVariableLValue(AST_NODE* idNode);
void gen_processVariableRValue(AST_NODE* idNode);
void gen_processConstValueNode(AST_NODE* constValueNode);
void gen_getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue);
void gen_evaluateExprValue(AST_NODE* exprNode);

SymbolTableStack symbolTableStack;
extern ARoffset;

//void 
//printParameter(Parameter* param) 
//{
//  int i = 1;
//  int j;
//  while (param) {
//    printf("parameter %d, name %s\n", i++, param->parameterName);
//    if (param->type->kind == ARRAY_TYPE_DESCRIPTOR) {
//      printf("array of type %d\n", param->type->properties.arrayProperties.elementType);
//      for (j = 0; j < param->type->properties.arrayProperties.dimension; ++j) {
//        printf("dimension %d\n", param->type->properties.arrayProperties.sizeInEachDimension[j]);
//      }
//    }
//    param = param->next;
//  }
//}

void codeGeneration(AST_NODE *root, char *cfile)
{
  char *outputname = strcat(strtok(basename(cfile), "."), ".asm");
  openfile(outputname);
  gen_processProgramNode(root);
  closefile();
}

void gen_processProgramNode(AST_NODE *programNode)
{
  AST_NODE* child = programNode->child;
  while (child) {
    switch(child->nodeType){
      case VARIABLE_DECL_LIST_NODE: {
        AST_NODE* varDeclNode = child->child;
        while (varDeclNode) {
          gen_processDeclarationNode(varDeclNode);
          varDeclNode = varDeclNode->rightSibling;
        }
        break;
      }
      case DECLARATION_NODE: {
        gen_processDeclarationNode(child);
        break;
      }
      default: {
        assert(0);
      }
    }
    child = child->rightSibling;
  }
}

void gen_processDeclarationNode(AST_NODE* declarationNode)
{
  //different declaration insert symbol table
  AST_NODE* child = declarationNode->child;
  DECL_KIND kind = declarationNode->semantic_value.declSemanticValue.kind;
  switch(kind){
    case TYPE_DECL:
      gen_processTypeNode(child);
      break;
    case VARIABLE_DECL: 
    {
      gen_declareIdList(child, VARIABLE_ATTRIBUTE, False);
      break;
    }
    case FUNCTION_DECL: 
    {
      gen_declareFunction(child);
      break;
    }
    default: 
    {
      assert(0);
    }
  }
}

void gen_processTypeNode(AST_NODE* idNodeAsType)
{
  AST_NODE* idNode = idNodeAsType->rightSibling;
  SymbolAttribute* typeAttr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
  typeAttr->attributeKind = TYPE_ATTRIBUTE;
  typeAttr->attr.typeDescriptor = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
  typeAttr->attr.typeDescriptor->properties.dataType 
    = retrieveType(idNodeAsType->semantic_value.identifierSemanticValue.identifierName);
  while (idNode) {
    //push into symbol table
    enterSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, typeAttr);
    //rightSibling until NULL
    idNode = idNode->rightSibling;
  }
}

void gen_declareIdList(AST_NODE* declarationNode, SymbolAttributeKind isVariableOrTypeAttribute, int ignoreArrayFirstDimSize)
{
  AST_NODE *idNode = declarationNode->rightSibling;
  while (idNode) {
    SymbolAttribute* varAttr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    TypeDescriptor* typeDesc = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    varAttr->attributeKind = VARIABLE_ATTRIBUTE;
    varAttr->attr.typeDescriptor = typeDesc;
    typeDesc->properties.dataType 
      = retrieveType(declarationNode->semantic_value.identifierSemanticValue.identifierName);

    gen_processDeclDimList(idNode, typeDesc, False);

    enterSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, varAttr);

    //code gen
    if (symbolTableStack.numberOfStack == 1) {
      gen_globalvar(idNode->semantic_value.identifierSemanticValue.identifierName);
    } else {
      symbolTableStack.table->offset = ARoffset;
      ARoffset -= 4;
    }

    idNode = idNode->rightSibling;
  }
}

void gen_declareFunction(AST_NODE* declarationNode)
{
  //get node
  AST_NODE* idNode = declarationNode->rightSibling;
  AST_NODE* paramListNode = idNode->rightSibling;
  AST_NODE* blockNode = paramListNode->rightSibling;

  SymbolAttribute *funcAttr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
  FunctionSignature* funcSig = (FunctionSignature*)malloc(sizeof(FunctionSignature));
  Parameter *param = NULL;
  //generate attribute
  funcAttr->attributeKind = FUNCTION_SIGNATURE;
  funcAttr->attr.functionSignature = funcSig;

  //generate signature
  funcSig->parametersCount = 0;
  funcSig->parameterList = NULL;
  funcSig->returnType = retrieveType(declarationNode->semantic_value.identifierSemanticValue.identifierName);

  //printf("get function %s\n", idNode->semantic_value.identifierSemanticValue.identifierName);
  //printf("return type %d\n", retrieveType(declarationNode->semantic_value.identifierSemanticValue.identifierName));

  //parse parameter node
  AST_NODE *paramNode = paramListNode->child;
  while(paramNode) {
    //push into Parameter node
    AST_NODE *varTypeNode = paramNode->child;
    AST_NODE *varidNode = varTypeNode->rightSibling;
    Parameter *nextParam = param;
    param = (Parameter*)malloc(sizeof(Parameter));
    param->type = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    param->parameterName = (char*)malloc(strlen(varidNode->semantic_value.identifierSemanticValue.identifierName+1));
    strcpy(param->parameterName, varidNode->semantic_value.identifierSemanticValue.identifierName);
    param->next = nextParam;
    ++(funcSig->parametersCount);
    param->type->properties.dataType
      = retrieveType(varTypeNode->semantic_value.identifierSemanticValue.identifierName);

    gen_processDeclDimList(varidNode, param->type, True);

    paramNode = paramNode->rightSibling;
  }
  funcSig->parameterList = param;
  //push into symbol table
  enterSymbol(idNode->semantic_value.identifierSemanticValue.identifierName, funcAttr);
  gen_head(idNode->semantic_value.identifierSemanticValue.identifierName);

  openScope();
  //generate param symbol
  SymbolAttribute *paramAttr = NULL;
  TypeDescriptor *paramType = NULL;
  Parameter *paramVar = param;
  while(paramVar){
    SymbolAttribute *paramAttr = (SymbolAttribute*)malloc(sizeof(SymbolAttribute));
    TypeDescriptor *paramType = (TypeDescriptor*)malloc(sizeof(TypeDescriptor));
    paramAttr->attributeKind = VARIABLE_ATTRIBUTE;
    paramAttr->attr.typeDescriptor = paramType;

    memcpy(paramType, param->type, sizeof(TypeDescriptor));
    //push into symbol table
    enterSymbol(paramVar->parameterName, paramAttr);

    paramVar = paramVar->next;
  }
  
  gen_prologue(idNode->semantic_value.identifierSemanticValue.identifierName);
  gen_processBlockNode(blockNode);
  closeScope();
  gen_epilogue(idNode->semantic_value.identifierSemanticValue.identifierName,ARoffset);
}

//void checkAssignOrExpr(AST_NODE* assignOrExprRelatedNode)
//{
//  AST_NODE *assignExpr = assignOrExprRelatedNode->child;
//  while (assignExpr) {
//    assignExpr = assignExpr->rightSibling;
//  }
//}
//
//void checkWhileStmt(AST_NODE* whileNode)
//{
//  AST_NODE *condNode = whileNode->child;
//  AST_NODE *blockNode = condNode->rightSibling;
//  processExprRelatedNode(condNode);
//  openScope();
//  gen_processBlockNode(blockNode);
//  closeScope();
//}
//
//void checkForStmt(AST_NODE* forNode)
//{
//  AST_NODE *initNode = forNode->child;
//  AST_NODE *condNode = initNode->rightSibling;
//  AST_NODE *incrNode = condNode->rightSibling;
//  AST_NODE *stmtNode = incrNode->rightSibling;
//  initNode = initNode->child;
//  while (initNode) {
//    checkAssignmentStmt(initNode);
//    initNode = initNode->rightSibling;
//  }
//  if (condNode->nodeType != NUL_NODE) {
//    condNode = condNode->child;
//    while (condNode) {
//      processExprRelatedNode(condNode);
//      condNode = condNode->rightSibling;
//    }
//  }
//  incrNode = incrNode->child;
//  while (incrNode) {
//    checkAssignmentStmt(incrNode);
//    incrNode = incrNode->rightSibling;
//  }
//  openScope();
//  gen_processBlockNode(stmtNode);
//  closeScope();
//}
//
//void checkAssignmentStmt(AST_NODE* assignmentNode)
//{
//  AST_NODE *idNode = assignmentNode->child;
//  AST_NODE *valNode = idNode->rightSibling;
//  processVariableLValue(idNode);
//  processVariableRValue(valNode);
//}
//
//void checkIfStmt(AST_NODE* ifNode)
//{
//  AST_NODE* exprNode = ifNode->child;
//  AST_NODE* blockNode1 = exprNode->rightSibling;
//  AST_NODE* blockNode2 = blockNode1->rightSibling;
//  openScope();
//  gen_processBlockNode(blockNode1);
//  closeScope();
//  if (blockNode2->nodeType == BLOCK_NODE) {
//    openScope();
//    gen_processBlockNode(blockNode2);
//    closeScope();
//  }
//}
//
//void checkWriteFunction(AST_NODE* functionCallNode)
//{
//  AST_NODE* exprNode = functionCallNode->child->rightSibling;
//  AST_NODE* paramNode = NULL;
//  if (exprNode->nodeType == NUL_NODE) {
//    printErrorMsg(functionCallNode->child, TOO_FEW_ARGUMENTS);
//  } else {
//    paramNode = exprNode->child;
//    if (paramNode->rightSibling) {
//      printErrorMsg(functionCallNode->child, TOO_MANY_ARGUMENTS);
//    } else if (paramNode->nodeType != CONST_VALUE_NODE) {
//      if (paramNode->semantic_value.const1->const_type != STRINGC) {
//        printErrorMsg(functionCallNode->child, PARAMETER_TYPE_UNMATCH);
//      }
//    }
//  }
//}
//
//void checkFunctionCall(AST_NODE* functionCallNode)
//{
//  AST_NODE *idNode = functionCallNode->child;
//  AST_NODE *firstParamNode = idNode->rightSibling;
//  AST_NODE *paramNode = NULL;
//  int actualParamNum = 0;
//  int paramNum = 0;
//  functionCallNode->dataType = ERROR_TYPE;
//  if (strcmp(idNode->semantic_value.identifierSemanticValue.identifierName, "write") == 0) {
//    checkWriteFunction(functionCallNode);
//  } else if(strcmp(idNode->semantic_value.identifierSemanticValue.identifierName, "read") == 0
//      || strcmp(idNode->semantic_value.identifierSemanticValue.identifierName, "fread") == 0) {
//  } else {
//    SymbolTableEntry *entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
//    functionCallNode->dataType = retrieveType(idNode->semantic_value.identifierSemanticValue.identifierName);
//    if (entry == NULL) {
//      printErrorMsg(idNode, SYMBOL_UNDECLARED); 
//    } else if (entry->attribute->attributeKind != FUNCTION_SIGNATURE) {
//      printErrorMsg(idNode, NOT_FUNCTION_NAME);
//    } else {
//      actualParamNum = entry->attribute->attr.functionSignature->parametersCount;
//      if (actualParamNum > 0 && firstParamNode->nodeType == NUL_NODE) {
//        printErrorMsg(idNode, TOO_FEW_ARGUMENTS);
//      } else {
//        firstParamNode = firstParamNode->child;
//        paramNode = firstParamNode;
//        while (firstParamNode) {
//          firstParamNode = firstParamNode->rightSibling;
//          ++paramNum;
//        }
//        if (paramNum < actualParamNum) {
//          printErrorMsg(idNode, TOO_FEW_ARGUMENTS);
//        } else if (paramNum > actualParamNum) {
//          printErrorMsg(idNode, TOO_MANY_ARGUMENTS);
//        } else {
//          checkParameterPassing(entry->attribute->attr.functionSignature->parameterList, paramNode);
//        }
//      }
//    }
//  }
//}
//
//DATA_TYPE
//getDataType(TypeDescriptor *type)
//{
//  switch(type->kind){
//    case SCALAR_TYPE_DESCRIPTOR:
//    {
//      return type->properties.dataType;
//      break;
//    }
//    case ARRAY_TYPE_DESCRIPTOR:
//    {
//      return type->properties.arrayProperties.elementType;
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}
//
//void checkParameterPassing(Parameter* formalParameter, AST_NODE* actualParameter)
//{
//  while (formalParameter && actualParameter) {
//    if (actualParameter->nodeType == EXPR_NODE) {
//      processExprNode(actualParameter);
//      if (formalParameter->type->kind == ARRAY_TYPE_DESCRIPTOR) {
//        printErrorMsg(actualParameter, PASS_SCALAR_TO_ARRAY);
//      }
//    } else if (actualParameter->nodeType == CONST_VALUE_NODE) {
//    } else if (actualParameter->nodeType == STMT_NODE) {
//      checkFunctionCall(actualParameter);
//      SymbolTableEntry *entry = retrieveSymbol(actualParameter->semantic_value.identifierSemanticValue.identifierName);
//      if (entry != NULL) {
//        if (formalParameter->type->kind == ARRAY_TYPE_DESCRIPTOR ) {
//          printErrorMsg(actualParameter, PASS_SCALAR_TO_ARRAY);
//        } else {
//          if (getDataType(formalParameter->type) != getDataType(entry->attribute->attr.typeDescriptor)) {
//            printErrorMsgSpecial(actualParameter, formalParameter->parameterName, PARAMETER_TYPE_UNMATCH);
//          }
//        }
//      }
//    } else if (actualParameter->nodeType == IDENTIFIER_NODE) {
//      checkParameterIdentifier(formalParameter, actualParameter);
//    }
//
//    formalParameter = formalParameter->next;
//    actualParameter = actualParameter->rightSibling;
//  }
//}
//
//void
//checkParameterIdentifier(Parameter *formalParameter, AST_NODE *actualParameter)
//{
//  SymbolTableEntry *entry = retrieveSymbol(actualParameter->semantic_value.identifierSemanticValue.identifierName);
//  if (entry == NULL) {
//    printErrorMsg(actualParameter, SYMBOL_UNDECLARED);
//  } else if (entry->attribute->attributeKind == FUNCTION_SIGNATURE 
//        || entry->attribute->attributeKind == TYPE_ATTRIBUTE) {
//    printErrorMsgSpecial(actualParameter, formalParameter->parameterName, PARAMETER_TYPE_UNMATCH);
//  } else {
//    if (formalParameter->type->kind == ARRAY_TYPE_DESCRIPTOR 
//        && entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
//      printErrorMsg(actualParameter, PASS_SCALAR_TO_ARRAY);
//    } else if (formalParameter->type->kind == SCALAR_TYPE_DESCRIPTOR 
//        && entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {
//      printErrorMsg(actualParameter, PASS_ARRAY_TO_SCALAR);
//    } else {
//      if (getDataType(formalParameter->type) != getDataType(entry->attribute->attr.typeDescriptor)) {
//        printErrorMsgSpecial(actualParameter, formalParameter->parameterName, PARAMETER_TYPE_UNMATCH);
//      }
//    }
//  }
//}
//
//void processExprRelatedNode(AST_NODE* exprRelatedNode)
//{
//  switch(exprRelatedNode->nodeType){
//    case IDENTIFIER_NODE:
//    {
//      checkIDNode(exprRelatedNode);
//      break;
//    }
//    case EXPR_NODE:
//    {
//      processExprNode(exprRelatedNode);
//      break;
//    }
//    case STMT_NODE:
//    {
//      checkFunctionCall(exprRelatedNode);
//      break;
//    }
//    case CONST_VALUE_NODE:
//    {
//      processConstValueNode(exprRelatedNode);
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}
//
//void checkIDNode(AST_NODE *idNode){
//  SymbolTableEntry *entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
//  idNode->dataType = retrieveType(idNode->semantic_value.identifierSemanticValue.identifierName);
//  if (entry == NULL) {
//    printErrorMsg(idNode, SYMBOL_UNDECLARED);
//  } else if (entry->attribute->attributeKind == FUNCTION_SIGNATURE) {
//    printErrorMsg(idNode, IS_FUNCTION_NOT_VARIABLE);
//  } else if (entry->attribute->attributeKind == TYPE_ATTRIBUTE) {
//    printErrorMsg(idNode, IS_TYPE_NOT_VARIABLE);
//  } else if (idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID
//      && entry->attribute->attr.typeDescriptor->kind == SCALAR_TYPE_DESCRIPTOR) {
//    printErrorMsg(idNode, NOT_ARRAY);
//  } else if (idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
//    checkSubscript(idNode);
//  }
//}
//
//int
//checkSubscript(AST_NODE *idNode)
//{
//  AST_NODE *dimNode = idNode->child;
//  SymbolTableEntry *entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
//  int dim = 0;
//  while (dimNode) {
//    processExprRelatedNode(dimNode);
//    evaluateExprValue(dimNode);
//    if (dimNode->dataType != INT_TYPE) {
//      printErrorMsg(dimNode, ARRAY_SUBSCRIPT_NOT_INT);
//    }
//    ++dim;
//    dimNode = dimNode->rightSibling;
//  }
//  if (dim != entry->attribute->attr.typeDescriptor->properties.arrayProperties.dimension) {
//    printErrorMsg(idNode, INCOMPATIBLE_ARRAY_DIMENSION);
//  }
//}
//
////most important, get its type
//void processExprNode(AST_NODE* exprNode)
//{
//  if (exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION) {
//    AST_NODE *child = exprNode->child;
//    processExprRelatedNode(child);
//    if (child->dataType == CONST_STRING_TYPE ) {
//      printErrorMsg(exprNode, STRING_OPERATION);
//      exprNode->dataType = ERROR_TYPE;
//      exprNode->semantic_value.exprSemanticValue.isConstEval = False;
//    } else if (exprNode->semantic_value.exprSemanticValue.op.unaryOp == UNARY_OP_LOGICAL_NEGATION
//        && child->dataType == FLOAT_TYPE) {
//      printErrorMsg(exprNode, FLOAT_LOGICAL_NEGATION);
//      exprNode->dataType = ERROR_TYPE;
//      exprNode->semantic_value.exprSemanticValue.isConstEval = False;
//    } else {
//      if ((child->nodeType == CONST_VALUE_NODE)
//          || (child->nodeType == EXPR_NODE && child->semantic_value.exprSemanticValue.isConstEval)) {
//        exprNode->semantic_value.exprSemanticValue.isConstEval = True;
//      }
//    }
//  } else if ( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION) {
//    AST_NODE *lnode = exprNode->child;
//    AST_NODE *rnode = lnode->rightSibling;
//    processExprRelatedNode(lnode);
//    processExprRelatedNode(rnode);
//    if (lnode->dataType == CONST_STRING_TYPE || rnode->dataType == CONST_STRING_TYPE) {
//      printErrorMsg(exprNode, STRING_OPERATION);
//      exprNode->dataType = ERROR_TYPE;
//      exprNode->semantic_value.exprSemanticValue.isConstEval = False;
//    } else {
//      exprNode->dataType = getBiggerType( lnode->dataType, rnode->dataType );
//      if (((lnode->nodeType == CONST_VALUE_NODE)
//          || (lnode->nodeType == EXPR_NODE && lnode->semantic_value.exprSemanticValue.isConstEval))
//          && ((rnode->nodeType == CONST_VALUE_NODE) ||
//             (rnode->nodeType == EXPR_NODE && rnode->semantic_value.exprSemanticValue.isConstEval))) {
//        exprNode->semantic_value.exprSemanticValue.isConstEval = True;
//      }
//    }
//  }
//}
//
//void getExprOrConstValue(AST_NODE* exprOrConstNode, int* iValue, float* fValue)
//{
//  if (exprOrConstNode->nodeType == EXPR_NODE && exprOrConstNode->semantic_value.exprSemanticValue.isConstEval) {
//    *iValue = exprOrConstNode->semantic_value.exprSemanticValue.constEvalValue.iValue;
//    *fValue = exprOrConstNode->semantic_value.exprSemanticValue.constEvalValue.fValue;
//  } else if (exprOrConstNode->nodeType == CONST_VALUE_NODE) {
//    *iValue = exprOrConstNode->semantic_value.const1->const_u.intval;
//    *fValue = exprOrConstNode->semantic_value.const1->const_u.fval;
//  }
//}
//
//void
//evaluateUnaryOp(AST_NODE *exprNode)
//{
//  int iValue = 0;
//  float fValue = 0;
//  AST_NODE *operand = exprNode->child;
//  evaluateExprValue(operand);
//  getExprOrConstValue(operand, &iValue, &fValue);
//  switch(exprNode->semantic_value.exprSemanticValue.op.unaryOp){
//    case UNARY_OP_POSITIVE:
//    {
//      exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = iValue;
//      exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = fValue;
//      break;
//    }
//    case UNARY_OP_NEGATIVE:
//    {
//      exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = -1*iValue;
//      exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = -1*fValue;
//      break;
//    }
//    case UNARY_OP_LOGICAL_NEGATION:
//    {
//      exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = ~iValue;
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}
//
//void
//evaluateBinaryOp(AST_NODE *exprNode)
//{
//  int iValue1 = 0, iValue2 = 0;
//  float fValue1 = 0, fValue2 = 0;
//  AST_NODE *operand1 = exprNode->child;
//  AST_NODE *operand2 = operand1->rightSibling;
//  evaluateExprValue(operand1);
//  evaluateExprValue(operand2);
//  getExprOrConstValue(operand1, &iValue1, &fValue1);
//  getExprOrConstValue(operand2, &iValue2, &fValue2);
//  switch(exprNode->semantic_value.exprSemanticValue.op.binaryOp){
//    case BINARY_OP_ADD:
//    {
//      if (exprNode->dataType == INT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = iValue1 + iValue2;
//      } else if(exprNode->dataType == FLOAT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = fValue1 + fValue2;
//      }
//      break;
//    }
//    case BINARY_OP_SUB:
//    {
//      if (exprNode->dataType == INT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = iValue1 - iValue2;
//      } else if(exprNode->dataType == FLOAT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = fValue1 - fValue2;
//      }
//      break;
//    }
//    case BINARY_OP_MUL:
//    {
//      if (exprNode->dataType == INT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = iValue1 * iValue2;
//      } else if(exprNode->dataType == FLOAT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = fValue1 * fValue2;
//      }
//      break;
//    }
//    case BINARY_OP_DIV:
//    {
//      if (exprNode->dataType == INT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.iValue = iValue1 / iValue2;
//      } else if(exprNode->dataType == FLOAT_TYPE) {
//        exprNode->semantic_value.exprSemanticValue.constEvalValue.fValue = fValue1 / fValue2;
//      }
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}
//
////ignore function call evaluate
////ignore variable evaluate
//void evaluateExprValue(AST_NODE* exprNode)
//{
//  //only evaluate when expr node is expr const
//  if (exprNode->nodeType == EXPR_NODE && exprNode->semantic_value.exprSemanticValue.isConstEval) {
//    if (exprNode->semantic_value.exprSemanticValue.kind == UNARY_OPERATION) {
//      evaluateUnaryOp(exprNode);
//    } else if ( exprNode->semantic_value.exprSemanticValue.kind == BINARY_OPERATION) {
//      evaluateBinaryOp(exprNode);
//    }
//  }
//}
//
//void processVariableLValue(AST_NODE* idNode)
//{
//  SymbolTableEntry *entry = retrieveSymbol(idNode->semantic_value.identifierSemanticValue.identifierName);
//  DATA_TYPE type = retrieveType(idNode->semantic_value.identifierSemanticValue.identifierName);
//  if (entry == NULL) {
//    printErrorMsg(idNode, SYMBOL_UNDECLARED);
//  } else if (entry->attribute->attributeKind == FUNCTION_SIGNATURE) {
//    printErrorMsg(idNode, IS_FUNCTION_NOT_VARIABLE);
//  } else if (entry->attribute->attributeKind == TYPE_ATTRIBUTE) {
//    printErrorMsg(idNode, IS_TYPE_NOT_VARIABLE);
//  } else if (type == VOID_TYPE) {
//    printErrorMsg(idNode, NOT_ASSIGNABLE);
//  }
//}
//
//void processVariableRValue(AST_NODE* idNode)
//{
//  processExprRelatedNode(idNode);
//}
//
//void processConstValueNode(AST_NODE* constValueNode)
//{
//  switch(constValueNode->semantic_value.const1->const_type){
//    case INTEGERC:
//    {
//      constValueNode->dataType = INT_TYPE;
//      break;
//    }
//    case FLOATC:
//    {
//      constValueNode->dataType = FLOAT_TYPE;
//      break;
//    }
//    case STRINGC:
//    {
//      constValueNode->dataType = CONST_STRING_TYPE;
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}
//
//
//void checkReturnStmt(AST_NODE* returnNode)
//{
//  AST_NODE *exprNode = returnNode->child;
//  switch(exprNode->nodeType){
//    case IDENTIFIER_NODE:
//    {
//      SymbolTableEntry *entry 
//        = retrieveSymbol(exprNode->semantic_value.identifierSemanticValue.identifierName);
//      if (entry == NULL) {
//        printErrorMsg(exprNode, SYMBOL_UNDECLARED);
//      } else if (exprNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
//        checkSubscript(exprNode);
//      } else if (exprNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID 
//           && entry->attribute->attr.typeDescriptor->kind == ARRAY_TYPE_DESCRIPTOR) {
//        printErrorMsg(exprNode, RETURN_ARRAY);
//      }
//      break;
//    }
//    case EXPR_NODE:
//    case CONST_VALUE_NODE:
//    {
//      processExprRelatedNode(exprNode);
//      //get function node
//      AST_NODE *funNode = returnNode;
//      while (funNode->nodeType != DECLARATION_NODE 
//          || funNode->semantic_value.declSemanticValue.kind != FUNCTION_DECL) {
//        funNode = funNode->parent;
//      }
//      if (exprNode->dataType != retrieveType(funNode->child->semantic_value.identifierSemanticValue.identifierName)) {
//        printErrorMsg(exprNode, RETURN_TYPE_UNMATCH);
//      } 
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}

void gen_processBlockNode(AST_NODE* blockNode)
{
  AST_NODE* child = blockNode->child;
  while (child) {
    switch(child->nodeType){
      case VARIABLE_DECL_LIST_NODE:
      {
        AST_NODE* varDeclNode = child->child;
        while (varDeclNode) {
          gen_processDeclarationNode(varDeclNode);
          varDeclNode = varDeclNode->rightSibling;
        }
        break;
      }
      //case STMT_LIST_NODE:
      //{
      //  AST_NODE* stmtNode = child->child;
      //  while (stmtNode) {
      //    processStmtNode(stmtNode);
      //    stmtNode = stmtNode->rightSibling;
      //  }
      //  break;
      //}
      default: 
      {
        break;
        assert(0);
      }
    }
    child = child->rightSibling;
  }
}


//void processStmtNode(AST_NODE* stmtNode)
//{
//  switch(stmtNode->semantic_value.stmtSemanticValue.kind){
//    case WHILE_STMT:
//    {
//      checkWhileStmt(stmtNode);
//      break;
//    }
//    case FOR_STMT:
//    {
//      checkForStmt(stmtNode);
//      break;
//    }
//    case IF_STMT:
//    {
//      checkIfStmt(stmtNode);
//      break;
//    }
//    case ASSIGN_STMT:
//    {
//      checkAssignmentStmt(stmtNode);
//      break;
//    }
//    case FUNCTION_CALL_STMT:
//    {
//      checkFunctionCall(stmtNode);
//      break;
//    }
//    case RETURN_STMT:
//    {
//      checkReturnStmt(stmtNode);
//      break;
//    }
//    default: 
//    {
//      assert(0);
//    }
//  }
//}
//
void gen_processDeclDimList(AST_NODE* idNode, TypeDescriptor* typeDescriptor, int ignoreFirstDimSize)
{ 
  if (idNode->semantic_value.identifierSemanticValue.kind == NORMAL_ID) {
    typeDescriptor->kind = SCALAR_TYPE_DESCRIPTOR;
  } else if (idNode->semantic_value.identifierSemanticValue.kind == ARRAY_ID) {
    typeDescriptor->kind = ARRAY_TYPE_DESCRIPTOR;
    AST_NODE *dimNode = idNode->child;
    int dim = 0;
    while (dimNode) {
      if (dimNode->nodeType != NUL_NODE) {
        processExprRelatedNode(dimNode);
        evaluateExprValue(dimNode);
      }
      typeDescriptor->properties.arrayProperties.sizeInEachDimension[dim] 
        = dimNode->semantic_value.exprSemanticValue.constEvalValue.iValue;
      ++dim;
      dimNode = dimNode->rightSibling;
    }
    typeDescriptor->properties.arrayProperties.dimension = dim;
  }
}
