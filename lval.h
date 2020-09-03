#ifndef LVAL_H
#define LVAL_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lbuiltin.h"
#include "lenv.h"
#include "mpc.h"

// Possible lisp types
enum lval_type {
    LVAL_ERR, // Errors
    LVAL_NUM, // Numbers
    LVAL_SYM, // Symbols (variable names, function names, etc.)
    LVAL_STR,
    LVAL_FUN, // Functions
    LVAL_SEXPR, // Symbolic expresisons
    LVAL_QEXPR // Quoted expressions
};

// Returns string representation of lisp type.
char* ltype_name(enum lval_type type);

// A "Lisp Value" represents a single number, error, function, or sexpr, etc.
struct lval {

    enum lval_type type;

    // Basic
    double num;
    char* err;
    char* sym;
    char* str;

    // Function
    lbuiltin builtin;
    lenv* env;
    lval* formals;
    lval* body;

    // Variable array of lvals and corresponding count
    // For expressions
    int count;
    struct lval** cell;

};

// Construct a pointer to a new Number lval
lval* lval_num(double x);

// Construct a pointer to a new Error lval
lval* lval_err(char* fmt, ...);

// Construct a pointer to a new Symbol lval
lval* lval_sym(char* s);

// Construct a pointer to a new String lval
lval* lval_str(char* s);

// Construct a pointer to a new built-in Function lval
lval* lval_fun(lbuiltin func);

// Construct a pointer to a new user-defined Function lval
lval* lval_lambda(lval* formals, lval* body);

// Construct a pointer to a new emtpy Sexpr lval
lval* lval_sexpr(void);

// Construct a pointer to a new empty Qexpr lval
lval* lval_qexpr(void);

// Copy an lval (useful when putting things in/out of the environment)
lval* lval_copy(lval* v);

// Delete a Lisp Value.
void lval_del(lval* v);

// Add an element to an S-Expression or Q-Expression
lval* lval_add(lval* v, lval* x);

// Convert an AST number to an lval number and check for error.
lval* lval_read_num(mpc_ast_t* t);

// Convert escaped string to unescaped form.
lval* lval_read_str(mpc_ast_t* t);

// Convert AST to S-Expression.
lval* lval_read(mpc_ast_t* t);

// Print an S-Expression type lval.
void lval_expr_print(lenv* e, lval* v, char open, char close);

// Print a Function type lval.
void lval_func_print(lenv* e, lval* v);

// Print a String type lval.
void lval_print_str(lval* v);

// Print an lval.
void lval_print(lenv* e, lval* v);

// Print an lval followed by a newline.
void lval_println(lenv* e, lval* v);

// Extract a single element from an S-Expression at index i and shift the rest of the list backwards
lval* lval_pop(lval* v, int i);

// Take element i from the list and delete the rest.
lval* lval_take(lval* v, int i);

// Join two Q-Expressions.
lval* lval_join(lval* x, lval* y);

// Evaluate an Expression.
lval* lval_eval(lenv* e, lval* v);

// Evaluate an S-Expression.
lval* lval_eval_sexpr(lenv* e, lval* v);

// Calls a built-in or user-defined function.
lval* lval_call(lenv* e, lval* f, lval* a);

// Checks if two lvals are equal
int lval_eq(lval* x, lval* y);

#endif