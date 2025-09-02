7250 #include "types.h"
7251 #include "x86.h"
7252 
7253 void*
7254 memset(void *dst, int c, uint n)
7255 {
7256   if ((int)dst%4 == 0 && n%4 == 0){
7257     c &= 0xFF;
7258     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
7259   } else
7260     stosb(dst, c, n);
7261   return dst;
7262 }
7263 
7264 int
7265 memcmp(const void *v1, const void *v2, uint n)
7266 {
7267   const uchar *s1, *s2;
7268 
7269   s1 = v1;
7270   s2 = v2;
7271   while(n-- > 0){
7272     if(*s1 != *s2)
7273       return *s1 - *s2;
7274     s1++, s2++;
7275   }
7276 
7277   return 0;
7278 }
7279 
7280 void*
7281 memmove(void *dst, const void *src, uint n)
7282 {
7283   const char *s;
7284   char *d;
7285 
7286   s = src;
7287   d = dst;
7288   if(s < d && s + n > d){
7289     s += n;
7290     d += n;
7291     while(n-- > 0)
7292       *--d = *--s;
7293   } else
7294     while(n-- > 0)
7295       *d++ = *s++;
7296 
7297   return dst;
7298 }
7299 
7300 // memcpy exists to placate GCC.  Use memmove.
7301 void*
7302 memcpy(void *dst, const void *src, uint n)
7303 {
7304   return memmove(dst, src, n);
7305 }
7306 
7307 int
7308 strncmp(const char *p, const char *q, uint n)
7309 {
7310   while(n > 0 && *p && *p == *q)
7311     n--, p++, q++;
7312   if(n == 0)
7313     return 0;
7314   return (uchar)*p - (uchar)*q;
7315 }
7316 
7317 char*
7318 strncpy(char *s, const char *t, int n)
7319 {
7320   char *os;
7321 
7322   os = s;
7323   while(n-- > 0 && (*s++ = *t++) != 0)
7324     ;
7325   while(n-- > 0)
7326     *s++ = 0;
7327   return os;
7328 }
7329 
7330 // Like strncpy but guaranteed to NUL-terminate.
7331 char*
7332 safestrcpy(char *s, const char *t, int n)
7333 {
7334   char *os;
7335 
7336   os = s;
7337   if(n <= 0)
7338     return os;
7339   while(--n > 0 && (*s++ = *t++) != 0)
7340     ;
7341   *s = 0;
7342   return os;
7343 }
7344 
7345 
7346 
7347 
7348 
7349 
7350 int
7351 strlen(const char *s)
7352 {
7353   int n;
7354 
7355   for(n = 0; s[n]; n++)
7356     ;
7357   return n;
7358 }
7359 
7360 
7361 
7362 
7363 
7364 
7365 
7366 
7367 
7368 
7369 
7370 
7371 
7372 
7373 
7374 
7375 
7376 
7377 
7378 
7379 
7380 
7381 
7382 
7383 
7384 
7385 
7386 
7387 
7388 
7389 
7390 
7391 
7392 
7393 
7394 
7395 
7396 
7397 
7398 
7399 
