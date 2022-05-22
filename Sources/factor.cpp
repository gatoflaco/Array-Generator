/* Array-Generator by Isaac Jung
Last updated 05/21/2022

|===========================================================================================================|
|   (to be written)                                                                                         |
|===========================================================================================================|
*/

#include "factor.h"
#include <algorithm>

/* CONSTRUCTOR - initializes the object
 * - overloaded: this is the default with no parameters, and should not be used
*/
Single::Single()
{
    c_issues = 0; l_issues = 0; d_issues = 0;
    factor = 0;
    value = 0;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on parameters
*/
Single::Single(long unsigned int f, long unsigned int v)
{
    c_issues = 0; l_issues = 0; d_issues = 0;   // to be incremented later
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
    id = 0;
    level = 0;
    singles = nullptr;
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on parameters
*/
Factor::Factor(long unsigned int i, long unsigned int l, Single **ptr_array)
{
    id = i;
    level = l;
    singles = ptr_array;
}

/* DECONSTRUCTOR - frees memory
*/
Factor::~Factor()
{
    for (long unsigned int i = 0; i < level; i++) delete singles[i];
    delete[] singles;
}