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

// Possible lval types
enum lval_type {
    LVAL_ERR, // Errors
    LVAL_NUM, // Numbers
    LVAL_SYM, // Operators i.e. +, -
    LVAL_SEXPR, // S-expresisons
    LVAL_QEXPR // Quoted expressions
};

// "Lisp Value" to represent a number/error/symbol/sexpr
typedef struct lval {
    enum lval_type type;
    double num;
    // Errors and symbols are strings
    char* err;
    char* sym;
    // Variable array of lvals and corresponding count
    int count;
    struct lval** cell;
} lval;

// Construct a pointer to a new Number lval
lval* lval_num(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct a pointer to a new Error lval
lval* lval_err(char* message) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(message) + 1);
    strcpy(v->err, message);
    return v;
}

// Construct a pointer to a new Symbol lval
lval* lval_sym(char* s ) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
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

void lval_del(lval* v) {

    switch(v->type) {
        // Do nothing special for number type
        case LVAL_NUM:
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
void lval_print(lval* v);

// Prints an s-expr type lval
void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for(int i = 0; i < v->count; ++i) {

        // Print value contained within
        lval_print(v->cell[i]);

        // Don't print trailing space if last element
        if(i != (v->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

// Prints an lval
void lval_print(lval* v) {
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
        
        case LVAL_SEXPR:
            lval_expr_print(v, '(', ')');
            break;
        
        case LVAL_QEXPR:
            lval_expr_print(v, '{', '}');
            break;
        
        default:
            break;
    }
}

// Prints an lval followed by a newline
void lval_println(lval* v) {
    lval_print(v);
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
lval* builtin_op(lval* a, char* op) {

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

#define lval_assert(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

// Takes a q-expr and returns a q-expr with only the first element
lval* builtin_head(lval* a) {

    // Error checking
    lval_assert(a, a->count == 1, "Function 'head' passed too many arguments!");
    lval_assert(a, a->cell[0]->type == LVAL_QEXPR, "Function 'head' passed incorrect types!");
    lval_assert(a, a->cell[0]->count != 0, "Function 'head' passed {}!");
    
    // Otherwise take first argument
    lval* v = lval_take(a, 0);

    // Delete all elements that are not head and return
    while(v->count > 1) {
        lval_del(lval_pop(v, 1));
    }

    return v;
}

// Takes a q-expr and returns a q-expr with the first element removed
lval* builtin_tail(lval* a) {

    // Error checking
    lval_assert(a, a->count == 1, "Function 'tail' passed too many arguments!");
    lval_assert(a, a->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect types!");
    lval_assert(a, a->cell[0]->count != 0, "Function 'tail' passed {}!");
    
    // Otherwise take first argument.
    lval* v = lval_take(a, 0);

    // Delete first element and return.
    lval_del(lval_pop(v, 0));

    return v;
}

// Converts s-expr into q-expr
lval* builtin_list(lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

// Forward declare
lval* lval_eval(lval* v);

// Converts a q-expr into an s-expr and evaluates it
lval* builtin_eval(lval* a) {
    // Error checking
    lval_assert(a, a->count == 1, "Function 'eval' passed too many arguments!");
    lval_assert(a, a->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect types!");
    
    lval* v = lval_take(a, 0);
    v->type = LVAL_SEXPR;
    
    return lval_eval(v);
}

// Joins 2+ q-expr
lval* builtin_join(lval* a) {
    for(int i = 0; i < a->count; ++i) {
        lval_assert(a, a->cell[0]->type == LVAL_QEXPR, "Function 'join' passed incorrect type!");
    }

    lval* v = lval_pop(a, 0);

    while(a->count) {
        v = lval_join(v, lval_pop(a, 0));
    }

    lval_del(a);
    return v;
}


// Takes a value and a Q-Expression and appends it to the front.
lval* builtin_cons(lval* a) {
    // Error checking
    lval_assert(a, a->count == 2, "Function 'cons' did not pass 2 arguments!");
    lval_assert(a, a->cell[0]->type == LVAL_NUM, "Function 'cons' did not pass number as first argument!");
    lval_assert(a, a->cell[1]->type == LVAL_QEXPR, "Function 'cons' did not pass q-expr as second argument!");

    lval* x = lval_qexpr();
    lval_add(x, lval_pop(a, 0));    
    x = lval_join(x, lval_pop(a, 0));

    lval_del(a);
    return x;
}


// Returns the number of elements in a Q-Expression
lval* builtin_len(lval* a) {
    // Error checking
    lval_assert(a, a->count == 1, "Function 'len' passed too many arguments!");
    lval_assert(a, a->cell[0]->type == LVAL_QEXPR, "Function 'len' passed incorrect type!");

    int numElements = a->cell[0]->count;

    return lval_num(numElements);
}


// Returns all of a Q-Expression except the final element.
lval* builtin_init(lval* a) {
    // Error checking 
    lval_assert(a, a->count == 1, "Function 'init' passed too many arguments!");
    lval_assert(a, a->cell[0]->type == LVAL_QEXPR, "Function 'init' passed incorrect type!");
    lval_assert(a, a->cell[0]->count != 0, "Function 'init' passed {}!");

    // Take the first argument.
    lval* v = lval_take(a, 0);

    // Delete final element.
    lval_del(lval_pop(v, v->count - 1));

    return v;
}


// Call appropriate builtin function
lval* builtin(lval* a, char* func) {

    if(strcmp("list", func) == 0) {
        return builtin_list(a);

    } else if(strcmp("head", func) == 0) {
        return builtin_head(a);

    } else if(strcmp("tail", func) == 0) {
        return builtin_tail(a);

    } else if(strcmp("join", func) == 0) {
        return builtin_join(a);
    
    } else if(strcmp("eval", func) == 0) {
        return builtin_eval(a);

    } else if(strcmp("cons", func) == 0) {
        return builtin_cons(a);

    } else if(strcmp("len", func) == 0) {
        return builtin_len(a);

    } else if(strcmp("init", func) == 0) {
        return builtin_init(a);
    
    } else if(strstr("+ - * / % ^ add sub mul div min max", func)) {
        return builtin_op(a, func);
    
    } else {
        lval_del(a);
        return lval_err("Unknown function!");
    }
}

// Forward declare
lval* lval_eval_sexpr(lval* v);

lval* lval_eval(lval* v) {
    // Evaluate s-expressions.
    if(v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(v);

    // All other lval types remain the same.
    } else {
        return v;
    }
}

// Evaluate s-expression
lval* lval_eval_sexpr(lval* v) {
    // Evaluate children
    for(int i = 0; i < v->count; ++i) {
        v->cell[i] = lval_eval(v->cell[i]);

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

    // Ensure first element is symbol
    lval* first = lval_pop(v, 0);
    if(first->type != LVAL_SYM) {
        lval_del(first);
        lval_del(v);
        return lval_err("S-expression does not start with symbol!");
    }

    // Call builtin with operator
    lval* result = builtin(v, first->sym);
    lval_del(first);
    return result;
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
              symbol : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"min\" | \"max\" | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | \"cons\" | \"len\" | \"init\"; \
              sexpr  : '(' <expr>* ')'; \
              qexpr  : '{' <expr>* '}'; \
              expr   : <number> | <symbol> | <sexpr> | <qexpr>; \
              blisp  : /^/ <expr>* /$/ ; \
            ", Number, Symbol, Sexpr, Qexpr, Expr, Blisp);

    // Print version and exit info
    puts("Brandon's Lisp Version 0.0.1");
    puts("hello there 😶");
    puts("Press Ctrl+c to Exit\n");

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
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
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

    return 0;
}
