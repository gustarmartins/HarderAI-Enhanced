/* Authored freestanding support for compiler-emitted structure copies.
 * Bodies match the native-qualified lane fixture's memory primitives. */
void *memcpy(void *destination,const void *source,unsigned long size){
    unsigned char *d=destination;const unsigned char *s=source;
    for(unsigned long k=0;k<size;k++)d[k]=s[k];return destination;
}
void *memset(void *destination,int value,unsigned long size){
    unsigned char *d=destination;for(unsigned long k=0;k<size;k++)d[k]=(unsigned char)value;
    return destination;
}
