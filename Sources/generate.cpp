/* Array-Generator by Isaac Jung
Last updated 07/07/2022

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
| - Prev_S_Data: struct-like class to hold previous state information regarding a Single                    |
| - Prev_I_Data: struct-like class to hold previous state information regarding an Interaction              |
| - Prev_T_Data: struct-like class to hold previous state information regarding a T                         |
|===========================================================================================================|
*/

#include "parser.h"
#include "array.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>


// ================================v=v=v== static global variables ==v=v=v================================ //

static verb_mode vm; // verbose mode
static out_mode om;  // output mode
static prop_mode pm; // property mode

// ================================^=^=^== static global variables ==^=^=^================================ //


// =========================v=v=v== static methods - forward declarations ==v=v=v========================= //

static void verbose_print(int d, int t, int delta);

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
int main(int argc, char *argv[])
{
    Parser p(argc, argv);           // create Parser object, immediately processes arguments and flags
    vm = p.v; om = p.o; pm = p.p;   // update flags based on those processed by the Parser
    
	int status = p.process_input(); // read in and process the array
    if (vm == v_on) verbose_print(p.d, p.t, p.delta);   // print status when verbose mode enabled
    if (status == -1) return 1;     // exit immediately if there is a basic syntactic or semantic error
    
    Array array(&p);    // create Array object that immediately builds appropriate data structures
    if (array.score == 0) {
        printf("Nothing to do.\n\n");
        return 0;
    }

    uint64_t row_count = 0; // for user's information
    if (om != silent) printf("Array score is currently %lu, adding row %lu.\n", array.score, ++row_count);
    array.add_random_row();
    //array.add_row_debug(0); // TODO: GET RID OF THIS EVENTUALLY
    while (array.score > 0) {   // add rows until the array is complete
        if (om != silent) printf("Array score is currently %lu, adding row %lu.\n", array.score, ++row_count);
        array.add_row();
    }
    if (om != silent) printf("Completed array with %lu rows.\n\n", row_count);

    if (p.out_filename.empty()) {
        if (om != silent) printf("The finished array is: \n");
        printf("%s\n", array.to_string().c_str());
    } else {
        try {
            p.out.open(p.out_filename.c_str(), std::ofstream::out);
        } catch ( ... ) {
            printf("Error opening file for writing. Please manually copy-paste the array as needed:\n%s\n",
                array.to_string().c_str());
            return 0;
        }
        std::string str = array.to_string();
        p.out.write(str.c_str(), static_cast<std::streamsize>(str.size()));
        p.out.close();
        if (om != silent) printf("Wrote array into file with path name <%s>.\n\n",p.out_filename.c_str());
    }
    return 0;
}

/* HELPER METHOD: verbose_print - prints the introductory status when verbose mode is enabled
 * 
 * parameters:
 * - d: value of d read from the command line (default should be 1)
 * - t: value of t read from the command line (default should be 2)
 * - delta: value of δ read from the command line (default should be 1)
 * 
 * returns:
 * - void; simply prints to console
*/
static void verbose_print(int d, int t, int delta) {
    int pid = getpid();
    printf("==%d== Verbose mode enabled\n", pid);
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