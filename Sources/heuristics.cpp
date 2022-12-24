/* Array-Generator by Isaac Jung
Last updated 12/23/2022

|===========================================================================================================|
|   This file contains definitions for methods belonging to the Array class which are declared in array.h.  |
| Specifically, any methods involving heuristics for scoring and altering a potential row to add are found  |
| here. The only reason they are not in array.cpp is for organization.                                      |
|===========================================================================================================|
*/

#include "array.h"
#include <sstream>
#include <unistd.h>
#include <algorithm>

/* SUB METHOD: add_row - adds a new row to the array using some predictive and scoring logic
 * - simply an interface for adding a row; method itself simply decides which heuristic to use
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::add_row()
{
    // choose a new random order for the column iterations this round
    for (uint16_t size = num_factors; size > 0; size--) {
        uint16_t rand_idx = rand() % size;
        uint16_t temp = permutation[size - 1];
        permutation[size - 1] = permutation[rand_idx];
        permutation[rand_idx] = temp;
    }   // at this point, permutation should be shuffled

    // choose how to initialize the new row based on current heuristic to be used
    uint16_t *new_row;
    Interaction *locked_interaction = nullptr;
    T *locked_set = nullptr;
    switch (heuristic_in_use) {
        case c_only:
        case c_and_l:
        case c_and_d:
            new_row = initialize_row_S();
            heuristic_c_only(new_row);
            break;
        case l_only:
            new_row = initialize_row_T(&locked_set, &locked_interaction);
            heuristic_l_only(new_row, locked_set, locked_interaction);
            break;
        case l_and_d:
            new_row = initialize_row_I(&locked_interaction);
            heuristic_l_and_d(new_row, locked_interaction);
            break;
        case d_only:
            new_row = initialize_row_R(&locked_interaction);
            if (!heuristic_all(new_row, locked_interaction)) {
                report_out_of_memory();
                //if (some flag) don't actually stop
                return;
            }
            break;
        case all:
            new_row = initialize_row_R();
            if (!heuristic_all(new_row)) {
                report_out_of_memory();
                // if (some flag) don't actually stop
                return;
            }
            break;
        case none:
        default:
            new_row = initialize_row_R();
            break;
    }   // at this point, new row should be initialized with values
    
    // tweak the row based on the current heuristic and then add to the array
    update_array(new_row);
}

/* SUB METHOD: add_row - adds a new row to the array
 * * - overloaded: this version takes an existing row and adds it
 * 
 * parameters:
 * - row: pointer to start of array representing new row to be added
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::add_row(uint16_t *row)
{
    uint16_t *new_row = new uint16_t[num_factors];
    for (uint16_t idx = 0; idx < num_factors; idx++) new_row[idx] = row[idx];
    update_array(new_row);
}

/* SUB METHOD: initialize_row_R - creates a randomly generated row
 * 
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
uint16_t *Array::initialize_row_R()
{
    uint16_t *new_row = new uint16_t[num_factors];
    for (uint16_t i = 0; i < num_factors; i++)
        new_row[i] = rand() % factors[i]->level;
    return new_row;
}

/* SUB METHOD: initialize_row_R - creates a randomly generated row
 * - overloaded: this version chooses a locked Interaction based on Single issues
 * 
 * parameters:
 * - locked: pointer to Interaction* that will drive a scoring heuristic later
 *  --> should be nullptr when passed in as a parameter; this method will assign the value
 * - ties: pointer to vector that will containin all Interactions that tie when scored by this method
 *  --> has a default value of nullptr, which will cause the method to break ties on its own, randomly
 *  --> caller should pass address of local vector when intending to further analyze ties
 * 
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
uint16_t *Array::initialize_row_R(Interaction **locked, std::vector<Interaction*> *ties)
{
    uint16_t *new_row = initialize_row_R();

    uint64_t worst_count = 0;
    std::vector<Interaction*> worst_interactions;   // there could be ties for the worst
    std::vector<Interaction*> *to_use = ties;   // assume ties will hold the worst interactions
    if (!to_use) to_use = &worst_interactions;  // if ties is nullptr, just use local
    for (Interaction *interaction : interactions) {
        uint64_t cur_count = 4*(num_tests - interaction->rows.size());  // bias towards picking unsued ones
        for (Single *s : interaction->singles)
            cur_count += s->c_issues + s->l_issues + s->d_issues;
        if (cur_count >= worst_count) {     // worse or tied
            if (cur_count > worst_count) {  // strictly worse
                worst_count = cur_count;
                to_use->clear();
            }
            to_use->push_back(interaction);
        }
    }
    if (ties && to_use->size() > 1) return new_row; // when caller intends to judge ties itself

    // choose the interaction with most Single issues (for ties, choose randomly from among those tied)
    *locked = to_use->at(static_cast<uint64_t>(rand()) % to_use->size());
    for (Single *s : (*locked)->singles) new_row[s->factor] = s->value;
    if (debug == d_on) printf("==%d== Locking interaction %s\n", getpid(), (*locked)->to_string().c_str());
    return new_row;
}

/* SUB METHOD: initialize_row_S - creates a row by considering which Singles have the most issues
 * 
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
uint16_t* Array::initialize_row_S()
{
    uint16_t *new_row = new uint16_t[num_factors]{0};

    // greedily select the values that appear to need the most attention
    for (uint16_t col = 0; col < num_factors; col++) {
        // check if column is don't care
        if ((p == all && dont_cares[permutation[col]] == all) ||
            (p == c_and_l && dont_cares[permutation[col]] == c_and_l) ||
            (p == c_only && dont_cares[permutation[col]] == c_only)) {
            new_row[permutation[col]] = rand() % factors[permutation[col]]->level;
            continue;
        }
        // assume 0 is the worst to start, then check if any others are worse
        Single *worst_single = factors[permutation[col]]->singles[0];
        uint64_t worst_score = worst_single->c_issues/3 + worst_single->l_issues/2 + worst_single->d_issues;
        for (uint16_t val = 1; val < factors[permutation[col]]->level; val++) {
            Single *cur_single = factors[permutation[col]]->singles[val];
            uint64_t cur_score = cur_single->c_issues/3 + cur_single->l_issues/2 + cur_single->d_issues;
            if (cur_score > worst_score || (cur_score == worst_score && rand() % 2 == 0)) {
                worst_single = cur_single;
                worst_score = cur_score;
            }
        }
        new_row[permutation[col]] = worst_single->value;
    }   // entire row is now initialized based on the greedy approach
    return new_row;
}

/* SUB METHOD: initialize_row_T - creates a row by considering which T sets have the most location conflicts
 * 
 * parameters:
 * - l_set: pointer to T* that will drive a scoring heuristic later
 *  --> *l_set should be nullptr when passed in as a parameter; this method will assign the value
 * - l_interaction: pointer to Interaction* that will drive a scoring heuristic later
 *  --> *l_interaction should be nullptr when passed in as a parameter; this method will assign the value
 *
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
uint16_t *Array::initialize_row_T(T **l_set, Interaction **l_interaction)
{
    std::vector<Interaction*> ties;
    uint16_t *new_row = initialize_row_R(l_interaction, &ties);
    
    uint64_t worst_count = 0;
    std::vector<T*> worst_sets, working_sets;
    for (T *t_set : sets) { // for all T sets,
        std::vector<Interaction*> *vec = &t_set->interactions;
        for (Interaction *i : ties) // for each Interaction in the list of candidates by issues,
            if (std::find(vec->begin(), vec->end(), i) != vec->end()) { // if this set contains one of those,
                working_sets.push_back(t_set);  // keep this set in the list of potential choices
                break;
            }
    }
    for (T *t_set : working_sets) {
        if (t_set->location_conflicts.size() >= worst_count) {      // worse or tied
            if (t_set->location_conflicts.size() > worst_count) {   // strictly worse
                worst_count = t_set->location_conflicts.size();
                worst_sets.clear();
            }
            worst_sets.push_back(t_set);
        }
    }

    // choose the set with most conflicts (for ties, choose randomly from among those tied)
    *l_set = worst_sets.at(static_cast<uint64_t>(rand()) % worst_sets.size());
    if (ties.size() == 1) {
        if (debug == d_on) printf("==%d== Locking t_set %s\n", getpid(), (*l_set)->to_string().c_str());
        return new_row;
    }

    *l_interaction = (*l_set)->interactions.at(static_cast<uint64_t>(rand()) % (*l_set)->interactions.size());
    for (Single *s : (*l_interaction)->singles) new_row[s->factor] = s->value;
    if (debug == d_on) {
        printf("==%d== Locking interaction %s\n", getpid(), (*l_interaction)->to_string().c_str());
        printf("==%d== Locking t_set %s\n", getpid(), (*l_set)->to_string().c_str());
    }
    return new_row;
}

/* SUB METHOD: initialize_row_I - creates a row by considering which Interactions have the lowest separation
 * 
 * parameters:
 * - locked: pointer to Interaction* that will drive a scoring heuristic later
 *  --> should be nullptr when passed in as a parameter; this method will assign the value

 * returns:
 * - a pointer to the first element in the array that represents the row
*/
uint16_t *Array::initialize_row_I(Interaction **locked)
{
    std::vector<Interaction*> ties;
    uint16_t *new_row = initialize_row_R(locked, &ties);
    if (ties.size() == 1) return new_row;   // no ties; initialize_row_R already locked an Interaction

    uint64_t worst_count = 0;
    std::vector<Interaction*> worst_interactions;   // there could be ties for the worst
    for (Interaction *interaction : ties) {
        uint64_t cur_count = 0;
        for (auto &kv : interaction->deltas)
            if (kv.second < delta) cur_count += delta - kv.second;
        if (cur_count >= worst_count) {     // worse or tied
            if (cur_count > worst_count) {  // strictly worse
                worst_count = cur_count;
                worst_interactions.clear();
            }
            worst_interactions.push_back(interaction);
        }
    }

    // choose the interaction with lowest separation (for ties, choose randomly from among those tied)
    *locked = worst_interactions.at(static_cast<uint64_t>(rand()) % worst_interactions.size());
    for (Single *s : (*locked)->singles) new_row[s->factor] = s->value;
    if (debug == d_on) printf("==%d== Locking interaction %s\n", getpid(), (*locked)->to_string().c_str());
    return new_row;
}

