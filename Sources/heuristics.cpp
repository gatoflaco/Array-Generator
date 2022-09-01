/* Array-Generator by Isaac Jung
Last updated 09/01/2022

|===========================================================================================================|
|   This file contains definitions for methods belonging to the Array class which are declared in array.h.  |
| Specifically, any methods involving heuristics for scoring and altering a potential row to add are found  |
| here. The only reason they are not in array.cpp is for organization.                                      |
|===========================================================================================================|
*/

#include "array.h"

/* SUB METHOD: tweak_row - chooses a heuristic to use for modifying a row based on current state of the Array
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - row_interactions: set containing all Interactions present in the row
 * 
 * returns:
 * - void, but after the method finishes, the row may be modified in an attempt to satisfy more issues
*/
void Array::tweak_row(int *row, std::set<Interaction*> *row_interactions)
{
    float ratio = static_cast<float>(score)/total_problems;
    bool satisfied = false; // used repeatedly to decide whether it is time to switch heuristics

    // first up, should we use the most in-depth scoring function:
    if (p == c_only) {
        if (total_problems < 500) satisfied = true; // arbitrary choice
        else if (ratio < 0.90) satisfied = true;    // arbitrary choice
    } else if (p == c_and_l) {
        if (total_problems < 450) satisfied = true; // arbitrary choice
        else if (ratio < 0.85) satisfied = true;    // arbitrary choice
    } else {    // p == all
        if (total_problems < 400) satisfied = true; // arbitrary choice
        else if (ratio < 0.80) satisfied = true;    // arbitrary choice
    }
    if (satisfied) {
        heuristic_all(row);
        return;
    }

    // TODO: come up with more heuristics
    
    // if not far enough yet, go with the simplistic coverage-only heuristic
    heuristic_c_only(row, row_interactions);
}

