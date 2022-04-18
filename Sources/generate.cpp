/* Array-Generator by Isaac Jung
Last updated 04/18/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "parser.h"
#include "array.h"
#include <sys/types.h>
#include <unistd.h>


// ================================v=v=v== static global variables ==v=v=v================================ //

static verb_mode vm; // verbose mode
static out_mode om;  // output mode
static prop_mode pm; // property mode

// ================================^=^=^== static global variables ==^=^=^================================ //


// =========================v=v=v== static methods - forward declarations ==v=v=v========================= //

static void verbose_print(int d, int t, int delta);
static int conclusion(int t, bool is_covering);
static int conclusion(int d, int t, bool is_covering, bool is_locating);
static int conclusion(int d, int t, int delta, bool is_covering, bool is_locating, bool is_detecting);

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
    Parser p(argc, argv);               // create Parser object, immediately processes arguments and flags
    vm = p.v; om = p.o; pm = p.p;       // update flags based on those processed by the Parser
    
	int status = p.process_input(); // read in and process the array
    if (vm == v_on) verbose_print(p.d, p.t, p.delta);   // print status when verbose mode enabled
    if (status == -1) return 1;     // exit immediately if there is a basic syntactic or semantic error
    if (status == 1) return conclusion(p.d, p.t, p.delta, false, false, false); // non-trivial issue
    
    Array array(&p);    // create Array object that immediately builds appropriate data structures
    bool is_covering = false, is_locating = false, is_detecting = false;
    
    // checking coverage must always happen, but whether to show output depends on property mode
    if (pm == all || pm == c_only || pm == c_and_l || pm == c_and_d)
        is_covering = array.is_covering(true);  // show output according to output mode
    else
        is_covering = array.is_covering(false); // don't show output (l and/or d flagged but not c)

    // may skip location check if array lacks coverage and verbose mode is disabled
    if (!is_covering && vm == v_off) is_locating = false;
    else    // may need to check location, and whether to show output depends on property mode
        if (pm == all || pm == l_only || pm == c_and_l || pm == l_and_d)
            is_locating = is_covering && array.is_locating(true);  // show output according to output mode
        else if (pm == d_only || pm == c_and_d)
            is_locating = is_covering && array.is_locating(false); // don't show output (d flagged but not l)
        // else property mode is c_only and location should not be computed
    
    // may skip detection check if array lacks location and verbose mode is disabled
    if (!is_locating && vm == v_off) is_detecting = false;
    else    // check detection if property mode is anything involving the d flag
        if (pm == all || pm == d_only || pm == c_and_d || pm == l_and_d) {  // check detection if requested
            is_detecting = is_locating && array.is_detecting();
            if (!is_detecting)
                return conclusion(array.d, array.t, array.delta, is_covering, is_locating, false);
            conclusion(array.d, array.t, array.delta, is_covering, is_locating, true);
            if (array.true_delta > array.delta)
                printf("The greatest separation is actually %lu.\n", array.true_delta);
            return 0;
        }   // else property mode is c_only, l_only, or c_and_l and detection should not be computed

    return conclusion(array.d, array.t, array.delta, is_covering, is_locating, is_detecting);
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
        printf("==%d== Checking: coverage, location, detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else if (pm == c_only) {
        printf("==%d== Checking: coverage\n", pid);
        printf("==%d== Using t = %d\n", pid, t);
    } else if (pm == l_only) {
        printf("==%d== Checking: location\n", pid);
        printf("==%d== Using d = %d, t = %d\n", pid, d, t);
    } else if (pm == d_only) {
        printf("==%d== Checking: detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else if (pm == c_and_l) {
        printf("==%d== Checking: coverage, location\n", pid);
        printf("==%d== Using d = %d, t = %d\n", pid, d, t);
    } else if (pm == c_and_d) {
        printf("==%d== Checking: coverage, detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else if (pm == l_and_d) {
        printf("==%d== Checking: location, detection\n", pid);
        printf("==%d== Using d = %d, t = %d, δ = %d\n", pid, d, t, delta);
    } else {
        printf("==%d== No properties to check\nQuitting...\n", pid);
        exit(1);
    }
    printf("\n");
}

/* HELPER METHOD: conclusion - prints a final conclusion for the user
 * - overloaded: this version outputs the result of the coverage check
 *
 * parameters:
 * - t: the strength of interactions (how many (factor, values) in the interaction)
 * - is_covering: whether or not the array has t-way coverage
 * 
 * returns:
 * - always 0, allowing the caller to return the return from this function, representing success
*/
static int conclusion(int t, bool is_covering)
{
    if (om != silent) printf("CONCLUSIONS:\n");
    if (pm == all || pm == c_only || pm == c_and_l || pm == c_and_d)
        printf("The array %s %d-covering.\n", (is_covering ? "is" : "is not"), t);
    return 0;
}

/* HELPER METHOD: conclusion - prints a final conclusion for the user
 * - overloaded: this version outputs the result of the location check
 *
 * parameters:
 * - d: the number of t-way interactions in a 𝒯 (set of interactions)
 * - t: the strength of interactions (how many (factor, values) in the interaction)
 * - is_covering: whether or not the array has t-way coverage
 * - is_locating: whether or not the array is a (d, t)-locating array
 * 
 * returns:
 * - always 0, allowing the caller to return the return from this function, representing success
*/
static int conclusion(int d, int t, bool is_covering, bool is_locating)
{
    conclusion(t, is_covering);
    if (pm == all || pm == l_only || pm == c_and_l || pm == l_and_d)
        printf("The array %s (%d, %d)-locating.\n", (is_locating ? "is" : "is not"), d, t);
    return 0;
}

/* HELPER METHOD: conclusion - prints a final conclusion for the user
 * - overloaded: this version outputs the result of the detection check
 *
 * parameters:
 * - d: the number of t-way interactions in a 𝒯 (set of interactions)
 * - t: the strength of interactions (how many (factor, values) in the interaction)
 * - delta: the desired separation (only used for checking detection, check README for info)
 * - is_covering: whether or not the array has t-way coverage
 * - is_locating: whether or not the array is a (d, t)-locating array
 * - is_detecting: whether or not the array is a locating array
 * 
 * returns:
 * - always 0, allowing the caller to return the return from this function, representing success
*/
static int conclusion(int d, int t, int delta, bool is_covering, bool is_locating, bool is_detecting)
{
    conclusion(d, t, is_covering, is_locating);
    if (pm == all || pm == d_only || pm == c_and_d || pm == l_and_d)
        printf("The array %s (%d, %d, %d)-detecting.\n", (is_detecting ? "is" : "is not"), d, t, delta);
    return 0;
}