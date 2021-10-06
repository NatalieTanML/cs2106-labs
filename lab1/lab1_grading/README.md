# A script to automate grading of coding assignments

To use the grader for self-testing:
* Zip submission and put it into a separated directory, which contains the zip file only.
* `cd` to the directory of `grader`.
* `./grader -b ./baseline -s <directory of zip>`.
* Check `results.csv` for scores and `grader.err` for errors.

**Exercise 2: 22 points**
* 2 points for sucessful compiling `ex2`, otherwise exercise 2 is marked 0 point.
* 2x 2 points for public testcases: `small` and `big`.
* 6x 2 points for private testcases: `ultra`, `insert`, `delete`, `rotatelist`, `reverselist` and `resetlist`. All testcases can be found in `baseline/ex2`.
* 2x 2 points for no memory allocation while reversing/rotating lists. Verifier can be found in `baseline/ex2`.

**Exercise 3: 8 points**
* 2 points for sucessful compiling `ex3`, otherwise exercise 3 is marked 0 point.
* 2 points for private testcases: `ultra`. Testcase can be found in `baseline/ex3`.
* 2 points for sucessful compiling `ex3.c` against a customized `function_pointer.h/.c` pair, which defines and initializes a different array of functions to the public version of `function.h/.c`. A correct implementation of `ex.3` should be agnostic to the content of the function array and should work well with the customized version natually.
* 2 points for private testcases: `fp_test` that performs on the executable built in previous step. The supported files can be found in `baseline/ex3`.

**Exercise 6: 4 points**
For using proper command: `strance --summary <exeutable>` (or equivalent).
