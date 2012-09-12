#ifndef EVALUATOR_H
#define EVALUATOR_H
#include "sexpr.h"
#include "hashtable.h"



typedef HashTable SymbolTable;

SExpression *eval(SExpression *expr, SymbolTable *ST);
SExpression *apply(SExpression *function, SExpression *args, SymbolTable *ST);



#endif
