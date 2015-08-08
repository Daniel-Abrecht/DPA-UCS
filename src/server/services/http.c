#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <files.h>
#include <utils.h>
#include <protocol/tcp.h>
#include <services/http.h>

#if DPAUCS_MAX_HTTP_CONNECTIONS > TRANSMISSION_CONTROL_BLOCK_COUNT
#undef DPAUCS_MAX_HTTP_CONNECTIONS 
#endif

#ifndef DPAUCS_MAX_HTTP_CONNECTIONS
#define DPAUCS_MAX_HTTP_CONNECTIONS TRANSMISSION_CONTROL_BLOCK_COUNT
#endif


DPAUCS_MODUL( http ){
  DPAUCS_DEPENDENCY( tcp );
}

#define HTTP_METHODS(X) \
  X( OPTIONS ) \
  X( HEAD ) \
  X( GET )

#define HTTP_METHODS_ENUMERATE( METHOD ) HTTP_METHOD_ ## METHOD,
#define HTTP_METHODS_LIST( METHOD ) { #METHOD, sizeof(#METHOD)-1 },


//////////////////////////////////////////////////////////////////


enum HTTP_Method {
  HTTP_METHODS( HTTP_METHODS_ENUMERATE )
  HTTP_METHOD_COUNT
};

static const struct {
  const char* name;
  size_t length;
} HTTP_methods[] = {
  HTTP_METHODS( HTTP_METHODS_LIST )
};

enum HTTP_ParseState {
  HTTP_PARSE_START  = 0,
  HTTP_PARSE_END    = 0,
  HTTP_PARSE_METHOD = 0,
  HTTP_PARSE_PATH,
  HTTP_PARSE_PROTOCOL,
  HTTP_PARSE_VERSION,
  HTTP_PARSE_HEADER_KEY,
  HTTP_PARSE_HEADER_VALUE,
  HTTP_PARSER_ERROR
};

typedef struct HTTP_Connection {
  void* cid;
  const DP_FLASH DPAUCS_resource_entry_t* resource;
  enum HTTP_Method method;
  enum HTTP_ParseState parseState;
} HTTP_Connections_t;

static HTTP_Connections_t connections[DPAUCS_MAX_HTTP_CONNECTIONS];


//////////////////////////////////////////////////////////////////

typedef struct HTTP_error {
  uint16_t code;
  const char* message;
  uint8_t length;
} HTTP_error_t;

static const HTTP_error_t HTTP_errors[] = {
  { 400, "Bad Request", 11 },
  { 401, "Unauthorized", 12 },
  { 402, "Payment Required", 16 },
  { 403, "Forbidden", 9 },
  { 404, "Not Found", 9 },
  { 405, "Method Not Allowed", 18 },
  { 406, "Not Acceptable", 14 },
  { 407, "Proxy Authentication Required", 29 },
  { 408, "Request Time-out", 16 },
  { 409, "Conflict", 8 },
  { 410, "Gone", 4 },
  { 411, "Length Required", 15 },
  { 412, "Precondition Failed", 19 },
  { 413, "Request Entity Too Large", 24 },
  { 414, "Request-URL Too Long", 20 },
  { 415, "Unsupported Media Type", 22 },
  { 416, "Requested range not satisfiable", 31 },
  { 417, "Expectation Failed", 18 },
  { 418, "I’m a teapot", 12 },
  { 420, "Policy Not Fulfilled", 20 },
  { 421, "There are too many connections from your internet address", 57 },
  { 422, "Unprocessable Entity", 20 },
  { 423, "Locked", 6 },
  { 424, "Failed Dependency", 17 },
  { 425, "Unordered Collection", 20 },
  { 426, "Upgrade Required", 16 },
  { 428, "Precondition Required", 21 },
  { 429, "Too Many Requests", 17 },
  { 431, "Request Header Fields Too Large", 31 },
  { 444, "No Response", 11 },
  { 449, "The request should be retried after doing the appropriate action", 64 },
  { 451, "Unavailable For Legal Reasons", 29 },
  { 500, "Internal Server Error", 21 },
  { 501, "Not Implemented", 15 },
  { 502, "Bad Gateway", 11 },
  { 503, "Service Unavailable", 19 },
  { 504, "Gateway Time-out", 16 },
  { 505, "HTTP Version not supported", 26 },
  { 506, "Variant Also Negotiates", 27 },
  { 507, "Insufficient Storage", 20 },
  { 508, "Loop Detected", 13 },
  { 509, "Bandwidth Limit Exceeded", 24 },
  { 510, "Not Extended", 12 }
};
const size_t HTTP_error_count = sizeof(HTTP_errors)/sizeof(*HTTP_errors);

struct writeErrorPageParams {
  uint16_t code;
  bool headOnly;
};

static DPAUCS_resource_entry_t* getResource( const char* path, unsigned length ){
  (void)path;
  (void)length;
  return 0;
}

