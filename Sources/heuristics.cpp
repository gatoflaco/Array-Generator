/* Array-Generator by Isaac Jung
Last updated 06/06/2022

|===========================================================================================================|
|   This file contains definitions for methods belonging to the Array class which are declared in array.h.  |
| Specifically, any methods involving heuristics for scoring and altering a potential row to add are found  |
| here. The only reason they are not in array.cpp is for organization.                                      |
|===========================================================================================================|
*/

#include "array.h"

// class used by heuristic_optimal_row()
class Prev_S_Data
{
    public:
        long unsigned int c_issues;
        long unsigned int l_issues;
        long unsigned int d_issues;
        Prev_S_Data(long unsigned int c, long unsigned int l, long unsigned int d) {
            c_issues = c; l_issues = l; d_issues = d;
        }
        void restore(Single *s) {   // replace fields of s with those stored in this object
            s->c_issues = c_issues; s->l_issues = l_issues; s->d_issues = d_issues;
        }
};

// class used by heuristic_optimal_row()
class Prev_I_Data
{
    public:
        Prev_I_Data(bool c, std::map<T*, long unsigned int> r, bool d) {
            is_covered = c; row_diffs = r; is_detectable = d;
        }
        void restore(Interaction *i) {   // replace fields of i with those stored in this object
            i->is_covered = is_covered; i->row_diffs = row_diffs; i->is_detectable = is_detectable;
        }

    private:
        bool is_covered;
        std::map<T*, long unsigned int> row_diffs;
        bool is_detectable;
};

// class used by heuristic_optimal_row()
class Prev_T_Data
{
    public:
        Prev_T_Data(std::set<T*> c, bool l) {
            location_conflicts = c; is_locatable = l;
        }
        void restore(T *t_set) {    // replace fields of t_set with those stored in this object
            t_set->location_conflicts = location_conflicts; t_set->is_locatable = is_locatable;
        }

    private:
        std::set<T*> location_conflicts;
        bool is_locatable;
};

void Array::tweak_row(int *row, std::set<Interaction*> *row_interactions)
{
    // TODO: turn this into a giant switch case function that chooses a heuristic function to call based on
    // the score:total_problems ratio
    float ratio = static_cast<float>(score)/total_problems;
    bool satisfied = false; // used repeatedly to decide whether it is time to switch heuristics

    // first up, should we use the most in-depth scoring function:
    if (p == c_only) {
        if (total_problems < 500) satisfied = true; // arbitrary choice
        else if (ratio < 0.60) satisfied = true;    // arbitrary choice
    } else if (p == c_and_l) {
        if (total_problems < 450) satisfied = true; // arbitrary choice
        else if (ratio < 0.50) satisfied = true;    // arbitrary choice
    } else {    // p == all
        if (total_problems < 400) satisfied = true; // arbitrary choice
        else if (ratio < 0.40) satisfied = true;    // arbitrary choice
    }//
    if (satisfied) {
        heuristic_optimal_row(row);
        return;
    }//*/

    // TODO: come up with more heuristics
    
    // if not far enough yet, go with the simplistic coverage-only heuristic
    heuristic_c_only(row, row_interactions);
}

