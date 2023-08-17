/*
 * Copyright ©2023 Chris Thachuk.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Spring Quarter 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "./CrawlFileTree.h"
#include "./DocTable.h"
#include "./MemIndex.h"

//////////////////////////////////////////////////////////////////////////////
// Helper function declarations, constants, etc

/*Prints out usage, and exits program*/
static void Usage(void);
/*Process the user input using MemIndex_Search*/
static void ProcessQueries(DocTable* dt, MemIndex* mi);
/*Get the user input from stdin, and copy into internal buffer to read*/
static int GetNextLine(FILE* f, char** ret_str);


//////////////////////////////////////////////////////////////////////////////
// Main
int main(int argc, char** argv) {
  if (argc != 2) {
    Usage();
  }

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - Crawl from a directory provided by argv[1] to produce and index
  //  - Prompt the user for a query and read the query from stdin, in a loop
  //  - Split a query into words (check out strtok_r)
  //  - Process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.
  //
  // Note that you should make sure the fomatting of your
  // searchshell output exactly matches our solution binaries
  // to get full points on this part.

  // get the user defined directory as the directory to be crawled
  char *directory = argv[1];
  DocTable *file_table;
  MemIndex *index;  // here I dont really understand

  printf("Indexing '%s'\n", directory);

  // here we need to check the crawling is correct or not
  // step 1: check the crawling
  if (!CrawlFileTree(directory, &file_table, &index)) {
    Usage();
  }

  // core process
  ProcessQueries(file_table, index);

  // free all of the space
  DocTable_Free(file_table);
  MemIndex_Free(index);



  return EXIT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
// Helper function definitions

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

static void ProcessQueries(DocTable* dt, MemIndex* mi) {
  int cnt = 0;
  // int number_of_elements;
  char* saver;
  LinkedList* search_result;

  while (1) {
    // malloc place for storaging user input, maximum length is 1024
    // indicating that user input should not be longer than 1024 char
    char* user_input = (char*) malloc(sizeof(char) * 1024);

    // get user input from stdin, copy it into *user_input
    if (GetNextLine(stdin, &user_input) != 0) {
      free(user_input);
      printf("shutting down ...\n");
      return;
    }

    // get the size of user input
    ssize_t user_input_length = strlen(user_input);

    // set the last element of user input to be \0 so further operations
    // can be achieved, including strtok_r
    user_input[user_input_length - 1] = '\0';  // use single quotes

    // convert all of the capitalized chars to lower case
    for (int i = 0; i < user_input_length; i++) {
      user_input[i] = tolower(user_input[i]);
    }

    // malloc space for the splited user input
    char** user_input_result = (char**) malloc(sizeof(char) * 1024);
    // define the saver pointer for sebsequent operations

    // use " " as delimeter
    *user_input_result = strtok_r(user_input, " ", &saver);
    cnt = 0;  // reset the value of cnt for each user input
    while (*(user_input_result + cnt) != NULL) {
      cnt++;  // increase the number of queries
      *(user_input_result + cnt) = strtok_r(NULL, " ", &saver);
    }

    // here we got the user input, just search
    search_result = MemIndex_Search(mi, user_input_result, cnt);

    // check if the result is null or not
    if (search_result == NULL) {
      free(user_input);
      free(user_input_result);
      continue;
    }

    LLIterator *ll_it;  // = LLIterator_Allocate(search_result);
    SearchResult *match;


    // traverse the linkedlist to find all of the documents containing queries
    for (ll_it = LLIterator_Allocate(search_result);
        LLIterator_IsValid(ll_it); LLIterator_Next(ll_it)) {
      LLIterator_Get(ll_it, (LLPayload_t*)&match);
      DocID_t doc_id = match->doc_id;
      char* doc_name = DocTable_GetDocName(dt, doc_id);

      printf("  %s", doc_name);
      printf(" (%d) \n", match->rank);
      // LLIterator_Next(ll_it);
    }

    // free all the contents
    LinkedList_Free(search_result, (LLPayloadFreeFnPtr)free);
    LLIterator_Free(ll_it);
    free(user_input);
    free(user_input_result);
  }
}

static int GetNextLine(FILE *f, char **ret_str) {
  // this function is used to get the input
  // from stdin (maximum 1024*sizeof(char))
  // and copy it into the destination position

  printf("enter query:\n");  // prompt the user to type in the words
  fgets(*ret_str, 1024, f);
  return feof(f);  // check if the user has typed ctrl + D
}
