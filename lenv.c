#include "lenv.h"

// Create a pointer to a new lenv
lenv* lenv_new(void) {

    lenv* e = malloc(sizeof(lenv));
    e->parent = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;

    return e;

}

// Copy a lisp environment.
lenv* lenv_copy(lenv* e) {

    lenv* new = malloc(sizeof(lenv));
    new->parent = e->parent;
    new->count = e->count;
    new->syms = malloc(sizeof(char*) * new->count);
    new->vals = malloc(sizeof(lval*) * new->count);

    for(int i = 0; i < e->count; ++i) {
        new->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(new->syms[i], e->syms[i]);
        new->vals[i] = lval_copy(e->vals[i]);
    }

    return new;

}

// Delete a lisp environment.
void lenv_del(lenv* e) {

    // Iterate through all symbols and values and deletes them.
    for(int i = 0; i < e->count; ++i) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    // Delete the actual arrays and object itself.
    free(e->syms);
    free(e->vals);
    free(e);

}

// Get a value from the environement.
lval* lenv_get(lenv* e, lval* k) {
    
    // Iterate over all items in environment
    for(int i = 0; i < e->count; ++i) {
        // Check if stored string matches symbol string.
        // If it does, return a copy of the value.
        if(strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    // If no symbol in current environment check in parent environment
    if(e->parent) {
        return lenv_get(e->parent, k);

    // Otherwise, no symbol found and return error
    } else {
        return lval_err("Unbound symbol: '%s'", k->sym);
    }

}

// Put values into local environment.
void lenv_put(lenv* e, lval* k, lval* v) {

    // Iterate over all itmes in environment to see if
    // variable already exists
    for(int i = 0; i < e->count; ++i) {

        // If variable is found, delete item at that position
        // and replace with variable supplied by user
        if(strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    // If no existing entry found, allocate space for new entry.
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    // Copy contents of lval and symbol string into new location
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);

}

// Put values into global environment
void lenv_def(lenv* e, lval* k, lval* v) {

    // Go up parent environments until no parents
    while(e->parent) {
        e = e->parent;
    }

    // Put value in global environment.
    lenv_put(e, k, v);

}

// Add a built-in function with its corresponding name to the environment.
void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {

    lval* key = lval_sym(name);
    lval* value = lval_fun(func);

    lenv_put(e, key, value);
    lval_del(key);
    lval_del(value);

}

// Add all our language built-in functions.
void lenv_add_builtins(lenv* e) {

    // List functions
    lenv_add_builtin(e, "list", builtin_list); 
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "len",  builtin_len);
    lenv_add_builtin(e, "init", builtin_init);

    // Mathematical functions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "^", builtin_pow);
    lenv_add_builtin(e, "add", builtin_add);
    lenv_add_builtin(e, "sub", builtin_sub);
    lenv_add_builtin(e, "mul", builtin_mul);
    lenv_add_builtin(e, "div", builtin_div);
    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);

    // Comparision functions
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, ">", builtin_greater);
    lenv_add_builtin(e, "<", builtin_less);
    lenv_add_builtin(e, ">=", builtin_greater_or_equal);
    lenv_add_builtin(e, "<=", builtin_less_or_equal);
    lenv_add_builtin(e, "==", builtin_equal);
    lenv_add_builtin(e, "!=", builtin_not_equal);

    // Logical functions
    lenv_add_builtin(e, "||", builtin_or);
    lenv_add_builtin(e, "&&", builtin_and);
    lenv_add_builtin(e, "!", builtin_not);

    // Variable functions
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=", builtin_put);
    lenv_add_builtin(e, "\\", builtin_lambda); // Single backslash

    // Zero argument functions
    lenv_add_builtin(e, "values", builtin_values);
    lenv_add_builtin(e, "exit", builtin_exit);

    // String functions
    lenv_add_builtin(e, "load", builtin_load);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "read", builtin_read);
    lenv_add_builtin(e, "show", builtin_show);

}
