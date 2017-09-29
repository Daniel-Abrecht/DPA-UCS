#define DPAUCS_WEBSOCKET_C

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <DPA/utils/logger.h>
#include <DPA/utils/crypto/sha1.h>
#include <DPA/utils/encoding/base64.h>
#include <DPA/UCS/protocol/tcp/tcp.h>
#include <DPA/UCS/services/http.h>
#include <DPA/UCS/services/websocket.h>

#if DPA_MAX_WEBSOCKET_CONNECTIONS > TRANSMISSION_CONTROL_BLOCK_COUNT
#undef DPA_MAX_WEBSOCKET_CONNECTIONS
#endif

#ifndef DPA_MAX_WEBSOCKET_CONNECTIONS
#define DPA_MAX_WEBSOCKET_CONNECTIONS TRANSMISSION_CONTROL_BLOCK_COUNT
#endif

DPA_MODULE( websocket ){
  DPA_DEPENDENCY( tcp );
}

enum websocket_state {
  WS_STATE_NONE,
  WS_STATE_HTTP_UPGRADE,
  WS_STATE_FRAME_BEGIN,
  WS_STATE_PARSE_OPCODE = WS_STATE_FRAME_BEGIN,
  WS_STATE_PARSE_PAYLOAD_LENGTH,
  WS_STATE_PARSE_EXTENDED_PAYLOAD_LENGTH,
  WS_STATE_PARSE_LARGE_EXTENDED_PAYLOAD_LENGTH,
  WS_STATE_PARSE_MASKING_KEY,
  WS_STATE_HANDLE_PAYLOAD
};

enum frame_opcode {
  FOP_CONTINUATION_FRAME,
  FOP_TEXT_FRAME,
  FOP_BINARY_FRAME,
  FOP_CONNECTION_CLOSE = 0x8,
  FOP_PING,
  FOP_PONG
};

typedef struct websocket_connection {
  enum websocket_state state;
  void* cid;
  const flash DPAUCS_websocket_subprotocol_t* subprotocol;
  union {
    struct {
      uint8_t sha1_key[20];
    } upgrade_data;
    struct {
      uint64_t payload_length;
      uint8_t mask_key[4];
      uint8_t tmp;
      bool fin;
      bool mask;
      enum frame_opcode opcode;
    } parser_state;
  };
} websocket_connection_t;

static websocket_connection_t connections[DPA_MAX_WEBSOCKET_CONNECTIONS];

static const flash char header_key[] = {"Sec-WebSocket-Key"};
static const flash char header_version[] = {"Sec-WebSocket-Version"};
static const flash char header_protocol[] = {"Sec-WebSocket-Protocol"};

static const flash char guid[] = {"258EAFA5-E914-47DA-95CA-C5AB0DC85B11"};

static websocket_connection_t* getConnection( void* cid ){
  websocket_connection_t *it, *end;
  for( it = connections, end = it + DPA_MAX_WEBSOCKET_CONNECTIONS; it < end; it++ )
    if( it->state && it->cid == cid )
      return it;
  return 0;
}

static websocket_connection_t* allocateConnection( void ){
  websocket_connection_t *it, *end;
  for( it = connections, end = it + DPA_MAX_WEBSOCKET_CONNECTIONS; it < end; it++ )
    if(!it->state)
      return it;
  return 0;
}

static void removeConnection( void* cid ){
  websocket_connection_t* c = getConnection(cid);
  if( !c ) return;
  c->cid = 0;
  c->state = WS_STATE_NONE;
}

static const flash DPAUCS_websocket_subprotocol_t* getSubprotocol( const char* name, size_t s ){
  for( struct websocket_subprotocol_list* it = websocket_subprotocol_list; it; it = it->next )
    if( strlen(it->entry->name) == s && !strncmp(it->entry->name,name,s) )
      return it->entry;
  return 0;
}

static bool onopen( void* cid, const void* ssdata ){
  const char* subprotocol = ssdata;
  DPA_LOG("onopen %p %s\n",cid,subprotocol);
  if( !subprotocol )
    return false;
  websocket_connection_t* c = getConnection( cid );
  if( !c ) c = allocateConnection();
  if( !c ) return false;
  c->subprotocol = getSubprotocol(subprotocol,strlen(subprotocol));
  if( !c->subprotocol )
    return false;
  c->state = WS_STATE_FRAME_BEGIN;
  c->cid = cid;
  if( c->subprotocol->onopen )
    c->subprotocol->onopen(cid);
  return true;
}

