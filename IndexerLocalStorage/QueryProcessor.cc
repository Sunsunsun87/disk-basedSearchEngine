/*
 * Copyright ©2023 Justin Hsia.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include "./QueryProcessor.h"

#include <iostream>
#include <algorithm>
#include <list>
#include <string>
#include <vector>
#include <map>
extern "C" {
  #include "./libhw1/CSE333.h"
}

#define INVALID_DOC -1

using std::list;
using std::sort;
using std::string;
using std::vector;
using std::map;
using std::pair;


namespace hw3 {

QueryProcessor::QueryProcessor(const list<string>& index_list, bool validate) {
  // Stash away a copy of the index list.
  index_list_ = index_list;
  array_len_ = index_list_.size();
  Verify333(array_len_ > 0);

  // Create the arrays of DocTableReader*'s. and IndexTableReader*'s.
  dtr_array_ = new DocTableReader* [array_len_];
  itr_array_ = new IndexTableReader* [array_len_];

  // Populate the arrays with heap-allocated DocTableReader and
  // IndexTableReader object instances.
  list<string>::const_iterator idx_iterator = index_list_.begin();
  for (int i = 0; i < array_len_; i++) {
    FileIndexReader fir(*idx_iterator, validate);
    dtr_array_[i] = fir.NewDocTableReader();
    itr_array_[i] = fir.NewIndexTableReader();
    idx_iterator++;
  }
}

QueryProcessor::~QueryProcessor() {
  // Delete the heap-allocated DocTableReader and IndexTableReader
  // object instances.
  Verify333(dtr_array_ != nullptr);
  Verify333(itr_array_ != nullptr);
  for (int i = 0; i < array_len_; i++) {
    delete dtr_array_[i];
    delete itr_array_[i];
  }

  // Delete the arrays of DocTableReader*'s and IndexTableReader*'s.
  delete[] dtr_array_;
  delete[] itr_array_;
  dtr_array_ = nullptr;
  itr_array_ = nullptr;
}

// This structure is used to store a index-file-specific query result.
typedef struct {
  DocID_t doc_id;  // The document ID within the index file.
  int     rank;    // The rank of the result so far.
} IdxQueryResult;

// helper functions

// Process first query word, and populate the map for query results.
// Parameters: a list of DocIDElementHeader found for the query word,
//   a map storing id->(queryresult, count) pair,
//   and a doctablereader for getting the file name strings.
void InitMap(const list<DocIDElementHeader>& docIDlist,
              map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map,
              DocTableReader* dtr);

// Process the rest of the queries.
// Parameters: a docIDlist, a map
void UpdateMap(const list<DocIDElementHeader>& docIDlist,
              map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map);

// Checking if the map has outdated entries (count smaller than
// processed queries, meaning not all words match.
void CheckMap(map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map,
              int queryLen);

// Adding index file's result into final result list.
// Parameters: map, a size equals query word count, and final result list.
void UpdateFinalResult(
      map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map,
      int size, vector<QueryProcessor::QueryResult>* final_result);

vector<QueryProcessor::QueryResult>
QueryProcessor::ProcessQuery(const vector<string>& query) const {
  Verify333(query.size() > 0);

  // STEP 1.
  // (the only step in this file)
  vector<QueryProcessor::QueryResult> final_result;
  for (int i = 0; i < array_len_; i++) {
    IndexTableReader* itr = itr_array_[i];
    DocTableReader* dtr = dtr_array_[i];

    map<DocID_t, pair<QueryProcessor::QueryResult, int>> map;
    DocIDTableReader* idr = itr->LookupWord(query[0]);
    if (!idr) {  // then nothing found. go to next idx file.
      continue;
    }
    list<DocIDElementHeader> docIDlist = idr->GetDocIDList();
    delete(idr);

    InitMap(docIDlist, &map, dtr);  // init map.

    // processing later queries.
    for (int j = 1; j < static_cast<int>(query.size()); j++) {
      idr = itr->LookupWord(query[j]);
      if (!idr) {  // empty query, query not found in idx
        map.clear();
        break;
      }
      list<DocIDElementHeader> docIDlist = idr->GetDocIDList();
      delete(idr);
      // we get lots of files here. for each file, we'll update
      // it in map. if not found, meaning this file doesn't match previous
      // queries, then we'll skip over this file.

      UpdateMap(docIDlist, &map);  // updating map fields.

      CheckMap(&map, j);  // checking map for outdated entries.
    }
    // adding map into final_result.
    UpdateFinalResult(&map, static_cast<int>(query.size() - 1), &final_result);
  }

  // Sort the final results.
  sort(final_result.begin(), final_result.end());
  return final_result;
}

void InitMap(const list<DocIDElementHeader>& docIDlist,
              map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map,
              DocTableReader* dtr) {
  for (auto& ele : docIDlist) {
    // creating a qr instance
    QueryProcessor::QueryResult qr;
    Verify333(dtr->LookupDocID(ele.doc_id, &(qr.document_name)));
    qr.rank = ele.num_positions;
    // inserting new qr into map, and count is 0.
    (*map)[ele.doc_id] = std::make_pair(qr, 0);
  }
}

void UpdateMap(const list<DocIDElementHeader>& docIDlist,
              map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map) {
  for (auto& ele : docIDlist) {  // getting each (docid, positions) header.
    auto it = map->find(ele.doc_id);
    // not found in map, then this doc should be skipped.
    if (it == map->end()) {
      continue;
    }
    if (it->second.second == INVALID_DOC) {  // if invalid, skip doc.
      continue;
    }
    auto entry = it->second;
    entry.first.rank += ele.num_positions;
    entry.second++;
    (*map)[ele.doc_id] = entry;  // updating map entry.
  }
}

void CheckMap(map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map,
              int queryLen) {
  for (auto& entry : *map) {  // ele is a pair<DocID_t, pair<QueryResult, int>>
    if (entry.second.second < queryLen) {
      entry.second.second = INVALID_DOC;
    }
  }
}

void UpdateFinalResult(
      map<DocID_t, pair<QueryProcessor::QueryResult, int>>* map,
      int size, vector<QueryProcessor::QueryResult>* final_result
      ) {
  for (auto& entry : *map) {
    if (entry.second.second != INVALID_DOC && entry.second.second == size) {
      final_result->push_back(entry.second.first);
    }
  }
}
}  // namespace hw3
