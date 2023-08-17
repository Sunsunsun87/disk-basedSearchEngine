# Bug 1

## A) How is your program acting differently than you expect it to?
- The output parameter of HashTable_Insert() function malfunctions, and test won't pass
at line 100 of test_hashtable.cc.

## B) Brainstorm a few possible causes of the bug
- The wrong pointer assignment. Use pass-by-address
- We need to malloc a memory on heap so that the variable persists through multiple functions.
- 

## C) How you fixed the bug and why the fix was necessary
- We used pass-by-address along with struct copy assignment so that the output parameter is modified to
store the correct keyvalue struct.
- We used malloc so that variable persists across multiple functions.


# Bug 2

## A) How is your program acting differently than you expect it to?
- The HTIterator_Next() function won't progress to next filled bucket, and test fails at line 240 of
test_hashtable.cc in the ASSERT_EQ of num_elements_times[htkey] being 0.

## B) Brainstorm a few possible causes of the bug
- Function isn't accomplished properly.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- We used gdp to set breakpoints, and backtrace to find function execution stack.
We found the problem to be wrong initialization: the new bucket_iterator is assigned with
the old bucket_idx so that it still points to same line.


# Bug 3

## A) How is your program acting differently than you expect it to?
- There are considerable memory leaks, while the tests all pass correctly. The memory leak
occurs near HTIterator_Allocate and _Next() at multiple locations in test.

## B) Brainstorm a few possible causes of the bug
- The HTIterator_Free() and LLIterator_Free() function have misused NULL conditions so that
some LLIterators won't be freed properly.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- Queried on Ed, and found that we didn't free the old LLIterators in HTIterator_Next() when
we try to update to the next line, we updated iter->bucket_it with newly allocated LLIterator,
but didn't free the previous one before hand.
