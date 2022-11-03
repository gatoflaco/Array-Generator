/* Array-Generator by Isaac Jung
Last updated 11/02/2022

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
 * - overloaded: this version can set its fields based on parameters
*/
Single::Single(uint64_t f, uint64_t v) : factor(f), value(v), str_rep(this->to_string_internal())
{
    // rows will be built later
}

std::string Single::to_string() const
{
    return str_rep;
}

std::string Single::to_string_internal() const
{
    return "f" + std::to_string(factor) + "," + std::to_string(value);
}

/* CONSTRUCTOR - initializes the object
 * - overloaded: this version can set its fields based on parameters
*/
Factor::Factor(uint16_t i, uint16_t l, Single **ptr_array) : id(i), level(l)
{
    singles = ptr_array;
}

/* DECONSTRUCTOR - frees memory
*/
Factor::~Factor()
{
    for (uint16_t i = 0; i < level; i++) delete singles[i];
    delete[] singles;
}