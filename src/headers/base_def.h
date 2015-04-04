#define STRINGIFY(x) #x

#define IPSTRING( a,b,c,d ) #a "." #b "." #c "." #d
#define IPCONST( a,b,c,d ) {a,b,c,d}
#define IPINT( a,b,c,d ) \
   ((uint32_t)a<<24) \
 | ((uint32_t)b<<16) \
 | ((uint32_t)c<<8) \
 | ((uint32_t)d)


#define MACSTRING( a,b,c,d,e,f ) #a ":" #b ":" #c ":" #d ":" #e ":" #f
#define MACCONST( a,b,c,d,e,f ) {0x##a,0x##b,0x##c,0x##d,0x##e,0x##f}

