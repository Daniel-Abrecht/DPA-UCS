#include <stdio.h>
#include <DPA/UCS/services/websocket.h>

void onreceive_utf8( void* cid, void* data, size_t size, bool fin ){
  (void)cid;
  char* it = data;
  while(size--)
    putchar(*it++);
  if(fin)
    putchar('\n');
}

const flash DPAUCS_websocket_subprotocol_t ws_echo = {
  .name = "echo",
  .onreceive_utf8 = &onreceive_utf8
};

DPA_LOOSE_LIST_ADD( websocket_subprotocol_list, &ws_echo )
