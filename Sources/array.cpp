/* Array-Generator by Isaac Jung
Last updated 05/21/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "array.h"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>
#include <map>

// method forward declarations
static void print_failure(Interaction *interaction);
static void print_failure(T *t_set_1, T *t_set_2);
static void print_failure(Interaction *interaction, T *t_set, long unsigned int delta, std::set<int> *dif);
static void print_singles(Factor **factors, int num_factors);
static void print_interactions(std::vector<Interaction*> interactions);
static void print_sets(std::vector<T*> sets);

static std::map<std::string, Interaction*> interaction_map;
static std::set<Interaction*> coverage_issues;
static std::set<T*> location_issues;
static std::set<Interaction*> detection_issues;

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Interaction::Interaction()
{
    // nothing to do
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a premade vector of Single pointers
*/
Interaction::Interaction(std::vector<Single*> *temp)
{
    // fencepost start: let the Interaction be the strength 1 interaction involving just the 0th Single in temp
    singles.insert(temp->at(0));
    rows = temp->at(0)->rows;

    // fencepost loop: for any t > 1, rows of the Interaction is the intersection of each Single's rows
    for (long unsigned int i = 1; i < temp->size(); i++) {
      singles.insert(temp->at(i));
      std::set<int> temp_set;
      std::set_intersection(rows.begin(), rows.end(),
        temp->at(i)->rows.begin(), temp->at(i)->rows.end(), std::inserter(temp_set, temp_set.begin()));
      rows = temp_set;
    }
}

