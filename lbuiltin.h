#ifndef LBUILTIN_H
#define LBUILTIN_H

// Forward declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// Declare new function pointer type named lbuiltin that is called
// with a lenv* and lval*, returning a lval*
typedef lval*(*lbuiltin) (lenv*, lval*);

#endif