#define DPAUCS_HTTP_C

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <DPA/UCS/files.h>
#include <DPA/utils/utils.h>
#include <DPA/utils/logger.h>
#include <DPA/UCS/ressource.h>
#include <DPA/UCS/protocol/tcp/tcp.h>
#include <DPA/UCS/services/http.h>

#if DPA_MAX_HTTP_CONNECTIONS > TRANSMISSION_CONTROL_BLOCK_COUNT
#undef DPA_MAX_HTTP_CONNECTIONS
#endif

#ifndef DPA_MAX_HTTP_CONNECTIONS
#define DPA_MAX_HTTP_CONNECTIONS TRANSMISSION_CONTROL_BLOCK_COUNT
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

#define S(a) (const flash char[]){a}
const flash char* HTTP_ConnectionActions[] = {
  S("Close"),
  S("Keep-Alive"),
  S("Upgrade")
};
#undef S


typedef struct HTTP_Connection {
  void* cid;
  const DPAUCS_ressource_entry_t* ressource;
  uint16_t status;
  struct {
    uint8_t major, minor;
  } version;
  enum HTTP_Method method;
  enum HTTP_ParseState parseState;
  enum HTTP_ConnectionAction connectionAction;
  const flash DPAUCS_http_upgrade_handler_t* upgrade_handler;
} HTTP_Connections_t;

static HTTP_Connections_t connections[DPA_MAX_HTTP_CONNECTIONS];


//////////////////////////////////////////////////////////////////

typedef struct HTTP_error {
  uint16_t code;
  const flash char* message;
  uint8_t length;
} HTTP_error_t;

