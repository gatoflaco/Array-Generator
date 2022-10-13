/* Array-Generator by Isaac Jung
Last updated 10/04/2022

|===========================================================================================================|
|   This header contains classes used for organizing data associated with the Array class. There should be  |
| no need for any module other than the Array to use these classes. The Single class is used to represent a |
| single (factor, value) pair; that is, for a single element in a single given row of the Array, both the   |
| column in which the element occurs, and its actual value, are tracked. However, if on another row, that   |
| same value occurs in the same column, it is useful to consider both of those occurrences to be related.   |
| To this end, JUST ONE SINGLE OBJECT SHOULD BE INSTANTIATED PER UNIQUE (factor, value) PAIR, and then for  |
| any row in which that (factor, value) appears, the same object in memory can be accessed, and its rows    |
| attribute can be updated to reflect the fact that it shows up in 1 or more places in the Array. To assist |
| with this accessing, the Factor class exists to associate columns with their levels, as well as with an   |
| array of pointers to the Singles whose factor matches. I.e., the nth column will have access to a list of |
| all Singles of the form (n, v_i), where v_i is some value in valid range of the nth column's levels.      |
| Please note that this module does not handle the building of data structures such that this property is   |
| maintained; the Array module must guarantee it as it instantiates Single and Factor objects.              |
|===========================================================================================================|
*/

#pragma once
#ifndef FACTOR
#define FACTOR

#include "parser.h"
#include <set>

// basically just a tuple, but with a set of rows in which it occurs
class Single
{
    public:
        uint64_t c_issues = 0;  // in how many coverage issues does this Single appear
        int64_t l_issues = 0;   // in how many location issues does this Single appear
        uint64_t d_issues = 0;  // in how many detection issues does this Single appear
        const uint64_t factor;    // represents the factor, or column of the array
        const uint64_t value;     // represents the actual value of the factor
        std::set<int> rows;         // tracks the set of rows in which this (factor, value) occurs
        std::string to_string() const;    // returns a string representing the (factor, value)
        const std::string str_rep;            // memoized to_string_internal
        Single(uint64_t f, uint64_t v); // constructor that takes the (factor, value)
    private:
        std::string to_string_internal() const;    // returns a string representing the (factor, value)
};

// think of this class as containing the information associated with a single column in the array
class Factor
{
    public:
        uint64_t c_issues = 0;  // total coverage issues of all associated Singles
        int64_t l_issues = 0;   // total location issues of all associated Singles
        uint64_t d_issues = 0;  // total detection issues of all associated Singles
        const uint64_t id = 0;        // column number
        const uint64_t level = 0;     // number of values the column can take on
        Single **singles = nullptr;   // pointer to array of Single pointers
        Factor(uint64_t i, uint64_t l, Single **ptr_array); // constructor that takes id, level, Single*
        ~Factor();
};

#endif // FACTOR
