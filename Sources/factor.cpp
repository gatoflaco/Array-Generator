/* Array-Generator by Isaac Jung
Last updated 10/04/2022

|===========================================================================================================|
|   This file contains mostly just constructors and deconstructors for the Single and Factor classes. The   |
| Single class also has a to_string() method, used for building reconstructable keys into a map used by the |
| Array module (see array.cpp for more details). The purpose of Single and Factor objects is described in   |
| this module's header file, factor.h.
|===========================================================================================================|
*/

#include "factor.h"
#include <algorithm>

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Single::Single()
{
    c_issues = 0; l_issues = 0; d_issues = 0;   // to be incremented later
    factor = 0;
    value = 0;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on parameters
*/
Single::Single(uint64_t f, uint64_t v) : Single::Single()
{
    factor = f;
    value = v;
    // rows will be built later
}

std::string Single::to_string()
{
    return "f" + std::to_string(factor) + "," + std::to_string(value);
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Factor::Factor()
{
    c_issues = 0; l_issues = 0; d_issues = 0;   // to be incremented later
    id = 0;
    level = 0;
    singles = nullptr;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on parameters
*/
Factor::Factor(uint64_t i, uint64_t l, Single **ptr_array) : Factor::Factor()
{
    id = i;
    level = l;
    singles = ptr_array;
}

/* DECONSTRUCTOR - frees memory
*/
Factor::~Factor()
{
    for (uint64_t i = 0; i < level; i++) delete singles[i];
    delete[] singles;
}