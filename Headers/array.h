/* Array-Generator by Isaac Jung
Last updated 10/30/2022

|===========================================================================================================|
|   This header contains classes for managing the array in an automated fashion. The Interaction and T      |
| classes are used to represent fundamental covering/locating/detecting array concepts that guide scoring   |
| decisions during array generation. They are used only by the Array class and should not be instantiated   |
| by any source file other than the one associated with this header. The Array class is the interface with  |
| which other source files should work. It contains a constructor that builds all the internal data         |
| structures, thereby allowing the instantiator to immediately call other methods which carry out random    |
| row generation based on this data. See check.cpp for an example.                                          |
|===========================================================================================================|
*/

#include "parser.h"
#include "factor.h"
#include <map>
#include <mutex>
#include <thread>

class T;    // forward declaration because Interaction and T have circular references

class Interaction
{
    public:
        // used only in verbose mode, to have some id associated with the interaction
        int id = -1;

        // the actual list of (factor, value) tuples
        std::vector<Single*> singles;

        // this tracks the set of tests (represented as row numbers) in which this interaction occurs;
        // this row coverage is vital to analyzing the array's properties
        std::set<int> rows;

        // easy lookup bool to cut down on redundant checks
        bool is_covered = false;

        // this tracks all the T sets in which this interaction occurs; using this, one can obtain all the
        // relevant sets when a new row with this interaction is added
        std::set<T*> sets;  

        // this tracks the set differences between the set of rows in which this Interaction occurs and the
        // sets of rows in which relevant T sets this Interaction is not part of occur; that is, this is
        // a field to map detection issues to their delta values
        std::map<T*, int64_t> deltas;

        // easy lookup bool to cut down on redundant checks
        bool is_detectable = false;

        std::string to_string();    // returns a string representing all Singles in the interaction
        Interaction(std::vector<Single*> *temp);    // constructor with a premade vector of Single pointers
};

// I wasn't sure what to name this, except after the formal parameter used in Dr. Colbourn's definitions
// Note that Colbourn's definitions use a script T: 𝒯 
// This class is used to more easily compare the row coverage of different size-d sets of t-way interactions
class T
{
    public:
        // used only in verbose mode, to have some id associated with the T set
        int id;

        // for easier access to the singles themselves
        std::vector<Single*> singles;

        // for easier access to the interactions themselves
        std::vector<Interaction*> interactions;

        // each interaction in a given T set has its own version of this; the ρ associated with a T is simply
        // the union of the ρ's for each interaction in that T
        std::set<int> rows;

        // this tracks all the T sets which occur in the same set of rows as this instance; when adding a
        // row to the array, each T set occurring in the row must be compared to every other T set to see
        // if their sets of rows are disjoint yet; if so, there is no longer a conflict; when the size of
        // "location_conflicts" becomes 0 while the above set, "rows", is greater than 0, this T becomes
        // locatable within the array
        std::set<T*> location_conflicts;

        // easy lookup bool to cut down on redundant checks
        bool is_locatable = false;

        std::string to_string();    // returns a string representing all Interactions in the set
        T(std::vector<Interaction*> *temp); // constructor with a premade vector of Interaction pointers
};

class Array
{
    public:
        // this is a measure of how close the array is to complete; 0 is complete
        uint64_t score;

        // this is the size of the sets of t-way interactions
        uint64_t d;

        // this is the strength of interest
        uint64_t t;

        // this is the desired separation of the array
        uint64_t delta;
        
        // tracks whether the array is t-covering
        bool is_covering;

        // tracks whether the array is (d, t)-locating
        bool is_locating;

        // tracks whether the array is (d, t, δ)-detecting
        bool is_detecting;

        // list of all individual Single (factor, value) pairs
        std::vector<Single*> singles;

        // list of all individual t-way interactions
        std::vector<Interaction*> interactions;

        // list of all size-d sets of t-way interactions
        std::vector<T*> sets;

        // really only needed by heuristic_all()
        std::map<std::string, Single*> single_map;

        // used by build_row_interactions()
        std::map<std::string, Interaction*> interaction_map;

        // really only needed by heuristic_all()
        std::map<std::string, T*> t_set_map;

        void print_stats(bool initial = false); // prints current stats such as score
        void add_row();             // adds a row to the array based on scoring
        std::string to_string();    // returns a string representing all rows
        Array();    // default constructor, don't use this
        Array(Parser *in);  // constructor with an initialized Parser object
        Array(uint64_t total_problems, uint64_t coverage_problems, uint64_t location_problems,
            uint64_t detection_problems, std::vector<int*> *rows, uint64_t num_tests, uint64_t num_factors,
            Factor **factors, prop_mode p, uint64_t d, uint64_t t, uint64_t delta);
        ~Array();   // deconstructor

    private:
        // the array's score starts off as this value
        uint64_t total_problems;

        // subset of total_issues representing just coverage
        uint64_t coverage_problems;

        // subset of total_issues representing just location
        uint64_t location_problems;

        // subset of total_issues representing just detection
        uint64_t detection_problems;
        
        // list of the rows themselves, as a vector of int arrays
        std::vector<int*> rows;

        // field to track the current number of rows
        uint64_t num_tests;

        // field to reference the upper bound on iterating through columns
        uint64_t num_factors;

        // pointer to the start of an array of pointers to Factor objects
        Factor **factors;

        // for tracking which factors have solved all issues of which categories
        prop_mode *dont_cares;

        // memoized heuristic_all scores
        std::map<std::string, uint64_t> row_scores;

        // used to help avoid redundant checks in heuristic_all
        uint64_t min_positive_score = UINT64_MAX;

        // used to help avoid redundant checks for heuristics that do something only on the first call
        bool just_switched_heuristics = false;

        // for dictating order of iteration; should be regularly shuffled
        int *permutation;

        // this makes the program print out data structures and program flow when enabled
        debug_mode debug;

        // this makes the program print out a score breakdown when enabled
        verb_mode v;

        // this dictates how much output should be printed; see parser.h for typedef
        out_mode o;

        // this tracks what type of array is under construction; decides work to be done in several places
        prop_mode p;

        // this keeps track of what heuristic the program is currently using
        prop_mode heuristic_in_use;

        // needed by heuristic_all_scorer to update scores in threads safely
        std::mutex scores_mutex;

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

        int *initialize_row_R();                                        // returns a randomly generated row
        int *initialize_row_S();                                        // based on Singles
        int *initialize_row_T(T **l_set, Interaction **l_interaction);  // based on T sets
        int *initialize_row_I(Interaction **locked);                    // based on Interactions

        void heuristic_c_only(int *row);
        int heuristic_c_helper(int *row, std::set<Interaction*> *row_interactions, int *problems);
        
        void heuristic_l_only(int *row, T *l_set, Interaction *l_interaction);

        void heuristic_d_only(int *row, Interaction *locked);

        void heuristic_all(int *row);
        void heuristic_all_helper(int *row, uint64_t cur_col, std::vector<std::thread*> *threads);
        void heuristic_all_scorer(int *row, std::string row_str);
        
        void update_array(int *row, bool keep = true);
        void update_scores(std::set<Interaction*> *row_interactions, std::set<T*> *row_sets);
        void update_dont_cares();
        void update_heuristic();

        Array *clone(); // for getting a copy of this, including deep copying of object references
};