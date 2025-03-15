/**
   GetRequestParser (Request Parse)

   This is the header file for the request_parse library which is used to
   parse HTTP GET request strings.
   
   by J. King, 4 March 2025.
   All rights granted.
   $Revision: 1.3 $
   $Date: 2025/03/08 05:15:04 $
*/

#ifndef GETREQUESTPARSER_H
#define GERREQUESTPARSER_H

#include <stdlib.h>
#include <string.h>

#define REQPARSE_OKAY          0
#define REQPARSE_NO_QUERY_ERR -1
#define REQPARSE_MALLOC_ERR   -2

class GetRequestParser {
private:
  char *queryString;
  char *currentPair;
  char *savePtr;
  
public:
  GetRequestParser();
  int parse(const char *request, bool isPost);
  bool nextRequest(char *key, char *value, size_t maxLen);
  void end();
  ~GetRequestParser();
};

/**
   urlDecode

   @brief typically passed strings that have already been parsed into
   names and values which allows special characters to be contained
   with these strings without screwing up the parsing.
   
   @param str the string to decode
   @return nothing
*/

void urlDecode(char *str);

#endif
