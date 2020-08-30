#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

// If compiling on Windows then include these functions
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

// Fake add_history function
void add_history(char* unused) {}

// Otherwise if compiling on Max/Unix, include the editline headers
#else
#include <editline/readline.h>
#endif

// Forward declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// Possible lisp types
enum lval_type {
    LVAL_ERR, // Errors
    LVAL_NUM, // Numbers
    LVAL_SYM, // Operators i.e. +, -
    LVAL_FUN, // Functions
    LVAL_SEXPR, // S-expresisons
    LVAL_QEXPR // Quoted expressions
};


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
        case LVAL_SEXPR:
            return "S-Expression";
        case LVAL_QEXPR:
            return "Q-Expression";
        default:
            return "Unknown";
    }
}


// Declare new function pointer type named lbuiltin that is called
// with a lenv* and lval*, returning a lval*
typedef lval*(*lbuiltin) (lenv*, lval*);

// "Lisp Value" to represent a number/error/symbol/sexpr
struct lval {
    enum lval_type type;

    double num;
    // Errors and symbols are strings
    char* err;
    char* sym;
    lbuiltin fun;

    // Variable array of lvals and corresponding count
    int count;
    struct lval** cell;
};


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


// Construct a pointer to a new Function lval
lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;
    return v;
}


// Construct a pointer to a new emtpy Sexpr lval
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


// Construct a pointer to a new empty Qexpr lval
lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}


// Copies an lval (useful when putting things in/out of the environment)
lval* lval_copy(lval* v) {
    
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    
    switch(v->type) {

        // Copy Functions and Numbers directly
        case LVAL_FUN:
            x->fun = v->fun;
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


void lval_del(lval* v) {

    switch(v->type) {
        // Do nothing special for number and function type
        case LVAL_NUM:
        case LVAL_FUN:
            break;
        
        // For Errors or Symbols free the allocated string
        case LVAL_ERR:
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
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

        default:
            break;
    }

    // Free the memory allocated for the "lval" structure itself
    free(v);
}


// Adds an element to an S-Expression or Q-Expression
lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}


// Converts an AST number to lval number and checks for error.
lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    double x = strtod(t->contents, NULL);
    return (errno != ERANGE)? lval_num(x) : lval_err("invalid number");
}


// Converts AST to S-Expression
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


// Forward declaration because lval_expr_print and lval_print depend on each other
void lval_print(lenv* e, lval* v);

// Prints an s-expr type lval
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


void lval_func_print(lenv* e, lval* v);

// Prints an lval
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


// Prints an lval followed by a newline
void lval_println(lenv* e, lval* v) {
    lval_print(e, v);
    putchar('\n');
}


// Extracts a single element from an s-expr at index i and shifts the rest of the list backwards
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


// Take element i from the list and delete the rest
lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}


