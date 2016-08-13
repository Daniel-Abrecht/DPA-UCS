#define DPAUCS_HTTP_C

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <DPA/UCS/files.h>
#include <DPA/UCS/utils.h>
#include <DPA/UCS/logger.h>
#include <DPA/UCS/ressource.h>
#include <DPA/UCS/protocol/tcp.h>
#include <DPA/UCS/services/http.h>

#if DPAUCS_MAX_HTTP_CONNECTIONS > TRANSMISSION_CONTROL_BLOCK_COUNT
#undef DPAUCS_MAX_HTTP_CONNECTIONS
#endif

#ifndef DPAUCS_MAX_HTTP_CONNECTIONS
#define DPAUCS_MAX_HTTP_CONNECTIONS TRANSMISSION_CONTROL_BLOCK_COUNT
#endif

DPA_MODULE( http ){
  DPA_DEPENDENCY( tcp );
  DPA_DEPENDENCY( ressource );
}

#define HTTP_METHODS(X) \
  X( HEAD ) \
  X( GET )

#define HTTP_METHODS_ENUMERATE( METHOD ) HTTP_METHOD_ ## METHOD,
#define HTTP_METHODS_LIST( METHOD ) { #METHOD, sizeof(#METHOD)-1 },


//////////////////////////////////////////////////////////////////


enum HTTP_Method {
  HTTP_METHODS( HTTP_METHODS_ENUMERATE )
  HTTP_METHOD_COUNT,
  HTTP_METHOD_UNKNOWN = HTTP_METHOD_COUNT
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
  HTTP_PARSE_SKIP_HEADER_FIELD_NEWLINE,
  HTTP_PARSE_SKIP_HEADER_FIELD,
  HTTP_PARSE_HEADER_FIELD,
  HTTP_PARSER_ERROR
};

enum HTTP_ConnectionAction {
  HTTP_CONNECTION_CLOSE,
  HTTP_CONNECTION_KEEP_ALIVE,
  HTTP_CONNECTION_UPGRADE,
  HTTP_CONNECTION_ACTION_COUNT,
  HTTP_CONNECTION_ACTION_UNKNOWN = HTTP_CONNECTION_ACTION_COUNT
};

const char* HTTP_ConnectionActions[] = {
  "Close",
  "Keep-Alive",
  "Upgrade"
};


typedef struct HTTP_Connection {
  void* cid;
  const flash DPAUCS_ressource_entry_t* ressource;
  uint16_t status;
  struct {
    uint8_t major, minor;
  } version;
  enum HTTP_Method method;
  enum HTTP_ParseState parseState;
  enum HTTP_ConnectionAction connectionAction;
  const DPAUCS_service_t* upgradeService;
} HTTP_Connections_t;

static HTTP_Connections_t connections[DPAUCS_MAX_HTTP_CONNECTIONS];


//////////////////////////////////////////////////////////////////

typedef struct HTTP_error {
  uint16_t code;
  const char* message;
  uint8_t length;
} HTTP_error_t;

