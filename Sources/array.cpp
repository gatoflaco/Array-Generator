/* Array-Generator by Isaac Jung
Last updated 07/14/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "array.h"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

// method forward declarations
static void print_failure(Interaction *interaction);
static void print_failure(T *t_set_1, T *t_set_2);
static void print_failure(Interaction *interaction, T *t_set, uint64_t delta, std::set<int> *dif);
static void print_singles(Factor **factors, uint64_t num_factors);
static void print_interactions(std::vector<Interaction*> interactions);
static void print_sets(std::vector<T*> sets);
static void print_debug(Factor **factors, uint64_t num_factors);

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Interaction::Interaction()
{
    id = -1;
    is_covered = false;
    is_detectable = false;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a premade vector of Single pointers
*/
Interaction::Interaction(std::vector<Single*> *temp) : Interaction::Interaction()
{
    // fencepost start: let the Interaction be the strength 1 interaction involving just the 0th Single in temp
    singles.insert(temp->at(0));
    rows = temp->at(0)->rows;

    // fencepost loop: for any t > 1, rows of the Interaction is the intersection of each Single's rows
    for (uint64_t i = 1; i < temp->size(); i++) {
      singles.insert(temp->at(i));
      std::set<int> temp_set;
      std::set_intersection(rows.begin(), rows.end(),
        temp->at(i)->rows.begin(), temp->at(i)->rows.end(), std::inserter(temp_set, temp_set.begin()));
      rows = temp_set;
    }
}

/* UTILITY METHOD: to_string - gets a string representation of the Interaction
 * 
 * returns:
 * - a string representing the Interaction
 *  --> This is not to be used for printing; rather, it is for mapping unique strings to their Interactions
*/
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
    is_locatable = false;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a premade vector of Interaction pointers
