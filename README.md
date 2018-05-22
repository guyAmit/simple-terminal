# Simple-terminal
simple terminal that was built as a homework from CASPL course at ben Gurion university
the terminal act as the real linux terminal. because the subject was very interesting to me
i decided to improve and expend this project. and over time i hope that i will fully implement
all the important features of the real linux terminal.

## Getting Started

Clone the repository, and use the Makefile ("make") to compile and run using:

```
./myshell -do not run using make run because it will cause signals not to reach the program
```
use the command:
```
jobs
```
to view all running and suspended(ctr_z) commands.
suspended command can be re-run using the command:
```
fg <command index>
```
commands can be moved to the background using the command:
```
bg <command index>
```

## Currently Supporting
* **input and output redirection**
* **all simple terminal commands**
* **support unlimited pipe lines**
* **currently does not support chaining of text files for programs**

## Prerequisites

This project was built on Linux operating system.
the terminal will not work on other operating systems

## Contributing

the LineParser.c was not built by me, and belongs to the "Ben Gurion university of the negev"
this file will hopefully get replaced in the future.

## Versioning

current version : 1.0.1

## Authors

* **Guy Amit** - [guyAmit](https://github.com/guyAmit)
* **Ben Gurion university of the negev**

## License

//ToDo add License
