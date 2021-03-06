/* Array-Generator by Isaac Jung
Last updated 07/14/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "parser.h"
#include "factor.h"
#include <map>

class T;    // forward declaration because Interaction and T have circular references

class Interaction
{
    public:
        int id; // used only in verbose mode, to have some id associated with the interaction

        std::set<Single*> singles;   // the actual list of (factor, value) tuples

        // this tracks the set of tests (represented as row numbers) in which this interaction occurs;
        std::set<int> rows; // this row coverage is vital to analyzing the array's properties
        bool is_covered;    // easy lookup bool to cut down on redundant checks

        // this tracks all the T sets in which this interaction occurs; using this, one can obtain all the
        std::set<T*> sets;  // relevant sets when a new row with this interaction is added

        // this tracks the set differences between the set of rows in which this Interaction occurs and the
        // sets of rows in which relevant T sets this Interaction is not part of occur; that is, this is
        std::map<T*, int64_t> deltas;   // a field to map detection issues to their delta values
        bool is_detectable; // easy lookup bool to cut down on redundant checks

        std::string to_string();    // returns a string representing all Singles in the interaction

        Interaction();   // default constructor, don't use this
        Interaction(std::vector<Single*> *temp);   // constructor with a premade vector of Single pointers
};

// I wasn't sure what to name this, except after the formal parameter used in Dr. Colbourn's definitions
// Note that Colbourn's definitions use a script T: 𝒯 
// This class is used to more easily compare the row coverage of different size-d sets of t-way interactions
class T
{
    public:
        int id; // used only in verbose mode, to have some id associated with the T set

        std::vector<Single*> singles;           // for easier access to the singles themselves
        std::set<Interaction*> interactions;    // for easier access to the interactions themselves

        // each interaction in a given T set has its own version of this; the ρ associated with a T is simply
        std::set<int> rows;  // the union of the ρ's for each interaction in that T

        // this tracks all the T sets which occur in the same set of rows as this instance; when adding a
        // row to the array, each T set occurring in the row must be compared to every other T set to see
        // if their sets of rows are disjoint yet; if so, there is no longer a conflict; when the size of
        // "location_conflicts" becomes 0 while the above set, "rows", is greater than 0, this T becomes
        std::set<T*> location_conflicts;    // locatable within the array
        bool is_locatable;  // easy lookup bool to cut down on redundant checks

        T();    // default constructor, don't use this
        T(std::vector<Interaction*> *temp);    // constructor with a premade vector of Interaction pointers
};

class Array
{
    public:
        uint64_t score;     // this is a measure of how close the array is to complete; 0 is complete
        uint64_t d;         // this is the size of the sets of t-way interactions
        uint64_t t;         // this is the strength of interest
        uint64_t delta;     // this is the desired separation of the array
        bool is_covering;   // checks whether the array is t-covering
        bool is_locating;   // checks whether the array is (d, t)-locating
        bool is_detecting;  // tracks whether the array is (d, t, δ)-detecting

        void add_row();             // adds a row to the array based on scoring
        void add_random_row();      // adds a random row to the array
        std::string to_string();                // returns a string representing all rows

        Array();    // default constructor, don't use this
        Array(Parser *in);  // constructor with an initialized Parser object
        ~Array();   // deconstructor

    private:
        uint64_t total_problems;        // the array's score starts off as this value
        uint64_t coverage_problems;     // subset of total_issues representing just coverage
        uint64_t location_problems;     // subset of total_issues representing just location
        uint64_t detection_problems;    // subset of total_issues representing just detection
        std::map<std::string, Interaction*> interaction_map;    // used by build_row_interactions()
        verb_mode v;    // this makes the program print out the data structures when enabled
        out_mode o;     // this dictates how much output should be printed; see parser.h for typedef
        prop_mode p;    // this is used to avoid building sets if it won't be needed anyway
        std::vector<int*> rows;         // list of the rows themselves, as a vector of int arrays
        uint64_t num_tests;     // field to reference the upper bound on iterating through rows
        uint64_t num_factors;   // field to reference the upper bound on iterating through columns
        Factor **factors;    // pointer to the start of an array of pointers to Factor objects
        std::vector<Interaction*> interactions; // list of all individual t-way interactions
        std::vector<T*> sets;   // list of all size-d sets of t-way interactions

        prop_mode *dont_cares;  // for tracking which factors have solved all issues of which categories
        int *permutation;   // for dictating order of iteration; should be regularly shuffled

        // this utility method is called in the constructor to fill out the vector of all interactions
        // almost certainly needs to be recursive in order to handle arbitrary values of t
        void build_t_way_interactions(uint64_t start, uint64_t t_cur, std::vector<Single*> *singles_so_far);

        // after the above method completes, call this one to fill out the set of all size-d sets
        // almost certainly needs to be recursive in order to handle arbitrary values of d
        void build_size_d_sets(uint64_t start, uint64_t d_cur,
            std::vector<Interaction*> *interactions_so_far);

        // this utility method closely mimics the build_t_way_interactions() method, but uses the information
        // from a given row to fill out a set of interactions representing those that appear in the row
        void build_row_interactions(int *row, std::set<Interaction*> *row_interactions,
            uint64_t start, uint64_t t_cur, std::string key);

        int *get_random_row();  // gets a randomly generated row        
        void tweak_row(int *row, std::set<Interaction*> *row_interactions); // improves a decision for a row

        // define more of these as needed; they are for deciding what needs changing
        void heuristic_c_only(int *row, std::set<Interaction*> *row_interactions);
        int heuristic_c_helper(int *row, std::set<Interaction*> *row_interactions, int *problems);
        // TODO: make a somewhere-in-the-middle heuristic
        void heuristic_all(int *row);
        void heuristic_all_helper(int *row, uint64_t cur_col,
            std::vector<int*> *best_rows, int64_t *best_score);

        // updates the internal data structures - counts, scores, rows, etc. - based on the row being added
        void update_array(int *row, std::set<Interaction*> *row_interactions, bool keep = true);
};