/* SUB METHOD: heuristic_c_only - lightweight heuristic that only concerns itself with coverage
 * - in the tradeoff between speed and better row choice, this heuristic is towards the speed extreme
 * - should only be used very early on in array construction
 *  --> can do well for longer when the desired array is simpler (i.e., covering as opposed to detecting)
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * 
 * returns:
 * - void, but after the method finishes, the row may be modified in an attempt to satisfy more issues
*/
void Array::heuristic_c_only(uint16_t *row)
{
    int32_t *problems = new int32_t[num_factors]{0};    // for counting how many "problems" each factor has
    prop_mode *dont_cares_c = new prop_mode[num_factors];   // local copy of the don't cares
    for (uint16_t col = 0; col < num_factors; col++) dont_cares_c[col] = dont_cares[col];

    std::set<Interaction*> row_interactions;
    build_row_interactions(row, &row_interactions, 0, t, "");
    for (Interaction *i : row_interactions) {
        if (i->rows.size() != 0) {  // Interaction is already covered
            bool can_skip = false;  // don't account for Interactions involving already-completed factors
            for (Single *s : i->singles)
                if (dont_cares_c[s->factor] != none) {
                    can_skip = true;
                    break;
                }
            if (can_skip) continue;
            for (Single *s : i->singles) // increment the problems counter for each Single involved
                problems[s->factor]++;
        } else {    // Interaction not covered; decrement the problems counters instead
            for (Single *s : i->singles) problems[s->factor]--;
        }
    }

    // find out what the worst score is among the factors
    int32_t max_problems = 0;   // largest value among all in the problems[] array created above
    for (uint16_t col = 0; col < num_factors; col++)
        if (problems[col] > max_problems) max_problems = problems[col];
    if (max_problems == 0) {    // row is good enough as is
        delete[] problems;
        delete[] dont_cares_c;
        return;
    }
    
    // else, try altering the value(s) with the most problems (whatever is currently contributing the least)
    int32_t cur_max = max_problems; // for comparing to max_problems to see if there is an improvement
    for (uint16_t col = 0; col < num_factors; col++) {  // go find any factors to change
        if (problems[permutation[col]] == max_problems) {   // found a factor to try altering
            int32_t *temp_problems = new int32_t[num_factors]{0};   // will be mutated by helper

            for (uint16_t i = 1; i < factors[permutation[col]]->level; i++) {   // try every possible value
                row[permutation[col]] = (row[permutation[col]] + 1) % factors[permutation[col]]->level;
                std::set<Interaction*> new_interactions;    // get the new Interactions
                build_row_interactions(row, &new_interactions, 0, t, "");

                cur_max = heuristic_c_helper(row, &new_interactions, temp_problems);    // test this change
                if (cur_max < max_problems) {   // this change improved the score, keep it
                    delete[] problems;
                    delete[] dont_cares_c;
                    delete[] temp_problems;
                    return;
                }
                cur_max = max_problems; // else this change was no good, reset and continue
            }
            delete[] temp_problems;
            row[permutation[col]] = (row[permutation[col]] + 1) % factors[permutation[col]]->level;
        }
    }

    // last resort, start looking for *anything* that is missing
    for (uint16_t col = 0; col < num_factors; col++) {  // for all factors
        if (dont_cares_c[permutation[col]] != none) continue;   // no need to check already completed factors
        bool improved = false;
        for (uint16_t i = 0; i < factors[permutation[col]]->level; i++) {   // try every possible value
            row[permutation[col]] = (row[permutation[col]] + 1) % factors[permutation[col]]->level;
            std::set<Interaction*> new_interactions;    // get the new Interactions
            build_row_interactions(row, &new_interactions, 0, t, "");

            improved = false;   // see if the change helped
            for (Interaction *interaction : new_interactions)
                if (interaction->rows.size() == 0) {    // the Interaction is not already covered
                    for (Single *s : interaction->singles) dont_cares_c[s->factor] = c_only;
                    improved = true;    // note: don't break, we want to set as many dont_cares_c as possible
                }
            if (improved) break;    // keep this factor as this value
        }
        if (improved) continue; // don't execute the next line
        row[permutation[col]] = rand() % factors[permutation[col]]->level;  // if not possible to improve
    }
    delete[] problems;
    delete[] dont_cares_c;
}