static void onreceive( void* cid, void* data, size_t size ){
  DPA_LOG("onreceive %p\n",cid);
  websocket_connection_t* c = getConnection( cid );
  if( !c ) return;
  uint8_t* it = data;
  size_t  n  = size;
  while( n ){
    switch( c->state ){
      case WS_STATE_PARSE_OPCODE: {
        uint8_t x = *(it++); n--;
        c->parser_state.fin = x & 0x80;
        if( ( x & 0xF ) != FOP_CONTINUATION_FRAME )
          c->parser_state.opcode = x & 0x0F;
        c->state = WS_STATE_PARSE_PAYLOAD_LENGTH;
        c->parser_state.tmp = 0;
      } continue;
      case WS_STATE_PARSE_PAYLOAD_LENGTH: {
        uint8_t x = *(it++); n--;
        c->parser_state.mask = x & 0x80;
        c->parser_state.payload_length = x & 0x7F;
        if( c->parser_state.payload_length == 126 ){
          c->parser_state.payload_length = 0;
          c->state = WS_STATE_PARSE_EXTENDED_PAYLOAD_LENGTH;
        }else if( c->parser_state.payload_length == 127 ){
          c->parser_state.payload_length = 0;
          c->state = WS_STATE_PARSE_LARGE_EXTENDED_PAYLOAD_LENGTH;
        }else{
          c->state = c->parser_state.mask ? WS_STATE_PARSE_MASKING_KEY : WS_STATE_HANDLE_PAYLOAD;
        }
      } continue;
      case WS_STATE_PARSE_EXTENDED_PAYLOAD_LENGTH: {
        uint8_t x = *(it++); n--;
        c->parser_state.payload_length |= (uint64_t)x << (8-c->parser_state.tmp*8);
        if( ++c->parser_state.tmp >= 2 ){
          c->state = c->parser_state.mask ? WS_STATE_PARSE_MASKING_KEY : WS_STATE_HANDLE_PAYLOAD;
          c->parser_state.tmp = 0;
        }
      } continue;
      case WS_STATE_PARSE_LARGE_EXTENDED_PAYLOAD_LENGTH: {
        uint8_t x = *(it++); n--;
        c->parser_state.payload_length |= (uint64_t)x << (56-c->parser_state.tmp*8);
        if( ++c->parser_state.tmp >= 8 ){
          c->state = c->parser_state.mask ? WS_STATE_PARSE_MASKING_KEY : WS_STATE_HANDLE_PAYLOAD;
          c->parser_state.tmp = 0;
        }
      } continue;
      case WS_STATE_PARSE_MASKING_KEY: {
        uint8_t x = *(it++); n--;
        c->parser_state.mask_key[c->parser_state.tmp] = x;
        if( ++c->parser_state.tmp >= 4 ){
          c->state = WS_STATE_HANDLE_PAYLOAD;
          c->parser_state.tmp = 0;
        }
      } continue;
      case WS_STATE_HANDLE_PAYLOAD: {
        size_t m = DPA_MIN( n, c->parser_state.payload_length );
        n -= m;
        if( c->parser_state.mask )
          for( size_t i=0; i<m; i++ )
            it[i] ^= c->parser_state.mask_key[i%4];
        if( c->parser_state.fin || m )
        switch( c->parser_state.opcode ){
          case FOP_TEXT_FRAME: {
            if( c->subprotocol->onreceive_utf8 )
              c->subprotocol->onreceive_utf8( cid, it, m, c->parser_state.fin && !n );
          } break;
          case FOP_BINARY_FRAME: {
            if( c->subprotocol->onreceive_binary )
              c->subprotocol->onreceive_binary( cid, it, m, c->parser_state.fin && !n );
          } break;
          case FOP_CONNECTION_CLOSE: {
            if( c->subprotocol->onclose )
              c->subprotocol->onclose( cid );
            removeConnection( cid );
            DPAUCS_tcp_close(cid);
          } break;
          case FOP_PING: break;
          default: {
            removeConnection(cid);
            DPAUCS_tcp_abord(cid);
          } break;
        }
        if(!( c->parser_state.payload_length -= m ))
          c->state = WS_STATE_FRAME_BEGIN;
        it += m;
      } continue;
      case WS_STATE_NONE:
      case WS_STATE_HTTP_UPGRADE:
        break;
    }
    break;
  }
}

static void oneof( void* cid ){
  DPA_LOG("oneof %p\n",cid);
  DPAUCS_tcp_close( cid );
}