// Joins two q-exprs
lval* lval_join(lval* x, lval* y) {

    // For each cell in y add x to it
    while(y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    // Delete the empty y and return x
    lval_del(y);
    return x;
}


// Perform operation op on all lvals in the list.
lval* builtin_op(lenv* e, lval* a, char* op) {

    // Ensure all arguments are numbers
    for(int i = 0; i < a->count; ++i) {
        if(a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    // Pop the first element
    lval* x = lval_pop(a, 0);

    // If no arguments and subtraction, then perform unary negation
    if((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    // While there are still elements remaining
    while(a->count > 0) {

        // Pop the next element
        lval* y = lval_pop(a, 0);
    
        if(strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
            x->num = x->num + y->num;

        } else if(strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
            x->num = x->num - y->num;

        } else if(strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
            x->num = x->num * y->num;

        } else if(strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
            if(y->num == 0) {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by zero!");
                break;
            }
            x->num = x->num / y->num;

        } else if(strcmp(op, "%") == 0) {
            x->num = (int)(x->num) % (int)(y->num);

        } else if(strcmp(op, "^") == 0) {
            x->num = pow(x->num, y->num);
    
        } else if(strcmp(op, "min") == 0) {
            x->num = (x->num < y->num) ? x->num : y->num;
    
        } else if(strcmp(op, "max") == 0) {
            x->num = (x->num > y->num) ? x->num : y->num;
    
        } else {
            lval_del(x);
            lval_del(y);
            x = lval_err("Bad operator!");
            break;
        }

        lval_del(y);
    }

    lval_del(a);
    return x;
}


lval* builtin_add(lenv* e, lval* a) {
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
    return builtin_op(e, a, "/");
}

lval* builtin_mod(lenv* e, lval* a) {
    return builtin_op(e, a, "%");
}

lval* builtin_pow(lenv* e, lval* a) {
    return builtin_op(e, a, "^");
}

lval* builtin_min(lenv* e, lval* a) {
    return builtin_op(e, a, "min");
}

lval* builtin_max(lenv* e, lval* a) {
    return builtin_op(e, a, "max");
}

#define lval_assert(args, cond, fmt, ...) \
    if (!(cond)) { \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args); \
        return err; \
    }


// Report argument count errors.
#define lval_check_argcount(name, args, expected_count) \
    lval_assert(args, args->count == expected_count, \
            "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
            name, args->count, expected_count);


// Report type errors.
#define lval_check_type(name, args, argnum, expected_type) \
    lval_assert(args, args->cell[argnum]->type == expected_type, \
            "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
            name, argnum, ltype_name(args->cell[argnum]->type), ltype_name(expected_type));


// Report empty list errors
#define lval_check_emptylist(name, args, argnum) \
    lval_assert(args, a->cell[argnum]->count != 0, \
            "Function '%s' passed {} for argument %i.", \
            name, argnum);


// Takes a q-expr and returns a q-expr with only the first element
lval* builtin_head(lenv* e, lval* a) {

    // Error checking
    lval_check_argcount("head", a, 1);
    lval_check_type("head", a, 0, LVAL_QEXPR);
    lval_check_emptylist("head", a, 0);

    // Otherwise take first argument
    lval* v = lval_take(a, 0);

    // Delete all elements that are not head and return
    while(v->count > 1) {
        lval_del(lval_pop(v, 1));
    }

    return v;
}


// Takes a q-expr and returns a q-expr with the first element removed
lval* builtin_tail(lenv* e, lval* a) {

    // Error checking
    lval_check_argcount("tail", a, 1);
    lval_check_type("tail", a, 0, LVAL_QEXPR);
    lval_check_emptylist("tail", a, 0); 
    
    // Otherwise take first argument.
    lval* v = lval_take(a, 0);

    // Delete first element and return.
    lval_del(lval_pop(v, 0));

    return v;
}


// Converts s-expr into q-expr
lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}


// Forward declare
lval* lval_eval(lenv* e, lval* v);

// Converts a q-expr into an s-expr and evaluates it
lval* builtin_eval(lenv* e, lval* a) {
    
    // Error checking
    lval_check_argcount("eval", a, 1);
    lval_check_type("eval", a, 0, LVAL_QEXPR);

    lval* v = lval_take(a, 0);
    v->type = LVAL_SEXPR;
    
    return lval_eval(e, v);

}


// Joins 2+ q-expr
lval* builtin_join(lenv* e, lval* a) {
    
    for(int i = 0; i < a->count; ++i) {
        lval_check_type("join", a, i, LVAL_QEXPR);
    }

    lval* v = lval_pop(a, 0);

    while(a->count) {
        v = lval_join(v, lval_pop(a, 0));
    }

    lval_del(a);
    return v;
}


// Takes a value and a Q-Expression and appends it to the front.
lval* builtin_cons(lenv* e, lval* a) {
    
    // Error checking
    lval_check_argcount("cons", a, 2);
    lval_check_type("cons", a, 0, LVAL_NUM);
    lval_check_type("cons", a, 1, LVAL_QEXPR);

    lval* x = lval_qexpr();
    lval_add(x, lval_pop(a, 0));    
    x = lval_join(x, lval_pop(a, 0));

    lval_del(a);
    return x;

}


// Returns the number of elements in a Q-Expression
lval* builtin_len(lenv* e, lval* a) {
    
    // Error checking
    lval_check_argcount("len", a, 1);
    lval_check_type("len", a, 0, LVAL_QEXPR);

    int num_elements = a->cell[0]->count;

    return lval_num(num_elements);
}


// Returns all of a Q-Expression except the final element.
lval* builtin_init(lenv* e, lval* a) {
    
    // Error checking 
    lval_check_argcount("init", a, 1);
    lval_check_type("init", a, 0, LVAL_QEXPR);
    lval_check_emptylist("init", a, 0);

    // Take the first argument.
    lval* v = lval_take(a, 0);

    // Delete final element.
    lval_del(lval_pop(v, v->count - 1));

    return v;

}


// Exit the interpreter.
lval* builtin_exit(lenv* e, lval* a) {
    return lval_sexpr();
}


void lenv_put(lenv* e, lval* k, lval* v);

// Define a new variable. Returns empty list on success.
lval* builtin_def(lenv* e, lval* a) {

    lval_check_type("def", a, 0, LVAL_QEXPR);

    // First argument is symbol list
    lval* syms = a->cell[0];

    // Ensure all elements of first list are symbols
    for(int i = 0; i < syms->count; ++i) {
        lval_assert(a, syms->cell[i]->type == LVAL_SYM, "Function 'def' cannot define non-symbol");
    }

    // Check correct number of symbols and values
    lval_assert(a, syms->count == a->count - 1, "Function 'def' cannot define incorrect number of values to symbols");

    // Assign copies of values to symbols
    for(int i = 0; i < syms->count; ++i) {
        lenv_put(e, syms->cell[i], a->cell[i + 1]);
    }

    lval_del(a);
    return lval_sexpr();

}

// Call appropriate builtin function
lval* builtin(lenv* e, lval* a, char* func) {

    if(strcmp("list", func) == 0) {
        return builtin_list(e, a);

    } else if(strcmp("head", func) == 0) {
        return builtin_head(e, a);

    } else if(strcmp("tail", func) == 0) {
        return builtin_tail(e, a);

    } else if(strcmp("join", func) == 0) {
        return builtin_join(e, a);
    
    } else if(strcmp("eval", func) == 0) {
        return builtin_eval(e, a);

    } else if(strcmp("cons", func) == 0) {
        return builtin_cons(e, a);

    } else if(strcmp("len", func) == 0) {
        return builtin_len(e, a);

    } else if(strcmp("init", func) == 0) {
        return builtin_init(e, a);
    
    } else if(strstr("+ - * / % ^ add sub mul div min max", func)) {
        return builtin_op(e, a, func);
    
    } else {
        lval_del(a);
        return lval_err("Unknown function!");
    }
}


// Forward declare
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lenv_get(lenv* e, lval* v);

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


// Evaluate s-expression
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
        lval_del(v);
        lval_del(first);
        return lval_err("First element is not a function!");
    }

    // If so, call function to get result.
    lval* result = first->fun(e, v);
    lval_del(first);
    return result;
}


// Define our environment structure. Encodes a list of relationships
// between names and values
struct lenv {
    int count;
    char** syms;
    lval** vals;
};


// Prints a function type lval.
void lval_func_print(lenv* e, lval* v) {
   
    // Iterate through all builtin functions
    for(int i = 0; i < e->count; ++i) {
        if(e->vals[i]->fun == v->fun) {
            printf("<function: %s>", e->syms[i]);
            return;
        }
    }

    // Function not found in environment.
    printf("<unknown function>");

}


// Print all named values in an environment up to number specified.
// If -1, print all.
lval* builtin_values(lenv* e, lval* a) {

    lval_check_argcount("values", a, 1);
    lval_check_type("values", a, 0, LVAL_NUM);

    double num = a->cell[0]->num;
    lval* x = lval_qexpr();

    // Print all named values.
    if(num == -1) {
        for(int i = 0; i < e->count; ++i) {
            lval_add(x, lval_sym(e->syms[i]));
        }

    // Print up to specified number of named values.
    } else {
        for(int i = 0; i < e->count && i < num; ++i) {
            lval_add(x, lval_sym(e->syms[i]));
        }
    }

    lval_del(a);
    return x;

}


// Create new pointer to a lenv
lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}