/* HELPER METHOD: heuristic_c_helper - performs redundant work for heuristic_c_only()
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - row_interactions: set containing all Interactions present in the row
 * - problems: pointer to start of array associating each column in the row with a score of sorts
 * 
 * returns:
 * - int representing the largest value in the problems array after scoring
*/
int32_t Array::heuristic_c_helper(uint16_t *row, std::set<Interaction*> *row_interactions, int32_t *problems)
{
    for (Interaction *i : *row_interactions) {
        if (i->rows.size() != 0) {  // Interaction is already covered
            bool can_skip = false;  // don't account for Interactions involving already-completed factors
            for (Single *s : i->singles)
                if (s->c_issues == 0) { // one of the Singles involved in the Interaction is completed
                    can_skip = true;
                    break;
                }
            if (can_skip) continue;
            for (Single *s : i->singles) // increment the problems counter for each Single involved
                problems[s->factor]++;
        } else {    // Interaction not covered; decrement the problems counters instead
            for (Single *s : i->singles) problems[s->factor]--;
        }
    }

    // find out what the worst score is among the factors
    int32_t max_problems = INT32_MIN;   // set max to a huge negative number to start
    for (uint16_t col = 0; col < num_factors; col++) {
        if (factors[col]->singles[row[col]]->c_issues == 0) continue;   // already completed factor
        if (problems[col] > max_problems) max_problems = problems[col];
    }
    return max_problems;
}

