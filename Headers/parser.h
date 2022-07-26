/* Array-Generator by Isaac Jung
Last updated 07/20/2022

|===========================================================================================================|
|   This header contains a class used for processing input. Should the input format change, this class can  |
| be updated accordingly. The way this class is used is to have the main code instantiate a single Parser   |
| object, then call the process_input() method. This will handle basic syntax and semantic checking on the  |
| input read from the provided input file (see README.md for details on expected command line calls). If no |
| errors are found, the process_input() method should also assign to the Parser's other public fields based |
| on command line arguments and the input read. The parser should attempt to stop the user when the user    |
| makes an impossible request, but this is a complicated task and currently a work in progress. This header |
| also contains enumeration typedefs used by other modules.                                                 |
|===========================================================================================================|
*/

#pragma once
#ifndef PARSER
#define PARSER

#include <string>
#include <vector>
#include <fstream>

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
// - none is only used for the generator's "don't cares"
// - c_only only checks coverage
// - l_only only checks location
// - d_only only checks detection
// - c_and_l checks coverage and location but not detection
// - c_and_d checks coverage and detection but not location
// - l_and_d checks location and detection but not coverage
// - all checks coverage, location, and detection
typedef enum {
    none    = 0,
    c_only  = 1,
    l_only  = 2,
    d_only  = 3,
    c_and_l = 4,
    c_and_d = 5,
    l_and_d = 6,
    all     = 7
} prop_mode;

class Parser
{
    public:
        std::string out_filename;       // output filename
        std::ofstream out;              // output file
        // arguments
        uint64_t d;     // magnitude of 𝒯 sets of t-way interactions
        uint64_t t;     // strength of interactions
        uint64_t delta; // desired separation

        // flags
        verb_mode v;    // verbose mode, v_off by default
        out_mode o;     // output mode, normal by default
        prop_mode p;    // properties mode, all by default

        // array stuff
        uint64_t num_rows = 0;          // rows, or tests, in the array
        uint64_t num_cols = 0;          // columns, or factors, in the array
        std::vector<uint64_t> levels;   // levels associated with each factor
        std::vector<uint64_t*> array;   // the array itself
        int process_input();            // call this to process standard in

        // constructor(s) and deconstructor
        Parser();                       // default constructor, probably won't be used
        Parser(int argc, char *argv[]); // constructor to read arguments and flags
        ~Parser();                      // deconstructor

    private:
        std::string in_filename;        // input filename
        std::ifstream in;               // input file
        void trim(std::string &s);
        void syntax_error(int lineno, std::string expected, std::string actual, bool verbose = true);
        void semantic_error(int lineno, int row, int col, int level, int value, bool verbose = true);
        void other_error(int lineno, std::string line, bool verbose = true);
};

#endif // PARSER
