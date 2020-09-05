#include "blisp.h"

int main(int argc, char** argv) {

    // Create some parsers
    Number = mpc_new("number");
    Boolean = mpc_new("boolean");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Blisp = mpc_new("blisp");

    // Define the parsers with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
            " number : /-?[0-9]+([.][0-9]+)?/ ; \
              boolean : /(true|false)/ ; \
              symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&^\\|%]+/ ; \
              string : /\"(\\\\.|[^\"])*\"/ ; \
              comment: /;[^\\r\\n]*/ ; \
              sexpr  : '(' <expr>* ')' ; \
              qexpr  : '{' <expr>* '}' ; \
              expr   : <number> | <boolean> | <symbol> | <string> | <comment> | <sexpr> | <qexpr> ; \
              blisp  : /^/ <expr>* /$/ ; \
            ", Number, Boolean, Symbol, String, Comment, Sexpr, Qexpr, Expr, Blisp);

    // Create global environment.
    lenv* e = lenv_new();
    lenv_add_builtins(e);

    // If we're supplied with a list of files
    if(argc >= 2) {

        // Loop over each supplied filename (starting from 1)
        for(int i = 1; i < argc; ++i) {

            // Argument list with a single argument, the filename
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

            // Pass to builtin load and get the result
            lval* x = builtin_load(e, args);

            // If the result is an error be sure to print it
            if(x->type == LVAL_ERR) {
                lval_println(e, x);
            }

            lval_del(x);

        }
    }

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
    mpc_cleanup(9, Number, Boolean, Symbol, String, Comment, Sexpr, Qexpr, Expr, Blisp);
    
    // Delete our environment
    lenv_del(e);
    
    return 0;

}
