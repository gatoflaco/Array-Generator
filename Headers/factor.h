/* Array-Generator by Isaac Jung
Last updated 06/22/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
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
        uint64_t c_issues;  // in how many coverage issues does this Single appear
        int64_t l_issues;   // in how many location issues does this Single appear
        uint64_t d_issues;  // in how many detection issues does this Single appear
        uint64_t factor;    // represents the factor, or column of the array
        uint64_t value;     // represents the actual value of the factor
        std::set<int> rows;         // tracks the set of rows in which this (factor, value) occurs
        std::string to_string();    // returns a string representing the (factor, value)
        Single();                       // default constructor, don't use this
        Single(uint64_t f, uint64_t v); // constructor that takes the (factor, value)
};

// think of this class as containing the information associated with a single column in the array
class Factor
{
    public:
        uint64_t id;
        uint64_t level;
        Single **singles;
        Factor();   // default constructor, don't use this
        Factor(uint64_t i, uint64_t l, Single **ptr_array);
        ~Factor();
};

#endif // FACTOR
