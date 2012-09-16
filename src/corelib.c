#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "corelib.h"
#include "common.h"


SymbolTable *CoreLibrary = NULL;
extern SymbolTable *UserLibrary;


SExpression *handle_define(SExpression *args) {
  if (!args ||
      list_length(args) != 2 ||
      !UserLibrary)
    return NULL;
  SExpression *func = NULL;
  char *name;
  if (args->pair->value->type == tt_mention) {
    name = strdup(args->pair->value->mention);
    func = duplicate_expression(args->pair->next->pair->value);
  } else if (args->pair->value->type == tt_pair) {
    SExpression *namewargs = args->pair->value;
    SExpression *body = args->pair->next->pair->value;
    func = alloc_term(tt_lambda);
    name = strdup(namewargs->pair->value->mention);
    func->lambda->args = duplicate_expression(namewargs->pair->next);
    func->lambda->body = duplicate_expression(body);
    func->lambda->arity = list_length(func->lambda->args); 
  } else
    return NULL;
  //printf("Handling define:\n");
  //printf("Name is %s.\n", name);
  //printf("Args:"); print_expression(l->lambda->args);
  //printf("Body:"); print_expression(l->lambda->body);
  //printf("Arity = %d.\n", l->lambda->arity);
  ht_insert(UserLibrary, name, func);
  return alloc_term(tt_nil);
}


SExpression *handle_if(SExpression *args) {
  SExpression *predic = (args->pair->value) ? duplicate_expression(args->pair->value) : NULL,
    *ifbranch = (args->pair->next->pair->value) ? duplicate_expression(args->pair->next->pair->value) : NULL,
    *elsebranch = (args->pair->next->pair->next) ? duplicate_expression(args->pair->next->pair->next->pair->value) : NULL;
  //printf("Handling if.\n");
  //printf("Predicate:\t"); print_expression(predic);
  //printf("If-branch:\t"); print_expression(ifbranch);
  //printf("Else-branch:\t"); print_expression(elsebranch);
  if (!predic ||
      !ifbranch ||
      !elsebranch)
    return NULL;
  //printf("Evaluating predicate.\n");
  predic = eval(predic);
  if ((predic->type == tt_nil) ||
      ((predic->type == tt_bool) &&
       (predic->boolean == false))) {
    //printf("Evaluating else-branch.\n");
    return eval(elsebranch);
  } else {
    //printf("Evaluating if-branch.\n");
    return eval(ifbranch);
  }
}

// Comparison functions

SExpression *handle_equal(SExpression *args) {
  if (!args)
    return NULL;
  SExpression *left = args->pair->value, *right = args->pair->next->pair->value;
  left = eval(left);
  right = eval(right);
  SExpression *answer = alloc_term(tt_bool);
  if (left->type != right->type) {
    if ((left->type == tt_int && right->type == tt_float) ||
	(left->type == tt_float && right->type == tt_int)) {
      double left_val = (left->type == tt_float) ? left->real : left->integer;
      double right_val = (right->type == tt_float) ? right->real : right->integer;
      answer->boolean = (left_val == right_val);
    } else
      answer->boolean = false;
  } else {
    switch (left->type) {
    case tt_bool:
      answer->boolean = (left->boolean == right->boolean) ? true : false;
      break;
    case tt_nil:
      answer->boolean = true;
      break;
    case tt_int:
      answer->boolean = (left->integer == right->integer) ? true : false;
      break;
    case tt_float:
      answer->boolean = (left->real == right->real) ? true : false;
      break;
    case tt_mention:
      answer->boolean = (strcmp(left->mention, right->mention) == 0) ? true : false;
      break;
    case tt_pair: // oh god, not this shit again
      answer = false;
    case tt_lambda:
      answer = false;
    default:
      break;
    }
  }
  return answer;
}


SExpression *handle_less(SExpression *args) {
  if (!args)
    return NULL;
  SExpression *left = args->pair->value, *right = args->pair->next->pair->value;
  left = eval(left);
  right = eval(right);
  SExpression *answer = alloc_term(tt_bool);
  if (left->type == tt_float &&
      right->type == tt_float)
    answer->boolean = (left->real < right->real) ? true : false;
  else if (left->type == tt_int &&
	     right->type == tt_int)
    answer->boolean = (left->integer < right->integer) ? true : false;
  else if ((left->type == tt_int && right->type == tt_float) ||
	     (left->type == tt_float && right->type == tt_int)) {
    double left_val = (left->type == tt_float) ? left->real : left->integer;
    double right_val = (right->type == tt_float) ? right->real : right->integer;
    answer->boolean = (left_val < right_val);
  } else
    answer->boolean = false;
  return answer;
}


