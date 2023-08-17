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

#include <iostream>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;
using std::cout;
using std::endl;

namespace hw4 {

static const char* kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;
static const int BUFFSIZE = 1024;

bool HttpConnection::GetNextRequest(HttpRequest* const request) {
  // Use WrappedRead from HttpUtils.cc to read bytes from the files into
  // private buffer_ variable. Keep reading until:
  // 1. The connection drops
  // 2. You see a "\r\n\r\n" indicating the end of the request header.
  //
  // Hint: Try and read in a large amount of bytes each time you call
  // WrappedRead.
  //
  // After reading complete request header, use ParseRequest() to parse into
  // an HttpRequest and save to the output parameter request.
  //
  // Important note: Clients may send back-to-back requests on the same socket.
  // This means WrappedRead may also end up reading more than one request.
  // Make sure to save anything you read after "\r\n\r\n" in buffer_ for the
  // next time the caller invokes GetNextRequest()!

  // STEP 1:

  size_t pos;
  int bytes;
  while ((pos = buffer_.find(kHeaderEnd)) == string::npos) {
    unsigned char buf[BUFFSIZE];
    bytes = WrappedRead(fd_, buf, BUFFSIZE);
    if (bytes <= 0)
      return false;
    string line(reinterpret_cast<const char*>(buf), bytes);
    buffer_.append(line);
  }
  string req = buffer_.substr(0, pos + kHeaderEndLen);
  buffer_ = buffer_.substr(pos + kHeaderEndLen);
  *request = ParseRequest(req);
  return true;
}

bool HttpConnection::WriteResponse(const HttpResponse& response) const {
  string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         reinterpret_cast<const unsigned char*>(str.c_str()),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(const string& request) const {
  HttpRequest req("/");  // by default, get "/".

  // Plan for STEP 2:
  // 1. Split the request into different lines (split on "\r\n").
  // 2. Extract the URI from the first line and store it in req.URI.
  // 3. For the rest of the lines in the request, track the header name and
  //    value and store them in req.headers_ (e.g. HttpRequest::AddHeader).
  //
  // Hint: Take a look at HttpRequest.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for:
  // - Splitting a string into lines on a "\r\n" delimiter
  // - Trimming whitespace from the end of a string
  // - Converting a string to lowercase.
  //
  // Note: If a header is malformed, skip that line.

  // STEP 2:
  // getting uri.
  vector<string> lines;
  boost::split(lines, request, boost::is_any_of("\r\n"),
               boost::token_compress_on);
  vector<string> components;
  boost::split(components, lines[0], boost::is_any_of(" "),
               boost::token_compress_on);
  req.set_uri(components[1]);

  for (size_t i = 1; i < lines.size(); i++) {
    boost::trim(lines[i]);
    vector<string> comps;
    boost::split(comps, lines[i], boost::is_any_of(": "),
                 boost::token_compress_on);
    if (comps.size() != 2) {
      continue;
    }
    boost::algorithm::to_lower(comps[0]);
    req.AddHeader(comps[0], comps[1]);
  }
  return req;
}

}  // namespace hw4