/* SUB METHOD: heuristic_l_only - middleweight heuristic that only concerns itself with location
 * - in the tradeoff between speed and better row choice, this heuristic is somewhere in the middle
 * - should be used when most, if not all, coverage problems have been solved
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - l_set: pointer to T set whose location conflicts will be used to pick column values
 * - l_interaction: pointer to Interaction whose Singles' columns should not be altered
 * 
 * returns:
 * - void, but after the method finishes, the row may be modified in an attempt to satisfy more issues
*/
void Array::heuristic_l_only(uint16_t *row, T* l_set, Interaction *l_interaction)
{
    // keep track of which columns should not be modified
    bool *locked_factors = new bool[num_factors]{false};
    for (Single *s : l_interaction->singles) locked_factors[s->factor] = true;

    std::map<std::string, uint64_t> scores; // create and initialize a map of every Single to a scoring
    for (uint16_t col = 0; col < num_factors; col++)
        for (uint16_t val = 0; val < factors[col]->level; val++)
            scores.insert({"f" + std::to_string(col) + "," + std::to_string(val), 0});
    
    for (T *conflict : l_set->location_conflicts)   // for every conflicting T set,
        for (Single *s : conflict->singles) // for every Single in that conflicting set,
            scores.at(s->to_string())++;    // increase the score of that Single

    // a larger value in the scores map means the Single is involved in more location conflicts
    for (uint16_t col = 0; col < num_factors; col++) {
        if (locked_factors[col]) continue;
        uint16_t best_val = rand() % factors[col]->level;
        uint64_t best_val_score = UINT64_MAX;
        for (uint16_t val = 0; val < factors[col]->level; val++) {
            uint64_t val_score = scores.at("f" + std::to_string(col) + "," + std::to_string(val));
            if (val_score < best_val_score) {
                best_val = val;
                best_val_score = val_score;
            }
        }
        if (best_val_score != 0) row[col] = best_val;   // else allow it to remain random
    }
    delete[] locked_factors;
}

