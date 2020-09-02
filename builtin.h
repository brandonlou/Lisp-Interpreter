#ifndef BUILTIN_H
#define BUILTIN_H

#include "lval.h"
#include "lenv.h"

// Report generic conditional checking errors.
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

mpc_parser_t* Lispy;

// Load a file.
lval* builtin_load(lenv* e, lval* a);

// Perform a numerical operation on all lvals in the given list.
lval* builtin_op(lenv* e, lval* a, char* op);

// Add numbers.
lval* builtin_add(lenv* e, lval* a);

// Subtract numbers.
lval* builtin_sub(lenv* e, lval* a);

// Multiply numbers.
lval* builtin_mul(lenv* e, lval* a);

// Divide numbers.
lval* builtin_div(lenv* e, lval* a);

// Modulus numbers.
lval* builtin_mod(lenv* e, lval* a);

// Exponentiate numbers.
lval* builtin_pow(lenv* e, lval* a);

// Return the minimum of numbers.
lval* builtin_min(lenv* e, lval* a);

// Return the maximum of numbers.
lval* builtin_max(lenv* e, lval* a);

// Take a Q-Expression and return a Q-Expression containing only the first element.
lval* builtin_head(lenv* e, lval* a);

// Take a Q-Expression and return a Q-Expression with the first element removed.
lval* builtin_tail(lenv* e, lval* a);

// Convert an S-Expression into a Q-Expression.
lval* builtin_list(lenv* e, lval* a);

// Convert a Q-Expression into an S-Expression and evaluate it.
lval* builtin_eval(lenv* e, lval* a);

// Join 2+ Q-Expressions.
lval* builtin_join(lenv* e, lval* a);

// Prepend a value into a Q-Expression.
lval* builtin_cons(lenv* e, lval* a);

// Compute the number of elements in a Q-Expression.
lval* builtin_len(lenv* e, lval* a);

// Return a Q-Expression with the final element removed.
lval* builtin_init(lenv* e, lval* a);

// Print all named values in an environment up to specified number.
// If -1, print all.
lval* builtin_values(lenv* e, lval* a);

// Exit the interpreter with specified exit code.
// DO NOT USE. PROBABLY MEMORY LEAKS.
lval* builtin_exit(lenv* e, lval* a);

// Define a new variable. Return an empty list on success.
lval* builtin_var(lenv* e, lval* a, char* name);

// Create global variable.
lval* builtin_def(lenv* e, lval* a);

// Create local variable.
lval* builtin_put(lenv* e, lval* a);

// Create a lambda function given a list of symbols and a list represneting the body.
lval* builtin_lambda(lenv* e, lval* a);

#endif