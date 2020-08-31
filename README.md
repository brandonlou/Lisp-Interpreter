# Blisp!

> Blisp stands for **B**randon's **lisp** or "you pro**b**ably don't want to use this **lisp**" -- whichever acronym resonates with you more.

## Overview
This lisp varient exists to serve as a practice in C programming for its creator. The codebase is inspired by [Daniel Holden's Build Your Own Lisp](http://www.buildyourownlisp.com/). Please check it out if you wish to create a lisp interpreter of your own!

## Building
Mac: `cc -std=c99 -Wall parsing.c mpc.c -ledit -o parsing`

### Debugging
You may find `gdb`, `lldb` (on mac), and/or `valgrind` useful.

## Documentation
Since blisp is simple relative to most languages, the entire language documentation will be available here.

## Todo
* Finish the book
* Use a makefile
* Separate code into multiple files