/* SUB METHOD: heuristic_l_and_d - middleweight heuristic that only concerns itself with detection
 * - in the tradeoff between speed and better row choice, this heuristic is somewhere in the middle
 * - should be used when detection problems are mostly all that remain to be solved
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - locked: pointer to Interaction whose Singles' columns should not be altered
 * 
 * returns:
 * - void, but after the method finishes, the row may be modified in an attempt to satisfy more issues
*/
void Array::heuristic_l_and_d(uint16_t *row, Interaction *locked)
{
    // keep track of which columns should not be modified
    bool *locked_factors = new bool[num_factors]{false};
    for (Single *s : locked->singles) locked_factors[s->factor] = true;

    std::map<std::string, uint64_t> scores; // create and initialize a map of every Single to a scoring
    for (uint16_t col = 0; col < num_factors; col++)
        for (uint16_t val = 0; val < factors[col]->level; val++)
            scores.insert({"f" + std::to_string(col) + "," + std::to_string(val), 0});
    
    for (auto &kv : locked->deltas) {   // for every t set from which the locked interaction needs separation,
        if (kv.second >= delta) continue;   // (skip if separation is already sufficient)
        for (Single *s : kv.first->singles)                 // for every Single in that set,
            scores.at(s->to_string()) += delta - kv.second; // increase the score of that Single
    }

    // a larger value in the scores map means the Single is involved in more sets that need separation
    for (uint16_t col = 0; col < num_factors; col++) {
        if (locked_factors[col]) continue;
        uint16_t best_val = rand() % factors[col]->level;
        uint64_t best_val_score = UINT64_MAX;
        for (uint16_t val = 0; val < factors[col]->level; val++) {
            uint64_t val_score = scores.at("f" + std::to_string(col) + "," + std::to_string(val));
            if (val_score < best_val_score) {
                best_val = val;
                best_val_score = val_score;
            }
        }
        if (best_val_score != 0) row[col] = best_val;   // else allow it to remain random
    }
    delete[] locked_factors;
}

