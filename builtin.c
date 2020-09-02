#include "builtin.h"

// Load a file.
lval* builtin_load(lenv* e, lval* a) {

    lval_check_argcount("load", a, 1);
    lval_check_type("load", a, 0, LVAL_STR);

    // Parse file given by string name.
    mpc_result_t r;
    if(mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {

        // Read contents
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        // Evaluate each expression
        while(expr->count) {

            lval* x = lval_eval(e, lval_pop(expr, 0));

            // If evaluation leads to error then print it
            if(x->type == LVAL_ERR) {
                lval_println(e, x);
            }

            lval_del(x);

        }

        // Delete expressions and arguments
        lval_del(expr);
        lval_del(a);

        // Return empty list.
        return lval_sexpr();

    } else {

        // Get parse error as string
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        // Create new error message using it
        lval* err = lval_err("Could not load Library %s", err_msg);

        // Cleanup and return error.
        free(err_msg);
        lval_del(a);
        return err;

    }
}

// Print values separated by space with a newline at end.
lval* builtin_print(lenv* e, lval* a) {

    // Print each argument followed by a space.
    for(int i = 0; i < a->count; ++i) {
        lval_print(e, a->cell[i]);
        putchar(' ');
    }

    // Print a newline and delete arguments.
    putchar('\n');
    lval_del(a);

    return lval_sexpr();

}

// Print an error.
lval* builtin_error(lenv* e, lval* a) {

    lval_check_argcount("error", a, 1);
    lval_check_type("error", a, 0, LVAL_STR);

    // Construct error from first argument
    lval* err = lval_err(a->cell[0]->str);

    // Delete arguments and return.
    lval_del(a);
    return err;

}

// Perform a numerical operation on all lvals in the given list.
lval* builtin_op(lenv* e, lval* a, char* op) {

    // Ensure all arguments are numbers.
    for(int i = 0; i < a->count; ++i) {
        if(a->cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    // Pop the first element.
    lval* x = lval_pop(a, 0);

    // If no arguments and subtraction, then perform unary negation.
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

// Add numbers.
lval* builtin_add(lenv* e, lval* a) {
    return builtin_op(e, a, "+");
}

// Subtract numbers.
lval* builtin_sub(lenv* e, lval* a) {
    return builtin_op(e, a, "-");
}

// Multiply numbers.
lval* builtin_mul(lenv* e, lval* a) {
    return builtin_op(e, a, "*");
}

// Divide numbers.
lval* builtin_div(lenv* e, lval* a) {
    return builtin_op(e, a, "/");
}

// Modulus numbers.
lval* builtin_mod(lenv* e, lval* a) {
    return builtin_op(e, a, "%");
}

// Exponentiate numbers.
lval* builtin_pow(lenv* e, lval* a) {
    return builtin_op(e, a, "^");
}

// Return the minimum of numbers.
lval* builtin_min(lenv* e, lval* a) {
    return builtin_op(e, a, "min");
}

// Return the maximum of numbers.
lval* builtin_max(lenv* e, lval* a) {
    return builtin_op(e, a, "max");
}

// Take a Q-Expression and return a Q-Expression containing only the first element.
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

// Take a Q-Expression and return a Q-Expression with the first element removed.
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

// Convert an S-Expression into a Q-Expression.
lval* builtin_list(lenv* e, lval* a) {

    a->type = LVAL_QEXPR;
    return a;

}

// Convert a Q-Expression into an S-Expression and evaluate it.
lval* builtin_eval(lenv* e, lval* a) {
    
    // Error checking
    lval_check_argcount("eval", a, 1);
    lval_check_type("eval", a, 0, LVAL_QEXPR);

    lval* v = lval_take(a, 0);
    v->type = LVAL_SEXPR;
    
    return lval_eval(e, v);

}

// Join 2+ Q-Expressions.
lval* builtin_join(lenv* e, lval* a) {
    
    // Check all arguments are Q-Expressions.
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

// Prepend a value into a Q-Expression.
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

// Compute the number of elements in a Q-Expression.
lval* builtin_len(lenv* e, lval* a) {
    
    // Error checking
    lval_check_argcount("len", a, 1);
    lval_check_type("len", a, 0, LVAL_QEXPR);

    int num_elements = a->cell[0]->count;

    return lval_num(num_elements);

}

// Return a Q-Expression with the final element removed.
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

// Print all named values in an environment up to specified number.
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

// Exit the interpreter with specified exit code.
// DO NOT USE. PROBABLY MEMORY LEAKS.
lval* builtin_exit(lenv* e, lval* a) {
    
    lval_check_argcount("exit", a, 1);
    lval_check_type("exit", a, 0, LVAL_NUM);

    int status = (int)(a->cell[0]->num);
    
    printf("Please come again...\n");
    printf("Exiting blisp: %i\n", status);
    exit(status);

}

// Define a new variable. Return an empty list on success.
lval* builtin_var(lenv* e, lval* a, char* name) {

    lval_check_type(name, a, 0, LVAL_QEXPR);

    // First argument is symbol list
    lval* syms = a->cell[0];

    // Ensure all elements of first list are symbols
    for(int i = 0; i < syms->count; ++i) {
        lval_assert(a, syms->cell[i]->type == LVAL_SYM,
                "Function '%s' cannot define non-symbol. Got %s, Expected %s.",
                name, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    // Check correct number of symbols and values
    lval_assert(a, syms->count == a->count - 1,
            "Function 'def' cannot define incorrect number of values to symbols");

    // Assign copies of values to symbols
    for(int i = 0; i < syms->count; ++i) {

        // If 'def', define variable globally.
        if(strcmp(name, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i + 1]);
        
        // If 'put', define variable lcoally.
        } else if(strcmp(name, "=") == 0) {
            lenv_put(e, syms->cell[i], a->cell[i + 1]);
        }
    }

    lval_del(a);
    return lval_sexpr();

}

// Create global variable.
lval* builtin_def(lenv* e, lval* a) {
    return builtin_var(e, a, "def");
}

// Create local variable.
lval* builtin_put(lenv* e, lval* a) {
    return builtin_var(e, a, "=");
}

// Create a lambda function given a list of symbols and a list represneting the body.
lval* builtin_lambda(lenv* e, lval* a) {

    // Check for 2 arguments, each of which are Q-Expressions.
    lval_check_argcount("λ", a, 2);
    lval_check_type("λ", a, 0, LVAL_QEXPR);
    lval_check_type("λ", a, 1, LVAL_QEXPR);

    // Check first Q-Expression contains only symbols
    for(int i = 0; i < a->cell[0]->count; ++i) {
        lval_check_type("λ parameters", a->cell[0], i, LVAL_SYM);
    }

    // Pop first two arguments and pass them to lval_lambda
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);

}
