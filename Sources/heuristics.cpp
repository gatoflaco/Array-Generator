/* Array-Generator by Isaac Jung
Last updated 10/29/2022

|===========================================================================================================|
|   This file contains definitions for methods belonging to the Array class which are declared in array.h.  |
| Specifically, any methods involving heuristics for scoring and altering a potential row to add are found  |
| here. The only reason they are not in array.cpp is for organization.                                      |
|===========================================================================================================|
*/

#include "array.h"
#include <sstream>

/* SUB METHOD: add_row - adds a new row to the array using some predictive and scoring logic
 * - simply an interface for adding a row; method itself simply decides which heuristic to use
 * 
 * returns:
 * - void, but after the method finishes, the array will have a new row appended to its end
*/
void Array::add_row()
{
    // choose a new random order for the column iterations this round
    for (uint64_t size = num_factors; size > 0; size--) {
        int rand_idx = rand() % static_cast<int>(size);
        int temp = permutation[size - 1];
        permutation[size - 1] = permutation[rand_idx];
        permutation[rand_idx] = temp;
    }   // at this point, permutation should be shuffled

    // choose how to initialize the new row based on current heuristic to be used
    int *new_row;
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
        case l_and_d:
            new_row = initialize_row_T(&locked_set, &locked_interaction);
            heuristic_l_only(new_row, locked_set, locked_interaction);
            break;
        case d_only:
            new_row = initialize_row_I(&locked_interaction);
            heuristic_d_only(new_row, locked_interaction);
            break;
        case all:
            new_row = initialize_row_R();
            heuristic_all(new_row);
            break;
        case none:
        default:
            new_row = initialize_row_R();
            break;
    }   // at this point, new row should be initialized with values
    
    // tweak the row based on the current heuristic and then add to the array
    update_array(new_row);
}

