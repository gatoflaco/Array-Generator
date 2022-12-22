/* Array-Generator by Isaac Jung
Last updated 12/22/2022

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

// typedef representing whether debug mode is set
// - d_off is normal
// - d_on causes the program to output data structure and flow control info to console
typedef enum {
    d_off   = 0,
    d_on    = 1
} debug_mode;

// typedef representing whether verbose mode is set
// - v_off is normal
// - v_on breaks down the score to individual factors and shows the current completion percentage
typedef enum {
    v_off   = 0,
    v_on    = 1
} verb_mode;

// typedef representing what sections of output to show
// - normal displays everything
// - halfway excludes the lines that state what row was added
// - silent only shows the final results or nothing at all if an output file was specified
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
        // output filename
        std::string out_filename;

        // output file
        std::ofstream out;
        
        // magnitude of 𝒯 sets of t-way interactions
        uint16_t d;

        // strength of interactions
        uint16_t t;

        // desired separation
        uint16_t delta;

        // debug mode, d_off by default
        debug_mode debug;

        // verbose mode, v_off by default
        verb_mode v;

        // output mode, normal by default
        out_mode o;

        // properties mode, all by default
        prop_mode p;

        // rows, or tests, in the array
        uint64_t num_rows = 0;

        // columns, or factors, in the array
        uint16_t num_cols = 0;

        // levels associated with each factor
        std::vector<uint16_t> levels;

        // the array itself, only used when the --partial flag is given
        std::vector<uint16_t*> array;

        int32_t process_input();            // call this to process the input file
        Parser();                           // default constructor, probably won't be used
        Parser(int32_t argc, char *argv[]); // constructor to read arguments and flags
        ~Parser();                      // deconstructor

    private:
        // input filename
        std::string in_filename;

        // input file
        std::ifstream in;

        // partial filename
        std::string partial_filename;

        // partial array file
        std::ifstream partial;

        void trim(std::string &s);  // trims a string of whitespace on either side
        void syntax_error(uint64_t lineno, std::string expected, std::string actual, bool verbose = true);
        void semantic_error(uint64_t lineno, uint64_t row, uint16_t col, uint16_t level, uint16_t value,
            int32_t neg_value = 0, bool verbose = true);
        void other_error(uint64_t lineno, std::string line, bool verbose = true);
};

#endif // PARSER
