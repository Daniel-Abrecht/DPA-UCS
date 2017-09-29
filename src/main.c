#include <DPA/UCS/server.h>
#include <DPA/UCS/services/http.h>
#include <DPA/UCS/protocol/address.h>

DPAUCS_INIT {
  DPAUCS_add_service( DPAUCS_ANY_ADDRESS, 80, &http_service, 0 );
}
