/* Array-Generator by Isaac Jung
Last updated 04/18/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#pragma once
#ifndef PARSER
#define PARSER

#include <string>
#include <vector>

// typedef representing whether verbose mode is set
// - v_off is normal
// - v_on means more detail should be shown in output
typedef enum {
    v_off   = 0,
    v_on    = 1
} verb_mode;

// typedef representing what sections of output to show
// - normal displays everything
// - halfway indicates what checks are happening but doesn't print any issues found
// - silent only shows the final conclusion(s)
typedef enum {
    normal  = 0,
    halfway = 1,
    silent  = 2
} out_mode;

// typedef representing which properties to check for
// - all checks coverage, location, and detection
// - c_only only checks coverage
// - l_only only checks location
// - d_only only checks detection
// - c_and_l checks coverage and location but not detection
// - c_and_d checks coverage and detection but not location
// - l_and_d checks location and detection but not coverage
typedef enum {
    all     = 0,
    c_only  = 1,
    l_only  = 2,
    d_only  = 3,
    c_and_l = 4,
    c_and_d = 5,
    l_and_d = 6
} prop_mode;

class Parser
{
    public:
        // arguments
        long unsigned int d;  // magnitude of 𝒯 sets of t-way interactions
        long unsigned int t;  // strength of interactions
        long unsigned int delta;  // desired separation

        // flags
        verb_mode v;    // verbose mode, v_off by default
        out_mode o;     // output mode, normal by default
        prop_mode p;    // properties mode, all by default

        // array stuff
        long unsigned int num_rows = 0; // rows, or tests, in the array
        long unsigned int num_cols = 0; // columns, or factors, in the array
        std::vector<long unsigned int> levels;  // levels associated with each factor
        std::vector<long unsigned int*> array;  // the array itself
        int process_input();            // call this to process standard in

        // constructor(s) and deconstructor
        Parser();                       // default constructor, probably won't be used
        Parser(int argc, char *argv[]); // constructor to read arguments and flags
        ~Parser();                      // deconstructor

    private:
        void trim(std::string &s);
        void syntax_error(int lineno, std::string expected, std::string actual, bool verbose = true);
        void semantic_error(int lineno, int row, int col, int level, int value, bool verbose = true);
        void other_error(int lineno, std::string line, bool verbose = true);
};

#endif // PARSER
