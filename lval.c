#include "lval.h"

// Returns string representation of type
char* ltype_name(enum lval_type type) {

    switch(type) {
        case LVAL_FUN:
            return "Function";
        case LVAL_NUM:
            return "Number";
        case LVAL_ERR:
            return "Error";
        case LVAL_SYM:
            return "Symbol";
        case LVAL_STR:
            return "String";
        case LVAL_SEXPR:
            return "S-Expression";
        case LVAL_QEXPR:
            return "Q-Expression";
        default:
            return "Unknown";
    }

}

// Construct a pointer to a new Number lval
lval* lval_num(double x) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;

    return v;

}

// Construct a pointer to a new Error lval
lval* lval_err(char* fmt, ...) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    // Create a va list and initialize it
    va_list va;
    va_start(va, fmt);

    // Allocate 512 bytes of space
    v->err = malloc(512);

    // printf the error string with a maximum of 511 characters
    vsnprintf(v->err, 511, fmt, va);

    // Reallocate to number of bytes actually used
    v->err = realloc(v->err, strlen(v->err) + 1);

    // Cleanup our va list
    va_end(va);
    
    return v;

}

// Construct a pointer to a new Symbol lval
lval* lval_sym(char* s) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);

    return v;

}

// Construct a pointer to a new String lval
lval* lval_str(char* s) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);

    return v;

}

// Construct a pointer to a new built-in Function lval
lval* lval_fun(lbuiltin func) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;

    return v;

}

// Construct a pointer to a new user-defined Function lval
lval* lval_lambda(lval* formals, lval* body) {
    
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;

    // Set Builtin to Null
    v->builtin = NULL;

    // Build new environment
    v->env = lenv_new();

    // Set formals and body
    v->formals = formals;
    v->body = body;

    return v;

}

// Construct a pointer to a new emtpy S-Expression lval
lval* lval_sexpr(void) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;

    return v;

}

// Construct a pointer to a new empty Q-Expression lval
lval* lval_qexpr(void) {

    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;

    return v;

}

// Copy an lval (useful when putting things in/out of the environment)
lval* lval_copy(lval* v) {
    
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    
    switch(v->type) {

        case LVAL_FUN:
            // Builtin function (copy function pointer)
            if(v->builtin) {
                x->builtin = v->builtin;
            
            // User-defined function (copy its lenv, parameters, and body)
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        
        case LVAL_NUM:
            x->num = v->num;
            break;

        // Copy Strings using malloc and strcpy
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;

        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym);
            break;

        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str);
            break;

        // Copy Lists by copying each sub-expression
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for(int i = 0; i < x->count; ++i) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;

        default:
            break;
    }

    return x;

}

// Delete a Lisp Value.
void lval_del(lval* v) {

    switch(v->type) {

        // Delete internals if user defined function
        case LVAL_FUN:
            if(!v->builtin) {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->body);
            }
            break;
        
        // For string based types, free the allocated string.
        case LVAL_ERR:
            free(v->err);
            break;

        case LVAL_SYM:
            free(v->sym);
            break;

        case LVAL_STR:
            free(v->str);
            break;

        // For Sexpr and Qexpr, recursively delete all the elements inside
        case LVAL_QEXPR:
        case LVAL_SEXPR:

            for(int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }

            // Free memory allocated to contain the pointers
            free(v->cell);
            break;

        // Do nothing special for Number types.
        case LVAL_NUM:
        default:
            break;
    }

    // Free the memory allocated for the "lval" structure itself
    free(v);

}

// Add an element to an S-Expression or Q-Expression.
lval* lval_add(lval* v, lval* x) {

    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;

    return v;

}

// Convert an AST number to an lval number and check for error.
lval* lval_read_num(mpc_ast_t* t) {

    errno = 0;
    double x = strtod(t->contents, NULL);

    return (errno != ERANGE)? lval_num(x) : lval_err("invalid number");

}

// Convert AST to S-Expression.
lval* lval_read(mpc_ast_t* t) {

    // If Symbol or Number, return conversion to that type
    if(strstr(t->tag, "number")) {
        return lval_read_num(t);

    } else if(strstr(t->tag, "symbol")) {
        return lval_sym(t->contents);
    }

    // If root (>) or sexpr then create empty list
    lval* x = NULL;
    if(strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
        x = lval_sexpr();
    }

    if(strstr(t->tag, "qexpr")) {
        x = lval_qexpr();
    }

    // Fill this list with any valid expression contained within
    for(int i = 0; i < t->children_num; ++i) {
        if(strcmp(t->children[i]->contents, "(") == 0 ||
           strcmp(t->children[i]->contents, ")") == 0 ||
           strcmp(t->children[i]->contents, "{") == 0 ||
           strcmp(t->children[i]->contents, "}") == 0 ||
           strcmp(t->children[i]->tag, "regex") == 0) {
            continue;
        }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;

}

// Print an S-Expression type lval.
void lval_expr_print(lenv* e, lval* v, char open, char close) {
    
    putchar(open);

    for(int i = 0; i < v->count; ++i) {

        // Print value contained within
        lval_print(e, v->cell[i]);

        // Don't print trailing space if last element
        if(i != (v->count - 1)) {
            putchar(' ');
        }
    }

    putchar(close);

}

// Print a Function type lval.
void lval_func_print(lenv* e, lval* v) {
   
    // Print out function params and body if user-defined
    if(!v->builtin) {
        printf("(λ ");
        lval_print(e, v->formals);
        putchar(' ');
        lval_print(e, v->body);
        putchar(')');
        return;
    }

    // Iterate through all builtin functions
    for(int i = 0; i < e->count; ++i) {

        // If function pointers are equal, print corresponding name.
        if(e->vals[i]->builtin == v->builtin) {
            printf("<builtin: %s>", e->syms[i]);
            return;
        }
    }

    // Function not found in environment.
    printf("<unknown function>");

}

// Print a String type lval.
void lval_print_str(lval* v) {

    // Make a copy of the string.
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);

    // Pass it through the escape function.
    escaped = mpcf_escape(escaped);

    // Print it between " characters.
    printf("\"%s\"", escaped);

    // Free the copied string.
    free(escaped);

}