void Array::heuristic_c_only(int *row, std::set<Interaction*> *row_interactions)
{
    int *problems = new int[num_factors]{0};    // for counting how many "problems" each factor has
    int max_problems;   // largest value among all in the problems[] array created above
    int cur_max;    // for comparing to max_problems to see if there is an improvement
    int *dont_cares_c = new int[num_factors];   // local copy of the don't cares
    for (long unsigned int col = 0; col < num_factors; col++)
        if (dont_cares[col] || factors[col]->singles[row[col]]->c_issues == 0)
            dont_cares_c[col] = dont_cares[col];

    for (Interaction *i : *row_interactions) {
        if (i->rows.size() != 0) {  // Interaction is already covered
            bool can_skip = false;  // don't account for Interactions involving already-completed factors
            for (Single *s : i->singles)
                if (dont_cares[s->factor]) {
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
    for (long unsigned int col = 0; col < num_factors; col++)
        if (problems[col] > max_problems) max_problems = problems[col];
    if (max_problems == 0) {    // row is good enough as is
        delete[] problems;
        return;
    }
    
    // else, try altering the value(s) with the most problems (whatever is currently contributing the least)
    cur_max = max_problems;
    for (long unsigned int col = 0; col < num_factors; col++) { // go find any factors to change
        if (problems[permutation[col]] == max_problems) {   // found a factor to try altering
            int *temp_problems = new int[num_factors]{0};   // deep copy problems[] because it will be mutated

            for (long unsigned int i = 1; i < factors[permutation[col]]->level; i++) {  // for every value
                row[permutation[col]] = (row[permutation[col]] + 1) %
                    static_cast<int>(factors[permutation[col]]->level); // try that value
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
    for (long unsigned int col = 0; col < num_factors; col++) { // for all factors
        if (dont_cares_c[permutation[col]]) continue;   // no need to check factors that are "don't cares"
        for (long unsigned int i = 1; i < factors[permutation[col]]->level; i++) {  // for every value
            row[permutation[col]] = (row[permutation[col]] + 1) %
                static_cast<int>(factors[permutation[col]]->level); // try that value
            std::set<Interaction*> new_interactions;    // get the new Interactions
            build_row_interactions(row, &new_interactions, 0, t, "");

            bool improved = false;  // see if the change helped
            for (Interaction *interaction : new_interactions)
                if (interaction->rows.size() == 0) {
                    for (Single *s : interaction->singles) dont_cares[s->factor] = true;
                    improved = true;
                }
            if (improved) break;    // keep this factor as this value
        }
    }
    delete[] dont_cares_c;
    delete[] problems;
}

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
    for (long unsigned int col = 0; col < num_factors; col++) {
        if (factors[col]->singles[row[col]]->c_issues == 0) continue;   // already completed factor
        if (problems[col] > max_problems) max_problems = problems[col];
    }
    return max_problems;
}

/* SUB METHOD: heuristic_optimal_row - tweaks a row so that it solves the most problems possible
 * - does the deepest inspection of all the heuristics; therefore, should not be used till close to complete
 * 
 * parameters:
 * - row: integer array representing a row up for consideration for appending to the array
 * 
 * returns:
 * - none, but the row will be altered such that it is subjectively as optimal as possible
 *  --> note that this does not mean that running this for the whole array will guarantee the smallest array;
 *      this is still a greedy algorithm for the current row, without any lookahead to future rows
*/
void Array::heuristic_optimal_row(int *row)
{
    std::vector<int*> best_rows;
    long unsigned int best_score = 0;
    heuristic_optimal_helper(row, 0, &best_rows, &best_score);

    // after heuristic_optimal_helper() finishes, best_rows holds one or more optimal choices for a row
    for (unsigned long int col = 0; col < num_factors; col++)   // break ties randomly
        row[col] = best_rows.at(static_cast<long unsigned int>(rand()) % best_rows.size())[col];
    
    // free memory
    for (int *r : best_rows) delete[] r;
}

void Array::heuristic_optimal_helper(int *row, long unsigned int cur_col, std::vector<int*> *best_rows,
    long unsigned int *best_score)
{
    // base case: row represents a unique combination and is ready for scoring
    if (cur_col == num_factors) {
        std::set<Interaction*> row_interactions;
        build_row_interactions(row, &row_interactions, 0, t, "");
        std::set<Single*> row_singles;
        std::set<T*> row_t_sets;

        // store current state of the array and other data structures; they are about to be modified
        long unsigned int prev_score = score;
        long unsigned int prev_c = coverage_problems, prev_l = location_problems, prev_d = detection_problems;
        std::map<Single*, Prev_S_Data*> prev_singles;   // for restoring changed Single data
        std::map<Interaction*, Prev_I_Data*> prev_interactions; // for restoring changed Interaction data
        std::map<T*, Prev_T_Data*> prev_t_sets;  // for restoring changed T set data
        for (Interaction *i : row_interactions) {
            for (Single *s : i->singles) row_singles.insert(s);
            for (T *t_set : i->sets) row_t_sets.insert(t_set);
            Prev_I_Data *x = new Prev_I_Data(i->is_covered, i->row_diffs, i->is_detectable);
            prev_interactions.insert({i, x});
        }
        for (Single *s : row_singles) {
            Prev_S_Data *x = new Prev_S_Data(s->c_issues, s->l_issues, s->d_issues);
            prev_singles.insert({s, x});
        }
        for (T *t_set : row_t_sets) {
            Prev_T_Data *x = new Prev_T_Data(t_set->location_conflicts, t_set->is_locatable);
            prev_t_sets.insert({t_set, x});
        }

        update_array(row, &row_interactions, false);    // see how all scores, etc., would change

        // define the row score to be the combination of net changes below, weighted by importance
        long unsigned int row_score = 2*(prev_score - score);
        for (Single *s : row_singles) { // improve the score based on individual Single improvement
            long unsigned int weight = (factors[s->factor]->level); // higher level factors hold more weight
            Prev_S_Data *x = prev_singles.at(s);
            row_score += weight*(x->c_issues - s->c_issues);
            if (s->l_issues < x->l_issues) row_score += weight*(x->l_issues - s->l_issues);
            row_score += 3*weight*(x->d_issues - s->d_issues);  // working towards detection is worth most
        }
        
        // decide what to do with this row based on its score compared to best_score
        if (row_score >= *best_score) { // it was better or it tied; either way, keep track of this row
            if (row_score > *best_score) {  // for an even better choice, can stop tracking the previous best
                *best_score = row_score;
                for (int *r : *best_rows) delete[] r;   // free memory; about to clear best_rows
                best_rows->clear();
            }   // or, if it tied, continue to track the previous best row(s) it is tied with
            int *new_row = new int[num_factors];
            for (unsigned long int col = 0; col < num_factors; col++) new_row[col] = row[col];
            best_rows->push_back(new_row);  // happens whether row_score > or =
        }
        
        // restore the original state of the array and data structures, and free memory
        score = prev_score;
        coverage_problems = prev_c, location_problems = prev_l, detection_problems = prev_d;
        for (Single *s : row_singles) {
            Prev_S_Data *x = prev_singles.at(s);
            x->restore(s);
            delete x;
        }
        for (T *t_set : row_t_sets) {
            Prev_T_Data *x = prev_t_sets.at(t_set);
            x->restore(t_set);
            delete x;
        }
        for (Interaction *i : row_interactions) {
            Prev_I_Data *x = prev_interactions.at(i);
            x->restore(i);
            delete x;
        }
        return;
    }

    // recursive case: need to introduce another loop for the next factor
    for (long unsigned int offset = 0; offset < factors[permutation[cur_col]]->level; offset++) {
        int temp = row[permutation[cur_col]];
        row[permutation[cur_col]] = (row[permutation[cur_col]] + static_cast<int>(offset)) %
            static_cast<int>(factors[permutation[cur_col]]->level); // try every value for this factor
        heuristic_optimal_helper(row, cur_col+1, best_rows, best_score);
        row[permutation[cur_col]] = temp;
    }
}