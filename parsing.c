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

int main(int argc, char** argv) {

    // Create some parsers
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Blisp = mpc_new("blisp");

    // Define the parsers with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
            " number   : /-?[0-9]+([.][0-9]+)?/ ; \
              operator : '+' | '-' | '*' | '/' | '%' | \"add\" | \"sub\" | \"mul\" | \"div\" ; \
              expr     : <number> | '(' <operator> <expr>+ ')' ; \
              blisp    : /^/ <operator> <expr>+ /$/ ; \
            ", Number, Operator, Expr, Blisp);

    // Print version and exit info
    puts("Brandon's Lisp Version 0.0.1");
    puts("Brandon says hello.");
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
            // On success print the AST
            mpc_ast_print(r.output);
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
