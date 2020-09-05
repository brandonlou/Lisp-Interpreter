#ifndef BLISP_H
#define BLISP_H

#include "lenv.h"
#include "lval.h"
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


// Parser declarations.
mpc_parser_t* Number;
mpc_parser_t* Boolean;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Blisp;

#endif