SExpression *handle_greater(SExpression *args) {
  if (!args)
    return NULL;
  SExpression *left = args->pair->value, *right = args->pair->next->pair->value;
  left = eval(left);
  right = eval(right);
  SExpression *answer = alloc_term(tt_bool);
  if (left->type == tt_float &&
      right->type == tt_float)
    answer->boolean = (left->real > right->real) ? true : false;
  else if (left->type == tt_int &&
	   right->type == tt_int)
    answer->boolean = (left->integer > right->integer) ? true : false;
  else if ((left->type == tt_int && right->type == tt_float) ||
	   (left->type == tt_float && right->type == tt_int)) {
    double left_val = (left->type == tt_float) ? left->real : left->integer;
    double right_val = (right->type == tt_float) ? right->real : right->integer;
    answer->boolean = (left_val > right_val);
  } else
    answer->boolean = false;    
  return answer;
}

// End of comparison functions.
// Logic functions.


SExpression *handle_and(SExpression *args) {
  if (!args)
    return NULL;
  SExpression *current = args, *tmp;
  SExpression *result = alloc_term(tt_bool);
  result->boolean = true;
  while (current) {
    tmp = eval(current->pair->value);
    if ((tmp->type == tt_bool && tmp->boolean == false) ||
	(tmp->type == tt_nil)) {
      result->boolean = false;
      break;
    }
    current = current->pair->next;
  }
  return result;
}


SExpression *handle_or(SExpression *args) {
  if (!args)
    return NULL;
  SExpression *current = args, *tmp;
  while (current) {
    tmp = eval(current->pair->value);
    if ((tmp->type == tt_bool && tmp->boolean == false) ||
	(tmp->type == tt_nil))
      current = current->pair->next;
    else
      return tmp;
  }
  SExpression *false_res = alloc_term(tt_bool);
  false_res->boolean = false;
  return false_res;
}


// End of logic functions.
// Arithmetic functions.
  
SExpression *handle_mult(SExpression *args) {
  if (!args)
    return NULL;
  double product = 1;
  int all_ints = 1;
  SExpression *current = args, *tmp;
  while (current) {
    tmp = eval(current->pair->value);
    if (tmp->type == tt_int)
      product *= tmp->integer;
    else if (tmp->type == tt_float) {
      all_ints = 0;
      product *= tmp->real;
    } else
      return NULL;
    current = current->pair->next;
  }
  SExpression *result = NULL;
  if (all_ints) {
    result = alloc_term(tt_int);
    result->integer = (long int) product;
  } else {
    result = alloc_term(tt_float);
    result->real = product;
  }
  return result;
}


SExpression *handle_plus(SExpression *args) {
  if (!args)
    return NULL;
  double sum = 0;
  int all_ints = 1;
  SExpression *current = args, *tmp;
  while (current) {
    tmp = eval(current->pair->value);
    if (tmp->type == tt_int)
      sum += tmp->integer;
    else if (tmp->type == tt_float) {
      all_ints = 0;
      sum += tmp->real;
    } else
      return NULL;
    current = current->pair->next;
  }
  SExpression *result = NULL;
  if (all_ints) {
    result = alloc_term(tt_int);
    result->integer = (long int) sum;
  } else {
    result = alloc_term(tt_float);
    result->real = sum;
  }
  return result;
}


SExpression *handle_minus(SExpression *args) {
  if (!args ||
      args->type != tt_pair)
    return NULL;
  int all_ints = 1;
  SExpression *start = eval(args->pair->value);
  double diff;
  if (start->type == tt_int)
    diff = start->integer;
  else if (start->type == tt_float) {
    all_ints = 0;
    diff = start->real;
  } else
    return NULL;
  if (!args->pair->next)
    diff = -diff;
  else {
    SExpression *current = args->pair->next, *tmp;
    while (current) {
      tmp = eval(current->pair->value);
      if (tmp->type == tt_int)
	diff -= tmp->integer;
      else if (tmp->type == tt_float) {
	all_ints = 0;
	diff -= tmp->real;
      } else
	return NULL;
      current = current->pair->next;
    }
  }
  SExpression *result = NULL;
  if (all_ints) {
    result = alloc_term(tt_int);
    result->integer = (long int) diff;
  } else {
    result = alloc_term(tt_float);
    result->real = diff;
  }    
  return result; 
}



SExpression *handle_divide(SExpression *args) {
  if (!args ||
      args->type != tt_pair)
    return NULL;
  SExpression *dividend = eval(args->pair->value);
  double quotient;
  if (dividend->type == tt_int)
    quotient = dividend->integer;
  else if (dividend->type == tt_float)
    quotient = dividend->real;
  else
    return NULL;
  SExpression *current = args->pair->next, *tmp;
  while (current) {
    tmp = eval(current->pair->value);
    if (tmp->type == tt_int)
      quotient /= (double) tmp->integer;
    else if (tmp->type == tt_float)
      quotient -= tmp->real;
    else
      return NULL;
    current = current->pair->next;
  }
  SExpression *result = alloc_term(tt_float);
  result->real = quotient;   
  return result; 
}


SExpression *handle_div(SExpression *args) {
  if (!args ||
      args->type != tt_pair)
    return NULL;
  SExpression *dividend = eval(args->pair->value);
  if (dividend->type != tt_int) 
    return NULL;
  long int quotient = dividend->integer;
  SExpression *current = args->pair->next, *tmp;
  while (current) {
    tmp = eval(current->pair->value);
    if (tmp->type == tt_int)
      quotient /= tmp->integer;
    else
      return NULL;
    current = current->pair->next;
  }
  SExpression *result = alloc_term(tt_int);
  result->integer = quotient;
  return result;
}


