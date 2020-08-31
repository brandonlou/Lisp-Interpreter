#ifndef LENV_H
#define LENV_H

#include "builtin.h"
#include "lval.h"

// Represents our lisp environment.
struct lenv {

    // Parent environment.
    lenv* parent;

    // Correspond names with values and the number of associations.
    int count;
    char** syms;
    lval** vals;

};

// Create a pointer to a new lenv.
lenv* lenv_new();

// Copy a lisp environment.
lenv* lenv_copy(lenv* e);

// Delete a lisp environment.
void lenv_del(lenv* e);

// Get a value from the environement.
lval* lenv_get(lenv* e, lval* k);

// Put values into local environment.
void lenv_put(lenv* e, lval* k, lval* v);

// Put values into global environment
void lenv_def(lenv* e, lval* k, lval* v);

// Add a built-in function with its corresponding name to the environment.
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);

// Add all our language built-in functions.
void lenv_add_builtins(lenv* e);

#endif