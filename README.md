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
The purpose of this program is to generate test suites of enumerated 2-dimensional arrays with the covering, locating, and detecting properties.

### This project is just getting started, so do not expect it to be functional for some weeks yet. 

## Additional Links
Colbourn and McClary, *[Locating and Detecting Arrays for Interaction Faults](https://drops.dagstuhl.de/opus/volltexte/2009/2240/pdf/09281.ColbournCharles.Paper.2240.pdf)*
- Paper cited as first to propose locating and detecting arrays
- Formal definitions for covering, locating, and detecting arrays can be found here

(Ignore the stuff below, I'm using it to test things by hand)

0   0   0
1   1   1   ->  [-2, -2, -2]
1   0   0  (0   0   0   ->  [ 2,  2,  2]    ->  1   0   0   ->  [ 0,  1,  1])
0   1   1   ->  [-2,  0,  0]
0   1   0  (0   0   0   ->  [ 0,  1,  1]    ->  0   1   0   ->  [ 0,  0,  0])
1   0   1   ->  [ 0, -1, -1]

1   1   0
0   0   1   ->  [-2, -2, -2]
0   0   0   ->  [ 0,  0,  0]
1   1   1   ->  [ 0,  0,  0]
1   0   0  (0   0   0   ->  [ 1,  1,  0]    ->  1   0   0   ->  [ 0,  0,  0])
0   1   1   ->  [-1, -1,  0]