/* SUB METHOD: heuristic_all - heavyweight heuristic that tries to solve the most problems possible
 * - in the tradeoff between speed and better row choice, this heuristic is towards the row choice extreme
 * - does the deepest inspection of all the heuristics; therefore, should not be used till close to complete
 * 
 * parameters:
 * - row: integer array representing a row up for consideration for appending to the array
 * 
 * returns:
 * - bool representing whether the method succeeded
 * - additionally, the row will be altered such that it solves as many problems singlehandedly as possible
 *  --> note that this does not mean that running this for the whole array will guarantee the smallest array;
 *      this is still a greedy algorithm for the current row, without any lookahead to future rows
*/
bool Array::heuristic_all(uint16_t *row)
{
    // check if there is even enough memory to use this heuristic
    if (!probe_memory_for_threads()) return false;

    // get scores for all relevant possible rows
    std::vector<std::thread*> threads;
    heuristic_all_helper(row, 0, &threads);
    for (std::thread *cur_thread : threads) {
        cur_thread->join();
        delete cur_thread;
    }

    // inspect the scores for the best one(s)
    uint64_t best_score = 0;
    min_positive_score = UINT64_MAX;
    std::vector<std::string> best_rows; // there could be ties for the best
    for (auto &kv : row_scores) {
        if (kv.second >= best_score) {  // it was better or it tied
            if (kv.second > best_score) {   // for an even better choice, can stop tracking the previous best
                best_score = kv.second;
                best_rows.clear();
            }
            best_rows.push_back(kv.first);  // whether it was better or only a tie, keep track of this row
        }
        if (kv.second < min_positive_score) min_positive_score = kv.second;
    }
    if (min_positive_score == UINT64_MAX) min_positive_score = 0;   // shouldn't ever happen
    min_positive_score = 2*(min_positive_score + best_score)/3; // for next time, skip rows below this value
    if (min_positive_score == 0) min_positive_score = 1;

    // choose the row that scored the best (for ties, choose randomly from among those tied for the best)
    uint64_t choice = static_cast<uint64_t>(rand()) % best_rows.size();  // for breaking ties randomly
    std::stringstream choice_ss = std::stringstream(best_rows.at(choice));
    for (uint16_t col = 0; col < num_factors; col++)
        choice_ss >> row[col];
    
    row_scores[best_rows.at(choice)] = delta <= 1 ? 0 : min_positive_score - 1;
    return true;
}

/* SUB METHOD: heuristic_all - heavyweight heuristic that tries to solve the most problems possible
 * - overloaded; this version takes a locked Interaction and chooses only from rows containing it
 *  --> does less work than the original version, but may not find the row that would have scored best
 * 
 * parameters:
 * - row: integer array representing a row up for consideration for appending to the array
 * - locked: pointer to Interaction whose Singles' columns should not be altered
 * 
 * returns:
 * - bool representing whether the method succeeded
*/
bool Array::heuristic_all(uint16_t *row, Interaction *locked)
{
    // check if there is even enough memory to use this heuristic
    if (!probe_memory_for_threads()) return false;

    // get scores for all relevant possible rows
    std::vector<std::thread*> threads;
    std::map<std::string, uint64_t> local_scores;
    heuristic_all_helper(row, 0, &threads, locked, &local_scores);
    for (std::thread *cur_thread : threads) {
        cur_thread->join();
        delete cur_thread;
    }

    // inspect the scores for the best one(s)
    uint64_t best_score = 0;
    std::vector<std::string> best_rows; // there could be ties for the best
    for (auto &kv : local_scores) {
        if (kv.second >= best_score) {  // it was better or it tied
            if (kv.second > best_score) {   // for an even better choice, can stop tracking the previous best
                best_score = kv.second;
                best_rows.clear();
            }
            best_rows.push_back(kv.first);  // whether it was better or only a tie, keep track of this row
        }
    }

    // choose the row that scored the best (for ties, choose randomly from among those tied for the best)
    uint64_t choice = static_cast<uint64_t>(rand()) % best_rows.size();  // for breaking ties randomly
    std::stringstream choice_ss = std::stringstream(best_rows.at(choice));
    for (uint16_t col = 0; col < num_factors; col++)
        choice_ss >> row[col];
    return true;
}

