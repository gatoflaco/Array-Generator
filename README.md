# Array-Generator
### Author: Isaac Jung

## Credits
[Violet R. Syrotiuk, Ph.D.](https://isearch.asu.edu/profile/518662)
- Supervisor of this project
- Author of research on locating arrays

[Charles J. Colbourn, Ph.D.](https://isearch.asu.edu/profile/384970)
- Advisor to the author of this project
- Credited for [proposing locating and detecting arrays](#additional-links)

[Kevin John](https://github.com/kevin-cgc)
- classmate
- contributor and tester

## Overview
The purpose of this program is to generate test suites of enumerated 2-dimensional arrays with the covering, locating, and detecting properties. This readme assumes that users already have a general understanding of what these properties are. If you do not, try checking out the [links at the bottom of this page](#additional-links).

## Usage
```
./generate [d|] [t|] [δ|] [-[flags*]|] [--multichar_flags]* <input_filepath> [<output_filepath>|]
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

There are x_0 total problems to solve.

Array score is currently x_0.
Adding row #1.
> Pushed row: v_11 v_12 ... v_1C

[
Array score is currently x_i.
Adding row #n_i.
> Pushed row: v_i1 v_i2 ... v_iC
]*

Comleted array with n rows.

[Wrote array into file with path name <output_filepath>|The finished array is:]
[ARRAY|]
```
- The values of `v_i1` through `v_iC` describe the row that was chosen and added to the array.
- `x_i` begins at a relatively large positive number and decreases until 0.
- `n_i` begins at 1 and increments by 1 for every additional line. The final `n_i` will be equal to the `n` displayed on the `Completed array with n rows` line.  
- If an output file was specified, the final line of output will be `Wrote array into file with path name <output_filepath>`. If not, then it will be `The finished array is:` followed by the entire array.
- The output may be made more or less verbose than this using various [flags](#flags) on the command line. 
- If something goes wrong during the generation, e.g., the program detects that the requested properties are impossible to satisfy, it will attempt to halt, report the problem, and either save the partially completed array into the provided output file, or display it on the console, depending once again on whether an output file was provided. In this case, the final line(s) of output may vary.

## Options
The program may be invoked with a number of additional command line arguments and flags to alter its behavior. This is different from I/O redirection. Refer to the [usage](#usage) section for an example of what it looks like. This section describes the details:
- Despite the simplified visual in the usage section, the filepaths, [command line arguments](#command-line-arguments), and [flags](#flags) may actually come in any order, so long as the input file is given somewhere before any potential output file. Flags are distingushed by a hyphen character (-).
- The command line arguments should *not* have leading hyphens and are simply delimited by whitespace. The relative order of these arguments **actually matters**. While flags can be mixed in anywhere between the arguments, the arguments are interpreted like this: the first integer encountered is assumed to be t. If a second integer is encountered, it is assumed to be t with the previous t assumed to instead be d. If a third is encountered, it is assumed to be δ. This means that in order to specify d, you must also specify t, and in order to specify δ, you must also specify both d and t.
- The single char flags are demarcated by a leading hyphen. These flags may use separate hyphens or share a single one. To "share" a single hyphen, additional flags beyond the first must succeed each other directly, i.e., without any whitespace. If whitespace is used between flags, a hyphen must be prepended for each of the whitespace-separated groups of flags.
- The multi char flags are demarcated by two leading hyphens. These flags may *not* share hyphens; that is, they *must* be separated by whitespace.
- If the program cannot interpret a command line argument, it will ignore it and continue, possibly using default values/behaviors.
### Command Line Arguments
d: an integer bounded between 1 and t, inclusive
- Represents the magnitude of sets of interactions used; these sets are used in comparisons necessary to analyzing the locating and detecting properties of arrays.
- If not given, desired property is assumed to be t-covering.

t: an integer bounded between 1 and the number of factors, inclusive
- Represents how many (factor, value) tuples are included in an interaction; 1-way interactions by definition are just single (factor, value) tuples.
- If not given, 2 is used by default.

δ: an integer bounded below by 1
- Represents separation.
- If not given, desired property is assumed to be (d, t)-locating, or even just t-covering when d is also not given.
### Flags
d: debug
- States what flags are set, as well as the relevant values of d, t, and δ, prior to reading input.
- Displys the state of all internal single (factor, value) pairs, t-way interactions, and, if more than coverage is requested, size-d sets of t-way interactions. Due to the nature of generation happening from scratch, the sets of rows on which these occur should always be empty at this point.
- States when a row becomes any type of "don't care". Don't cares are categorized into coverage, location, and detection. E.g., if factor 2 is "don't care" type location, then no matter what value is chosen for that factor, no more coverage or location problems will be solved.

v: verbose
- Breaks down the `Array score is currently x_i` line into sub scores for coverage, location, and detection individually, as applicable.
- States what heuristic is being used to choose the current row.

h: halfway
- Reduces output by condensing to one line per row added.
- Gets rid of the `> Pushed row: v_i1 v_i2 ... v_iC` line altogether.
- Mutually exclusive with the s flag; if both are specified, the last one seen takes priority.

s: silent
- Does not produce any output, except for the finished array when no output file was specified.
- Mutually exclusive with the h flag; if both are specified, the last one seen takes priority.
- Higher priority than the d and v flags; if d and/or v is specified along with s, both d and v will be disabled.
### Multichar Flags
partial <partial_filepath>:
- Provides the program with a partial array to start; it is possible for the partial array to already satisfy all properties, in which case the program generates no additional rows.
- The format of partial array input file should follow the same conventions as the output array produced by this program.
- Note that the partial_filename argument must follow directly after the --partial argument, separated by whitespace.

help:
- Prints out simple explanation of usage.
- When the executable is run with no additional arguments, program defaults to this behavior.

## Details and Definitions
The program begins by interpreting command line arguments and flags to set state variables, then getting input from the specified input file. It passes all of this info to an Array object constructor, which sets up a lot of internal vectors and sets for organizing data and tracking scores, etc. When this is done, the main program adds the first row, which is completely randomly generated within the constraints provided. After the first row, the program then enters a loop in which it calls a method that adds a row based on scoring heuristics. It does this until the array is completed with the requested properties. After every row added, even the first, the array object updates its internal data structures. This is important for making scoring decisions in the heuristics that decide what rows to add, and for tracking the overall progress of the array generation. An overall score based on the total "problems" to solve determines when the array is completed; the number starts off large and decreases as problems are solved. When the overall score is 0, all problems are solved and the array is completed with the requested properties.

If, during the construction process, the main loop appears to get stuck making no further progress towards completion, the program is capable of interrupting itself and saving/displaying how far it was able to get before getting stuck. The reason for this is that the user may accidentally request a combination of properties that are actually impossible to satisfy based on the given factors and their levels. The lookahead logic to detect this is not well understood yet, so while some basic checks attempt to catch impossible requests before generation even begins, sometimes and impossible request makes it through and needs to be caught as generation is happening. I believe that all types of impossible requests can be generalized to mathematical relationships (albeit, some are quite complicated), so if possible, I would like to eventually be able to catch all impossible requests in the beginning. This would save the user the time of waiting for the program to get stuck just to report a request as impossible.

The most interesting part of this program is the problem of scoring a potential choice for a row to add and tweaking it to improve without necessarily exploring all possible combinations (for efficiency). To tackle this problem, different heuristics have been implemented that tradeoff row choice for time/space resources needed in the computation. That is, some heuristics make a decision quickly, but it may not have been the best decision possible (to find the actual best decision possible would likely require exploring not just alternate choices for the current row, but also future rows; the tradeoff in computing resources needed eventually becomes not worthwhile). These heuristics are generally good early on in array construction, when the number of problems to consider is large; when there is too much to think about, one shouldn't overthink, and instead make the assumption that many choices can come close enough to the "best" one with a rough estimate. Other heuristics spend a long time computing in order to come up with better decisions. These heuristics are not used until many problems have already been solved, thereby reducing the amount of work they really need to do. What follows is a running list of heuristics used. Please note that the names are based on roughly when they should be used, but not exactly; e.g., heuristic_l_and_d will be used when there are many location and detection problems but not many coverage problems, however, it is possible even when this heuristic is used that there are still a subjectively substantial number of coverage problems remaining. In other words, do not mistake the names for a rule for when they are used.

1. heuristic_c_only:
  This heuristic aims to solve missing coverage in a relatively quick manner. It beings with a 1-dimensional vector of size equal to the number of factors, wherein each element of the vector will correspond to each factor. All values in this vector begin at 0. The heuristic looks at the current choice for the row and considers all t-way interactions that occur. If a given t-way interaction is already covered, the values in the vector whose positions correspond to the factors involved have their values incremented. If the interaction is not covered, those values are instead decremented. At the end, the values in the vector are summed. If the sum is 0 or less, the row is decided to be "good enough" and allowed to be added. If the score is not good enough, a helper method is called repeatedly to modify the row in an attempt to improve it. It begins with the "most suspect" factor - the one whose corresponding value in the vector was highest, and tweaks one factor's level in the row at a time in this way, rescoring the vector after each change in a similar manner as before. The goal at this point becomes simply to reduce the vector's sum to a number less than the original. When that happens, it must be true that some interaction was found that was not covered, and the row is kept (again, the goal of this heuristic is just to be extremely fast). The heuristic also uses the concept of "don't cares"; ignoring the scoring of factors for which all interactions involving said factors are already covered. Still, for covering arrays, this heuristic has poor performance when the array is close to completed. However, covering arrays are small compared to locating and especially detecting arrays, so it is less important to achieve optimality in array size. Even so, the heuristic should not be used the whole time. For locating and detecting arrays, the heuristic should be used only briefly at the beginning. The hope is that even if it doesn't make the best choice in terms of coverage, many location and detection issues will be resolved no matter what near the beginning, and so it's fine to make an almost-thoughtless decision.

2. heuristic_l_only:
  This heuristic aims to solve missing location under the assumption that coverage is low priority. It starts by finding the interaction involved in the most problems. When there are ties, all those tied are tracked. Then, for every set of interactions possible, this heuristic filters down a working list comprised of only the sets that contain the tracked interaction(s) from the beginning. From thiese, the set of interactions with the most location conflicts is chosen to be locked in. Ties here are resolved randomly, since any set chosen should contain at least one of the interactions being tracked. The columns in the current row corresponding to one of the interactions in the chosen set are locked into the values necessary to form the interaction, guaranteeing that the set is present in the row. With the remaining columns that are not locked, it attempts to pick values that look like they would solve as many of the locked set's location conflicts as possible. The way this is done is to loop over all the sets in the locked one's location conflicts, incrementing a counter in a map. The map is from (factor, value) singles to the number of times they have shown up in the locked set's location conflicts. After all the counts are complete, for each unlocked column, the value with the minimum count is chosen. The idea is that the values with high counts are very bad choices, as they are likely to result in continued location conflicts. Because the counting loop iterates over all location conflicts, this heuristic becomes faster as the array gets closer to completion, because it is more likely that even the worst set of interactions will have less location conflicts to iterate over. Meanwhile the number of unlocked columns to consider will depend on the values of d and t chosen by the user compared to the total number of factors.

3. heuristic_l_and_d:
  This heuristic aims to solve missing detection under the assumption that coverage is low priority, but location may still be medium to high priority. It starts by finding the interaction involved in the most problems. When there are ties, all those tied are tracked. Ties are broken by considering which interaction has the lowest separation from all sets of interactions possible. Once an interaction is definitely chosen, the columns in the current row corresponding to the interaction are locked into the values necessary to form the interaction, guaranteeing that the interaction is present in the row. With the remaining columns that are not locked, it attempts to pick values that look like they will increase the as much of the locked interaction's separation as possible. The way this is done is to loop over all the sets in the locked interaction's map of deltas, adding the [remaining needed] separation to a counter in a map that starts at 0. The map is from (factor, value) singles to the amount of separation they still need, totaled over all sets in which they occur. After all the counts are complete, for each unlocked column, the value with the maximum counter is chosen. The idea is that the values with high counts are very good choices, as they are likely to be involved in sets that require further separation from the locked interaction. Because the counting loop skips incrementing the counters for singles associated with any set which already has sufficient separation from the locked interaction, it is possible for this heuristic to become slightly faster as the array gets closer to completion, because it is more likely that a given set will already have sufficient separation. Also note that because separation is so closely related to location conflicts, this heuristic can also serve to solve location conflicts well, albeit with slightly more work.

3. heuristic_d_only:
  This heuristic can be used to solve all types of missing properties with decent efficacy, but likely should not be used till the array is getting closer to complete, because it performs a significant amount of work more than the other heuristics above. The explanation of how it works is the same as heuristic_all below, except that the total number of rows to be scored is much less, because instead of scoring all rows indiscriminately, it chooses a locked interaction and scores only the rows that contain said interaction. The method of choosing the locked interaction is the same as in heuristic_l_only and heuristic_l_and_d; the one involved in the most problems of all types is chosen. Here, ties are simply broken randomly. The purpose of this heuristic is to be a lighter-weight version of heuristic_all.

4. heuristic_all:
  This heuristic can be used to solve all types of missing properties with great efficacy. The way it works is to pretend that the row up for consideration is going to be added; that is, it literally adds the row and calls the method that updates internal data structures, comparing the states of things before and after. In order to do this, a thread is started on the scoring method, which begins by creating a copy of all relevant internal data. It is important to perform the addition of the row on this clone of the array, so that after making the comparison to the original and noting the score, the clone can simply be deleted, the original remaining unchanged. This way, when another row is considered, the same steps may be followed. This also lends itself to the possibility of multithreading; since every thread can make its own local copy of the original array without modifying it, multiple potential rows can be tested at the same time, speeding up an otherwise time-cumbersome process (at the tradeoff of a higher cost in memory usage). A top-down recursive helper method goes through the construction of all possible rows, spawning a new thread to test every row that gets formed. Once all rows have been scored, the main thread proceeds to pick the row that scored best. When there is a tie, a winner is selected randomly. As for how the scoring is done, it is more-or-less simply the summation of the individul improvements in coverage, location, and detection at the level of single (factor, value) pairs. Weight is given to each category such that solving detection issues is worth more than solving location issues, and solving location issues is worth more than solving coverage issues. The thinking is that in general, detection is harder to satisfy than location, and location is harder to satisfy than coverage. So, the heuristic should not select a row simply because it solves a lot of problems, if for example, those problems are mostly to do with coverage. Besides, in attempting to solve detection issues, many location/coverage issues are solved in the process anyway. Also note that because this heuristic calls the method that updates internal data structures - over and over (once per row) - its time behavior is dominated by that method, which is known to be one of the most computationally intensive parts of the program. So, similarly to that method, execution speed improves as problems are solved. This means that this heuristic can score faster the closer the array is to complete, providing one more reason why weight is assigned to each sub category of the scoring; by the time this heuristic is realistically ready to be called, most of the easieer problems to solve are probably already solved or close to being solved anyway. In short, while this method takes all types of properties into account, it is mainly intended to clean up the last missing ones near the end, which are likely to be primarily detection problems.

## Additional Links
Colbourn and McClary, *[Locating and Detecting Arrays for Interaction Faults](https://drops.dagstuhl.de/opus/volltexte/2009/2240/pdf/09281.ColbournCharles.Paper.2240.pdf)*
- Paper cited as first to propose locating and detecting arrays
- Formal definitions for covering, locating, and detecting arrays can be found here

[My Array-Checker](https://github.com/gatoflaco/Array-Checker)
- Tool to verify the properties of arrays that this project generates
- The input-format-simple branch of that project is meant to augment the generator in a convenient manner; for example, I personally do things like `./generate 1 2 1 example_in.tsv example_out.tsv && ./check 1 2 1 <example_out.tsv` - this would not normally be possible without manually formatting `example_out.tsv`