*/
T::T(std::vector<Interaction*> *temp) : T::T()
{
    // fencepost start: let the Interaction be the strength 1 interaction involving just the 0th Single in s
    interactions.insert(temp->at(0));
    rows = temp->at(0)->rows;

    // fencepost loop: for any t > 1, rows of the Interaction is the intersection of each Single's rows
    for (uint64_t i = 1; i < temp->size(); i++) {
      interactions.insert(temp->at(i));
      std::set<int> temp_set;
      std::set_union(rows.begin(), rows.end(),
        temp->at(i)->rows.begin(), temp->at(i)->rows.end(), std::inserter(temp_set, temp_set.begin()));
      rows = temp_set;
    }

    // next, give all involved Interactions a reference to this set and this set a reference to its Singles
    for (Interaction *interaction : *temp) {
        interaction->sets.insert(this);
        for (Single *single : interaction->singles) singles.push_back(single);
    }
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Array::Array()
{
    total_problems = 0;
    coverage_problems = 0; location_problems = 0; detection_problems = 0;
    score = 0;
    d = 0; t = 0; delta = 0;
    v = v_off; o = normal; p = all;
    num_tests = 0; num_factors = 0;
    factors = nullptr;
    is_covering = false; is_locating = false; is_detecting = false;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a pointer to a Parser object
*/
Array::Array(Parser *in) : Array::Array()
{
    srand(time(nullptr));   // seed rand() using current time
    d = in->d; t = in->t; delta = in->delta;
    v = in->v; o = in->o; p = in->p;
    num_tests = in->num_rows;
    num_factors = in->num_cols;
    dont_cares = new prop_mode[num_factors]{none};
    permutation = new int[num_factors];
    for (uint64_t col = 0; col < num_factors; col++) permutation[col] = col;
    
    if (o != silent) printf("Building internal data structures....\n\n");
    try {
        // build all Singles, associated with an array of Factors
        factors = new Factor*[num_factors];
        for (uint64_t i = 0; i < num_factors; i++) {
            factors[i] = new Factor(i, in->levels.at(i), new Single*[in->levels.at(i)]);
            for (uint64_t j = 0; j < factors[i]->level; j++)
                factors[i]->singles[j] = new Single(i, j);
        }
        if (v == v_on) print_singles(factors, num_factors);

        // build all Interactions
        std::vector<Single*> temp_singles;
        build_t_way_interactions(0, t, &temp_singles);
        if (v == v_on) print_interactions(interactions);
        total_problems += interactions.size();  // to account for all the coverage issues
        coverage_problems += interactions.size();
        score += interactions.size();   // the array is considered completed when this reaches 0
        if (p == c_only) return;    // no need to spend effort building Ts if they won't be used

        // build all Ts
        std::vector<Interaction*> temp_interactions;
        build_size_d_sets(0, d, &temp_interactions);
        if (v == v_on) print_sets(sets);
        total_problems += sets.size();  // to account for all the location issues
        location_problems += sets.size();
        score += sets.size();   // need to update this
        if (p != all) return;   // can skip the following stuff if not doing detection

        // build all Interactions' maps of detection issues to their deltas (row difference magnitudes)
        for (Interaction *i : interactions) {   // for all Interactions in the array
            for (T *t_set : sets)   {// for every T set this Interaction is NOT part of
                if (i->sets.find(t_set) == i->sets.end()) {
                    i->deltas.insert({t_set, 0});
                    for (Single *s: i->singles) {
                        s->d_issues += delta;
                        total_problems += delta;
                        score += delta;
                    }
                }
            }
        }
        total_problems += interactions.size();  // to account for all the detection issues
        detection_problems += interactions.size();
        score += interactions.size();   // need to update this one last time

    } catch (const std::bad_alloc& e) {
        printf("ERROR: not enough memory to work with given array for given arguments\n");
        exit(1);
    }
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
 * - void, but after the method finishes, the array's interactions vector will be initialized
*/
void Array::build_t_way_interactions(uint64_t start, uint64_t t_cur, std::vector<Single*> *singles_so_far)
{
    // base case: interaction is completed and ready to store
    if (t_cur == 0) {
        Interaction *new_interaction = new Interaction(singles_so_far);
        interactions.push_back(new_interaction);
        interaction_map.insert({new_interaction->to_string(), new_interaction});    // for later accessing
        for (Single *single : new_interaction->singles) {
            single->c_issues++;
            total_problems++;
            score++;
        }
        return;
    }

    // recursive case: need to introduce another loop for higher strength
    for (uint64_t col = start; col < num_factors - t_cur + 1; col++) {
        for (uint64_t level = 0; level < factors[col]->level; level++) {
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
 * - void, but after the method finishes, the array's sets set will be initialized
*/
void Array::build_size_d_sets(uint64_t start, uint64_t d_cur, std::vector<Interaction*> *interactions_so_far)
{
    // base case: set is completed and ready to store
    if (d_cur == 0) {
        T *new_set = new T(interactions_so_far);
        sets.push_back(new_set);
        return;
    }

    // recursive case: need to introduce another loop for higher magnitude
    for (uint64_t i = start; i < interactions.size() - d_cur + 1; i++) {
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
 * - row: integer array representing a row up for consideration for appending to the array
 * - row_interactions: initially empty set to hold the Interactions as they are recovered
 * - start: left side of row at which to begin the for loop
 * - t_cur: distance from right side of row at which to end the for loop
 * - key: auxiliary vector of pointers used to track the current combination of Singles
 * 
 * returns:
 * - void, but after the method finishes, the row_interactions set will hold all the interactions in the row
*/
void Array::build_row_interactions(int *row, std::set<Interaction*> *row_interactions,
    uint64_t start, uint64_t t_cur, std::string key)
{
    if (t_cur == 0) {
        row_interactions->insert(interaction_map.at(key));
        return;
    }

    for (uint64_t col = start; col < num_factors - t_cur + 1; col++) {
        std::string cur = key + "f" + std::to_string(col) + "," + std::to_string(row[col]);
        build_row_interactions(row, row_interactions, col+1, t_cur-1, cur);
    }
}

/* SUB METHOD: add_row - adds a new row to the array using some predictive and scoring logic
 * - initializes a row with a greedy approach, then tweaks till good enough
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::add_row()
{
    uint64_t prev_score = score;
    int *new_row = new int[num_factors]{0};
    for (uint64_t size = num_factors; size > 0; size--) {
        int rand_idx = rand() % static_cast<int>(size);
        int temp = permutation[size - 1];
        permutation[size - 1] = permutation[rand_idx];
        permutation[rand_idx] = temp;
    }   // at this point, permutation should be shuffled

    // greedily select the values that appear to need the most attention
    for (uint64_t col = 0; col < num_factors; col++) {

        // assume 0 is the worst to start, then check if any others are worse
        Single *worst_single = factors[permutation[col]]->singles[0];
        int worst_score = static_cast<int64_t>(worst_single->c_issues) + worst_single->l_issues + 
            3*static_cast<int64_t>(worst_single->d_issues);
        for (uint64_t val = 1; val < factors[permutation[col]]->level; val++) {
            Single *cur_single = factors[permutation[col]]->singles[val];
            int cur_score = static_cast<int64_t>(cur_single->c_issues) + cur_single->l_issues +
                3*static_cast<int64_t>(cur_single->d_issues);
            if (cur_score > worst_score || (cur_score == worst_score && rand() % 2 == 0)) {
                worst_single = cur_single;
                worst_score = cur_score;
            }
        }
        new_row[permutation[col]] = worst_single->value;
        
        if (dont_cares[permutation[col]] == none && worst_single->c_issues == 0)
            dont_cares[permutation[col]] = c_only;
        if (p != c_only && dont_cares[permutation[col]] == c_only && worst_single->l_issues == 0)
            dont_cares[permutation[col]] = c_and_l;
        if (p == all && dont_cares[permutation[col]] == c_and_l && worst_single->d_issues == 0)
            dont_cares[permutation[col]] = all;
        
        if ((p == all && dont_cares[permutation[col]] == all) ||
            (p == c_and_l && dont_cares[permutation[col]] == c_and_l) ||
            (p == c_only && dont_cares[permutation[col]] == c_only))
            new_row[permutation[col]] = static_cast<uint64_t>(rand()) % factors[permutation[col]]->level;
    }   // entire row is now initialized based on the greedy approach
    
    std::set<Interaction*> row_interactions;
    build_row_interactions(new_row, &row_interactions, 0, t, "");
    tweak_row(new_row, &row_interactions);  // next, go and score this decision, modifying values as needed
    row_interactions.clear(); build_row_interactions(new_row, &row_interactions, 0, t, ""); // rebuild
    update_array(new_row, &row_interactions);
}

/* SUB METHOD: add_random_row - adds a randomly generated row to the array without scoring it
 * - should only ever be called to initialize the first row of a brand new array from scratch
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::add_random_row()
{
    int *new_row = get_random_row();
    std::set<Interaction*> row_interactions;
    build_row_interactions(new_row, &row_interactions, 0, t, "");
    update_array(new_row, &row_interactions);
}

/* HELPER METHOD: get_random_row - creates a randomly generated row without any logic to tweak it
 * 
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
int *Array::get_random_row()
{
    int *new_row = new int[num_factors];
    for (uint64_t i = 0; i < num_factors; i++)
        new_row[i] = static_cast<uint64_t>(rand()) % factors[i]->level;
    return new_row;
}

/* SUB METHOD: update_array - updates data structures to reflect changes caused by adding a new row
 * 
 * parameters:
 * - row: integer array representing a row that should be added to the array
 * - row_interactions: set containing all Interactions present in the new row
 * - keep: boolean representing whether or not the changes are intended to be kept
 *  --> true by default; when false, score changes are kept but the row itself is not added
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::update_array(int *row, std::set<Interaction*> *row_interactions, bool keep)
{
    rows.push_back(row);
    if (v == v_on && keep) {
        printf(">Pushed row:\t");
        for (uint64_t i = 0; i < num_factors; i++)
            printf("%d\t", row[i]);
        printf("\n");
    }
    num_tests++;

    std::set<T*> row_sets;  // all T sets that occur in this row
    for (Interaction *i : *row_interactions) {
        for (Single *s: i->singles) s->rows.insert(num_tests); // add the row to Singles in this Interaction
        i->rows.insert(num_tests); // add the row to this Interaction itself
        for (T *t_set : i->sets) {
            t_set->rows.insert(num_tests);  // add the row to all T sets this Interaction is part of
            row_sets.insert(t_set);         // also add the T set to row_sets
        }
    }
    
    // coverage and detection are associated with interactions
    for (Interaction *i : *row_interactions) {
        // coverage
        if (!i->is_covered) {   // if true, this Interaction just became covered
            i->is_covered = true;
            for (Single *s: i->singles) {
                s->c_issues--;
                score--;
            }
            score--;    // array score improves for the solved coverage problem
            if (--coverage_problems == 0) is_covering = true;
        }

        // detection
        if (p == all) { // the following is only done if we care about detection
            if (i->is_detectable) continue; // can skip all this checking if already detectable
            i->is_detectable = true;    // about to set it back to false if anything is unsatisfied still
            // updating detection issues for this Interaction:
            std::set<T*> other_sets = row_sets; // this will hold all row T sets this Interaction is NOT in
            for (T *t_set : i->sets) other_sets.erase(t_set);
            for (T *t_set : other_sets) {   // for every T set in this row that this Interaction is not in,
                if (i->deltas.at(t_set) <= static_cast<int64_t>(delta))
                    for (Single *s: i->singles) {
                        s->d_issues++;  // to balance out a -- later
                        score++;
                    }
                i->deltas.at(t_set)--;  // to balance out all deltas getting ++ after this
            }
            for (auto& kv : i->deltas) {    // for all T sets,
                kv.second++;    // increase their separation; offset by the -- earlier for T sets in this row
                if (kv.second < static_cast<int64_t>(delta)) i->is_detectable = false;    // separation still not high enough
                if (kv.second <= static_cast<int64_t>(delta)) // detection issue heading towards solved for all Singles involved
                    for (Single *s: i->singles) {
                        s->d_issues--;
                        score--;
                    }
            }
            if (i->is_detectable) { // if true, this Interaction just became detectable
                score--;    // array score improves for the solved detection problem
                if (--detection_problems == 0) is_detecting = true;
            }
        }
    }

    // location is associated with sets of interactions
    if (p != c_only && !is_locating) {  // the following is only done if we care about location
        for (T *t1 : row_sets) {    // for every T set in this row,
            if (t1->is_locatable) continue;
            if (t1->rows.size() == 1) {   // if true, this is the first time the set has been added, so
                for (T *t2 : row_sets) {    // for every other T set in this row,
                    if (t1 == t2 || t2->rows.size() > 1) continue;  // (skip when either of these is true)
                    t1->location_conflicts.insert(t2);  // can assume there is a location conflict
                    for (Single *s: t1->singles) {  // scores actually worsen here
                        s->l_issues++;
                        score++;
                        total_problems++;
                    }
                }
            } else {    // need to check if location issues were solved
                std::set<T*> temp = t1->location_conflicts; // make a deep copy (for mutating), and
                uint64_t solved = 0;
                std::set<T*> others;
                for (T *t2 : t1->location_conflicts)    // for every T set in the current T's conflicts,
                    if (row_sets.find(t2) == row_sets.end()) {  // if the conflicting set is not in this row,
                        temp.erase(t2); // it is no longer an issue for the current T
                        solved++;
                        if (t2->location_conflicts.erase(t1) == 1) {    // vice versa:
                            for (Single *s : t2->singles) { // conflicting T also had a location issue solved
                                s->l_issues--;
                                score--;
                            }
                            if (t2->location_conflicts.size() == 0) {   // if true,
                                t2->is_locatable = true;    // conflicting T just became locatable
                                score--;    // array score improves for the solved location problem
                                location_problems--;
                                if (location_problems == 0) is_locating = true;
                            }
                        }
                    }
                for (Single *s: t1->singles) {  // update scores
                    s->l_issues -= solved;
                    score -= solved;
                }
                t1->location_conflicts = temp;  // mutating completed, can update original now
                if (t1->location_conflicts.size() == 0) {   // if true,
                    t1->is_locatable = true;    // this T just became locatable
                    score--;    // array score improves for the solved location problem
                    location_problems--;
                    if (location_problems == 0) is_locating = true;
                }
            }
        }
    }

    // note: if keep == false, then caller must store previous score and coverage, location, and detection
    // issue counts for array and individual Singles; after this method completes, caller should restore them
    if (!keep) {
        for (Interaction *i : *row_interactions) {
            for (Single *s: i->singles) s->rows.erase(num_tests);
            i->rows.erase(num_tests);
        }
        for (T *t_set : row_sets) t_set->rows.erase(num_tests);
        num_tests--;
        rows.pop_back();
    }
}

/* UTILITY METHOD: to_string - gets a string representation of the array
 * 
 * returns:
 * - a string representing the array
*/
std::string Array::to_string()
{
    std::string ret = "";
    for (int *row : rows) {
        for (uint64_t i = 0; i < num_factors; i++)
            ret += std::to_string(row[i]) + '\t';
        ret += '\n';
    }
    return ret;
}

/* DECONSTRUCTOR - frees memory
*/
Array::~Array()
{
    for (uint64_t i = 0; i < num_tests; i++) delete[] rows[i];
    for (uint64_t i = 0; i < num_factors; i++) delete factors[i];
    delete[] factors;
    for (Interaction *i : interactions) delete i;
    for (T *t_set : sets) delete t_set;
    delete[] dont_cares;
    delete[] permutation;
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

static void print_failure(Interaction *interaction, T *t_set, uint64_t delta, std::set<int> *dif)
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

static void print_singles(Factor **factors, uint64_t num_factors)
{
    int pid = getpid();
    printf("\n==%d== Listing all Singles below:\n\n", pid);
    for (uint64_t col = 0; col < num_factors; col++) {
        printf("Factor %lu:\n", factors[col]->id);
        for (uint64_t level = 0; level < factors[col]->level; level++) {
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
        t_set->id = ++i;
        printf("Set %d:\n\tSet: {", i);
        for (Interaction *interaction : t_set->interactions) printf(" %d", interaction->id);
        printf(" }\n\tRows: {");
        for (int row : t_set->rows) printf(" %d", row);
        printf(" }\n\n");
    }
}

static void print_debug(Factor **factors, uint64_t num_factors)
{
    for (uint64_t col = 0; col < num_factors; col++) {
        for (uint64_t val = 0; val < factors[col]->level; val++) {
            Single *s = factors[col]->singles[val];
            printf("DEBUG: for (f%lu, %lu):\n\t%lu c_issues\n\t%ld l_issues\n\t%lu d_issues\n\n",
                col, val, s->c_issues, s->l_issues, s->d_issues);
        }
    }
}