4200 struct buf {
4201   int flags;
4202   uint dev;
4203   uint blockno;
4204   struct sleeplock lock;
4205   uint refcnt;
4206   struct buf *prev; // LRU cache list
4207   struct buf *next;
4208   struct buf *qnext; // disk queue
4209   uchar data[BSIZE];
4210 };
4211 #define B_VALID 0x2  // buffer has been read from disk
4212 #define B_DIRTY 0x4  // buffer needs to be written to disk
4213 
4214 
4215 
4216 
4217 
4218 
4219 
4220 
4221 
4222 
4223 
4224 
4225 
4226 
4227 
4228 
4229 
4230 
4231 
4232 
4233 
4234 
4235 
4236 
4237 
4238 
4239 
4240 
4241 
4242 
4243 
4244 
4245 
4246 
4247 
4248 
4249 
