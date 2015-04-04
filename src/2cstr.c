#include <stdio.h>
#include <stdbool.h>

#define ll 100

long n;
void pch(const char c){
  putchar(c);
  --n;
}

int main(){
  unsigned char c;
  bool lastHexEscape = false;
  n=ll;
  pch('"');
  while(1){
    c = getchar();
    if(feof(stdin)) break;
    if( n-2 <= 0 || c == '\n' ){
      pch('"');
      pch('\n');
      n = ll;
      pch('"');
      lastHexEscape = false;
    }
    switch(c){
      case '\a':
      case '\b':
      case '\f':
      case '\n':
      case '\r':
      case '\t':
      case '\v':
      case '\\':
      case '"' :
      case '\?':
        pch('\\');
        switch(c){
          case '\a': pch('a'); break;
          case '\b': pch('b'); break;
          case '\f': pch('f'); break;
          case '\n': pch('n'); break;
          case '\r': pch('r'); break;
          case '\t': pch('t'); break;
          case '\v': pch('v'); break;
          case '\\': pch('\\'); break;
          case '"' :
          case '\?':
            pch(c);
          break;
        }
      lastHexEscape = false;
      continue;
    }
    if( c >= 0x20 && c <= 0x7E ){
      if( lastHexEscape && ( 
        ( c >= '0' && c <= '9' ) ||
        ( c >= 'a' && c <= 'f' ) ||
        ( c >= 'A' && c <= 'F' ) 
      )){
        pch('"');
        pch('"');
      }
      pch(c);
      lastHexEscape = false;
      continue;
    }
    pch('\\');
    pch('x');
    pch("0123456789ABCDEF"[c>>4] );
    pch("0123456789ABCDEF"[c&0xf]);
    lastHexEscape = true;
  }
  pch('"');  
  pch('\n');
  return 0;
}
