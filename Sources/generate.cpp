/* Array-Generator by Isaac Jung
Last updated 12/13/2022

|===========================================================================================================|
|   This file contains the main() method which reflects the high level flow of the program. It starts by    |
| creating a Parser object which processes the command line arguments and flags in its constructor. It then |
| calls the process_input() method on the Parser object, which reads the file to establish the properties   |
| desired for the array. It uses the processed input to create an Array object, whose constructor organizes |
| objects representing (factor, value) pairs, interactions among pairs, sets of interactions, and more, and |
| the relationships among all these things. From there, the main() method adds a completely random row to   |
| the array to start, before entering a loop of adding one row at a time until the array is completed with  |
| the desired properties as requested by the user. Once completed, the array is saved to a file, or printed |
| to std out, depending on what arguments were provided on the command line (see README.md). If, for some   |
| reason, the array cannot be completed, main() attempts to detect this and stop early, rather than get     |
| caught in an infinite loop. In this case, whatever was able to be generated will still be saved/printed,  |
| along with a warning to the user about the failure.                                                       |
|   Other classes used by this program are: Single, Factor, Interaction, T, Prev_S_Data, Prev_I_Data, and   |
| Prev_T_Data. The Single and Factor classes can be found in the factor.h and factor.cpp files. The         |
| Interaction and T classes can be found in the array.h and array.cpp files. All of the Prev_x_data classes |
| are self-contained in heuristics.cpp, and do not need to be used by other modules. Here are general       |
| descriptions of how all of the classes in this program are intended to be used:                           |
| - Parser: parses input from the user to set flags and get info about the array to be generated            |
| - Array: stores important traits of the array and serves as an interface for adding rows                  |
| - Single: simple struct-like class for use by the Array object, representing a (factor, value)            |
| - Factor: another struct-like class that associates lists of Singles with their corresponding factors     |
| - Interaction: struct-like class to group Singles together, fundamental to the defining of coverage       |
| - T: struct-like class to group Interactions together, fundamental to defining location and detection     |
|===========================================================================================================|
*/

#include "parser.h"
#include "array.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


// ================================v=v=v== static global variables ==v=v=v================================ //

static debug_mode dm;   // debug mode
static verb_mode vm;    // verbose mode
static out_mode om;     // output mode
static prop_mode pm;    // property mode

// ================================^=^=^== static global variables ==^=^=^================================ //


// =========================v=v=v== static methods - forward declarations ==v=v=v========================= //

static int32_t print_usage();
static int32_t print_results(Parser *p, Array *array, bool success);
static void debug_print(uint8_t d, uint8_t t, uint8_t delta);

// =========================^=^=^== static methods - forward declarations ==^=^=^========================= //


/* MAIN METHOD: main - called when program is executed
 *
 * parameters:
 * - argc: number of arguments given by caller (including the token used to call)
 * - argv: vector containing the arguments given by the caller; can have exactly 0 or 2 additional arguments
 *     - if 2 additional arguments provided, should be ints specifying d and t
 * 
 * returns:
 * - exit code representing the state of the program (0 means the program finished successfully)
*/
int32_t main(int32_t argc, char *argv[])
{
    if (argc < 2 ||  strcmp(argv[1], "--help") == 0) return print_usage();  // user gave no args or --help
    Parser p(argc, argv);           // create Parser object, immediately processes arguments and flags
    dm = p.debug; vm = p.v; om = p.o; pm = p.p; // update flags based on those processed by the Parser
    
	int32_t status = p.process_input();             // read in and process the array
    if (status == -1) return 1;         // exit immediately if there is a basic syntactic or semantic error
    if (dm == d_on) debug_print(p.d, p.t, p.delta); // print status when verbose mode enabled
    
    Array array(&p);    // create Array object that immediately builds appropriate data structures
    if (array.score == 0) {
        printf("Nothing to do.\n\n");
        return 0;
    }

    array.print_stats(true);        // report initial state of array
    uint64_t prev_score;            // for comparing to current score to see if nothing is changing
    uint8_t no_change_counter = 0;  // need this to stop an infinite loop if the array cannot be completed
    while (array.score > 0) {       // add rows until the array is complete
        prev_score = array.score;   // needed for catching impossible scenarios
        array.add_row();            // add another row
        if (array.out_of_memory) break;
        if (array.score == prev_score) no_change_counter++;
        else no_change_counter = 0;
        if (no_change_counter > 10) break;
        array.print_stats();        // report current state of array
    }
    return print_results(&p, &array, (no_change_counter == 0 || array.out_of_memory));
}

