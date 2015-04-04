#define enc28j60PhyWrite(x,y) do{}while(0)
#define enc28j60clkout(x) do{}while(0)

void enc28j60Init( unsigned char* macaddr );
void enc28j60PacketSend(unsigned int len, unsigned char *packet);
unsigned int enc28j60PacketReceive(unsigned int maxlen, unsigned char *packet);

