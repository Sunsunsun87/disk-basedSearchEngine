# Bug 1

## A) How is your program acting differently than you expect it to?
- The FileIndexReader.cc would constantly fail at Verifycations of fread.
  The read bytes was always 1, instead of sizeof(Header)

## B) Brainstorm a few possible causes of the bug
- Could be file stream buffer modes (unbuffered)
- Could be ferrors
- 

## C) How you fixed the bug and why the fix was necessary
- I looked up fread definitions, and found that the return of fread is not byte
count, but element count. Thus 1 should be correct.


# Bug 2

## A) How is your program acting differently than you expect it to?
- At step 3 of HashTableReader, the read position value was negative, and 
would not pass tests.

## B) Brainstorm a few possible causes of the bug
- Could be erroneous file read.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- I realized I didn't use wrapper class ElementPositionRecord, and 
I didn't convert to host format.


# Bug 3

## A) How is your program acting differently than you expect it to?
- In QueryProcessor, the return size of final result was 0.

## B) Brainstorm a few possible causes of the bug
- Could be wrong list updating.
- 
- 

## C) How you fixed the bug and why the fix was necessary
- I cleared whole list during counting docs, which shouldn't happen.
