/**
   request_parse (Request Parse)

   This library is used to parse HTTP GET request strings.
   
   by J. King, 4 March 2025.
   All rights granted.
   $Revision: 1.3 $
   $Date: 2025/03/08 05:14:36 $
*/

#include "GetRequestParser.h"

/** @class ParseGetRequest
    @brief This class takes a string containing an HTTP get request
    for parsing. Get each name value pair by calling nextRequest.
    @return a class instance.
*/

GetRequestParser::GetRequestParser() : queryString(NULL), currentPair(NULL) {}

/** @method parse
    @brief Call this method once before calling nextRequest().  It
    makes a copy of the request string passed in so it is
    nondestructive.  Memory is release when nextRequest has returned
    all the name value pairs in the request string. It expects the
    request string to take the form of an html GET request contain
    form name/value pairs.  E.g.:
    GET /?ssid1=this &pw1=is fun!&ssid2=let's&pw2=go do it! HTTP/1.1
    @param request a charater string containing an HTTP GET request.
    @param set isPost to true if this is a post event. The default is GET.
    @return an int containing REQPARSE_OKAY if successful. Otherwise,
    an error code, REQPARSE_NO_QUERY_ERR or REQPARSE_MALLOC_ERR.
*/

int GetRequestParser::parse(const char *request, bool isPost) {
  end(); // this is supposed to free memory but I'm not sure
  
  char *query = (char *)request;
  if ( !isPost ) {
    query = strchr(request, '?');
    if ( !query ) {
      return REQPARSE_NO_QUERY_ERR;
    }
    query++;
  }
  
  queryString = (char *)malloc(strlen(query)+1);
  if ( !queryString ) {
    return REQPARSE_MALLOC_ERR;
  }
  strcpy(queryString, query);

  // snip off the HTTP 1.1 at the end
  char *p = strstr(queryString, " HTTP/1.1");
  if ( p ) *p = '\0';
  
  // start the parsing
  currentPair = strtok_r(queryString, "&", &savePtr);
  return REQPARSE_OKAY;
}

/** method nextRequest
    @brief call nextRequest repeatedly after calling parse().  Each
    successful call returns a request name/value pair in key and
    value.
    @param key (char *) the form variable name.
    @param value (char *) the value associated with name (e.g. key=value)
    @maxLen (size_t) the maximum length of keys and values. The keys and values
    will be truncated to this length if they exceed it.
    @return true if a pair was successfully returned, false
    otherwise. This method returns false when all the name/value pairs
    have been returned.
*/

bool GetRequestParser::nextRequest(char *key, char *value, size_t maxLen) {
  if ( !currentPair ) return false;
  
  char *eqPos = strchr(currentPair, '=');
  if ( !eqPos ) return false;
  
  size_t keyLen = eqPos - currentPair;
  size_t valueLen = strlen(eqPos+1);
  
  if (keyLen >= maxLen) keyLen = maxLen - 1;
  if (valueLen >= maxLen) valueLen = maxLen - 1;
  strncpy(key, currentPair, keyLen);
  key[keyLen] = '\0';
  strncpy(value, eqPos+1, valueLen);
  value[valueLen] = '\0';
  
  urlDecode(key);
  urlDecode(value);
  
  currentPair = strtok_r(NULL, "&", &savePtr);
  return true;
}

void GetRequestParser::end() {
  if ( queryString ) {
    free(queryString);
    queryString = NULL;
    currentPair = NULL;
  }
}

GetRequestParser::~GetRequestParser() {
    end();
}

/**
  urlDecode

  @brief typically passed strings that have already been parsed into
  names and values which allows special characters to be contained
  with these strings without screwing up the parsing.  This function
  does an in-place conversion.
  
  @param str the string to decode
  @return nothing
*/

void urlDecode(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '+') {
            *dst = ' ';  // Convert '+' to space
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = {src[1], src[2], '\0'};
            *dst = (char)strtol(hex, NULL, 16);  // Convert %XX hex to char
            src += 2;  // Skip the next two characters
        } else {
            *dst = *src;
        }
        src++;
        dst++;
    }
    *dst = '\0';  // Null-terminate the modified string
}
