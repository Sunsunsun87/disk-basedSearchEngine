# Bug 1

## A) How is your program acting differently than you expect it to?
- There are large number of memory errors in FileParser test. They all read: "Invalid memory read of size 1"

## B) Brainstorm a few possible causes of the bug
- There must be some invalid memory utilizations.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- I gdb'ed and backtraced the issue. I found that it happens because when I malloc a char*
for a string pointer to be a copy of another existing pointer, I calculate the pointer
length by strlen(ptr0), but there should be 1 more for '\0'. I used length "strlen(ptr0) + 1"
and the issue is solved.


# Bug 2

## A) How is your program acting differently than you expect it to?
- The Memindx tests fail at first test assert. It reads 1 query and there should
be 2 found docs. The return list size is correct, but reading the list
is wrong.

## B) Brainstorm a few possible causes of the bug
- There could be errors in either Memindex_addpostings or Memindex_search
when dealing with index table insertions and looking up.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- I used gdb, wrote lines of codes, invoked methods and defined variables to clarify
the index table's correctness. It turns out my memindex_addpostings is correct,
but memindex_search is problematic in that it skips the first entry (a wrong
LLIterator_Next() line position).


# Bug 3

## A) How is your program acting differently than you expect it to?
- The Memindex segment faults in one test. The search result is wrong.

## B) Brainstorm a few possible causes of the bug
- There should be a wrongly referenced memory variable.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- I specified the codes and error messages and found that the
LLIterator_Get() method's parameter demands a LLPayload* (void **),
but I input a SearchResult *. It's wrong type and wrong pointer referencing.
