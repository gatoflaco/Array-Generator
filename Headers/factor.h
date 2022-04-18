/* LA-Checker by Isaac Jung
Last updated 03/21/2022

|===========================================================================================================|
|   This header contains classes used for organizing data associated with the arrays used by check.cpp. See |
| parser.cpp for a look at how these classes are used. The idea behind the organization is also covered in  |
| the README. The short of it is that it is useful to know in what rows a given interaction can occur, and  |
| it is also useful to have a class representing an entire column in the array. Each column corresponds to  |
| factor, and each factor has an associated range of levels it can take on.
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
        long unsigned int factor; // represents the factor, or column of the array
        long unsigned int value;  // represents the actual value of the factor
        std::set<int> rows;    // tracks the set of rows in which this (factor, value) occurs
        Single();   // default constructor, don't use this
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