// Print an lval.
void lval_print(lenv* e, lval* v) {

    switch(v->type) {
        case LVAL_NUM:
            printf("%g", v->num);
            break;
        
        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;
        
        case LVAL_SYM:
            printf("%s", v->sym);
            break;

        case LVAL_STR:
            lval_print_str(v);
            break;

        case LVAL_FUN:
            lval_func_print(e, v);
            break;
        
        case LVAL_SEXPR:
            lval_expr_print(e, v, '(', ')');
            break;
        
        case LVAL_QEXPR:
            lval_expr_print(e, v, '{', '}');
            break;
        
        default:
            break;
    }

}

// Print an lval followed by a newline.
void lval_println(lenv* e, lval* v) {

    lval_print(e, v);
    putchar('\n');

}

// Extract a single element from an S-Expression at index i and shift the rest of the list backwards
lval* lval_pop(lval* v, int i) {
    
    // Find the item at index i
    lval* x = v->cell[i];

    // Shift memory after the item at i is over the top
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval*) * (v->count - i - 1));

    // Decrease the count of items in the list
    --(v->count);

    // Reallocate the memory used
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);

    return x;

}

// Take element i from the list and delete the rest.
lval* lval_take(lval* v, int i) {

    lval* x = lval_pop(v, i);

    lval_del(v);
    return x;
    
}

// Join two Q-Expressions.
lval* lval_join(lval* x, lval* y) {

    // For each cell in y add x to it
    while(y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    // Delete the empty y and return x
    lval_del(y);
    return x;
    
}

// Evaluate an Expression.
lval* lval_eval(lenv* e, lval* v) {
   
    // Evaluate symbols
    if(v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v); // Environment already returns copy of v
        return x;
    }

    // Evaluate s-expressions.
    else if(v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);

    // All other lval types remain the same.
    } else {
        return v;
    }

}


// Evaluate an S-Expression.
lval* lval_eval_sexpr(lenv* e, lval* v) {

    // Evaluate children
    for(int i = 0; i < v->count; ++i) {
        v->cell[i] = lval_eval(e, v->cell[i]);

        // Error checking
        if(v->cell[i]->type == LVAL_ERR) {
            return lval_take(v, i);
        }
    }

    // Empty expression
    if(v->count == 0) {
        return v;
    }

    // Single expression
    if(v->count == 1) {
        return lval_take(v, 0);
    }

    // Ensure first element is a function after evaluation
    lval* first = lval_pop(v, 0);
    if(first->type != LVAL_FUN) {
        lval* err = lval_err("S-Expression starts with incorrect type. Got %s, Expected %s",
                              ltype_name(first->type), ltype_name(LVAL_FUN));
        lval_del(v);
        lval_del(first);
        return err;
    }

    // If so, call function to get result.
    lval* result = lval_call(e, first, v);
    
    lval_del(first);
    return result;

}

// Calls a built-in or user-defined function.
lval* lval_call(lenv* e, lval* f, lval* a) {

    // If built-in function then simply call that
    if(f->builtin) {
        return f->builtin(e, a);
    }

    // Record argument counts
    int given_args = a->count;
    int total_args = f->formals->count;

    // While arguments still remain to be processed
    while(a->count) {

        // If we've ran out of formal arguments to bind
        if(f->formals->count == 0) {
            lval_del(a);
            return lval_err("Function passed too many arguments. Got %i, Expected %i.", given_args, total_args);
        }

        // Pop the first symbol from the formals
        lval* sym = lval_pop(f->formals, 0);

        // Special case to deal with '&'
        if(strcmp(sym->sym, "&") == 0) {

            // Ensure '&' is followed by another symbol
            if(f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
            }

            // Next formal should be bound to remaining arguments
            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;

        }

        // Pop the next argument from the list
        lval* val = lval_pop(a, 0);

        // Bind a copy into the function's environment
        lenv_put(f->env, sym, val);

        // Delete symbol and value.
        lval_del(sym);
        lval_del(val);

    }

    // If '&' remains in formal list, then bind to empty list.
    if(f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {

        // Check to ensure that & is not passed invalidly
        if(f->formals->count != 2) {
            return lval_err("Function format invalid. Symbol '&' not followed by single symbol");
        }

        // Pop and delete '&' symbol.
        lval_del(lval_pop(f->formals, 0));

        // Pop next symbol and create empty list
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        // Bind to environment and delete.
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);

    }

    // Argument list is now bound so can be cleaned up.
    lval_del(a);

    // If all formals have been bound, then evaluate.
    if(f->formals->count == 0) {

        // Set the parent environment to evaluation environment.
        f->env->parent = e;

        // Evaluate and return
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));

    // Otherwise, return partially evaluated function.
    } else {
        return lval_copy(f);
    }

}