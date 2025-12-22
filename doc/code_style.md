# Introduction:
The document gives base introduction into code style.
File .clang-format at project root helps in formatting.

# Motivation
_"Indeed, the ratio of time spent reading versus writing is well over 10 to 1. We are constantly reading old code as part of the effort to write new code. …[Therefore,] making it easy to read makes it easier to write."_
(c) Robert C. Martin, Clean Code: A Handbook of Agile Software Craftsmanship

* Objective #1: Code must be easy to read
* Objective #2: Code must be acceptably comfortable to write
* Objective #3: consistant rules

# Code style
## General
 * all names must be in **lower case** letters
 * words & prefixes delimeter is **_**

Different pattern were compared from *Objective #1* standpoint of view to choose the most appropriate:
  1) *myFileForSomething.cpp* << **hard** to read, **hard** to remember what is lowercase, what is caps, but slightly **shorter**
  2) *myfileforsomething.cpp* << **even harder** to read
  3) *myFile_for_Something.cpp* << mix of styles, more **difficult to follow and remember**
  4) *my_file_for_something.hpp* << **easy** to read, **easy** to remember the style, but slightly **longer**

## File names
 Example:
  * my_file_for_something.cpp
  * my_file_for_something.hpp

## Prefixes
Each variable or type **must** have prefix, separated from name with standard delimeter **_**, see chapter [Examples](#Examples) below.

### Types
Each declared type **must** has one of the prefixes:
  * s - signed integer
  * u - unsigned integer
  * x - utf char
  * z - size_t
  * b - bool
  * c - class
  * d - double
  * f - float
  * p - pointer
  * r - reference
  * v - r-value reference
  * o - object (class instance)
  * s - structure
  * e - enum type or enum values
  * t - template
  * f - function pointer, lambda

### Variables
Variable prefix must consist of access rights/visibility, one of the following:

* Function parameters:
   * i - input
   * io - input & output
   * o - output (input value is ignored)
* scoped
   * g - global
   * l - local
* members:
  * m - member of class or structure

and followed by type from chapter above, see chapter [Examples](#Examples) below.

## Functions
 * C++ class funcstion: fisrt word (prefix) is verb, `do_somethings`
 * C functions: first word (prefix) is module name `mymodule_do_something`

## Preprocessor defines
 Must use all capitals. **The only exception** due to historical reasons and millions of code lines already existing.

# Examples
```cpp
#include "my_header_file.h"

#define MY_DEFINE 0

//enum
enum class e_my_enum
{
  e_val_1,
  e_val_2,
  e_vals_count
};

struct s_my_struct
{
  uint32_t mu_val1;
  int32_t  mi_val2;
};

//global variable
const static s_my_struct gs_my_global_val = {0, 0};

//Class
class c_my_class
{
  void *m_data = nullptr;
public:
  c_my_class(uint32_t iu_init_var);
};


void my_function(c_my_class *&or_object)
{
  or_object = new c_my_class(10);
}
```