static void onclose( void* cid ){
  DPA_LOG("onclose %p\n",cid);
  removeConnection(cid);
}

bool upgrade_start( void* cid ){
  DPA_LOG("upgrade_start %p\n",cid);
  websocket_connection_t* c = allocateConnection();
  if(!c) return false;
  c->cid = cid;
  c->state = WS_STATE_HTTP_UPGRADE;
  memset(c->upgrade_data.sha1_key,0,sizeof(c->upgrade_data.sha1_key));
  return true;
}

bool upgrade_process_header(
  void* cid,
  size_t key_length, const char key[key_length],
  size_t value_length, const char value[value_length]
){
  DPA_LOG("upgrade_process_header %p\n",cid);
  websocket_connection_t* c = getConnection(cid);
  if( !c )
    return false;
  if( sizeof(header_key)-1 == key_length && DPA_streq_nocase_fn( header_key, key, key_length ) ){
    DPA_crypto_sha1_state_t sha1;
    DPA_crypto_sha1_init( &sha1 );
    DPA_crypto_sha1_add( &sha1, value, value_length );
    DPA_crypto_sha1_add_flash( &sha1, guid, sizeof(guid)-1 );
    DPA_crypto_sha1_done( &sha1, c->upgrade_data.sha1_key );
  }else if( sizeof(header_version)-1 == key_length && DPA_streq_nocase_fn( header_version, key, key_length ) ){
    int version = 0;
    for( size_t i=0; i < value_length; i++ ){
      char c = value[i];
      if( c >= '0' && c <= '9' ){
        version *= 10;
        version += c - '0';
      }
      if( i+1 == value_length || c < '0' || c > '9' ){
        if( version == 13 )
          break;
        version = 0;
      }
    }
    if( !version ){
      DPA_LOG("No supported websocket version offered\n");
      removeConnection(cid);
      return false;
    }
  }else if( sizeof(header_protocol)-1 == key_length && DPA_streq_nocase_fn( header_protocol, key, key_length ) ){
    c->subprotocol = getSubprotocol( value, value_length );
    if(!c->subprotocol){
      DPA_LOG("Unsupported subprotocol\n");
      return false;
    }
  }
  return true;
}

bool upgrade_add_response_headers( void* cid, DPA_stream_t* stream ){
  DPA_LOG("upgrade_add_response_headers %p\n",cid);
  websocket_connection_t* c = getConnection(cid);
  if( !c )
    return false;
  if( !c->subprotocol )
    return false;
  static const flash char default_headers[] = {"Sec-WebSocket-Version: 13\r\nSec-WebSocket-Protocol: "};
  if( !DPA_stream_progmemWrite(stream,default_headers,sizeof(default_headers)-1) )
    return false;
  if( !DPA_stream_referenceWrite(stream,c->subprotocol->name,strlen(c->subprotocol->name)) )
    return false;
  static const flash char key_header[] = {"\r\nSec-WebSocket-Accept: "};
  if( !DPA_stream_progmemWrite(stream,key_header,sizeof(key_header)-1) )
    return false;
  char key[28];
  DPA_encoding_base64_encode( key, sizeof(key), c->upgrade_data.sha1_key, 20 );
  if( !DPA_stream_copyWrite(stream,key,sizeof(key)) )
    return false;
  static const flash char CR_LF[] = {"\r\n"};
  if( !DPA_stream_progmemWrite(stream,CR_LF,sizeof(CR_LF)-1) )
    return false;
  return true;
}

const void* getssdata( void* cid ){
  websocket_connection_t* c = getConnection(cid);
  if( !c )
    return 0;
  return c->subprotocol->name;
}

void upgrade_abort( void* cid ){
  DPA_LOG("upgrade_abort %p\n",cid);
  removeConnection(cid);
}

const flash DPAUCS_service_t websocket_service = {
  .tos = PROTOCOL_TCP,
  .onopen = &onopen,
  .onreceive = &onreceive,
  .oneof = &oneof,
  .onclose = &onclose
};

const flash DPAUCS_http_upgrade_handler_t upgrade_handler = {
  .protocol = (const flash char[]){"websocket"},
  .service = &websocket_service,
  .start = &upgrade_start,
  .process_header = &upgrade_process_header,
  .add_response_headers = &upgrade_add_response_headers,
  .getssdata = &getssdata,
  .abort = &upgrade_abort
};

DPA_LOOSE_LIST_ADD( http_upgrade_handler_list, &upgrade_handler )
