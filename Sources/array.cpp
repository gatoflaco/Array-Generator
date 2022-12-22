/* Array-Generator by Isaac Jung
Last updated 12/22/2022

|===========================================================================================================|
|   This file contains the meat of the project's logic. The constructor for the Array class takes a pointer |
| to a Parser object which should have already called its process_input() method. Using that, the Array     |
| object under construction is able to organize the data structures that support the generation of an array |
| with the desired properties. The add_row() method can then be called repeatedly, with the score property  |
| reflecting how close the array is to satisfying the desired properties. When the Array's score is 0, it   |
| means that all desired properties are satisfied. Note that the add_row() method assumes there is at least |
| one row already added to the Array; then, the module in charge of calling add_row() should add at least   |
| one row by some other method before entering any loop that calls add_row(), if randomness is desired.     |
| For this reason, there is an add_random_row() method that adds a completely random row without scoring    |
| the goodness of the choice. This method should only be called for adding the first row of the Array.      |
| All scoring and tracking of data structures, counts, etc. which influence decisions is self-contained;    |
| the user of the Array object need not concern itself with these details.                                  |
|=====================================KT======================================================================|
*/

#include "array.h"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

// method forward declarations
static void print_singles(Factor **factors, uint16_t num_factors);
static void print_interactions(std::vector<Interaction*> interactions);
static void print_sets(std::vector<T*> sets);

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a premade vector of Single pointers
*/
Interaction::Interaction(std::vector<Single*> *temp) : str_rep(this->to_string_internal(temp))
{
    for (uint64_t i = 0; i < temp->size(); i++) singles.push_back(temp->at(i));
}

/* UTILITY METHOD: to_string - gets a string representation of the Interaction
 * 
 * returns:
 * - a string representing the Interaction
 *  --> This is not to be used for printing; rather, it is for mapping unique strings to their Interactions
*/
std::string Interaction::to_string() const
{
    return str_rep;
}

