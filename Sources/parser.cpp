/* Array-Generator by Isaac Jung
Last updated 04/18/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "parser.h"
#include <sstream>
#include <iostream>
#include <algorithm>

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Parser::Parser()
{
    d = 1; t = 2; delta = 1;
    v = v_off; o = normal; p = all;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on the command line arguments
*/
Parser::Parser(int argc, char *argv[]) : Parser()
{
    int itr = 1, num_params = 0;
    while (itr < argc) {
        std::string arg(argv[itr]);    // cast to std::string
        if (arg.at(0) == '-') { // flags
            for (char c : arg.substr(1, arg.length() - 1)) {
                switch(c) {
                    case 'v':
                        v = v_on;
                        break;
                    case 'h':
                        o = halfway;
                        break;
                    case 's':
                        o = silent;
                        break;
                    default:
                        try {
                            long unsigned int param =
                                static_cast<long unsigned int>(std::stoi(std::string(1, c)));
                            printf("NOTE: bad flag \'%lu\'; ignored", param);
                            printf(" (looks like an int, did you mean to specify without a hyphen?)\n");
                        } catch ( ... ) {
                            printf("NOTE: bad flag \'%c\'; ignored\n", c);
                        }
                        break;
                }
            }
        } else {    // command line arguments for specifying d, t, and δ
            try {
                long unsigned int param = static_cast<long unsigned int>(std::stoi(arg));
                if (num_params < 1) {
                    t = param;
                    p = c_only;
                } else if (num_params < 2) {
                    d = t;
                    t = param;
                    p = c_and_l;
                } else if (num_params < 3) {
                    delta = param;
                    p = all;
                } else {
                    printf("NOTE: too many int arguments given; ignored <%s>", arg.c_str());
                    printf(" (be sure to specify 3 at most)\n");
                }
                num_params++;
            } catch ( ... ) {
                printf("NOTE: couldn't parse command line argument <%s> as int; ignored\n", arg.c_str());
            }
        }
        itr++;
    }
}

/* SUB METHOD: process_input - reads from standard in to initialize program data
 * 
 * parameters:
 * - none
 * 
 * returns:
 * - code representing success/failure
*/
int Parser::process_input()
{
    if (o != silent) printf("Reading input....\n\n");
    int ret = 0;
    std::string cur_line;
    
    // C
    std::getline(std::cin, cur_line);
    try {
        std::istringstream iss(cur_line);
        num_rows = 0;                       // assume that we are generating an array from scratch
        if (!(iss >> num_cols)) throw 0;    // error when columns not given or not int
        if (num_cols < 1) throw 0;          // error when values define impossible array
    } catch (...) {
        syntax_error(2, "C", cur_line);
        return -1;
    }

    // levels
    std::getline(std::cin, cur_line);
    try {
        std::istringstream iss(cur_line);
        long unsigned int level;
        for (long unsigned int i = 0; i < num_cols; i++) {
            if (!(iss >> level)) throw 0;   // error when not enough levels given or not int
            levels.push_back(level);
        }
    } catch (...) {
        syntax_error(3, "L_1 L_2 ... L_C", cur_line);
        return -1;
    }

    return ret;
}

// ======================================================================================================= //
/* HELPER METHOD: trim - chops off all leading and trailing whitespace characters from a string
 * 
 * parameters:
 * - s: string to be trimmed
 * 
 * returns:
 * - void (the string will be changed at a global scope)
 * 
 * credit:
 * - this function was adapted from Evan Teran's post at https://stackoverflow.com/a/217605
*/
void Parser::trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}
// ======================================================================================================= //

/* HELPER METHOD: syntax_error - prints an error message regarding input format
 * 
 * parameters:
 * - lineno: line number on which the input format was violated
 * - expected: the input format that was expected
 * - actual: the actual input received
 * - verbose: whether to print extra error output (true by default)
 * 
 * returns:
 * - void (caller should decide whether to quit or continue)
*/
void Parser::syntax_error(int lineno, std::string expected, std::string actual, bool verbose)
{
    printf("\t-- ERROR --\n\tInput format violated on line %d. Expected \"%s\" but got \"%s\" instead.\n",
        lineno, expected.c_str(), actual.c_str());
    if (verbose) printf("\tFor formatting details, please check the README.\n");
    printf("\n");
}

/* HELPER METHOD: semantic_error - prints an error message regarding array format
 * 
 * parameters:
 * - lineno: line number on which the array format was violated
 * - row: row number of the array in which the array format was violated
 * - col: column number of the array in which the array format was violated
 * - level: factor level corresponding to the column in which the array format was violated
 * - value: the array value which violated expected format
 * - verbose: whether to print extra error output (true by default)
 * 
 * returns:
 * - void (caller should decide whether to quit or continue)
*/
void Parser::semantic_error(int lineno, int row, int col, int level, int value, bool verbose)
{
    printf("\t-- ERROR --\n\tArray format violated at row %d, column %d, on line %d.\n", row, col, lineno);
    if (value < 0)
        printf("\tArray values should not be negative. Value in array was %d.\n", value);
    else
        printf("\tLevel for that factor was given as %d, but value in array was %d which is too large.\n",
            level, value);
    if (verbose) printf("\tFor formatting details, please check the README.\n");
    printf("\n");
}

/* HELPER METHOD: other_error - prints a general error message
 *
 * parameters:
 * - lineno: line number on which the error occurred
 * - line: the entire line which caused the error
 * - verbose: whether to print extra error output (true by default)
 * 
 * returns:
 * - void (caller should decide whether to quit or continue)
*/
void Parser::other_error(int lineno, std::string line, bool verbose)
{
    printf("\t-- ERROR --\n\tError with line %d: \"%s\".\n", lineno, line.c_str());
    if (verbose) printf("\tFor formatting details, please check the README.\n");
    printf("\n");
}

/* DECONSTRUCTOR - frees memory
*/
Parser::~Parser()
{
    for (long unsigned int *row : array) delete[] row;
}