/* HELPER METHOD: print_usage - prints info about the usage of the program
 * 
 * returns:
 * - exit code representing the state of the program (0 means the program finished successfully)
*/
static int32_t print_usage()
{
    printf("usage: ./generate [flags] (<t> | <d> <t> | <d> <t> <δ>) <input file> [output file]\n");
    printf("flags (some can be combined):\n");
    printf("\t-d          : debug mode (prints extra state information while running)\n");
    printf("\t-h          : halfway mode (prints less output than normal)\n");
    printf("\t-s          : silent mode (prints no output, cancels other flags)\n");
    printf("\t-v          : verbose mode (prints more output than normal)\n");
    printf("arguments (assume order matters):\n");
    printf("\tt           : strength of interactions, needed for all types of arrays\n");
    printf("\td           : size of sets of interactions, needed for locating and detecting arrays\n");
    printf("\tδ           : separation of interactions from other sets, needed for detecting arrays\n");
    printf("\tinput file  : file containing array parameter info\n");
    printf("\toutput file : file in which to print finished array (if not specified, stdout is used)\n");
    printf("for more details, please refer to the README, or visit https://github.com/gatoflaco/Array-Generator\n");
    return 0;
}

/* SUB METHOD: print_results - prints the completion status after the array is finished being generated
 * 
 * parameters:
 * - p: Parser object that has already had its process_input() method called
 * - array: Array object that has already been completely constructed
 * - row_count: number of rows that have been added to the array
 * - success: whether the array was completed with all requested properties satisfied or not
 * 
 * returns:
 * - exit code representing the state of the program (0 means the program finished successfully)
*/
static int32_t print_results(Parser *p, Array *array, bool success)
{
    if (!success) {
        printf("\nWARNING: It appears impossible to complete array with requested properties.\n");
        if (vm == v_off) printf("\tTry rerunning with the -v and -d flags for more details.\n");
        printf("\nCancelling array generation....\n");
    }

    if (p->out_filename.empty()) {
        if (!success) printf("The array up to this point was:\n");
        else if (om != silent) printf("The finished array is:\n");
        printf("%s\n", array->to_string().c_str());
    } else {
        try {
            p->out.open(p->out_filename.c_str(), std::ofstream::out);
        } catch ( ... ) {
            if (!success) {
                printf("Tried to write what rows the array had into file, but an error occurred.\n");
                printf("Please manually copy-paste it if needed:\n%s\n", array->to_string().c_str());
                return 0;
            }
            printf("Error opening file for writing. Please manually copy-paste the array as needed:\n%s\n",
                array->to_string().c_str());
            return 0;
        }
        std::string str = array->to_string();
        p->out.write(str.c_str(), static_cast<std::streamsize>(str.size()));
        p->out.close();
        if (!success) {
            printf("Wrote what rows the array had up to this point into file with path name <./%s>.\n\n",
                p->out_filename.c_str());
        }
        else if (om != silent)
            printf("Wrote array into file with path name <./%s>.\n\n", p->out_filename.c_str());
    }
    return 0;
}

/* HELPER METHOD: debug_print - prints the introductory status when debug mode is enabled
 * 
 * parameters:
 * - d: value of d read from the command line (default should be 1)
 * - t: value of t read from the command line (default should be 2)
 * - delta: value of δ read from the command line (default should be 1)
 * 
 * returns:
 * - void; simply prints to console
*/
static void debug_print(uint8_t d, uint8_t t, uint8_t delta) {
    int32_t pid = getpid();
    printf("==%d== Debug mode is enabled. Look for liness preceeded by the PID.\n", pid);
    if (vm == v_off) printf("==%d== Verbose mode: disabled\n", pid);
    else if (vm == v_on) printf("==%d== Verbose mode: enabled\n", pid);
    if (om == normal) printf("==%d== Output mode: normal\n", pid);
    else if (om == halfway) printf("==%d== Output mode: halfway\n", pid);
    else if (om == silent) printf("==%d== Output mode: silent\n", pid);
    else printf("==%d== Output mode: UNDEFINED\n", pid);
    if (pm == all) {
        printf("==%d== Generating: coverage, location, detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else if (pm == c_only) {
        printf("==%d== Generating: coverage\n", pid);
        printf("==%d== Using t = %d\n", pid, t);
    } else if (pm == l_only) {
        printf("==%d== Generating: location\n", pid);
        printf("==%d== Using d = %d, t = %d\n", pid, d, t);
    } else if (pm == d_only) {
        printf("==%d== Generating: detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else if (pm == c_and_l) {
        printf("==%d== Generating: coverage, location\n", pid);
        printf("==%d== Using d = %d, t = %d\n", pid, d, t);
    } else if (pm == c_and_d) {
        printf("==%d== Generating: coverage, detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else if (pm == l_and_d) {
        printf("==%d== Generating: location, detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else {
        printf("==%d== No properties to check\nQuitting...\n", pid);
        exit(1);
    }
    printf("\n");
}