#define S(STR) STR, sizeof(STR)-1
static const HTTP_error_t HTTP_errors[] = {
  { 200, S("OK") },
  { 400, S("Bad Request") },
  { 401, S("Unauthorized") },
  { 402, S("Payment Required") },
  { 403, S("Forbidden") },
  { 404, S("Not Found") },
  { 405, S("Method Not Allowed") },
  { 406, S("Not Acceptable") },
  { 407, S("Proxy Authentication Required") },
  { 408, S("Request Time-out") },
  { 409, S("Conflict") },
  { 410, S("Gone") },
  { 411, S("Length Required") },
  { 412, S("Precondition Failed") },
  { 413, S("Request Entity Too Large") },
  { 414, S("Request-URL Too Long") },
  { 415, S("Unsupported Media Type") },
  { 416, S("Requested range not satisfiable") },
  { 417, S("Expectation Failed") },
  { 418, S("I’m a teapot") },
  { 420, S("Policy Not Fulfilled") },
  { 421, S("There are too many connections from your internet address") },
  { 422, S("Unprocessable Entity") },
  { 423, S("Locked") },
  { 424, S("Failed Dependency") },
  { 425, S("Unordered Collection") },
  { 426, S("Upgrade Required") },
  { 428, S("Precondition Required") },
  { 429, S("Too Many Requests") },
  { 431, S("Request Header Fields Too Large") },
  { 444, S("No Response") },
  { 480, S("Invalid Protocol") },
  { 449, S("The request should be retried after doing the appropriate action") },
  { 451, S("Unavailable For Legal Reasons") },
  { 500, S("Internal Server Error") },
  { 501, S("Not Implemented") },
  { 502, S("Bad Gateway") },
  { 503, S("Service Unavailable") },
  { 504, S("Gateway Time-out") },
  { 505, S("HTTP Version not supported") },
  { 506, S("Variant Also Negotiates") },
  { 507, S("Insufficient Storage") },
  { 508, S("Loop Detected") },
  { 509, S("Bandwidth Limit Exceeded") },
  { 510, S("Not Extended") }
};
#undef S
const size_t HTTP_error_count = sizeof(HTTP_errors)/sizeof(*HTTP_errors);

struct writeErrorPageParams {
  uint16_t code;
  bool headOnly;
};

#define S(STR) STR, sizeof(STR)-1
static bool writeErrorPage( DPA_stream_t* stream, void* ptr ){

  struct writeErrorPageParams* params = ptr;
  const HTTP_error_t* error = 0;

  for( unsigned i=0,n=HTTP_error_count; i<n; i++ )
  if( HTTP_errors[i].code == params->code ){
    error = HTTP_errors + i;
    break;
  }

  DPA_stream_referenceWrite( stream, S("HTTP/1.0 ") );
  char code_buf[7];
  char* code_string = code_buf + sizeof(code_buf);
  *--code_string = 0;
  *--code_string = ' ';
  {
    uint16_t c = params->code;
    do *--code_string = c % 10 + '0'; while( c /= 10 );
  }

  DPA_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(error) DPA_stream_referenceWrite( stream, error->message, error->length );

  DPA_stream_referenceWrite( stream, S( "\r\n"
    "Connection: Close" "\r\n"
    "Content-Type: text/plain" "\r\n"
    "\r\n"
  ));

  if( params->headOnly )
    return true;

  DPA_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(error) DPA_stream_referenceWrite( stream, error->message, error->length );

  return true;

}
#undef S

#define S(STR) STR, sizeof(STR)-1
static bool writeRessource( DPA_stream_t* stream, void* ptr ){

  HTTP_Connections_t* c = ptr;

  const HTTP_error_t* code = 0;

  for( unsigned i=0,n=HTTP_error_count; i<n; i++ )
  if( HTTP_errors[i].code == c->status ){
    code = HTTP_errors + i;
    break;
  }

  DPA_stream_referenceWrite( stream, S("HTTP/1.0 ") );
  char code_buf[7];
  char* code_string = code_buf + sizeof(code_buf);
  *--code_string = 0;
  *--code_string = ' ';
  {
    uint16_t s = c->status;
    do *--code_string = s % 10 + '0'; while( s /= 10 );
  }

  DPA_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(code) DPA_stream_referenceWrite( stream, code->message, code->length );

  DPA_stream_referenceWrite( stream, S( "\r\n"
    "Connection: Close" "\r\n"
  ));

  switch( c->method ){
    case HTTP_METHOD_GET:
    case HTTP_METHOD_HEAD: {
      DPAUCS_writeRessourceHeaders( stream, c->ressource );
      DPA_stream_referenceWrite( stream, S("\r\n") );
    } if( c->method == HTTP_METHOD_GET ) {
      DPAUCS_writeRessource( stream, c->ressource );
    } break;
    default: {
      DPA_stream_referenceWrite( stream, S("\r\n") );
    } break;
  }

  return true;
}
#undef S

