#include "blisp.h"

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