/* HELPER METHOD: to_string_internal - called by the constructor to build the string representation
 * - do not call outside of the constructor
 * 
 * returns:
 * - a string representing the Interaction
*/
std::string Interaction::to_string_internal(std::vector<Single*> *temp) const
{
    std::string ret = "";
    for (Single *single : *temp) ret += single->to_string();
    return ret;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a premade vector of Interaction pointers
*/
T::T(std::vector<Interaction*> *temp) : str_rep(this->to_string_internal(temp))
{
    for (uint64_t i = 0; i < temp->size(); i++) interactions.push_back(temp->at(i));

    // next, give all involved Interactions a reference to this set and this set a reference to its Singles
    for (Interaction *interaction : *temp) {
        interaction->sets.insert(this);
        for (Single *single : interaction->singles) singles.push_back(single);
    }
}

/* UTILITY METHOD: to_string - gets a string representation of the T set
 * 
 * returns:
 * - a string representing the T set
 *  --> This is not to be used for printing; rather, it is for mapping unique strings to their T sets
*/
std::string T::to_string() const
{
    return str_rep;
}

/* HELPER METHOD: to_string_internal - called by the constructor to build the string representation
 * - do not call outside of the constructor
 * 
 * returns:
 * - a string representing the T set
*/
std::string T::to_string_internal(std::vector<Interaction*> *temp) const
{
    std::string ret = "";
    for (Interaction *interaction : *temp) ret += interaction->to_string();
    return ret;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Array::Array()
{
    debug = d_off;
    total_problems = 0;
    coverage_problems = 0; location_problems = 0; detection_problems = 0;
    score = 0;
    d = 0; t = 0; delta = 0;
    num_tests = 0; num_factors = 0;
    factors = nullptr;
    v = v_off; o = normal; p = all;
    heuristic_in_use = none;
    is_covering = false; is_locating = false; is_detecting = false;
    dont_cares = nullptr;
    permutation = nullptr;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on a pointer to a Parser object
*/
Array::Array(Parser *in) : Array::Array()
{
    srand(time(nullptr));   // seed rand() using current time
    d = in->d; t = in->t; delta = in->delta;
    num_tests = 0;  // previously in->num_rows, but generation always starts from 0 rows
    num_factors = in->num_cols;
    dont_cares = new prop_mode[num_factors]{none};
    permutation = new uint16_t[num_factors];
    for (uint16_t col = 0; col < num_factors; col++) permutation[col] = col;
    debug = in->debug; v = in->v; o = in->o; p = in->p;
    
    if (o != silent) printf("Building internal data structures....\n");
    if (debug == d_on) printf("==%d== max_threads is %d\n", getpid(), max_threads);
    try {
        // build all Singles, associated with an array of Factors
        factors = new Factor*[num_factors];
        for (uint16_t i = 0; i < num_factors; i++) {
            factors[i] = new Factor(i, in->levels.at(i), new Single*[in->levels.at(i)]);
            for (uint16_t j = 0; j < factors[i]->level; j++) {
                factors[i]->singles[j] = new Single(i, j);
                singles.push_back(factors[i]->singles[j]);
                single_map.insert({factors[i]->singles[j]->to_string(), factors[i]->singles[j]});
            }
        }
        if (debug == d_on) print_singles(factors, num_factors);

        // build all Interactions
        std::vector<Single*> temp_singles;
        build_t_way_interactions(0, t, &temp_singles);
        if (debug == d_on) print_interactions(interactions);
        total_problems += interactions.size();  // to account for all the coverage problems
        coverage_problems += interactions.size();
        score += interactions.size();   // the array is considered completed when this reaches 0
        if (p == c_only) return;    // no need to spend effort building Ts if they won't be used

        // build all Ts
        std::vector<Interaction*> temp_interactions;
        build_size_d_sets(0, d, &temp_interactions);
        if (debug == d_on) print_sets(sets);
        for (T *t_set : sets) {
            for (Single *s : t_set->singles) {
                factors[s->factor]->l_issues += sets.size();
                s->l_issues += sets.size();
                total_problems += sets.size();
            }
            for (T *other_set : sets) {
                if (t_set == other_set) continue;
                t_set->location_conflicts.insert(other_set);
            }
        }
        total_problems += sets.size();  // to account for all the location problems
        location_problems += sets.size();
        score = total_problems; // need to update this
        if (p != all) return;   // can skip the following stuff if not doing detection

        // build all Interactions' maps of detection issues to their deltas (row difference magnitudes)
        for (Interaction *i : interactions) {   // for all Interactions in the array
            for (T *t_set : sets) { // for every T set this Interaction is NOT part of
                if (i->sets.find(t_set) == i->sets.end()) {
                    i->deltas.insert({t_set, 0});
                    for (Single *s: i->singles) {
                        factors[s->factor]->d_issues += delta;
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

    } catch (const std::bad_alloc &e) {
        printf("ERROR: not enough memory to work with given array for given arguments\n");
        exit(1);
    }
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its private fields based on existing data
 *  --> intended to be used ONLY BY Array::clone()
*/
Array::Array(uint64_t total_problems_o, uint64_t coverage_problems_o, uint64_t location_problems_o,
    uint64_t detection_problems_o, std::vector<uint16_t*> *rows_o, uint64_t num_tests_o,
    uint16_t num_factors_o, Factor **factors_o, prop_mode p_o, uint16_t d_o, uint16_t t_o, uint16_t delta_o):
    Array::Array()
{
    total_problems = total_problems_o;
    coverage_problems = coverage_problems_o;
    location_problems = location_problems_o;
    detection_problems = detection_problems_o;
    d = d_o; t = t_o; delta = delta_o;
    num_tests = num_tests_o; num_factors = num_factors_o;
    o = silent; p = p_o;
    memory_mutex.lock();
    try {
        for (uint16_t *row_o : *rows_o) {
            uint16_t *row = new uint16_t[num_factors];
            for (uint16_t col = 0; col < num_factors; col++) row[col] = row_o[col];
            rows.push_back(row);
        }
        factors = new Factor*[num_factors];
        for (uint16_t i = 0; i < num_factors; i++) {
            factors[i] = new Factor(i, factors_o[i]->level, new Single*[factors_o[i]->level]);
            for (uint16_t j = 0; j < factors[i]->level; j++) {
                factors[i]->singles[j] = new Single(i, j);
                singles.push_back(factors[i]->singles[j]);
                single_map.insert({factors[i]->singles[j]->to_string(), factors[i]->singles[j]});
            }
        }
        std::vector<Single*> temp_singles;
        build_t_way_interactions(0, t, &temp_singles);
        if (p == c_only) return;
        std::vector<Interaction*> temp_interactions;
        build_size_d_sets(0, d, &temp_interactions);
    } catch (const std::bad_alloc &e) { // give up and free memory for now, caller can wait for other threads
        for (uint64_t i = 0; i < num_tests; i++) delete[] rows[i];
        for (uint16_t i = 0; i < num_factors; i++) delete factors[i];
        delete[] factors;
        for (Interaction *i : interactions) delete i;
        for (T *t_set : sets) delete t_set;
        delete[] dont_cares;
        delete[] permutation;
        scores_mutex.unlock();
        throw e;
    }
    scores_mutex.unlock();
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
void Array::build_t_way_interactions(uint16_t start, uint16_t t_cur, std::vector<Single*> *singles_so_far)
{
    // base case: interaction is completed and ready to store
    if (t_cur == 0) {
        Interaction *new_interaction = new Interaction(singles_so_far);
        if (!new_interaction) throw std::bad_alloc();   // will unwind to original caller who should handle
        interactions.push_back(new_interaction);
        interaction_map.insert({new_interaction->to_string(), new_interaction});    // for later accessing
        for (Single *single : new_interaction->singles) {
            factors[single->factor]->c_issues++;
            single->c_issues++;
            total_problems++;
            score++;
        }
        return;
    }

    // recursive case: need to introduce another loop for higher strength
    uint16_t end = num_factors - t_cur + 1;
    for (uint16_t col = start; col < end; col++) {
        for (uint16_t level = 0; level < factors[col]->level; level++) {
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
void Array::build_size_d_sets(uint16_t start, uint16_t d_cur, std::vector<Interaction*> *interactions_so_far)
{
    // base case: set is completed and ready to store
    if (d_cur == 0) {
        T *new_set = new T(interactions_so_far);
        if (!new_set) throw std::bad_alloc();   // will unwind to original caller who should handle
        sets.push_back(new_set);
        t_set_map.insert({new_set->to_string(), new_set});  // for later accessing
        return;
    }

    // recursive case: need to introduce another loop for higher magnitude
    for (uint16_t i = start; i < interactions.size() - d_cur + 1; i++) {
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
void Array::build_row_interactions(uint16_t *row, std::set<Interaction*> *row_interactions,
    uint16_t start, uint16_t t_cur, std::string key)
{
    if (t_cur == 0) {
        row_interactions->insert(interaction_map.at(key));
        return;
    }

    uint16_t end = num_factors - t_cur + 1;
    for (uint16_t col = start; col < end; col++) {
        std::string cur = key + "f" + std::to_string(col) + "," + std::to_string(row[col]);
        build_row_interactions(row, row_interactions, col+1, t_cur-1, cur);
    }
}

/* UTILITY METHOD: print_stats - outputs current state of the Array to console
 * - output details vary depending on what flags are set
 * 
 * parameters:
 * - initial: whether this is the introductory 
 * 
 * returns:
 * - void, but after the method finishes, the row_interactions set will hold all the interactions in the row
*/
void Array::print_stats(bool initial)
{
    if (o != silent) {
        if (initial) {
            if (score == 0) {
                printf("\nThe partial array already meets all requirements, no rows need to be added.\n");
                return;
            }
            printf("\nThere are %lu total problems to solve.\n", total_problems);
            if (o == normal) printf("\nArray score is currently %lu.\n", score);
            else printf("\nArray score is currently %lu, adding row #%lu.\n", score, num_tests+1);
        } else {
            if (score == 0) {
                printf("\nCompleted array with %lu rows.\n\n", num_tests);
                return;
            }
            if (o == normal) printf("\nArray score is currently %lu.\n", score);
            else printf("\nArray score is currently %lu, adding row #%lu.\n", score, num_tests+1);
        }
    }
    if (v == v_on) {
        uint64_t c_score = coverage_problems, l_score = location_problems, d_score = detection_problems;
        for (Single *s : singles) {
            c_score += s->c_issues;
            l_score += s->l_issues;
            d_score += s->d_issues;
        }
        printf("\t- Current coverage score: %lu\n", c_score);
        if (p != c_only) printf("\t- Current location score: %lu\n", l_score);
        if (p == all) printf("\t- Current detection score: %lu\n", d_score);
        if (!initial) printf("\t- The array is now at %.4f%% completion.\n",
            static_cast<float>((total_problems - score))/total_problems*100);
    }
    if (o == normal) printf("Adding row #%lu.\n", num_tests+1);
    if (v == v_on) {
        if (heuristic_in_use == c_only) printf("\t- Using heuristic_c_only.\n");
        else if (heuristic_in_use == l_only) printf("\t- Using heuristic_l_only.\n");
        else if (heuristic_in_use == l_and_d) printf("\t- Using heuristic_l_and_d.\n");
        else if (heuristic_in_use == d_only) printf("\t- Using heuristic_d_only.\n");
        else if (heuristic_in_use == all) printf("\t- Using heuristic_all.\n");
    }
}

/* SUB METHOD: update_array - updates data structures to reflect changes caused by adding a new row
 * 
 * parameters:
 * - row: integer array representing a row that should be added to the array
 * - keep: boolean representing whether or not the changes are intended to be kept
 *  --> true by default; when false, score changes are kept but the row itself is not added
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::update_array(uint16_t *row, bool keep)
{
    rows.push_back(row);
    if (o == normal && keep) {
        printf("> Pushed row:\t");
        for (uint16_t i = 0; i < num_factors; i++) printf("%hu\t", row[i]);
        printf("\n");
    }
    num_tests++;

    std::set<Interaction*> row_interactions;    // all Interactions that occur in this row
    build_row_interactions(row, &row_interactions, 0, t, "");
    std::set<T*> row_sets;  // all T sets that occur in this row
    for (Interaction *i : row_interactions) {
        for (Single *s: i->singles) s->rows.insert(num_tests); // add the row to Singles in this Interaction
        i->rows.insert(num_tests);          // add the row to this Interaction itself
        for (T *t_set : i->sets) {
            t_set->rows.insert(num_tests);  // add the row to all T sets this Interaction is part of
            row_sets.insert(t_set);         // also add the T set to row_sets
        }
    }
    
    update_scores(&row_interactions, &row_sets);
    if (!keep) {
        num_tests--;
        rows.pop_back();
        return;
    }
    update_dont_cares();
    if (heuristic_in_use != all) {
        std::string row_str = std::to_string(row[0]);   // string representation of the row
        for (uint16_t col = 1; col < num_factors; col++)
            row_str += ' ' + std::to_string(row[col]);
        row_scores[row_str] = delta <= 1 ? 1 : UINT64_MAX;  // will allow heuristic_all to skip some work
    }
    update_heuristic();
}

/* HELPER METHOD: update_scores - updates overall scores as well as for individual Singles, Interactions, Ts
 * 
 * parameters:
 * - row_interactions: set containing all Interactions present in the new row
 * - row_sets: set containing all T sets present in the new row
 * 
 * returns:
 * - void, but after the method finishes, scores will be updated
 *  --> additionally, all Singles, Interactions, and Ts will have their data structures updated accordingly
*/
void Array::update_scores(std::set<Interaction*> *row_interactions, std::set<T*> *row_sets)
{
    // coverage and detection are associated with interactions
    for (Interaction *i : *row_interactions) {
        // coverage
        if (!i->is_covered) {   // if true, this Interaction just became covered
            i->is_covered = true;
            for (Single *s: i->singles) {
                factors[s->factor]->c_issues--;
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
            std::set<T*> other_sets = *row_sets;    // will hold all row T sets this Interaction is NOT in
            for (T *t_set : i->sets) other_sets.erase(t_set);
            for (T *t_set : other_sets) {   // for every T set in this row that this Interaction is not in,
                if (i->deltas.at(t_set) <= delta)
                    for (Single *s: i->singles) {
                        factors[s->factor]->d_issues++;
                        s->d_issues++;  // to balance out a -- later
                        score++;
                    }
                i->deltas.at(t_set)--;  // to balance out all deltas getting ++ after this
            }
            for (auto &kv : i->deltas) {    // for all T sets,
                kv.second++;    // increase their separation; offset by the -- earlier for T sets in this row
                if (kv.second < delta) i->is_detectable = false;    // separation still not high enough
                if (kv.second <= delta) // detection issue heading towards solved for all Singles involved
                    for (Single *s: i->singles) {
                        factors[s->factor]->d_issues--;
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
        for (T *t1 : *row_sets) {   // for every T set in this row,
            if (t1->is_locatable) continue;
            if (t1->rows.size() == 1) {   // if true, this is the first time the set has been added, so
                for (Single *s : t1->singles) {
                    factors[s->factor]->l_issues -= sets.size();
                    s->l_issues -= sets.size();
                    score -= sets.size();
                }
                t1->location_conflicts.clear();
                for (T *t2 : *row_sets) {   // for every other T set in this row,
                    if (t1 == t2 || t2->rows.size() > 1) continue;  // (skip when either of these is true)
                    t1->location_conflicts.insert(t2);  // can assume there is a location conflict
                    for (Single *s: t1->singles) {  // scores actually worsen here
                        factors[s->factor]->l_issues++;
                        s->l_issues++;
                        score++;
                    }
                }
            } else {    // need to check if location issues were solved
                std::set<T*> temp = t1->location_conflicts; // make a shallow copy (for mutating), and
                uint64_t solved = 0;
                std::set<T*> others;
                for (T *t2 : t1->location_conflicts)    // for every T set in the current T's conflicts,
                    if (row_sets->find(t2) == row_sets->end()) {    // if that set is not in this row,
                        temp.erase(t2); // it is no longer an issue for the current T
                        solved++;
                        if (t2->location_conflicts.erase(t1) == 1) {    // vice versa:
                            for (Single *s : t2->singles) { // conflicting T also had a location issue solved
                                factors[s->factor]->l_issues--;
                                s->l_issues--;
                                score--;
                            }
                            if (t2->location_conflicts.size() == 0) {   // if true,
                                t2->is_locatable = true;    // conflicting T just became locatable
                                score--;    // array score improves for the solved location problem
                                location_problems--;
                                if (location_problems == 0) {
                                    printf("ERROR: Unexpected behavior here, rerun in debug mode\n");
                                    exit(-1);
                                }
                            }
                        } else {
                            printf("ERROR: Unexpected behavior here, rerun in debug mode\n");
                            exit(-1);
                        }
                    }
                for (Single *s: t1->singles) {  // update scores
                    factors[s->factor]->l_issues -= solved;
                    s->l_issues -= solved;
                    score -= solved;
                }
                t1->location_conflicts = temp;  // mutating completed, can update original now
            }
            if (t1->location_conflicts.size() == 0) {   // if true,
                t1->is_locatable = true;    // this T just became locatable
                score--;    // array score improves for the solved location problem
                location_problems--;
                if (location_problems == 0) is_locating = true;
            }
        }
    }
}

/* HELPER METHOD: update_dont_cares - updates column-total information to track don't care states
 *  --> should only call when adding (and keeping) a row, after update_scores() is called
 * 
 * returns:
 * - void, but after the method finishes, don't cares will be updated
*/
void Array::update_dont_cares()
{
    for (uint16_t col = 0; col < num_factors; col++) {
        if (dont_cares[col] == none && factors[col]->c_issues == 0) {
            dont_cares[col] = c_only;
            if (debug == d_on) printf("==%d== All coverage issues associated with factor %hu are solved\n",
                getpid(), col);
        }
        if (p != c_only && dont_cares[col] == c_only && factors[col]->l_issues == 0) {
            dont_cares[col] = c_and_l;
            if (debug == d_on) printf("==%d== All location issues associated with factor %hu are solved\n",
                getpid(), col);
        }
        if (p == all && dont_cares[col] == c_and_l && factors[col]->d_issues == 0) {
            dont_cares[col] = all;
            if (debug == d_on) printf("==%d== All detection issues associated with factor %hu are solved\n",
                getpid(), col);
        }
    }
}

/* HELPER METHOD: update_heuristic - looks at overall states and decides whether to switch heuristics
 *  --> should only call when adding (and keeping) a row, after update_scores() is called
 * 
 * returns:
 * - void, but after the method finishes, heuristic_in_use may have changed
*/
void Array::update_heuristic()
{
    just_switched_heuristics = true;    // assume true until determined to be false
    float ratio = static_cast<float>(score)/total_problems;

    // first up, should we use the most in-depth scoring function:
    if (p == c_only) {
        if (heuristic_in_use != all && total_problems < 20000)
            heuristic_in_use = all;
        else if (heuristic_in_use == d_only && ratio < 0.20 && score < 100000)
            heuristic_in_use = all;
        else if (heuristic_in_use == c_only && ratio < 0.40 && score < 500000)
            heuristic_in_use = d_only;  // small misnomer
        else if (heuristic_in_use == none)
            heuristic_in_use = c_only;
        else just_switched_heuristics = false;
        return;
    }
    
    if (p == c_and_l) {
        if (heuristic_in_use != all && total_problems < 15000)
            heuristic_in_use = all;
        else if (heuristic_in_use == d_only && ratio < 0.15 && score < 75000)
            heuristic_in_use = all;
        else if (heuristic_in_use == l_only && ratio < 0.30 && score < 250000)
            heuristic_in_use = d_only;  // small misnomer
        else if (heuristic_in_use == c_only && ratio < 0.80 && score < 750000)
            heuristic_in_use = l_only;
        else if (heuristic_in_use == none)
            heuristic_in_use = c_only;
        else just_switched_heuristics = false;
        return;
    }

    if (p == all) {
        if (heuristic_in_use != all && total_problems < 10000)
            heuristic_in_use = all;
        else if (heuristic_in_use == d_only && ratio < 0.10 && score < 50000)
            heuristic_in_use = all;
        else if (heuristic_in_use == l_and_d && ratio < 0.20 && score < 100000)
            heuristic_in_use = d_only;
        else if (heuristic_in_use == l_only && ratio < 0.60 && score < 500000)
            heuristic_in_use = l_and_d;
        else if (heuristic_in_use == c_only && ratio < 0.85 && score < 1000000)
            heuristic_in_use = l_only;
        else if (heuristic_in_use == none)
            heuristic_in_use = c_only;
        else just_switched_heuristics = false;
        return;
    }
}

Array *Array::clone()
{
    // instantiate with private fields, copy public fields manually
    Array *clone;
    try {
        clone = new Array(total_problems, coverage_problems, location_problems, detection_problems,
            &rows, num_tests, num_factors, factors, p, d, t, delta);
    } catch (const std::bad_alloc &e) { // when there isn't enough memory to create a clone
        return nullptr;
    }
    clone->score = score;
    clone->is_covering = is_covering;
    clone->is_locating = is_locating;
    clone->is_detecting = is_detecting;

    // brand new Singles, Interactions, and Ts had to be allocated, so deep copying of data needed
    for (Single *this_s : singles) {
        Single *clone_s = clone->single_map.at(this_s->to_string());
        clone_s->rows = this_s->rows;
        clone_s->c_issues = this_s->c_issues;
        clone_s->l_issues = this_s->l_issues;
        clone_s->d_issues = this_s->d_issues;
    }
    for (Interaction *this_i : interactions) {
        Interaction *clone_i = clone->interaction_map.at(this_i->to_string());
        clone_i->rows = this_i->rows;
        clone_i->is_covered = this_i->is_covered;
        clone_i->is_detectable = this_i->is_detectable;
        for (auto &kv : this_i->deltas) {
            T *clone_t = clone->t_set_map.at(kv.first->to_string());
            clone_i->deltas.insert({clone_t, kv.second});
        }
    }   // it hurts my brain so much to try and understand what's happening here lol
    for (T *this_t : sets) {
        T *clone_t = clone->t_set_map.at(this_t->to_string());
        clone_t->rows = this_t->rows;
        clone_t->is_locatable = this_t->is_locatable;
        for (T *other_t : this_t->location_conflicts) {
            T *clone_other_t = clone->t_set_map.at(other_t->to_string());
            clone_t->location_conflicts.insert(clone_other_t);
        }
    }

    return clone;
}

/* UTILITY METHOD: report_out_of_memory - prints an error message and sets out_of_memory to true
 * 
 * returns:
 * - void, but after the method finishes, the out_of_memory field will be switched on
 *  --> note that it should never be reset to false after calling this method, and caller should
 *      attempt to exit gracefully as quickly as possible
*/
void Array::report_out_of_memory()
{
    printf("\n\t-- ERROR --\n");
    printf("\tOut of memory.\n");
    printf("\tThe problem size is too large for the current environment.\n");
    printf("\tYou may be able to succeed using an environment with more RAM.\n\n");
    out_of_memory = true;
}

/* UTILITY METHOD: to_string - gets a string representation of the array
 * 
 * returns:
 * - a string representing the array
*/
std::string Array::to_string()
{
    std::string ret = "";
    for (uint16_t *row : rows) {
        for (uint16_t i = 0; i < num_factors; i++)
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
    for (uint16_t i = 0; i < num_factors; i++) delete factors[i];
    delete[] factors;
    for (Interaction *i : interactions) delete i;
    for (T *t_set : sets) delete t_set;
    delete[] dont_cares;
    delete[] permutation;
}

// ==============================   LOCAL HELPER METHODS BELOW THIS POINT   ============================== //

static void print_singles(Factor **factors, uint16_t num_factors)
{
    uint32_t pid = getpid();
    printf("\n==%d== Listing all Singles below:\n\n", pid);
    for (uint16_t col = 0; col < num_factors; col++) {
        printf("Factor %hu:\n", factors[col]->id);
        for (uint16_t level = 0; level < factors[col]->level; level++) {
            printf("\t(f%hu, %hu): {", col, factors[col]->singles[level]->value);
            for (uint64_t row : factors[col]->singles[level]->rows) printf(" %lu", row);
            printf(" }\n");
        }
        printf("\n");
    }
}

static void print_interactions(std::vector<Interaction*> interactions)
{
    uint32_t pid = getpid();
    printf("\n==%d== Listing all Interactions below:\n\n", pid);
    uint32_t i = 0;
    for (Interaction *interaction : interactions) {
        interaction->id = ++i;
        printf("Interaction %u:\n\tInt: {", i);
        for (Single *s : interaction->singles) printf(" (f%hu, %hu)", s->factor, s->value);
        printf(" }\n\tRows: {");
        for (uint64_t row : interaction->rows) printf(" %lu", row);
        printf(" }\n\n");
    }
}

static void print_sets(std::vector<T*> sets)
{
    uint32_t pid = getpid();
    printf("\n==%d== Listing all Ts below:\n\n", pid);
    uint32_t i = 0;
    for (T *t_set : sets) {
        t_set->id = ++i;
        printf("Set %u:\n\tSet: {", i);
        for (Interaction *interaction : t_set->interactions) printf(" %u", interaction->id);
        printf(" }\n\tRows: {");
        for (uint64_t row : t_set->rows) printf(" %lu", row);
        printf(" }\n\n");
    }
}