void HTTP_sendErrorPage( void* cid, uint16_t code, bool headOnly ){
  struct writeErrorPageParams params = {
    .code = code,
    .headOnly = headOnly
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
  DPA_LOG("http_service->onopen\n");
  HTTP_Connections_t *it, *end;
  for( it = connections, end = it + DPAUCS_MAX_HTTP_CONNECTIONS; it < end; it++ )
    if(!it->cid) goto add_connection;
  return false;
 add_connection:
  it->cid = cid;
  it->parseState = HTTP_PARSE_START;
  DPA_LOG("New connection added\n");
  return true;
}


#define MEMEQ( STR, MEM, N ) ( N == sizeof(STR)-1 && !memcmp( (MEM), (STR), sizeof(STR)-1 ) )
#define S(STR) STR, sizeof(STR)-1
static void onreceive( void* cid, void* data, size_t size ){
  DPA_LOG("http_service->onrecive: \n");

  HTTP_Connections_t* c = getConnection(cid);
  if(!c){
    DPAUCS_tcp_abord( cid );
    return;
  }

  if( c->status == HTTP_PARSE_START ){
    c->status = 200;
    c->method = HTTP_METHOD_UNKNOWN;
    c->connectionAction = HTTP_CONNECTION_CLOSE;
    c->upgradeService = 0;
    c->version.major = 1;
    c->version.minor = 0;
  }

  char* it = data;
  size_t n = size;



  while( n ){
    switch( c->parseState ){


      case HTTP_PARSE_METHOD: {

        char* methodEnd = memchr(it,' ',n);
        if(!methodEnd){
          c->status = 400;
          c->parseState = HTTP_PARSER_ERROR;
          break;
        }

        enum HTTP_Method method;
        for( method=0; method<HTTP_METHOD_COUNT; method++ )
        if( HTTP_methods[method].length == (size_t)(methodEnd-it)
         && !memcmp( HTTP_methods[method].name, it, HTTP_methods[method].length )
        ) break;

        if( method == HTTP_METHOD_UNKNOWN )
          c->status = 405;

        c->method = method;
        c->parseState = HTTP_PARSE_PATH;

        n -= methodEnd - it + 1;
        it = methodEnd + 1;

      } continue;


      case HTTP_PARSE_PATH: {

        size_t lineEndPos;
        if( !DPA_mempos( &lineEndPos, it, n, S("\r\n") ) ){
          c->status = 400;
          c->parseState = HTTP_PARSER_ERROR;
          break;
        }

        size_t urlEnd;
        if( !DPA_memrcharpos( &urlEnd, lineEndPos, it, '/' )
         || !DPA_memrcharpos( &urlEnd, urlEnd,     it, ' ' )
        ){
          c->status = 400;
          c->parseState = HTTP_PARSER_ERROR;
          break;
        }

        c->ressource = getRessource( it, urlEnd );
        c->parseState = HTTP_PARSE_PROTOCOL;

        if( c->status < 400 && !c->ressource )
          c->status = 404;

        n  -= urlEnd + 1;
        it += urlEnd + 1;

      } continue;


      case HTTP_PARSE_PROTOCOL: {

        size_t lineEndPos = 0;
        size_t protocolEndPos = 0;

        DPA_mempos( &lineEndPos, it, n, S("\r\n") );
        DPA_memrcharpos( &protocolEndPos, lineEndPos, it, '/' );
        if( !DPA_streq_nocase( "HTTP", it, protocolEndPos ) )
          c->status = 480;

        c->parseState = HTTP_PARSE_VERSION;

        n  -= protocolEndPos + 1;
        it += protocolEndPos + 1;

      } continue;


      case HTTP_PARSE_VERSION: {

        size_t lineEndPos = 0;
        DPA_mempos( &lineEndPos, it, n, S("\r\n") );
        if(!memchr(it,'.',lineEndPos)){
          c->status = 400;
          c->parseState = HTTP_PARSER_ERROR;
          break;
        }

        const char* v = it;

        c->version.major = 0;
        while( *v >= '0' && *v <= '9' ){
          c->version.major *= 10;
          c->version.major += *v - '0';
          v++;
        }
        v++;

        c->version.minor = 0;
        while( *v >= '0' && *v <= '9' ){
          c->version.minor *= 10;
          c->version.minor += *v - '0';
          v++;
        }

        c->parseState = HTTP_PARSE_HEADER_FIELD;
        n  -= lineEndPos + 2;
        it += lineEndPos + 2;

      } continue;


      case HTTP_PARSE_SKIP_HEADER_FIELD_NEWLINE: if( *it == '\n' ){
        it++; n--; c->parseState = HTTP_PARSE_HEADER_FIELD;
        continue;
      }
      case HTTP_PARSE_SKIP_HEADER_FIELD: {
        size_t lineEndPos = 0;
        if(!DPA_mempos( &lineEndPos, it, n, S("\r\n") ))
          break;
        n  -= lineEndPos + 2;
        it += lineEndPos + 2;
        c->parseState = HTTP_PARSE_HEADER_FIELD;
      } continue;


      case HTTP_PARSE_HEADER_FIELD: {

        if( ( n >= 1 && *it == '\n' ) || ( n >= 2 && it[0]=='\r' && it[1]=='\n' ) ){
          c->parseState = HTTP_PARSE_END;
          it += 1 + (*it != '\n');
          n  -= 1 + (*it != '\n');
          break;
        }

        size_t key_length=0, lineEndPos=0;
        if( !DPA_mempos( &lineEndPos, it, n, S("\r\n") )
         || !DPA_mempos( &key_length, it, lineEndPos, S(":") )
        ){
          if( it[n-1] == '\r' ){
            c->parseState = HTTP_PARSE_SKIP_HEADER_FIELD_NEWLINE;
          }else{
            c->parseState = HTTP_PARSE_SKIP_HEADER_FIELD;
          }
          break;
        }

        const char* key = it;
        const char* value = it + key_length + 1;
        size_t value_length = lineEndPos - key_length - 1;
        DPA_memtrim( &value, &value_length, ' ' );

        if( DPA_streq_nocase( "Connection", key, key_length ) ){
          enum HTTP_ConnectionAction ca;
          for( ca=0; ca<HTTP_CONNECTION_ACTION_COUNT; ca++ )
            if( DPA_streq_nocase( HTTP_ConnectionActions[ca], value, value_length ) )
              break;
          c->connectionAction = ca;
        }else if( DPA_streq_nocase( "Upgrade", key, key_length ) ){
          
        }


        DPA_LOG("Header field: key=\"%.*s\" value=\"%.*s\"\n",
          (int)key_length,key,
          (int)value_length,value
        );

        n  -= lineEndPos + 2;
        it += lineEndPos + 2;

      } continue;


      case HTTP_PARSER_ERROR: return;


    }
    break;
  }



  if( c->parseState == HTTP_PARSER_ERROR ){
    HTTP_sendErrorPage( cid, c->status, c->method == HTTP_METHOD_HEAD );
    return;
  }

  if( c->parseState != HTTP_PARSE_END )
    return;

  if( c->status >= 400 ){
    HTTP_sendErrorPage( cid, c->status, c->method == HTTP_METHOD_HEAD );
    return;
  }

  DPAUCS_tcp_send( &writeRessource, &cid, 1, c );
  DPAUCS_tcp_close( cid );

}
#undef S
#undef MEMEQ

static void oneof( void* cid ){
  DPA_LOG("http_service->oneof\n");
  DPAUCS_tcp_close( cid );
}

static void onclose( void* cid ){
  DPA_LOG("http_service->onclose\n");
  HTTP_Connections_t* c = getConnection(cid);
  if(!c) return;
  c->cid = 0;
  DPA_LOG("Connection closed\n");
}

DPAUCS_service_t http_service = {
  .tos = PROTOCOL_TCP,
  .onopen = &onopen,
  .onreceive = &onreceive,
  .oneof = &oneof,
  .onclose = &onclose
};
