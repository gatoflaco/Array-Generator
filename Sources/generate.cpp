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
    
    //Array array(&p);    // create Array object that immediately builds appropriate data structures
    // TODO: figure out what to do from here lol
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