#define S(STR) (const flash char[]){STR}, sizeof(STR)-1
static const flash HTTP_error_t HTTP_errors[] = {
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
const flash size_t HTTP_error_count = sizeof(HTTP_errors)/sizeof(*HTTP_errors);

struct writeErrorPageParams {
  uint16_t code;
  bool headOnly;
};

static const flash char http_1_0_[] = {"HTTP/1.0 "};

#define S(STR) STR, sizeof(STR)-1
static bool writeErrorPage( DPA_stream_t* stream, void* ptr ){

  struct writeErrorPageParams* params = ptr;
  const flash HTTP_error_t* error = 0;

  for( unsigned i=0,n=HTTP_error_count; i<n; i++ )
  if( HTTP_errors[i].code == params->code ){
    error = HTTP_errors + i;
    break;
  }

  DPA_LOG("HTTP %"PRIu16": %"PRIsFLASH"\n",params->code,(error?error->message:(const flash char*)0));

  DPA_stream_progmemWrite( stream, S(http_1_0_) );
  char code_buf[7];
  char* code_string = code_buf + sizeof(code_buf);
  *--code_string = 0;
  *--code_string = ' ';
  {
    uint16_t c = params->code;
    do *--code_string = c % 10 + '0'; while( c /= 10 );
  }

  DPA_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(error) DPA_stream_progmemWrite( stream, error->message, error->length );

  static const flash char headers[] = {
    "\r\n"
    "Connection: Close" "\r\n"
    "Content-Type: text/plain" "\r\n"
    "\r\n"
  };
  DPA_stream_progmemWrite( stream, S(headers) );

  if( params->headOnly )
    return true;

  DPA_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(error) DPA_stream_progmemWrite( stream, error->message, error->length );

  return true;

}
#undef S

#define S(STR) STR, sizeof(STR)-1
static bool writeRessource( DPA_stream_t* stream, void* ptr ){

  HTTP_Connections_t* c = ptr;

  const flash HTTP_error_t* code = 0;

  for( unsigned i=0,n=HTTP_error_count; i<n; i++ )
  if( HTTP_errors[i].code == c->status ){
    code = HTTP_errors + i;
    break;
  }

  DPA_LOG("HTTP %"PRIu16": %"PRIsFLASH"\n",c->status,(code?code->message:(const flash char*)0));

  DPA_stream_progmemWrite( stream, S(http_1_0_) );
  char code_buf[7];
  char* code_string = code_buf + sizeof(code_buf);
  *--code_string = 0;
  *--code_string = ' ';
  {
    uint16_t s = c->status;
    do *--code_string = s % 10 + '0'; while( s /= 10 );
  }

  DPA_stream_copyWrite( stream, code_string, code_buf + sizeof(code_buf) - code_string - 1 );
  if(code) DPA_stream_progmemWrite( stream, code->message, code->length );

  static const flash char connection_header[] = {"\r\nConnection: Close"};
  DPA_stream_progmemWrite( stream, S(connection_header) );

  static const flash char CR_LF_CR_LF[] = {"\r\n\r\n"};

  switch( c->method ){
    case HTTP_METHOD_GET:
    case HTTP_METHOD_HEAD: {
      const flash char* mime = 0;
      if( c->ressource && c->ressource->handler->getMime )
        mime = c->ressource->handler->getMime(c->ressource);
      const char* hash = 0;
      if( c->ressource && c->ressource->handler->getHash )
        hash = c->ressource->handler->getHash(c->ressource);
      if( mime ){
        static const flash char content_type_header[] = {"\r\nContent-Type: "};
        DPA_stream_progmemWrite( stream, S(content_type_header) );
        DPA_stream_progmemWrite( stream, mime, DPA_progmem_strlen(mime) );
      }
      if( hash ){
        static const flash char etag_header[] = {"\r\nEtag: "};
        DPA_stream_progmemWrite( stream, S(etag_header) );
        DPA_stream_copyWrite( stream, hash, strlen(hash) );
      }
      DPA_stream_progmemWrite( stream, S(CR_LF_CR_LF) );
    } if( c->method == HTTP_METHOD_GET ) {
      if( c->ressource && c->ressource->handler->read )
        c->ressource->handler->read( c->ressource, stream );
    } break;
    default: {
      DPA_stream_progmemWrite( stream, S(CR_LF_CR_LF) );
    } break;
  }

  return true;
}
#undef S

#define S(STR) STR, sizeof(STR)-1
static bool writeUpgrade( DPA_stream_t* stream, void* ptr ){

  HTTP_Connections_t* c = ptr;

  DPA_LOG("HTTP 101 Switching Protocols\n");

  static const flash char upgrade_headers[] = {
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Connection: Upgrade\r\n"
    "Upgrade: "
  };
  DPA_stream_progmemWrite( stream, S(upgrade_headers) );
  DPA_stream_progmemWrite( stream, c->upgrade_handler->protocol, DPA_progmem_strlen(c->upgrade_handler->protocol) );

  static const flash char CR_LF[] = {"\r\n"};
  DPA_stream_progmemWrite( stream, S(CR_LF) );

  if( c->upgrade_handler->add_response_headers )
    if( !c->upgrade_handler->add_response_headers( c->cid, stream ) )
      return false;

  if( !DPA_stream_progmemWrite( stream, S(CR_LF) ) ){
    if( c->upgrade_handler->abort )
      c->upgrade_handler->abort(c->cid);
    return false;
  }

  const void* ssdata = 0;
  if( c->upgrade_handler->getssdata )
    ssdata = (*c->upgrade_handler->getssdata)(c->cid);
  DPAUCS_tcp_change_service(c->cid,c->upgrade_handler->service,ssdata);

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
  for( it = connections, end = it + DPA_MAX_HTTP_CONNECTIONS; it < end; it++ )
    if( it->cid == cid )
      return it;
  return 0;
}

static bool onopen( void* cid, const void* ssdata ){
  (void)ssdata;
  DPA_LOG("http_service->onopen %p\n",cid);
  HTTP_Connections_t *it, *end;
  for( it = connections, end = it + DPA_MAX_HTTP_CONNECTIONS; it < end; it++ )
    if(!it->cid) goto add_connection;
  return false;
 add_connection:
  it->cid = cid;
  it->parseState = HTTP_PARSE_START;
  it->status = 200;
  it->method = HTTP_METHOD_UNKNOWN;
  it->connectionAction = HTTP_CONNECTION_CLOSE;
  it->upgrade_handler = 0;
  it->version.major = 1;
  it->version.minor = 0;
  DPA_LOG("New connection added %p -> %"PRIuSIZE"\n",cid,it-connections);
  return true;
}


#define S(STR) STR, sizeof(STR)-1
static void onreceive( void* cid, void* data, size_t size ){
  HTTP_Connections_t* c = getConnection(cid);
  DPA_LOG("http_service->onreceive: %p -> %"PRIuSIZE"\n",cid,c-connections);
  if(!c){
    DPAUCS_tcp_abord( cid );
    return;
  }

  char* it = data;
  char* headers_start=data;
  size_t n = size;
  size_t headers_size=n;

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

        c->ressource = ressourceOpen( it, urlEnd );
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
        static const flash char fstr_http[] = {"HTTP"};
        if( ( sizeof(fstr_http)-1 == protocolEndPos ) && !DPA_streq_nocase_fn( fstr_http, it, protocolEndPos ) )
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
        headers_start = it;
        headers_size = n;

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
        headers_start = it;
        headers_size = n;
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

        static const flash char fstr_Connection[] = {"Connection"};
        static const flash char fstr_Upgrade[] = {"Upgrade"};

        if( c->upgrade_handler ){
          if( c->upgrade_handler->process_header ){
            if( !c->upgrade_handler->process_header( cid, key_length, key, value_length, value ) ){
              c->status = 400;
              c->upgrade_handler = 0;
              c->parseState = HTTP_PARSER_ERROR;
              c->connectionAction = HTTP_CONNECTION_CLOSE;
            }
          }
        }else{
          if( sizeof(fstr_Connection)-1 == key_length && DPA_streq_nocase_fn( fstr_Connection, key, key_length ) ){
            enum HTTP_ConnectionAction ca;
            for( ca=0; ca<HTTP_CONNECTION_ACTION_COUNT; ca++ )
              if( DPA_progmem_strlen(HTTP_ConnectionActions[ca]) == value_length && DPA_streq_nocase_fn( HTTP_ConnectionActions[ca], value, value_length ) )
                break;
            c->connectionAction = ca;
          }else if( sizeof(fstr_Upgrade)-1 == key_length && DPA_streq_nocase_fn( fstr_Upgrade, key, key_length ) ){
            c->status = 501;
            for( struct http_upgrade_handler_list* uhe = http_upgrade_handler_list; uhe; uhe = uhe->next ){
              if( DPA_progmem_strlen(uhe->entry->protocol) != value_length || !DPA_streq_nocase_fn( uhe->entry->protocol, value, value_length ))
                continue;
              c->status = 101;
              c->connectionAction = HTTP_CONNECTION_UPGRADE;
              c->upgrade_handler = uhe->entry;
              if( c->ressource && c->ressource->handler->close )
                c->ressource->handler->close( c->ressource );
              c->ressource = 0;
              if( uhe->entry->start ){
                if( !(*uhe->entry->start)( cid ) ){
                  c->status = 503;
                  c->upgrade_handler = 0;
                  c->parseState = HTTP_PARSER_ERROR;
                  c->connectionAction = HTTP_CONNECTION_CLOSE;
                }
              }
              n = headers_size;
              it = headers_start;
              continue;
            }
          }
        }

        if( c->parseState == HTTP_PARSER_ERROR )
          break;

/*        DPA_LOG("Header field: key=\"%.*s\" value=\"%.*s\"\n",
          (int)key_length,key,
          (int)value_length,value
        );*/

        n  -= lineEndPos + 2;
        it += lineEndPos + 2;

      } continue;


      case HTTP_PARSER_ERROR: return;


    }
    break;
  }



  if( c->parseState == HTTP_PARSER_ERROR ){
    if( c->ressource && c->ressource->handler->close )
      c->ressource->handler->close( c->ressource );
    if( c->upgrade_handler && c->upgrade_handler->abort )
      c->upgrade_handler->abort( cid );
    HTTP_sendErrorPage( cid, c->status, c->method == HTTP_METHOD_HEAD );
    return;
  }

  if( c->parseState != HTTP_PARSE_END )
    return;

  if( c->status >= 400 ){
    if( c->ressource && c->ressource->handler->close )
      c->ressource->handler->close( c->ressource );
    if( c->upgrade_handler && c->upgrade_handler->abort )
      c->upgrade_handler->abort( cid );
    HTTP_sendErrorPage( cid, c->status, c->method == HTTP_METHOD_HEAD );
    return;
  }

  if( c->upgrade_handler ){
    if( !DPAUCS_tcp_send( &writeUpgrade, &cid, 1, c ) )
      HTTP_sendErrorPage( cid, 400, c->method == HTTP_METHOD_HEAD );
  }else{
    DPAUCS_tcp_send( &writeRessource, &cid, 1, c );
    DPAUCS_tcp_close( cid );
    if( c->ressource && c->ressource->handler->close )
      c->ressource->handler->close( c->ressource );
  }

}
#undef S

static void oneof( void* cid ){
  DPA_LOG("http_service->oneof %p\n",cid);
  DPAUCS_tcp_close( cid );
}

static void onclose( void* cid ){
  DPA_LOG("http_service->onclose %p\n",cid);
  HTTP_Connections_t* c = getConnection(cid);
  if(!c) return;
  if( c->ressource && c->ressource->handler->close )
    c->ressource->handler->close( c->ressource );
  c->cid = 0;
  DPA_LOG("Connection closed\n");
}

static void ondisown( void* cid ){
  DPA_LOG("http_service->ondisown %p\n",cid);
  HTTP_Connections_t* c = getConnection(cid);
  if(!c) return;
  if( c->ressource && c->ressource->handler->close )
    c->ressource->handler->close( c->ressource );
  c->cid = 0;
}

const flash DPAUCS_service_t http_service = {
  .tos = PROTOCOL_TCP,
  .onopen = &onopen,
  .onreceive = &onreceive,
  .oneof = &oneof,
  .ondisown = &ondisown,
  .onclose = &onclose
};
