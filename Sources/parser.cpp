/* Array-Generator by Isaac Jung
Last updated 07/14/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "parser.h"
#include <sstream>
#include <iostream>
#include <algorithm>

// method forward declarations
bool bad_t(uint64_t t, uint64_t num_cols);
bool bad_d(uint64_t d, uint64_t t, std::vector<uint64_t> *levels, prop_mode p);
bool bad_delta(uint64_t d, uint64_t t, uint64_t delta);

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
    p = c_only;
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
                            uint64_t param = static_cast<uint64_t>(std::stoi(std::string(1, c)));
                            printf("NOTE: bad flag \'%lu\'; ignored", param);
                            printf(" (looks like an int, did you mean to specify without a hyphen?)\n");
                        } catch ( ... ) {
                            printf("NOTE: bad flag \'%c\'; ignored\n", c);
                        }
                        break;
                }
            }
        } else {    // command line arguments for specifying d, t, and δ
            try {   // see if it is an int
                uint64_t param = static_cast<uint64_t>(std::stoi(arg));
                if (num_params < 1) {
                    t = param;
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
            } catch ( ... ) {   // assume it is a filename
                if (in_filename.empty()) in_filename = arg;
                else if (out_filename.empty()) out_filename = arg;
                else printf("NOTE: couldn't parse command line argument <%s>; ignored\n", arg.c_str());
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
    try {
        in.open(in_filename.c_str(), std::ifstream::in);
    } catch ( ... ) {
        printf("\t-- ERROR --\n\tUnable to open file with path name <%s>.\n", in_filename.c_str());
        printf("\tFor usage details, please check the README.\n");
        printf("\n");
        return -1;
    }
    std::string cur_line;
    
    // C
    std::getline(in, cur_line);
    try {
        std::istringstream iss(cur_line);
        num_rows = 0;                       // assume that we are generating an array from scratch
        if (!(iss >> num_cols)) throw 0;    // error when columns not given or not int
        if (num_cols < 1) throw 0;          // error when values define impossible array
    } catch (...) {
        syntax_error(2, "C", cur_line);
        in.close();
        return -1;
    }

    // levels
    std::getline(in, cur_line);
    try {
        std::istringstream iss(cur_line);
        uint64_t level;
        for (uint64_t i = 0; i < num_cols; i++) {
            if (!(iss >> level)) throw 0;   // error when not enough levels given or not int
            levels.push_back(level);
        }
    } catch (...) {
        syntax_error(3, "L_1 L_2 ... L_C", cur_line);
        in.close();
        return -1;
    }

    in.close();
    if (bad_t(t, num_cols)) return -1;
    if (bad_d(d, t, &levels, p)) return -1;
    if (bad_delta(d, t, delta)) return -1;
    return 0;
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

/* DECONSTRUCTOR - frees memory
*/
Parser::~Parser()
{
    for (uint64_t *row : array) delete[] row;
}

// ==============================   LOCAL HELPER METHODS BELOW THIS POINT   ============================== //

bool bad_t(uint64_t t, uint64_t num_cols)
{
    if (t > num_cols) {
        printf("\t-- ERROR --\n");
        printf("\tImpossible to generate array with higher interaction strength than number of factors.\n");
        printf("\tstrength          --> %lu\n", t);
        printf("\tnumber of factors --> %lu\n", num_cols);
        return true;
    }
    if (t == 0) {
        printf("\t-- ERROR --\n\tt cannot be 0.\n");
        return true;
    }
    return false;
}

bool bad_d(uint64_t d, uint64_t t, std::vector<uint64_t> *levels, prop_mode p)
{
    if (p == all || p == c_and_l) { // location
        uint64_t count = 0;
        for (uint64_t level : *levels) {
            if (level < d) {
                printf("\t-- ERROR --\n");
                printf("\tImpossible to generate (%lu-%lu)-locating array ", d, t);
                printf("when any factor has less than %lu possible levels.\n", d);
                return true;
            }
            if (level == d) {
                count++;
                // I'm not yet sure of the math behind this, so leaving it commented out for now.
                if (count >= 2) {
                    printf("\t-- ERROR --\n\tImpossible to generate (%lu-%lu)-locating array ", d, t);
                    printf("when 2 or more factors have exactly %lu possible levels.\n", d);
                }//*/
            }
        }
    }
    return false;
}

bool bad_delta(uint64_t d, uint64_t t, uint64_t delta)
{
    if (delta == 0) {
        printf("\t-- ERROR --\n\tδ cannot be 0.\n");
        return true;
    }
    /* TODO: figure out if there is a way to see in advance that detection would not be possible
    if (p == all) { // detection
        for (uint64_t level : *levels) {
            if (level == d) {

            }
        }
        
    }//*/
    return false;
}