// Deletes an lenv
void lenv_del(lenv* e) {

    // Iterate through all symbols and values and deletes them
    for(int i = 0; i < e->count; ++i) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    // Delete the actual arrays and object itself
    free(e->syms);
    free(e->vals);
    free(e);
}


// Gets a value from the environement
lval* lenv_get(lenv* e, lval* k) {
    
    // Iterate over all items in environment
    for(int i = 0; i < e->count; ++i) {
        // Check if stored string matches symbol string.
        // If it does, return a copy of the value.
        if(strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }

    // If no symbol found return error
    return lval_err("Unbound symbol: '%s'", k->sym);
}


// Puts values into environment
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


// Adds a function with its corresponding name to the environment
void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* key = lval_sym(name);
    lval* value = lval_fun(func);
    lenv_put(e, key, value);
    lval_del(key);
    lval_del(value);
}


// Adds all our language builtin functions
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

    // Variable functions
    lenv_add_builtin(e, "def", builtin_def);

    // Zero argument functions
    lenv_add_builtin(e, "values", builtin_values);
    lenv_add_builtin(e, "exit", builtin_exit);

}


int main(int argc, char** argv) {

    // Create some parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Blisp = mpc_new("blisp");

    // Define the parsers with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
            " number : /-?[0-9]+([.][0-9]+)?/ ; \
              symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^%]+/; \
              sexpr  : '(' <expr>* ')'; \
              qexpr  : '{' <expr>* '}'; \
              expr   : <number> | <symbol> | <sexpr> | <qexpr>; \
              blisp  : /^/ <expr>* /$/ ; \
            ", Number, Symbol, Sexpr, Qexpr, Expr, Blisp);

    // Print version and exit info
    puts("Brandon's Lisp Version 0.0.1");
    puts("hello there 😶");
    puts("Press Ctrl+c to Exit\n");

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    // Infinite prompt
    while(1) {
        
        // Output prompt and get input
        char* input = readline("blisp> ");

        // Add input to history
        add_history(input);

        // Attempt to parse user input
        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Blisp, &r)) {
            
            // On success, evaluate and print the result.
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(e, x);
            lval_del(x);

            mpc_ast_delete(r.output);
        
        } else {
            // Otherwise print error
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // Free retrieved input
        free(input);
    }

    // Undefine and delete our parsers
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Blisp);
    
    // Delete our environment
    lenv_del(e);
    
    return 0;
}