/* SUB METHOD: heuristic_c_only - lightweight heuristic that only concerns itself with coverage
 * - in the tradeoff between speed and optimal row choice, this heuristic is towards the speed extreme
 * - should only be used very early on in array construction
 * --> can do well for longer when the desired array is simpler (i.e., covering as opposed to detecting)
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - row_interactions: set containing all Interactions present in the row
 * 
 * returns:
 * - void, but after the method finishes, the row may be modified in an attempt to satisfy more issues
*/
void Array::heuristic_c_only(int *row, std::set<Interaction*> *row_interactions)
{
    int *problems = new int[num_factors]{0};    // for counting how many "problems" each factor has
    int max_problems;   // largest value among all in the problems[] array created above
    int cur_max;    // for comparing to max_problems to see if there is an improvement
    prop_mode *dont_cares_c = new prop_mode[num_factors];   // local copy of the don't cares
    for (uint64_t col = 0; col < num_factors; col++) dont_cares_c[col] = dont_cares[col];

    for (Interaction *i : *row_interactions) {
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
    delete[] dont_cares_c;
    delete[] problems;
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

/* SUB METHOD: heuristic_all - tweaks a row so that it solves the most problems possible
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
    std::map<int*, int64_t> scores;
    heuristic_all_helper(row, 0, &scores);
    //TODO: wait for all child processes to terminate (once threading has been implemented)

    // inspect the scores for the best one(s)
    int64_t best_score = INT64_MIN;
    std::vector<int*> best_rows;    // there could be ties for the best
    for (auto& kv : scores) {
        if (kv.second >= best_score) {  // it was better or it tied
            if (kv.second > best_score) {   // for an even better choice, can stop tracking the previous best
                best_score = kv.second;
                best_rows.clear();
            }
            best_rows.push_back(kv.first);  // whether it was better or only a tie, keep track of this row
        } else delete[] kv.first;
    }

    // choose the row that scored the best (for ties, choose randomly from among those tied for the best)
    int choice = static_cast<uint64_t>(rand()) % best_rows.size(); // for breaking ties randomly
    for (uint64_t col = 0; col < num_factors; col++)
        row[col] = best_rows.at(choice)[col];
    
    // free memory
    for (int *r : best_rows) delete[] r;
}

/* HELPER METHOD: heuristic_all_helper - performs top-down recursive logic for heuristic_all()
 * - heuristic_all() does the auxilary work to start the recursion, and handle the result
 * - this method uses recursion to form all possible combinations; its base case scores a given combination
 * 
 * parameters:
 * - row: integer array representing a row being considered for adding to the array
 * - cur_col: which column should have its levels looped over in the recursive case
 * --> overhead caller should pass 0 to this method initially
 * --> value should increment by 1 with each recursive call
 * --> triggers the base case when value is equal to the total number of columns
 * - scores: pointer to a map whose keys are pointers to rows and whose values are the scores of those rows
 * --> overhead caller should pass the address of an empty map to this method initially
 * --> for each row inspected by the base case, a separate thread should handle the scoring and map updating
 * 
 * returns:
 * - none, but scores will be modified to contain all the rows inspected and their scores
*/
void Array::heuristic_all_helper(int *row, uint64_t cur_col, std::map<int*, int64_t> *scores)
{
    // base case: row represents a unique combination and is ready for scoring
    if (cur_col == num_factors) {
        /*
        // First, check if this exact row has already been added to array at least δ times.
        // If so, do not bother inspecting this row, for it will definitely add nothing
        //
        if (DEBUG_FLAG) {
            printf("\tDEBUG: row { ");
            for (uint64_t i = 0; i < num_factors; i++) printf("%d ", row[i]);
            printf("} ");
        }
        uint64_t count = 0;
        for (int *array_row : rows) {
            bool matched = true;
            for (uint64_t i = 0; i < num_factors; i++) {
                if (row[i] != array_row[i]) {
                    matched = false;
                    break;
                }
            }
            if (matched) count++;
        }
        if (count >= delta) {
            if (DEBUG_FLAG) printf("skipped\n");
            return;
        }//*/

        int *row_copy = new int[num_factors];   // must be deleted by heuristic_all() later
        for (uint64_t col = 0; col < num_factors; col++) row_copy[col] = row[col];
        heuristic_all_scorer(row_copy, scores);
        return;
    }

    // recursive case: need to introduce another loop for the next factor
    /*if ((p == all && dont_cares[permutation[cur_col]] == all) ||
        (p == c_and_l && dont_cares[permutation[cur_col]] == c_and_l) ||
        (p == c_only && dont_cares[permutation[cur_col]] == c_only)) {
        heuristic_all_helper(row, cur_col+1, best_rows, best_score);
        return;
    }//*/
    for (uint64_t offset = 0; offset < factors[permutation[cur_col]]->level; offset++) {
        int temp = row[permutation[cur_col]];
        row[permutation[cur_col]] = (row[permutation[cur_col]] + static_cast<int>(offset)) %
            static_cast<int>(factors[permutation[cur_col]]->level); // try every value for this factor
        heuristic_all_helper(row, cur_col+1, scores);
        row[permutation[cur_col]] = temp;
    }
}

/* HELPER METHOD: heuristic_all_scorer - scores a given row by testing what would change if it was added
 * - should be called in a unique thread
 * - heuristic_all() should await the termination of all sub threads before inspecting scores
 * 
 * parameters:
 * - row_v: a vector representing the row to be scored
 *  --> cannot be a pointer to the row being modified by heuristic_all_helper() (would result in data races)
 * - scores: pointer to 
 * 
 * returns:
 * - none, but best_rows will be modified to contain whichever row(s) scored best
 * --> also, best_score will be modified, but this value will likely not be needed by the caller
*/
void Array::heuristic_all_scorer(int *row, std::map<int*, int64_t> *scores)
{
    // current thread will work with unique copies of the data structures being modified
    Array *copy = clone();
    std::set<Interaction*> row_interactions;
    copy->build_row_interactions(row, &row_interactions, 0, t, "");

    copy->update_array(row, &row_interactions, false);  // see how all scores, etc., would change

    // define the row score to be the combination of net changes below, weighted by importance
    int64_t row_score = 0; //= prev_score - static_cast<int64_t>(score);
    for (Single *this_s : singles) { // improve the score based on individual Single improvement
        Single *copy_s = copy->single_map.at(this_s->to_string());
        uint64_t weight = (factors[this_s->factor]->level); // higher level factors hold more weight
        row_score += static_cast<int64_t>(weight*(this_s->c_issues - copy_s->c_issues));
        row_score += 2*weight*(this_s->l_issues - copy_s->l_issues);
        row_score += static_cast<int64_t>(3*weight*(this_s->d_issues - copy_s->d_issues));
    }

    scores_mutex.lock();
    scores->insert({row, row_score});
    scores_mutex.unlock();

    delete copy;
    // note: do not delete row here, as heuristic_all() will need to reference it
}