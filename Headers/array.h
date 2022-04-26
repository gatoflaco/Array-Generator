/* Array-Generator by Isaac Jung
Last updated 04/18/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "parser.h"
#include "factor.h"

class Interaction
{
    public:
        int id; // used only in verbose mode, to have some id associated with the interaction
        //int strength;   // the interaction strength t is the number of (factor, value) tuples involved
        std::set<Single*> singles;   // the actual list of (factor, value) tuples

        // this tracks the set of tests (represented as row numbers) in which this interaction occurs;
        std::set<int> rows; // this row coverage is vital to analyzing the locating and detecting properties

        Interaction();   // default constructor, don't use this
        Interaction(std::vector<Single*> *temp);   // constructor with a premade vector of Single pointers
};

// I wasn't sure what to name this, except after the formal parameter used in Dr. Colbourn's definitions
// Note that Colbourn's definitions use a script T: 𝒯 
// This class is used to more easily compare the row coverage of different size-d sets of t-way interactions
class T
{
    public:
        std::set<Interaction*> interactions; // for easier access to the interactions themselves

        // each interaction in a given T set has its own version of this; the ρ associated with a T is simply
        std::set<int> rows;  // the union of the ρ's for each interaction in that T

         // this helper bool is for remembering that this particular T is not locating; that is, it should be
        bool is_locating;   // set true upon instantion, but false once determined to violate the property
                            // once; it can be checked to cut down on computation time when false
        
        bool is_detecting;  // same as above

        T();    // default constructor, don't use this
        T(std::vector<Interaction*> *temp);    // constructor with a premade vector of Interaction pointers
};

class Array
{
    public:
        float score;                // this is a measure of how close the array is to complete
        long unsigned int d;        // this is the size of the sets of t-way interactions
        long unsigned int t;        // this is the strength of interest
        long unsigned int delta;    // this is the desired separation of the array

        void add_row(long unsigned int rand);   // adds another row to the array that improves the score

        void print();               // prints all rows

        // checks whether the array is covering; this means that every interaction of strength t occurs in
        // the array at least 1 time (TODO: extend this to at least δ times for (t, δ)-coverage)
        bool is_covering(bool report = true);

        // checks whether the array is (d, t)-locating; this means that for every pair of size-d sets of
        // t-way interactions, the rows covered by those sets are not equal
        bool is_locating(bool report = true);

        // checks whether the array is (d, t, δ)-detecting; this checks for all t-way interactions, for all
        // size-d sets, T is a member of the set OR T's rows minus the set's rows has >= δ elements
        bool is_detecting(bool report = true);

        Array();    // default constructor, don't use this
        Array(Parser *in);  // constructor with an initialized Parser object
        ~Array();   // deconstructor

    private:
        verb_mode v;    // this makes the program print out the data structures when enabled
        out_mode o;     // this dictates how much output should be printed; see parser.h for typedef
        prop_mode p;    // this is used to avoid building sets if it won't be needed anyway
        std::vector<int*> rows;         // list of the rows themselves, as a vector of int arrays
        long unsigned int num_tests;    // field to reference the upper bound on iterating through rows
        long unsigned int num_factors;  // field to reference the upper bound on iterating through columns
        Factor **factors;    // pointer to the start of an array of pointers to Factor objects
        std::vector<Interaction*> interactions; // list of all individual t-way interactions
        std::vector<T*> sets;  // list of all size-d sets of t-way interactions

        // this utility method is called in the constructor to fill out the vector of all interactions
        // almost certainly needs to be recursive in order to handle arbitrary values of t
        void build_t_way_interactions(long unsigned int start, long unsigned int t_cur,
            std::vector<Single*> *singles_so_far);

        // after the above method completes, call this one to fill out the set of all size-d sets
        // almost certainly needs to be recursive in order to handle arbitrary values of d
        void build_size_d_sets(long unsigned int start, long unsigned int d_cur,
            std::vector<Interaction*> *interactions_so_far);
};