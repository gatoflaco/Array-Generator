/* Array-Generator by Isaac Jung
Last updated 05/21/2022

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
        long unsigned int c_issues; // in how many coverage issues does this Single appear
        long unsigned int l_issues; // in how many location issues does this Single appear
        long unsigned int d_issues; // in how many detection issues does this Single appear
        long unsigned int factor;   // represents the factor, or column of the array
        long unsigned int value;    // represents the actual value of the factor
        std::set<int> rows;         // tracks the set of rows in which this (factor, value) occurs
        std::string to_string();    // returns a string representing the (factor, value)
        Single();                                           // default constructor, don't use this
        Single(long unsigned int f, long unsigned int v);   // constructor that takes the (factor, value)
};

// think of this class as containing the information associated with a single column in the array
class Factor
{
    public:
        long unsigned int id;
        long unsigned int level;
        Single **singles;
        Factor();   // default constructor, don't use this
        Factor(long unsigned int i, long unsigned int l, Single **ptr_array);
        ~Factor();
};

#endif // FACTOR
