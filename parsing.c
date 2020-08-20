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
enum { LVAL_NUM, LVAL_ERR };

// Possible error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// "Lisp Value" to represent a number or error
typedef struct {
    int type;
    double num;
    int err;
} lval;

// Creates new lval number type
lval lval_num(double x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// Creates new lval error type
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

// Prints an lval
void print_lval(lval v) {
    switch(v.type) {
        case LVAL_NUM:
            printf("%g", v.num);
            break;
        case LVAL_ERR:
            switch(v.err) {
                case LERR_DIV_ZERO:
                    printf("Error: Division by zero!");
                    break;
                case LERR_BAD_OP:
                    printf("Error: Invalid operator!");
                    break;
                case LERR_BAD_NUM:
                    printf("Error: Invalid number!");
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

// Prints an lval followed by a newline
void println_lval(lval v) {
    print_lval(v);
    putchar('\n');
}


// Use operator string to see which operation to perform
lval eval_op(lval x, char* op, lval y) {

    // If either x or y is an error then return it
    if(x.type == LVAL_ERR) {
        return x;
    } else if(y.type == LVAL_ERR) {
        return y;
    }

    // Otherwise, do the math on the numbers
    if(strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
        return lval_num(x.num + y.num);
    
    } else if(strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
        return lval_num(x.num - y.num);
    
    } else if(strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
        return lval_num(x.num * y.num);
    
    } else if(strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
        return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    
    } else if(strcmp(op, "%") == 0) {
        return lval_num((int)x.num % (int)y.num);

    } else if(strcmp(op, "^") == 0) {
        return lval_num(pow(x.num, y.num));

    } else if(strcmp(op, "min") == 0) {
        return (x.num < y.num) ? lval_num(x.num) : lval_num(y.num);

    } else if(strcmp(op, "max") == 0) {
        return (x.num > y.num) ? lval_num(x.num) : lval_num(y.num);

    } else {
        return lval_err(LERR_BAD_OP);
    }
}


lval eval(mpc_ast_t* t) {
    
    // Check if tagged as number
    if(strstr(t->tag, "number")) {
        // Check if error in converting string to number
        errno = 0;
        double x = strtod(t->contents, NULL);
        return (errno == ERANGE)? lval_err(LERR_BAD_NUM) : lval_num(x);
    }

    // Operator is always second child
    char* op = t->children[1]->contents;

    // Store the third child
    lval x = eval(t->children[2]);

    // Minus operator only one argument
    if(t->children_num <= 4 && strcmp(op, "-") == 0) {
        return lval_num(-x.num);
    }

    // Iterate through remaining children and combine
    size_t i = 3;
    while(strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        ++i;
    }

    return x;
}


int main(int argc, char** argv) {

    // Create some parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Blisp = mpc_new("blisp");

    // Define the parsers with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
            " number   : /-?[0-9]+([.][0-9]+)?/ ; \
              operator : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"min\" | \"max\"; \
              expr     : <number> | '(' <operator> <expr>+ ')' ; \
              blisp    : /^/ <operator> <expr>+ /$/ ; \
            ", Number, Operator, Expr, Blisp);

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
            lval result = eval(r.output);
            println_lval(result);
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
    mpc_cleanup(4, Number, Operator, Expr, Blisp);

    return 0;
}