SExpression *handle_remainder(SExpression *args) {
  if (!args ||
      args->type != tt_pair)
    return NULL;
  SExpression *dividend = eval(args->pair->value);
  if (dividend->type != tt_int) 
    return NULL;
  long int remainder = dividend->integer;
  SExpression *current = args->pair->next, *tmp;
  while (current) {
    tmp = eval(current->pair->value);
    if (tmp->type == tt_int)
      remainder %= tmp->integer;
    else
      return NULL;
    current = current->pair->next;
  }
  SExpression *result = alloc_term(tt_int);
  result->integer = remainder;
  return result;
}

// End of arithmetic functions.
// List functions


SExpression *handle_car(SExpression *arg) {
  if (!arg ||
      arg->type != tt_pair)
    return NULL;
  SExpression *evald =  eval(arg->pair->value);
  return evald->pair->value;
}


SExpression *handle_cdr(SExpression *arg) {
  if (!arg ||
      arg->type != tt_pair)
    return NULL;
  SExpression *list = eval(arg->pair->value);
  return (list->pair->next) ? list->pair->next : alloc_term(tt_nil);
}


SExpression *handle_list(SExpression *args) {
  if (!args ||
      args->type != tt_pair)
    return NULL;  
  SExpression *current = args, *tmp;
  SExpression *root = NULL, *last = NULL;
  while (current) {
    tmp = eval(current->pair->value);
    if (!root)
      last = root = alloc_term(tt_pair);
    else
      last = last->pair->next = alloc_term(tt_pair);
    last->pair->value = tmp;
    current = current->pair->next;
  }
  return root;
}


SExpression *handle_cons(SExpression *args) {
  if (!args ||
      args->type != tt_pair)
    return NULL;
  SExpression *val_arg = eval(args->pair->value);
  SExpression *result = alloc_term(tt_pair);
  result->pair->value = val_arg;
  SExpression *list_arg = eval(args->pair->next->pair->value);
  if (list_arg->type == tt_pair) {  
    result->pair->next = list_arg;
  } else if (list_arg->type == tt_nil)
    ;//do nothing
  else
    return NULL;
  return result;
}

// End of list functions.
// Type predicates.

SExpression *handle_is_int(SExpression *arg) {
  if (!arg ||
      arg->type != tt_pair)
    return NULL;
  SExpression *result = alloc_term(tt_bool);
  result->boolean = (arg->pair->value->type == tt_int) ? true : false;
  return result;
}


SExpression *handle_is_nil(SExpression *arg) {
  if (!arg ||
      arg->type != tt_pair)
    return NULL;
  SExpression *result = alloc_term(tt_bool);
  result->boolean = (arg->pair->value->type == tt_nil) ? true : false;
  return result;
}


SExpression *handle_is_float(SExpression *arg) {
  if (!arg ||
      arg->type != tt_pair)
    return NULL;
  SExpression *result = alloc_term(tt_bool);
  result->boolean = (arg->pair->value->type == tt_float) ? true : false;
  return result;
}


// End of type predicates.

void load_core_library() {
  if (CoreLibrary)
    return;
  CoreLibrary = ht_create(32);
  // Comparison.
  ht_insert(CoreLibrary, "=", handle_equal);
  ht_insert(CoreLibrary, ">", handle_greater);
  ht_insert(CoreLibrary, "<", handle_less);
  // Logic.
  ht_insert(CoreLibrary, "or", handle_or);
  ht_insert(CoreLibrary, "and", handle_and);
  // Arithmetic.
  ht_insert(CoreLibrary, "+", handle_plus);
  ht_insert(CoreLibrary, "-", handle_minus);
  ht_insert(CoreLibrary, "*", handle_mult);
  ht_insert(CoreLibrary, "/", handle_divide);
  ht_insert(CoreLibrary, "div", handle_div);
  ht_insert(CoreLibrary, "rem", handle_remainder);
  // Conditions.
  ht_insert(CoreLibrary, "if", handle_if);
  // Define.
  ht_insert(CoreLibrary, "define", handle_define);
  // Loops?
  // I/O?
  // Random?
  // Lists.
  ht_insert(CoreLibrary, "car", handle_car);
  ht_insert(CoreLibrary, "cdr", handle_cdr);
  ht_insert(CoreLibrary, "cons", handle_cons);
  ht_insert(CoreLibrary, "list", handle_list);
  // Type predicates.
  ht_insert(CoreLibrary, "int?", handle_is_int);
  ht_insert(CoreLibrary, "float?", handle_is_float);
  ht_insert(CoreLibrary, "nil?", handle_is_nil);
}


void *find_core_function(char *mention) {
  if (!mention || 
      !CoreLibrary)
    return NULL;
  void *result = ht_lookup(CoreLibrary, mention);
  return result;
}