/* HELPER METHOD: heuristic_all_helper - performs top-down recursive logic for heuristic_all()
 * - heuristic_all() does the auxilary work to start the recursion, and handle the result
 * - this method uses recursion to form all possible combinations; its base case scores a given combination
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - cur_col: which column should have its levels looped over in the recursive case
 *  --> overhead caller should pass 0 to this method initially
 *  --> value should increment by 1 with each recursive call
 *  --> triggers the base case when value is equal to the total number of columns
 * - threads: pointer to vector of pointers to threads, needed so caller can join threads later
 *  --> overhead caller should pass the address of an empty vector to this method initially
 *  --> a separate thread should be started for each row inspected by the base case
 * - locked: pointer to Interaction whose Singles' columns should not be altered
 *  --> has a default value of nullptr, meaning all rows will be scored by default
 * - local_scores: pointer to map in which scoring resultsshould be stored
 *  --> has a default value of nullptr; when nullptr, scores are stored into memoized field in Array.h
 * 
 * returns:
 * - void, but scores will be modified to contain all the rows inspected and their scores
*/
void Array::heuristic_all_helper(uint16_t *row, uint16_t cur_col, std::vector<std::thread*> *threads,
    Interaction *locked, std::map<std::string, uint64_t> *local_scores)
{
    // base case: row represents a unique combination and is ready for scoring
    if (cur_col == num_factors) {
        std::string row_str = std::to_string(row[0]); // string representation of the row
        for (uint16_t col = 1; col < num_factors; col++) row_str += ' ' + std::to_string(row[col]);
        if (just_switched_heuristics && heuristic_in_use == all) row_scores[row_str] += UINT64_MAX;
        if (heuristic_in_use == all && row_scores[row_str] < min_positive_score) return;
        uint16_t *row_copy = new uint16_t[num_factors]; // must be deleted by thread later
        for (uint16_t col = 0; col < num_factors; col++) row_copy[col] = row[col];
        if (threads->size() == max_threads) {
            for (std::thread *cur_thread : *threads) {
                cur_thread->join();
                delete cur_thread;
            }
            threads->clear();
        }
        std::thread *new_thread = new std::thread(&Array::heuristic_all_scorer, this, row_copy, row_str, local_scores);
        if (!new_thread) {
            for (std::thread *cur_thread : *threads) {
                cur_thread->join();
                delete cur_thread;
            }
            threads->clear();
            new_thread = new std::thread(&Array::heuristic_all_scorer, this, row_copy, row_str, local_scores);
        }
        
        threads->push_back(new_thread); // add the thread's reference to the threads vector
        return;
    }

    // recursive case: need to introduce another loop for the next factor
    if (locked) // if not nullptr, skip modifying this column if it is locked
        for (Single *s : locked->singles)
            if (s->factor == permutation[cur_col]) {
                heuristic_all_helper(row, cur_col+1, threads, locked, local_scores);
                return;
            }
    for (uint16_t offset = 0; offset < factors[permutation[cur_col]]->level; offset++) {
        uint16_t temp = row[permutation[cur_col]];
        row[permutation[cur_col]] = (row[permutation[cur_col]] + offset) %
            factors[permutation[cur_col]]->level;   // try every value for this factor
        heuristic_all_helper(row, cur_col+1, threads, locked, local_scores);
        row[permutation[cur_col]] = temp;
    }
}

