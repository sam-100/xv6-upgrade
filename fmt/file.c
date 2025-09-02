6200 //
6201 // File descriptors
6202 //
6203 
6204 #include "types.h"
6205 #include "defs.h"
6206 #include "param.h"
6207 #include "fs.h"
6208 #include "spinlock.h"
6209 #include "sleeplock.h"
6210 #include "file.h"
6211 
6212 struct devsw devsw[NDEV];
6213 struct {
6214   struct spinlock lock;
6215   struct file file[NFILE];
6216 } ftable;
6217 
6218 void
6219 fileinit(void)
6220 {
6221   initlock(&ftable.lock, "ftable");
6222 }
6223 
6224 // Allocate a file structure.
6225 struct file*
6226 filealloc(void)
6227 {
6228   struct file *f;
6229 
6230   acquire(&ftable.lock);
6231   for(f = ftable.file; f < ftable.file + NFILE; f++){
6232     if(f->ref == 0){
6233       f->ref = 1;
6234       release(&ftable.lock);
6235       return f;
6236     }
6237   }
6238   release(&ftable.lock);
6239   return 0;
6240 }
6241 
6242 
6243 
6244 
6245 
6246 
6247 
6248 
6249 
6250 // Increment ref count for file f.
6251 struct file*
6252 filedup(struct file *f)
6253 {
6254   acquire(&ftable.lock);
6255   if(f->ref < 1)
6256     panic("filedup");
6257   f->ref++;
6258   release(&ftable.lock);
6259   return f;
6260 }
6261 
6262 // Close file f.  (Decrement ref count, close when reaches 0.)
6263 void
6264 fileclose(struct file *f)
6265 {
6266   struct file ff;
6267 
6268   acquire(&ftable.lock);
6269   if(f->ref < 1)
6270     panic("fileclose");
6271   if(--f->ref > 0){
6272     release(&ftable.lock);
6273     return;
6274   }
6275   ff = *f;
6276   f->ref = 0;
6277   f->type = FD_NONE;
6278   release(&ftable.lock);
6279 
6280   if(ff.type == FD_PIPE)
6281     pipeclose(ff.pipe, ff.writable);
6282   else if(ff.type == FD_INODE){
6283     begin_op();
6284     iput(ff.ip);
6285     end_op();
6286   }
6287 }
6288 
6289 
6290 
6291 
6292 
6293 
6294 
6295 
6296 
6297 
6298 
6299 
6300 // Get metadata about file f.
6301 int
6302 filestat(struct file *f, struct stat *st)
6303 {
6304   if(f->type == FD_INODE){
6305     ilock(f->ip);
6306     stati(f->ip, st);
6307     iunlock(f->ip);
6308     return 0;
6309   }
6310   return -1;
6311 }
6312 
6313 // Read from file f.
6314 int
6315 fileread(struct file *f, char *addr, int n)
6316 {
6317   int r;
6318 
6319   if(f->readable == 0)
6320     return -1;
6321   if(f->type == FD_PIPE)
6322     return piperead(f->pipe, addr, n);
6323   if(f->type == FD_INODE){
6324     ilock(f->ip);
6325     if((r = readi(f->ip, addr, f->off, n)) > 0)
6326       f->off += r;
6327     iunlock(f->ip);
6328     return r;
6329   }
6330   panic("fileread");
6331 }
6332 
6333 
6334 
6335 
6336 
6337 
6338 
6339 
6340 
6341 
6342 
6343 
6344 
6345 
6346 
6347 
6348 
6349 
6350 // Write to file f.
6351 int
6352 filewrite(struct file *f, char *addr, int n)
6353 {
6354   int r;
6355 
6356   if(f->writable == 0)
6357     return -1;
6358   if(f->type == FD_PIPE)
6359     return pipewrite(f->pipe, addr, n);
6360   if(f->type == FD_INODE){
6361     // write a few blocks at a time to avoid exceeding
6362     // the maximum log transaction size, including
6363     // i-node, indirect block, allocation blocks,
6364     // and 2 blocks of slop for non-aligned writes.
6365     // this really belongs lower down, since writei()
6366     // might be writing a device like the console.
6367     int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
6368     int i = 0;
6369     while(i < n){
6370       int n1 = n - i;
6371       if(n1 > max)
6372         n1 = max;
6373 
6374       begin_op();
6375       ilock(f->ip);
6376       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
6377         f->off += r;
6378       iunlock(f->ip);
6379       end_op();
6380 
6381       if(r < 0)
6382         break;
6383       if(r != n1)
6384         panic("short filewrite");
6385       i += r;
6386     }
6387     return i == n ? n : -1;
6388   }
6389   panic("filewrite");
6390 }
6391 
6392 
6393 
6394 
6395 
6396 
6397 
6398 
6399 