std::string Interaction::to_string()
{
    std::string ret = "";
    for (Single *single : singles) ret += single->to_string();
    return ret;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
T::T()
{
    // nothing to do
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a premade vector of Interaction pointers
*/
T::T(std::vector<Interaction*> *temp)
{
    // fencepost start: let the Interaction be the strength 1 interaction involving just the 0th Single in s
    interactions.insert(temp->at(0));
    rows = temp->at(0)->rows;

    // fencepost loop: for any t > 1, rows of the Interaction is the intersection of each Single's rows
    for (long unsigned int i = 1; i < temp->size(); i++) {
      interactions.insert(temp->at(i));
      std::set<int> temp_set;
      std::set_union(rows.begin(), rows.end(),
        temp->at(i)->rows.begin(), temp->at(i)->rows.end(), std::inserter(temp_set, temp_set.begin()));
      rows = temp_set;
    }
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Array::Array()
{
    total_issues = 0; score = 0;
    d = 0; t = 0; delta = 0;
    v = v_off; o = normal; p = all;
    num_tests = 0; num_factors = 0;
    factors = nullptr;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a pointer to a Parser object
*/
Array::Array(Parser *in)
{
    total_issues = 0;   // will be increased as Interactions and sets of Interactions are created
    d = in->d; t = in->t; delta = in->delta;
    v = in->v; o = in->o; p = in->p;
    num_tests = in->num_rows;
    num_factors = in->num_cols;
    if(d <= 0) {
        printf("NOTE: bad value for d, continuing with d = 1\n");
        d = 1;
    }
    if(t <= 0 || t > num_factors) {
        printf("NOTE: bad value for t, continuing with t = 2\n");
        t = 2;
    }
    if(delta <= 0) {
        printf("NOTE: bad value for δ, continuing with δ = 1\n");
        delta = 1;
    }
    if (o != silent) printf("Building internal data structures....");

    try {
        // build all Singles, associated with an array of Factors
        factors = new Factor*[num_factors];
        for (long unsigned int i = 0; i < num_factors; i++) {
            factors[i] = new Factor(i, in->levels.at(i), new Single*[in->levels.at(i)]);
            for (long unsigned int j = 0; j < factors[i]->level; j++)
                factors[i]->singles[j] = new Single(i, j);
        }
        for (long unsigned int row = 0; row < in->num_rows; row++)
            for (long unsigned int col = 0; col < in->num_cols; col++)
                factors[col]->singles[in->array.at(row)[col]]->rows.insert(row + 1);
        if (v == v_on) print_singles(factors, num_factors);

        // build all Interactions
        std::vector<Single*> temp_singles;
        build_t_way_interactions(0, t, &temp_singles);
        if (v == v_on) print_interactions(interactions);
        total_issues += coverage_issues.size();

        // build all Ts
        if (p == c_only) return;    // no need to spend effort building Ts if they won't be used
        std::vector<Interaction*> temp_interactions;
        build_size_d_sets(0, d, &temp_interactions);
        if (v == v_on) print_sets(sets);
    } catch (const std::bad_alloc& e) {
        printf("ERROR: not enough memory to work with given array for given arguments\n");
        exit(1);
    }
    score = total_issues;   // the Array's score starts off here and is considered completed when 0
}

/* HELPER METHOD: build_t_way_interactions - initializes the interactions vector recursively
 * - the factors array must be initialized before calling this method
 * - top down recursive; auxiliary caller should use 0, t, and an empty vector as initial parameters
 *   --> do not use the interactions vector itself as the parameter
 * - this method should not be called more than once
 * 
 * parameters:
 * - start: left side of factors array at which to begin the outer for loop
 * - t: desired strength of interactions
 * - singles_so_far: auxiliary vector of pointers used to track the current combination of Singles
 * 
 * returns:
 * - void, but after the method finishes, the Array's interactions vector will be initialized
*/
void Array::build_t_way_interactions(long unsigned int start, long unsigned int t_cur,
    std::vector<Single*> *singles_so_far)
{
    // base case: interaction is completed and ready to store
    if (t_cur == 0) {
        Interaction *new_interaction = new Interaction(singles_so_far);
        interactions.push_back(new_interaction);
        interaction_map.insert({new_interaction->to_string(), new_interaction});    // for later accessing
        coverage_issues.insert(new_interaction);    // when generating an array from scratch, always true
        return;
    }

    // recursive case: need to introduce another loop for higher strength
    for (long unsigned int col = start; col < num_factors - t_cur + 1; col++) {
        for (long unsigned int level = 0; level < factors[col]->level; level++) {
            singles_so_far->push_back(factors[col]->singles[level]);    // note these are Single *
            build_t_way_interactions(col+1, t_cur-1, singles_so_far);
            singles_so_far->pop_back();
        }
    }
}

/* HELPER METHOD: build_size_d_sets - initializes the sets set recursively (a set of sets of interactions)
 * - the interactions vector must be initialized before calling this method
 * - top down recursive; auxiliary caller should use 0, d, and an empty set as initial parameters
 *   --> do not use the sets set itself as the parameter
 * - this method should not be called more than once
 * 
 * parameters:
 * - start: left side of interactions vector at which to begin the for loop
 * - d: desired magnitude of sets
 * - interactions_so_far: auxiliary vector of pointers used to track the current combination of Interactions
 * 
 * returns:
 * - void, but after the method finishes, the Array's sets set will be initialized
*/
void Array::build_size_d_sets(long unsigned int start, long unsigned int d_cur,
    std::vector<Interaction*> *interactions_so_far)
{
    // base case: set is completed and ready to store
    if (d_cur == 0) {
        T *new_set = new T(interactions_so_far);
        sets.push_back(new_set);
        for (Interaction *interaction : *interactions_so_far)   // all involved interactions get a reference
            interaction->sets.insert(new_set);
        return;
    }

    // recursive case: need to introduce another loop for higher magnitude
    for (long unsigned int i = start; i < interactions.size() - d_cur + 1; i++) {
        interactions_so_far->push_back(interactions[i]);    // note these are Interaction *
        build_size_d_sets(i+1, d_cur-1, interactions_so_far);
        interactions_so_far->pop_back();
    }
}

/* HELPER METHOD: build_row_interactions - recovers the Interaction objects based on the given row
 * - top down recursive; auxiliary caller should use 0, t, and an empty string as initial parameters
 * - this method should be called for every unique row considered; note that this gets expensive
 * 
 * parameters:
 * - row: integer array representing a row up for consideration for appending to the Array
 * - row_interactions: initially empty set to hold the Interactions as they are recovered
 * - start: left side of row at which to begin the for loop
 * - t_cur: distance from right side of row at which to end the for loop
 * - key: auxiliary vector of pointers used to track the current combination of Singles
 * 
 * returns:
 * - void, but after the method finishes, the row_interactions set will hold all the interactions in the row
*/
void Array::build_row_interactions(int *row, std::set<Interaction*> *row_interactions,
    long unsigned int start, long unsigned int t_cur, std::string key)
{
    if (t_cur == 0) {
        row_interactions->insert(interaction_map.at(key));
        return;
    }

    for (long unsigned int col = start; col < num_factors - t_cur + 1; col++) {
        std::string cur = key + "f" + std::to_string(col) + "," + std::to_string(row[col]);
        build_row_interactions(row, row_interactions, col+1, t_cur-1, cur);
    }
}

// dummy for now
void Array::add_row(long unsigned int rand_num)
{
    if (score == 0) return; // nothing to do if the array already satisfies all properties
    int *new_row = new int[num_factors];
    for (long unsigned int i = 0; i < num_factors; i++)
        new_row[i] = (rand_num / (i+1)) % factors[i]->level;
    tweak_row(new_row);
    rows.push_back(new_row);
    update_array();
}

int Array::tweak_row(int *row)
{
    // TODO: turn this into a giant switch case function that chooses a heuristic function to call based on
    // the score:total_issues ratio

    // after build_row_interactions is called, row_interactions will hold all interactions in this row
    std::set<Interaction*> row_interactions;
    build_row_interactions(row, &row_interactions, 0, t, "");
    int problems[num_factors] = {0};    // for counting how many "problems" each factor has
    int max_problems = 0;   // in case there is a tie for the most number of problems
    int actual_max = 0;     // because max_problems can change

    // coverage only heuristic
    while (true) {
        //long unsigned int best_score = 0;
        //long unsigned int cur_score = 0;
        
        for (Interaction *i : row_interactions) {
            if (i->rows.size() != 0) {  // interaction is already covered
                for (Single *s : i->singles) {  // increment the problems counter for each Single involved
                    problems[s->factor]++;
                    if (problems[s->factor] > max_problems) max_problems = problems[s->factor];
                }
            } else  // interaction not covered; decrement the problems counters instead
                for (Single *s : i->singles) problems[s->factor]--;
        }
        if (max_problems == 0) return;  // row is as good as possible as is

        actual_max = INT32_MIN; // set actual_max to a huge negative number to start
        // try altering the values with the most "problems" - those that contribute the least
        for (int col = 0; col < num_factors; col++) {
            if (problems[col] == max_problems) {
                std::set<Interaction*> temp = row_interactions; // deep copy because we are about to mutate
                // TODO: update row_interactions
                actual_max = max_problems;
                break;
            }
            if (problems[col] > actual_max) actual_max = problems[col];
        }
        if (actual_max == max_problems) continue;
        break;
    }
    return 0;
}

void Array::update_array()
{
    score -= 1;     // dummy
    if (score < 0) score = 0;
    num_tests++;
}

std::string Array::to_string()
{
    std::string ret = "";
    for (int *row : rows) {
        for (long unsigned int i = 0; i < num_factors; i++)
            ret += std::to_string(row[i]) + '\t';
        ret += '\n';
    }
    return ret;
}

/* SUB METHOD: is_covering - performs the analysis for coverage
 * 
 * parameters:
 * - report: when true, output is based on flags, else there will be no output whatsoever
 * 
 * returns:
 * - true if the array has t-way coverage, false if not
*/
bool Array::is_covering(bool report)
{
    if (report && o != silent) printf("Checking coverage....\n\n");
    bool passed = true;
    for (Interaction *i : interactions) {
        if (i->rows.size() == 0) {  // coverage issue
            if (o != normal) {  // if not reporting failures, can reduce work
                if (report && o == halfway) printf("COVERAGE CHECK: FAILED\n\n");
                return false;
            }
            print_failure(i);
            passed = false;
        }
    }
    if (report && o != silent) printf("COVERAGE CHECK: %s\n\n", passed ? "PASSED" : "FAILED");
    return passed;
}

/* SUB METHOD: is_locating - performs the analysis for location
 * - assumes the is_covering() method has already been called and returned true
 * 
 * parameters:
 * - report: when true, output is based on flags, else there will be no output whatsoever
 * 
 * returns:
 * - true if the array has (d-t)-location, false if not
*/
bool Array::is_locating(bool report)
{
    if (report && o != silent) printf("Checking location....\n\n");
    bool passed = true;
    for (long unsigned int i = 0; i < sets.size() - 1; i++) {
        for (long unsigned int j = i + 1; j < sets.size(); j++) {
            if (sets.at(i)->rows == sets.at(j)->rows) { // location issue
                if (o != normal) {   // if not reporting failures, can reduce work
                    if (report && o == halfway) printf("LOCATION CHECK: FAILED\n\n");
                    return false;
                }
                print_failure(sets.at(i), sets.at(j));
                passed = false;
            }
        }
    }
    if (report && o != silent) printf("LOCATION CHECK: %s\n\n", passed ? "PASSED" : "FAILED");
    return passed;
}

/* SUB METHOD: is_detecting - performs the analysis for location
 * 
 * parameters:
 * - report: when true, output is based on flags, else there will be no output whatsoever
 * 
 * returns:
 * - true if the array has (d, t, δ)-detection, false if not
*/
bool Array::is_detecting(bool report)
{
    if (report && o != silent) printf("Checking detection....\n\n");
    bool passed = true;
    for (Interaction *i : interactions) {
        for (T *t_set : sets) {
            if (t_set->interactions.find(i) != t_set->interactions.end()) continue; // interaction is in the set
            std::set<int> dif;
            std::set_difference(i->rows.begin(), i->rows.end(), t_set->rows.begin(), t_set->rows.end(),
                std::inserter(dif, dif.begin()));
            if (dif.size() < delta) {   // and if the set difference is less than δ
                if (o != normal) {   // if not reporting failures, can reduce work
                    if (report && o == halfway) printf("DETECTION CHECK: FAILED\n\n");
                    return false;
                }
                print_failure(i, t_set, delta, &dif);
                passed = false;
            }
        }
    }
    if (report && o != silent) printf("DETECTION CHECK: %s\n\n", passed ? "PASSED" : "FAILED");
    return passed;
}

/* DECONSTRUCTOR - frees memory
*/
Array::~Array()
{
    for (long unsigned int i = 0; i < num_tests; i++) delete[] rows[i];
    for (long unsigned int i = 0; i < num_factors; i++) delete factors[i];
    delete[] factors;
    for (Interaction *i : interactions) delete i;
    for (T *t_set : sets) delete t_set;
}



// ==============================   LOCAL HELPER METHODS BELOW THIS POINT   ============================== //

static void print_failure(Interaction *interaction)
{
    printf("\t-- %lu-WAY INTERACTION NOT PRESENT --\n", interaction->singles.size());
    std::string output("\t{");
    for (Single *s : interaction->singles)
        output += "(f" + std::to_string(s->factor) + ", " + std::to_string(s->value) + "), ";
    output = output.substr(0, output.size() - 2) + "}\n";
    std::cout << output << std::endl;
}

static void print_failure(T *t_set_1, T *t_set_2)
{
    printf("\t-- DISTINCT SETS WITH EQUAL ROWS --\n");
    std::string output("\tSet 1: { {");
    for (Interaction *i : t_set_1->interactions) {
        for (Single *s : i->singles)
            output += "(f" + std::to_string(s->factor) + ", " + std::to_string(s->value) + "), ";
        output = output.substr(0, output.size() - 2) + "}; ";
    }
    output = output.substr(0, output.size() - 2) + " }\n\tSet 2: { {";
    for (Interaction *i : t_set_2->interactions) {
        for (Single *s : i->singles)
            output += "(f" + std::to_string(s->factor) + ", " + std::to_string(s->value) + "), ";
        output = output.substr(0, output.size() - 2) + "}; ";
    }
    output = output.substr(0, output.size() - 2) + " }\n\tRows: { ";
    for (int row : t_set_1->rows) output += std::to_string(row) + ", ";
    output = output.substr(0, output.size() - 2) + " }\n";
    std::cout << output << std::endl;
}

static void print_failure(Interaction *interaction, T *t_set, long unsigned int delta, std::set<int> *dif)
{
    printf("\t-- ROW DIFFERENCE LESS THAN %lu --\n", delta);
    std::string output("\tInt: {");
    for (Single *s : interaction->singles)
        output += "(f" + std::to_string(s->factor) + ", " + std::to_string(s->value) + "), ";
    output = output.substr(0, output.size() - 2) + "}, { ";
    for (int row : interaction->rows) output += std::to_string(row) + ", ";
    output = output.substr(0, output.size() - 2) + " }\n\tSet: { {";
    for (Interaction *i : t_set->interactions) {
        for (Single *s : i->singles)
            output += "(f" + std::to_string(s->factor) + ", " + std::to_string(s->value) + "), ";
        output = output.substr(0, output.size() - 2) + "}; {";
    }
    output = output.substr(0, output.size() - 3) + " }, { ";
    for (int row : t_set->rows) output += std::to_string(row) + ", ";
    output = output.substr(0, output.size() - 2) + " }\n\tDif: { ";
    for (int row : *dif) output += std::to_string(row) + ", ";
    if (dif->size() > 0) output = output.substr(0, output.size() - 2) + " }\n";
    else output += "}\n";
    std::cout << output << std::endl;
}

static void print_singles(Factor **factors, int num_factors)
{
    int pid = getpid();
    printf("\n==%d== Listing all Singles below:\n\n", pid);
    for (int col = 0; col < num_factors; col++) {
        printf("Factor %lu:\n", factors[col]->id);
        for (long unsigned int level = 0; level < factors[col]->level; level++) {
            printf("\t(f%lu, %lu): {", factors[col]->singles[level]->factor, factors[col]->singles[level]->value);
            for (int row : factors[col]->singles[level]->rows) printf(" %d", row);
            printf(" }\n");
        }
        printf("\n");
    }
}

static void print_interactions(std::vector<Interaction*> interactions)
{
    int pid = getpid();
    printf("\n==%d== Listing all Interactions below:\n\n", pid);
    int i = 0;
    for (Interaction *interaction : interactions) {
        interaction->id = ++i;
        printf("Interaction %d:\n\tInt: {", i);
        for (Single *s : interaction->singles) printf(" (f%lu, %lu)", s->factor, s->value);
        printf(" }\n\tRows: {");
        for (int row : interaction->rows) printf(" %d", row);
        printf(" }\n\n");
    }
}

static void print_sets(std::vector<T*> sets)
{
    int pid = getpid();
    printf("\n==%d== Listing all Ts below:\n\n", pid);
    int i = 0;
    for (T *t_set : sets) {
        printf("Set %d:\n\tSet: {", ++i);
        for (Interaction *interaction : t_set->interactions) printf(" %d", interaction->id);
        printf(" }\n\tRows: {");
        for (int row : t_set->rows) printf(" %d", row);
        printf(" }\n\n");
    }
}