#define S(STR) STR, sizeof(STR)-1
static bool writeErrorPage( DPAUCS_stream_t* stream, void* ptr ){

  struct writeErrorPageParams* params = ptr;
  const HTTP_error_t* error = 0;

  for( unsigned i=0,n=HTTP_error_count; i<n; i++ )
  if( HTTP_errors[i].code == params->code ){
    error = HTTP_errors + i;
    break;
  }

  DPAUCS_stream_referenceWrite( stream, S("HTTP/1.0 ") );
  char code_buf[7];
  char* code_string = code_buf + sizeof(code_buf);
  *--code_string = 0;
  *--code_string = ' ';
  {
    uint16_t c = params->code;
    do *--code_string = c % 10 + '0'; while( c /= 10 );
  }

  DPAUCS_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(error) DPAUCS_stream_referenceWrite( stream, error->message, error->length );

  DPAUCS_stream_referenceWrite( stream, S( "\r\n"
    "Connection: Close" "\r\n"
    "Content-Type: text/plain" "\r\n"
    "\r\n"
  ));

  if( params->headOnly )
    return true;

  DPAUCS_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(error) DPAUCS_stream_referenceWrite( stream, error->message, error->length );

  return true;

}
#undef S

void HTTP_sendErrorPageHead( void* cid, uint16_t code ){
  struct writeErrorPageParams params = {
    .code = code,
    .headOnly = true
  };
  DPAUCS_tcp_send( &writeErrorPage, &cid, 1, &params );
  DPAUCS_tcp_close( cid );
}

void HTTP_sendErrorPage( void* cid, uint16_t code ){
  struct writeErrorPageParams params = {
    .code = code,
    .headOnly = false
  };
  DPAUCS_tcp_send( &writeErrorPage, &cid, 1, &params );
  DPAUCS_tcp_close( cid );
}

//////////////////////////////////////////////////////////////////

static HTTP_Connections_t* getConnection( void* cid ){
  HTTP_Connections_t *it, *end;
  for( it = connections, end = it + DPAUCS_MAX_HTTP_CONNECTIONS; it < end; it++ )
    if( it->cid == cid )
      return it;
  return 0;
}

static bool onopen( void* cid ){
  printf("http_service->onopen\n");
  HTTP_Connections_t *it, *end;
  for( it = connections, end = it + DPAUCS_MAX_HTTP_CONNECTIONS; it < end; it++ )
    if(!it->cid) goto add_connection;
  return false;
 add_connection:
  it->cid = cid;
  it->parseState = HTTP_PARSE_START;
  printf("New connection added\n");
  return true;
}



#define S(STR) STR, sizeof(STR)-1
static void onreceive( void* cid, void* data, size_t size ){
  printf("http_service->onrecive: \n");

  HTTP_Connections_t* c = getConnection(cid);
  if(!c){
    DPAUCS_tcp_abord( cid );
    return;
  }

  char* it = data;
  size_t n = size;

  while( true ){
  switch( c->parseState ){


    case HTTP_PARSE_METHOD: {

      enum HTTP_Method method;
      for( method=0; method<HTTP_METHOD_COUNT; method++ )
      if( HTTP_methods[method].length < n
       && !memcmp( HTTP_methods[method].name, it, HTTP_methods[method].length )
       && it[HTTP_methods[method].length] == ' '
      ) break;

      if( method >= HTTP_METHOD_COUNT ){
        HTTP_sendErrorPage( cid, 405 );
        c->parseState = HTTP_PARSER_ERROR;
        break;
      }

      c->method = method;
      c->parseState = HTTP_PARSE_PATH;
      it += HTTP_methods[method].length + 1;
      n  -= HTTP_methods[method].length + 1;

    } continue;


    case HTTP_PARSE_PATH: {

      size_t lineEndPos;
      if( !mempos( &lineEndPos, it, n, S("\r\n") ) ){
        HTTP_sendErrorPage( cid, 400 );
        c->parseState = HTTP_PARSER_ERROR;
        break;
      }

      size_t urlEnd;
      if( !memrcharpos( &urlEnd, lineEndPos, it, '/' )
       || !memrcharpos( &urlEnd, urlEnd,     it, ' ' )
      ){
        HTTP_sendErrorPage( cid, 400 );
        c->parseState = HTTP_PARSER_ERROR;
        break;
      }

      if( !( c->resource = getResource( it, urlEnd ) ) ){
        printf("Resource \"%.*s\" not found\n",(int)urlEnd,it);
        HTTP_sendErrorPage( cid, 404 );
        c->parseState = HTTP_PARSER_ERROR;
        break;
      }

      c->parseState = HTTP_PARSE_PROTOCOL;
      it += urlEnd + 1;
      n  -= urlEnd + 1;

    } continue;

    case HTTP_PARSE_PROTOCOL: {
      break;
    } continue;

    case HTTP_PARSE_VERSION: {
      
    } continue;

    case HTTP_PARSE_HEADER_KEY: {
      
    } continue;

    case HTTP_PARSE_HEADER_VALUE: {
      
    } continue;

    case HTTP_PARSER_ERROR: break;

  } break; }

}
#undef S

static void oneof( void* cid ){
  printf("http_service->oneof\n");
  DPAUCS_tcp_close( cid );
}

static void onclose( void* cid ){
  printf("http_service->onclose\n");
  HTTP_Connections_t* c = getConnection(cid);
  if(!c) return;
  c->cid = 0;
  printf("Connection closed\n");
}

DPAUCS_service_t http_service = {
  .tos = PROTOCOL_TCP,
  .onopen = &onopen,
  .onreceive = &onreceive,
  .oneof = &oneof,
  .onclose = &onclose
};