/* HELPER METHOD: heuristic_all_scorer - scores a given row by testing what would change if it was added
 * - should be called in a unique thread
 * - heuristic_all() should await the termination of all sub threads before inspecting scores
 * 
 * parameters:
 * - row: integer array representing a row needing scoring
 *  --> pass nullptr when probing whether there is enough memory to start threads
 * - row_str: string representation of the row
 *  --> pass "dummy" when probing whether there is enough memory to start threads
 * - local_scores: pointer to map in which scoring resultsshould be stored
 *  --> has a default value of nullptr; when nullptr, scores are stored into memoized field in Array.h
 * 
 * returns:
 * - void, but scores may be updates
*/
void Array::heuristic_all_scorer(uint16_t *row, std::string row_str,
    std::map<std::string, uint64_t> *local_scores)
{
    if (row_str.compare("dummy") == 0) return;  // see method header for explanation

    // current thread will work with unique copies of the data structures being modified
    Array *copy = nullptr;
    while (!copy) copy = clone();   // if out of memory, try waiting for other threads to finish
    copy->update_array(row, false); // see how all scores, etc., would change

    // define the row score to be the combination of net changes below, weighted by importance
    uint64_t row_score = 0;
    for (Single *this_s : singles) { // improve the score based on individual Single improvement
        Single *copy_s = copy->single_map.at(this_s->to_string());
        uint64_t weight = static_cast<uint64_t>(factors[this_s->factor]->level);    // higher level factors hold more weight
        row_score += (this_s->c_issues - copy_s->c_issues)*weight/3;
        row_score += (this_s->l_issues - copy_s->l_issues)*weight/2;
        row_score += (this_s->d_issues - copy_s->d_issues)*weight;
    }
    delete copy;

    if (debug == d_on) {
        std::stringstream thread_output;
        thread_output << "==" << std::this_thread::get_id() << "== For row [" << row[0];
        for (uint16_t col = 1; col < num_factors; col++) thread_output << " " << row[col];
        thread_output << "], score is " << row_score << std::endl;
        scores_mutex.lock();
        printf("%s", thread_output.str().c_str());
        scores_mutex.unlock();
    }
    delete[] row;

    // need to add result to data structure containing all thread's results; use mutex for thread safety
    if (local_scores) { // if not nullptr, do not permanently memoize these scores, simply store to local map
        scores_mutex.lock();
        (*local_scores)[row_str] = row_score;
        scores_mutex.unlock();
    } else {
        scores_mutex.lock();
        row_scores[row_str] = row_score;
        scores_mutex.unlock();
    }
}

/* UTILITY METHOD: probe_memory_for_threads - checks if there is enough memory for heuristic_all() to execute
 * 
 * returns:
 * - bool representing whether there is enough memory that heuristic_all() could work
*/
bool Array::probe_memory_for_threads()
{
    Array *copy = clone();  // this operation will have to be performed by at least one thread at a time
    if (!copy) return false;

    uint16_t *row_copy = new uint16_t[num_factors]; // same as above
    if (!row_copy) {
        delete copy;
        return false;
    }

    // also need to check if there is enough memory for at least one thread that would be using a copy
    std::vector<std::thread*> threads;
    for (uint32_t count = 0; count < max_threads; count++) {
        std::thread *temp = new std::thread(&Array::heuristic_all_scorer, this, nullptr, "dummy", nullptr);
        if (!temp) {
            delete copy;
            delete[] row_copy;
            for (std::thread *thread : threads) {
                thread->join();
                delete thread;
            }
            return false;
        }
        threads.push_back(temp);
    }

    // if made it to this point, success
    delete copy;
    delete[] row_copy;
    for (std::thread *thread : threads) {
        thread->join();
        delete thread;
    }
    return true;
}