# Array-Generator
### Author: Isaac Jung

## Credits
[Violet R. Syrotiuk, Ph.D.](https://isearch.asu.edu/profile/518662)
- Supervisor of this project
- Author of research on locating arrays

[Charles J. Colbourn, Ph.D.](https://isearch.asu.edu/profile/384970)
- Advisor to the author of this project
- Credited for [proposing locating and detecting arrays](#additional-links)

## Overview
The purpose of this program is to generate test suites of enumerated 2-dimensional arrays with the covering, locating, and detecting properties. This readme assumes that users already have a general understanding of what these properties are. If you do not, try checking out the [links at the bottom of this page](#additional-links).

## Usage
```
./generate [d|] [t|] [δ|] [-[flags*]|] <input_filepath> [<output_filepath>|]
```
Example:
```
./generate 1 2 1 -v Sample-Input/TWC.tsv out.tsv
```
- This would run the program with d = 1, t = 2, δ = 1, with verbose mode enabled, using the file `Sample-Input/TWC.tsv` for input and the file `out.tsv` for output. Read on for full details.
### IMPORTANT: This program uses inductive reasoning to determine what type of array you are trying to generate (covering, locating, detecting).
To generate a covering array, specify just t. To generate a locating array, specify both d and t. To generate a detecting array, specify all of d, t, and δ.
### Compiling
Included is a simple makefile for quick compilation of the exectuable. After downloading the project, the makefile can be used in a terminal by running the following command from within its directory:
```
make
```
This will create the executable with the name "generate" in the same directory as the makefile, unless the executable already exists and is up to date. Of course, the makefile can be edited, or compilation can be done manually, for a different executable.
### Running
At the very least, you must provide an input file with the call:
```
./generate Sample-Input/example.tsv
```
This would use `Sample-Input/example.tsv` for certain parametric information, [as described in the next section](#input). Note that if no [additional arguments](#command-line-arguments) are given, the program has certain default behaviors (read about this in the [options](#options) section further below).
### Input
The format of the input should be as follows:
```
C
L_1 L_2 ... L_C
```
- On the first line, give the number of columns in the array, C.
- On the second line, give the number of levels that each factor can take on, respectively, each separated by whitespace. The number of values given here should be equal to the value of C given on the first line.
Violating the format will result in some sort of error, meaning the program will not attempt to generate anything. The program is capable of some very basic error identification, to assist you in fixing small issues in your input. Typically the file format is a TSV (tab separated values), meaning that any whitespace characters are actually tabs. However, it is fine to use standard whitespace as well. Note that any additional lines after the second will not be looked at, meaning you can use that space to add notes or other useful info without disrupting the program.
### Output
By default, when there are no input format errors, the output of this program relays progress on constructing the requested array. Specifically, it will look like this:
```
Reading input....

Building internal data structures....

[Array score is currently x_i, adding row n_i]*
Comleted array with n rows.

[Wrote array into file with path name <output_filepath>|The finished array is:]
[ARRAY|]
```
- `x_i` begins at a relatively large positive number and fluctuates, generally decreasing, until 0.
- `n_i` begins at 1 and increments by 1 for every additional line. The final `n_i` will be equal to the `n` displayed on the `Completed array with n rows` line.  
- If an output file was specified, the final line of output will be `Wrote array into file with path name <output_filepath>`. If not, then it will be `The finished array is:` followed by the entire array.
- The output may be made more or less verbose than this using various [flags](#flags) on the command line. 
- If something goes wrong during the generation, e.g., the program detects that the requested properties are impossible to satisfy, it will attempt to halt, report the problem, and either save the partially completed array into the provided output file, or display it on the console, depending once again on whether an output file was provided. In this case, the final line(s) of output may vary.

## Options
The program may be invoked with a number of additional command line arguments and flags to alter its behavior. This is different from I/O redirection. Refer to the [usage](#usage) section for an example of what it looks like. This section describes the details:
- Despite the simplified visual in the usage section, the filepaths, [command line arguments](#command-line-arguments), and [flags](#flags) may actually come in any order, so long as the input file is given somewhere before any potential output file. Flags are distingushed by a hyphen character (-).
- The command line arguments should *not* have leading hyphens and are simply delimited by whitespace. The relative order of these arguments **actually matters**. While flags can be mixed in anywhere between the arguments, the arguments are interpreted like this: the first integer encountered is assumed to be t. If a second integer is encountered, it is assumed to be t with the previous t assumed to instead be d. If a third is encountered, it is assumed to be δ. This means that in order to specify d, you must also specify t, and in order to specify δ, you must also specify both d and t.
- The flags are demarcated by a leading hyphen. Flags may use separate hyphens or share a single one. To "share" a single hyphen, additional flags beyond the first must succeed each other directly, i.e., without any whitespace. If whitespace is used between flags, a hyphen must be prepended for each of the whitespace-separated groups of flags.
- If the program cannot interpret a command line argument, it will ignore it and continue, possibly using default values/behaviors.
### Command Line Arguments
d: an integer bounded between 1 and t, inclusive
- Represents the magnitude of sets of interactions used; these sets are used in comparisons necessary to analyzing the locating and detecting properties of arrays.
- If not given, desired property is assumed to be t-covering

t: an integer bounded between 1 and the number of factors, inclusive
- Represents how many (factor, value) tuples are included in an interaction; 1-way interactions by definition are just single (factor, value) tuples.
- If not given, 2 is used by default.

δ: an integer bounded below by 1
- Represents separation.
- If not given, desired property is assumed to be (d, t)-locating, or even just t-covering when d is also not given.
### Flags
v: verbose
- States what flags are set, as well as the relevant values of d, t, and δ, prior to reading input.
- Displys the state of all internal single (factor, value) pairs, t-way interactions, and, if more than coverage is requested, size-d sets of t-way interactions. Due to the nature of generation happening from scratch, the sets of rows on which these occur should always be empty at this point.
- Shows each row that is added as it goes.

h: halfway
- Currently does not do anything different than when not specified. Plans underway to make it reduce the overall output.
- Mutually exclusive with the s flag; if both are specified, the last one seen takes priority.

s: silent
- Does not produce any output, except for the finished array when no output file was specified.
- Mutually exclusive with the h flag; if both are specified, the last one seen takes priority.

## Details and Definitions
The program begins by interpreting command line arguments and flags to set state variables, then getting input from the specified input file. It passes all of this info to an Array object constructor, which sets up a lot of internal vectors and sets for organizing data and tracking scores, etc. When this is done, the main program adds the first row, which is completely randomly generated within the constraints provided. After the first row, the program then enters a loop in which it calls a method that adds a row based on scoring heuristics. It does this until the array is completed with the requested properties. After every row added, even the first, the array object updates its internal data structures. This is important for making scoring decisions in the heuristics that decide what rows to add, and for tracking the overall progress of the array generation. An overall score based on the total "problems" to solve determines when the array is completed; the number starts off large and decreases as problems are solved. When the overall score is 0, all problems are solved and the array is completed with the requested properties.

If, during the construction process, the main loop appears to get stuck making no further progress towards completion, the program is capable of interrupting itself and saving/displaying how far it was able to get before getting stuck. The reason for this is that the user may accidentally request a combination of properties that are actually impossible to satisfy based on the given factors and their levels. The lookahead logic to detect this is not well understood yet, so while some basic checks attempt to catch impossible requests before generation even begins, sometimes and impossible request makes it through and needs to be caught as generation is happening. I believe that all types of impossible requests can be generalized to mathematical relationships (albeit, some are quite complicated), so if possible, I would like to eventually be able to catch all impossible requests in the beginning. This would save the user the time of waiting for the program to get stuck just to report a request as impossible.

The most interesting part of this program is the problem of scoring a potential choice for a row to add and tweaking it to improve without necessarily exploring all possible combinations (for efficiency). To tackle this problem, different heuristics are being implemented that tradeoff optimal row choice for time/space resources needed in the computation. That is, some heuristics make a decision quickly, but it may not have been the best decision possible (to find the actual best decision possible would likely require exploring not just alternate choices for the current row, but also future rows; the tradeoff in computing resources needed eventually becomes not worthwhile). These heuristics are generally good early on in array construction, when the number of problems to consider is large; when there is too much to think about, one shouldn't overthink, and instead make the assumption that many choices can come close enough to the "best" one with a rough estimate. Other heuristics spend a long time computing in order to come up with better decisions. These heuristics are not used until many problems have already been solved, thereby reducing the amount of work they really need to do. What follows is a running list of heuristics used:

1. heuristic_c_only:
  This heuristic aims to solve missing coverage in a relatively quick manner. It beings with a 1-dimensional vector of size equal to the number of factors, wherein each element of the vector will correspond to each factor. All values in this vector begin at 0. The heuristic looks at the current choice for the row and considers all t-way interactions that occur. If a given t-way interaction is already covered, the values in the vector whose positions correspond to the factors involved have their values incremented. If the interaction is not covered, those values are instead decremented. At the end, the values in the vector are summed. If the sum is 0 or less, the row is decided to be "good enough" and allowed to be added. If the score is not good enough, a helper method is called repeatedly to modify the row in an attempt to improve it. It begins with the "most suspect" factor - the one whose corresponding value in the vector was highest, and tweaks one factor's level in the row at a time in this way, rescoring the vector after each change in a similar manner as before. The goal at this point becomes simply to reduce the vector's sum to a number less than the original. When that happens, it must be true that some interaction was found that was not covered, and the row is kept (again, the goal of this heuristic is just to be extremely fast). The heuristic also uses the concept of "don't cares"; ignoring the scoring of factors for which all interactions involving said factors are already covered. Still, for covering arrays, this heuristic has poor performance when the array is close to completed. However, covering arrays are small compared to locating and especially detecting arrays, so it is less important to achieve optimality. Even so, the heuristic should not be used the whole time. For locating and detecting arrays, the heuristic should be used only briefly at the beginning. The hope is that even if it doesn't make the best choice in terms of coverage, many location and detection issues will be resolved no matter what near the beginning, and so it's fine to make an almost-thoughtless decision.

2. heuristic_l_only (TODO):
  This heuristic aims to solve missing location under the assumption that coverage is low priority. This heuristic is still in design phase.

3. heuristic_d_only (TODO):
  This heuristic aims to solve missing detection under the assumption that coverage and location are low priority. This heuristic is still in design phase.

4. heuristic_all:
  This heuristic can be used to solve all types of missing properties. The way it works is to pretend that the row up for consideration is going to be added; that is, it literally adds the row and calls the method that updates internal data structures, comparing the states of things before and after. In order to do this, the previous state of the array must be saved to memory, which has a high cost in space. A top-down recursive helper method is called to go through all non-"don't care" factors' levels and indiscriminately test the result of adding such a row. After the row is added, the new state of the array is compared to the saved state and a score is assigned based on a combination of the covering, locating, and detecting progress (including weights assigned to each category). The best score is kept track of, and when a given row outscores the previous best, its levels are saved to a vector for remembering what achieved this. When a row ties the current best score, it is appended to the vector, as opposed to overriding it. In this way, if at the end there are multiple best choices, the heuristic can randomly choose from among them. After every row that is tested, the helper method then removes the row and restores the saved state of the array. Iterating through all viable combinations of levels and doing this has a high cost in time. So, overall, this method is extremely expensive in computational resources, and should be saved for last, when the array is close to complete. However, it guarantees a best decision at the scope of a single row.

## Additional Links
Colbourn and McClary, *[Locating and Detecting Arrays for Interaction Faults](https://drops.dagstuhl.de/opus/volltexte/2009/2240/pdf/09281.ColbournCharles.Paper.2240.pdf)*
- Paper cited as first to propose locating and detecting arrays
- Formal definitions for covering, locating, and detecting arrays can be found here

[My Array-Checker](https://github.com/gatoflaco/Array-Checker)
- Tool to verify the properties of arrays that this project generates
- The input-format-simple branch of that project is meant to augment the generator in a convenient manner; for example, I personally do things like `./generate 1 2 1 example_in.tsv example_out.tsv && ./check 1 2 1 <example_out.tsv` - this would not normally be possible without manually formatting `example_out.tsv`
