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
    LVAL_SEXPR // S-expresisons
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

        // For Sexpr, recursively delete all the elements inside
        case LVAL_SEXPR:
            for(int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
        default:
            break;
    }

    // Free the memory allocated for the "lval" structure itself
    free(v);
}

// Adds an element to an S-Expression
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
    if(strcmp(t-> tag, ">") == 0 || strstr(t->tag, "sexpr")) {
        x = lval_sexpr();
    }

    // Fill this list with any valid expression contained within
    for(int i = 0; i < t->children_num; ++i) {
        if(strcmp(t->children[i]->contents, "(") == 0 ||
           strcmp(t->children[i]->contents, ")") == 0 ||
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
        default:
            break;
    }
}

// Prints an lval followed by a newline
void lval_println(lval* v) {
    lval_print(v);
    putchar('\n');
}

// Use operator string to see which operation to perform
//lval eval_op(lval x, char* op, lval y) {
//
//    // If either x or y is an error then return it
//    if(x.type == LVAL_ERR) {
//        return x;
//    } else if(y.type == LVAL_ERR) {
//        return y;
//    }
//
//    // Otherwise, do the math on the numbers
//    if(strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
//        return lval_num(x.num + y.num);
//    
//    } else if(strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
//        return lval_num(x.num - y.num);
//    
//    } else if(strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
//        return lval_num(x.num * y.num);
//    
//    } else if(strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
//        return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
//    
//    } else if(strcmp(op, "%") == 0) {
//        return lval_num((int)x.num % (int)y.num);
//
//    } else if(strcmp(op, "^") == 0) {
//        return lval_num(pow(x.num, y.num));
//
//    } else if(strcmp(op, "min") == 0) {
//        return (x.num < y.num) ? lval_num(x.num) : lval_num(y.num);
//
//    } else if(strcmp(op, "max") == 0) {
//        return (x.num > y.num) ? lval_num(x.num) : lval_num(y.num);
//
//    } else {
//        return lval_err(LERR_BAD_OP);
//    }
//}


//lval eval(mpc_ast_t* t) {
//    
//    // Check if tagged as number
//    if(strstr(t->tag, "number")) {
//        // Check if error in converting string to number
//        errno = 0;
//        double x = strtod(t->contents, NULL);
//        return (errno == ERANGE)? lval_err(LERR_BAD_NUM) : lval_num(x);
//    }
//
//    // Operator is always second child
//    char* op = t->children[1]->contents;
//
//    // Store the third child
//    lval x = eval(t->children[2]);
//
//    // Minus operator only one argument
//    if(t->children_num <= 4 && strcmp(op, "-") == 0) {
//        return lval_num(-x.num);
//    }
//
//    // Iterate through remaining children and combine
//    size_t i = 3;
//    while(strstr(t->children[i]->tag, "expr")) {
//        x = eval_op(x, op, eval(t->children[i]));
//        ++i;
//    }
//
//    return x;
//}


int main(int argc, char** argv) {

    // Create some parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Blisp = mpc_new("blisp");

    // Define the parsers with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
            " number : /-?[0-9]+([.][0-9]+)?/ ; \
              symbol : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"min\" | \"max\"; \
              sexpr  : '(' <expr>* ')'; \
              expr   : <number> | <symbol> | <sexpr> ; \
              blisp  : /^/ <expr>* /$/ ; \
            ", Number, Symbol, Sexpr, Expr, Blisp);

    // Print version and exit info
    puts("Brandon's Lisp Version 0.0.1");
    puts("\" hello there 😶\" ");
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
            // lval result = eval(r.output);
            lval* x = lval_read(r.output);
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
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Blisp);

    return 0;
}