/* SUB METHOD: initialize_row_R - creates a randomly generated row
 * 
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
int *Array::initialize_row_R()
{
    int *new_row = new int[num_factors];
    for (uint64_t i = 0; i < num_factors; i++)
        new_row[i] = static_cast<uint64_t>(rand()) % factors[i]->level;
    return new_row;
}

/* SUB METHOD: initialize_row_S - creates a row by considering which Singles have the most issues
 * 
 * returns:
 * - a pointer to the first element in the array that represents the row
*/
int* Array::initialize_row_S()
{
    int *new_row = new int[num_factors]{0};

    // greedily select the values that appear to need the most attention
    for (uint64_t col = 0; col < num_factors; col++) {
        // check if column is don't care
        if ((p == all && dont_cares[permutation[col]] == all) ||
            (p == c_and_l && dont_cares[permutation[col]] == c_and_l) ||
            (p == c_only && dont_cares[permutation[col]] == c_only)) {
            new_row[permutation[col]] = static_cast<uint64_t>(rand()) % factors[permutation[col]]->level;
            continue;
        }
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
int *Array::initialize_row_T(T **l_set, Interaction **l_interaction)
{
    int *new_row = initialize_row_R();
    
    int64_t worst_count = INT64_MIN;
    std::vector<T*> worst_sets; // there could be ties for the worst
    for (T *t_set : sets) {
        if (static_cast<int64_t>(t_set->location_conflicts.size()) >= worst_count) {    // worse or tied
            if (static_cast<int64_t>(t_set->location_conflicts.size()) > worst_count) { // strictly worse
                worst_count = t_set->location_conflicts.size();
                worst_sets.clear();
            }
            worst_sets.push_back(t_set);
        }
    }

    // choose the set with most conflicts (for ties, choose randomly from among those tied)
    *l_set = worst_sets.at(static_cast<uint64_t>(rand()) % worst_sets.size());
    *l_interaction = (*l_set)->interactions.at(static_cast<uint64_t>(rand()) % (*l_set)->interactions.size());
    for (Single *s : (*l_interaction)->singles) new_row[s->factor] = s->value;
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
int *Array::initialize_row_I(Interaction **locked)
{
    int *new_row = initialize_row_R();
    
    int64_t worst_count = INT64_MIN;
    std::vector<Interaction*> worst_interactions;   // there could be ties for the worst
    for (Interaction *interaction : interactions) {
        int64_t cur_count = 0;
        for (auto &kv : interaction->deltas)
            if (kv.second < static_cast<int64_t>(delta)) cur_count += delta - kv.second;
        if (cur_count >= worst_count) { // worse or tied
            if (cur_count > worst_count) {
                worst_count = cur_count;
                worst_interactions.clear();
            }
            worst_interactions.push_back(interaction);
        }
    }

    // choose the interaction with lowest separation (for ties, choose randomly from among those tied)
    *locked = worst_interactions.at(static_cast<uint64_t>(rand()) % worst_interactions.size());
    for (Single *s : (*locked)->singles) new_row[s->factor] = s->value;
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
void Array::heuristic_c_only(int *row)
{
    int *problems = new int[num_factors]{0};    // for counting how many "problems" each factor has
    int max_problems;   // largest value among all in the problems[] array created above
    int cur_max;    // for comparing to max_problems to see if there is an improvement
    prop_mode *dont_cares_c = new prop_mode[num_factors];   // local copy of the don't cares
    for (uint64_t col = 0; col < num_factors; col++) dont_cares_c[col] = dont_cares[col];

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
    max_problems = 0;
    for (uint64_t col = 0; col < num_factors; col++)
        if (problems[col] > max_problems) max_problems = problems[col];
    if (max_problems == 0) {    // row is good enough as is
        delete[] problems;
        delete[] dont_cares_c;
        return;
    }
    
    // else, try altering the value(s) with the most problems (whatever is currently contributing the least)
    cur_max = max_problems;
    for (uint64_t col = 0; col < num_factors; col++) {  // go find any factors to change
        if (problems[permutation[col]] == max_problems) {   // found a factor to try altering
            int *temp_problems = new int[num_factors]{0};   // deep copy problems[] because it will be mutated

            for (uint64_t i = 1; i < factors[permutation[col]]->level; i++) {   // for every value
                row[permutation[col]] = (row[permutation[col]] + 1) %
                    static_cast<int64_t>(factors[permutation[col]]->level); // try that value
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
            row[permutation[col]] = (row[permutation[col]] + 1) % static_cast<int>(factors[col]->level);
        }
    }

    // last resort, start looking for *anything* that is missing
    for (uint64_t col = 0; col < num_factors; col++) {  // for all factors
        if (dont_cares_c[permutation[col]] != none) continue;   // no need to check already completed factors
        bool improved = false;
        for (uint64_t i = 0; i < factors[permutation[col]]->level; i++) {   // for every value
            row[permutation[col]] = (row[permutation[col]] + 1) %
                static_cast<int64_t>(factors[permutation[col]]->level); // try that value
            std::set<Interaction*> new_interactions;    // get the new Interactions
            build_row_interactions(row, &new_interactions, 0, t, "");

            improved = false;   // see if the change helped
            for (Interaction *interaction : new_interactions)
                if (interaction->rows.size() == 0) {    // the Interaction is not already covered
                    for (Single *s : interaction->singles) dont_cares_c[s->factor] = c_only;
                    improved = true;
                }
            if (improved) break;    // keep this factor as this value
        }
        if (improved) continue;
        row[permutation[col]] = static_cast<uint64_t>(rand()) % factors[permutation[col]]->level;
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
int Array::heuristic_c_helper(int *row, std::set<Interaction*> *row_interactions, int *problems)
{
    for (Interaction *i : *row_interactions) {
        if (i->rows.size() != 0) {  // Interaction is already covered
            bool can_skip = false;  // don't account for Interactions involving already-completed factors
            for (Single *s : i->singles)
                if (s->c_issues == 0) { // one of the Singles involved in the Interaction is completed
                    // TODO: make this check more than just c_issues?
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
    int max_problems = INT32_MIN;   // set max to a huge negative number to start
    for (uint64_t col = 0; col < num_factors; col++) {
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
void Array::heuristic_l_only(int *row, T* l_set, Interaction *l_interaction)
{
    // keep track of which columns should not be modified
    bool *locked_factors = new bool[num_factors]{false};
    for (Single *s : l_interaction->singles) locked_factors[s->factor] = true;

    std::map<std::string, uint64_t> scores; // create and initialize a map of every Single to a scoring
    for (uint64_t col = 0; col < num_factors; col++)
        for (uint64_t val = 0; val < factors[col]->level; val++)
            scores.insert({"f" + std::to_string(col) + "," + std::to_string(val), 0});
    
    for (T *conflict : l_set->location_conflicts)   // for every conflicting T set,
        for (Single *s : conflict->singles) // for every Single in that conflicting set,
            scores.at(s->to_string())++;    // increase the score of that Single

    // a larger value in the scores map means the Single is involved in more location conflicts
    for (uint64_t col = 0; col < num_factors; col++) {
        if (locked_factors[col]) continue;
        uint64_t best_val = static_cast<uint64_t>(rand()) % factors[col]->level;
        uint64_t best_val_score = UINT64_MAX;
        for (uint64_t val = 0; val < factors[col]->level; val++) {
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

/* SUB METHOD: heuristic_d_only - middleweight heuristic that only concerns itself with detection
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
void Array::heuristic_d_only(int *row, Interaction *locked)
{
    // keep track of which columns should not be modified
    bool *locked_factors = new bool[num_factors]{false};
    for (Single *s : locked->singles) locked_factors[s->factor] = true;

    std::map<std::string, uint64_t> scores; // create and initialize a map of every Single to a scoring
    for (uint64_t col = 0; col < num_factors; col++)
        for (uint64_t val = 0; val < factors[col]->level; val++)
            scores.insert({"f" + std::to_string(col) + "," + std::to_string(val), 0});
    
    for (auto &kv : locked->deltas) {   // for every t set from which the locked interaction needs separation,
        if (kv.second >= static_cast<int64_t>(delta)) continue; // (skip if separation is already sufficient)
        for (Single *s : kv.first->singles)                 // for every Single in that set,
            scores.at(s->to_string()) += delta - kv.second; // increase the score of that Single
    }

    // a larger value in the scores map means the Single is involved in more sets that need separation
    for (uint64_t col = 0; col < num_factors; col++) {
        if (locked_factors[col]) continue;
        uint64_t best_val = static_cast<uint64_t>(rand()) % factors[col]->level;
        uint64_t best_val_score = UINT64_MAX;
        for (uint64_t val = 0; val < factors[col]->level; val++) {
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
 * - none, but the row will be altered such that it solves as many problems singlehandedly as possible
 *  --> note that this does not mean that running this for the whole array will guarantee the smallest array;
 *      this is still a greedy algorithm for the current row, without any lookahead to future rows
*/
void Array::heuristic_all(int *row)
{
    // get scores for all relevant possible rows
    std::vector<std::thread*> threads;
    heuristic_all_helper(row, 0, &threads);
    for (std::thread *cur_thread : threads) {
        cur_thread->join();
        delete cur_thread;
    }

    // inspect the scores for the best one(s)
    uint64_t best_score = 0;
    std::vector<std::string> best_rows; // there could be ties for the best
    for (auto &kv : row_scores) {
        if (kv.second >= best_score) {  // it was better or it tied
            if (kv.second > best_score) {   // for an even better choice, can stop tracking the previous best
                //for (int *r : best_rows) delete[] r;
                best_score = kv.second;
                best_rows.clear();
            }
            best_rows.push_back(kv.first);  // whether it was better or only a tie, keep track of this row
        } //else delete[] kv.first;
    }

    // choose the row that scored the best (for ties, choose randomly from among those tied for the best)
    uint64_t choice = static_cast<uint64_t>(rand()) % best_rows.size();  // for breaking ties randomly
    std::stringstream choice_ss = std::stringstream(best_rows.at(choice));
    for (uint64_t col = 0; col < num_factors; col++)
        choice_ss >> row[col];
        //row[col] = best_rows.at(choice)[col];
    
    // free memory
    //for (int *r : best_rows) delete[] r;
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
 * 
 * returns:
 * - none, but scores will be modified to contain all the rows inspected and their scores
*/
void Array::heuristic_all_helper(int *row, uint64_t cur_col, std::vector<std::thread*> *threads)
{
    // base case: row represents a unique combination and is ready for scoring
    if (cur_col == num_factors) {
        std::string row_str = std::to_string(row[0]); // string representation of the row
        for (uint64_t col = 1; col < num_factors; col++)
            row_str += ' ' + std::to_string(row[col]);
        //if (!just_switched_heuristics && row_scored_zero.at(row_as_str)) return;
        if (!(just_switched_heuristics || row_scores[row_str])) return; // skips if score is 0 already
        int *row_copy = new int[num_factors];   // must be deleted by heuristic_all() later
        for (uint64_t col = 0; col < num_factors; col++)
            row_copy[col] = row[col];
        std::thread *new_thread = new std::thread(&Array::heuristic_all_scorer, this, row_copy, row_str);
        threads->push_back(new_thread);
        return;
    }

    // recursive case: need to introduce another loop for the next factor
    for (uint64_t offset = 0; offset < factors[permutation[cur_col]]->level; offset++) {
        int temp = row[permutation[cur_col]];
        row[permutation[cur_col]] = (row[permutation[cur_col]] + static_cast<int>(offset)) %
            static_cast<int>(factors[permutation[cur_col]]->level); // try every value for this factor
        heuristic_all_helper(row, cur_col+1, threads);
        row[permutation[cur_col]] = temp;
    }
}

/* HELPER METHOD: heuristic_all_scorer - scores a given row by testing what would change if it was added
 * - should be called in a unique thread
 * - heuristic_all() should await the termination of all sub threads before inspecting scores
 * 
 * parameters:
 * - row: integer array representing a row needing scoring
 * - row_str: string representation of the row
 * 
 * returns:
 * - none, but scores may be updates
*/
void Array::heuristic_all_scorer(int *row, std::string row_str)
{
    // current thread will work with unique copies of the data structures being modified
    Array *copy = clone();
    copy->update_array(row, false); // see how all scores, etc., would change

    // define the row score to be the combination of net changes below, weighted by importance
    int64_t row_score = 0; //= prev_score - static_cast<int64_t>(score);
    for (Single *this_s : singles) { // improve the score based on individual Single improvement
        Single *copy_s = copy->single_map.at(this_s->to_string());
        uint64_t weight = (factors[this_s->factor]->level); // higher level factors hold more weight
        row_score += static_cast<int64_t>(weight*(this_s->c_issues - copy_s->c_issues));
        row_score += 2*weight*(this_s->l_issues - copy_s->l_issues);
        row_score += static_cast<int64_t>(3*weight*(this_s->d_issues - copy_s->d_issues));
    }
    delete copy;

    if (debug == d_on) {
        std::stringstream thread_output;
        thread_output << "==" << std::this_thread::get_id() << "== For row [" << row[0];
        for (uint64_t col = 1; col < num_factors; col++) thread_output << " " << row[col];
        thread_output << "], score is " << row_score << std::endl;
        scores_mutex.lock();
        printf("%s", thread_output.str().c_str());
        scores_mutex.unlock();
    }
    delete[] row;

    // need to add result to data structure containing all thread's results; use mutex for thread safety
    scores_mutex.lock();
    //scores->insert({row, row_score});
    row_scores[row_str] = row_score;
    scores_mutex.unlock();

    /* possibly update row_scored_zero map
    if (just_switched_heuristics) {
        scores_mutex.lock();
        row_scored_zero.insert({row_as_str, row_score == 0});
        scores_mutex.unlock();
    } else if (row_score == 0) {
        scores_mutex.lock();
        row_scored_zero.at(row_as_str) = true;
        scores_mutex.unlock();
    }//*/
}