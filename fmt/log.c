5050 #include "types.h"
5051 #include "defs.h"
5052 #include "param.h"
5053 #include "spinlock.h"
5054 #include "sleeplock.h"
5055 #include "fs.h"
5056 #include "buf.h"
5057 
5058 // Simple logging that allows concurrent FS system calls.
5059 //
5060 // A log transaction contains the updates of multiple FS system
5061 // calls. The logging system only commits when there are
5062 // no FS system calls active. Thus there is never
5063 // any reasoning required about whether a commit might
5064 // write an uncommitted system call's updates to disk.
5065 //
5066 // A system call should call begin_op()/end_op() to mark
5067 // its start and end. Usually begin_op() just increments
5068 // the count of in-progress FS system calls and returns.
5069 // But if it thinks the log is close to running out, it
5070 // sleeps until the last outstanding end_op() commits.
5071 //
5072 // The log is a physical re-do log containing disk blocks.
5073 // The on-disk log format:
5074 //   header block, containing block #s for block A, B, C, ...
5075 //   block A
5076 //   block B
5077 //   block C
5078 //   ...
5079 // Log appends are synchronous.
5080 
5081 // Contents of the header block, used for both the on-disk header block
5082 // and to keep track in memory of logged block# before commit.
5083 struct logheader {
5084   int n;
5085   int block[LOGSIZE];
5086 };
5087 
5088 struct log {
5089   struct spinlock lock;
5090   int start;
5091   int size;
5092   int outstanding; // how many FS sys calls are executing.
5093   int committing;  // in commit(), please wait.
5094   int dev;
5095   struct logheader lh;
5096 };
5097 
5098 
5099 
5100 struct log log;
5101 
5102 static void recover_from_log(void);
5103 static void commit();
5104 
5105 void
5106 initlog(int dev)
5107 {
5108   if (sizeof(struct logheader) >= BSIZE)
5109     panic("initlog: too big logheader");
5110 
5111   struct superblock sb;
5112   initlock(&log.lock, "log");
5113   readsb(dev, &sb);
5114   log.start = sb.logstart;
5115   log.size = sb.nlog;
5116   log.dev = dev;
5117   recover_from_log();
5118 }
5119 
5120 // Copy committed blocks from log to their home location
5121 static void
5122 install_trans(void)
5123 {
5124   int tail;
5125 
5126   for (tail = 0; tail < log.lh.n; tail++) {
5127     struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
5128     struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
5129     memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
5130     bwrite(dbuf);  // write dst to disk
5131     brelse(lbuf);
5132     brelse(dbuf);
5133   }
5134 }
5135 
5136 // Read the log header from disk into the in-memory log header
5137 static void
5138 read_head(void)
5139 {
5140   struct buf *buf = bread(log.dev, log.start);
5141   struct logheader *lh = (struct logheader *) (buf->data);
5142   int i;
5143   log.lh.n = lh->n;
5144   for (i = 0; i < log.lh.n; i++) {
5145     log.lh.block[i] = lh->block[i];
5146   }
5147   brelse(buf);
5148 }
5149 
5150 // Write in-memory log header to disk.
5151 // This is the true point at which the
5152 // current transaction commits.
5153 static void
5154 write_head(void)
5155 {
5156   struct buf *buf = bread(log.dev, log.start);
5157   struct logheader *hb = (struct logheader *) (buf->data);
5158   int i;
5159   hb->n = log.lh.n;
5160   for (i = 0; i < log.lh.n; i++) {
5161     hb->block[i] = log.lh.block[i];
5162   }
5163   bwrite(buf);
5164   brelse(buf);
5165 }
5166 
5167 static void
5168 recover_from_log(void)
5169 {
5170   read_head();
5171   install_trans(); // if committed, copy from log to disk
5172   log.lh.n = 0;
5173   write_head(); // clear the log
5174 }
5175 
5176 // called at the start of each FS system call.
5177 void
5178 begin_op(void)
5179 {
5180   acquire(&log.lock);
5181   while(1){
5182     if(log.committing){
5183       sleep(&log, &log.lock);
5184     } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
5185       // this op might exhaust log space; wait for commit.
5186       sleep(&log, &log.lock);
5187     } else {
5188       log.outstanding += 1;
5189       release(&log.lock);
5190       break;
5191     }
5192   }
5193 }
5194 
5195 
5196 
5197 
5198 
5199 
5200 // called at the end of each FS system call.
5201 // commits if this was the last outstanding operation.
5202 void
5203 end_op(void)
5204 {
5205   int do_commit = 0;
5206 
5207   acquire(&log.lock);
5208   log.outstanding -= 1;
5209   if(log.committing)
5210     panic("log.committing");
5211   if(log.outstanding == 0){
5212     do_commit = 1;
5213     log.committing = 1;
5214   } else {
5215     // begin_op() may be waiting for log space,
5216     // and decrementing log.outstanding has decreased
5217     // the amount of reserved space.
5218     wakeup(&log);
5219   }
5220   release(&log.lock);
5221 
5222   if(do_commit){
5223     // call commit w/o holding locks, since not allowed
5224     // to sleep with locks.
5225     commit();
5226     acquire(&log.lock);
5227     log.committing = 0;
5228     wakeup(&log);
5229     release(&log.lock);
5230   }
5231 }
5232 
5233 // Copy modified blocks from cache to log.
5234 static void
5235 write_log(void)
5236 {
5237   int tail;
5238 
5239   for (tail = 0; tail < log.lh.n; tail++) {
5240     struct buf *to = bread(log.dev, log.start+tail+1); // log block
5241     struct buf *from = bread(log.dev, log.lh.block[tail]); // cache block
5242     memmove(to->data, from->data, BSIZE);
5243     bwrite(to);  // write the log
5244     brelse(from);
5245     brelse(to);
5246   }
5247 }
5248 
5249 
5250 static void
5251 commit()
5252 {
5253   if (log.lh.n > 0) {
5254     write_log();     // Write modified blocks from cache to log
5255     write_head();    // Write header to disk -- the real commit
5256     install_trans(); // Now install writes to home locations
5257     log.lh.n = 0;
5258     write_head();    // Erase the transaction from the log
5259   }
5260 }
5261 
5262 // Caller has modified b->data and is done with the buffer.
5263 // Record the block number and pin in the cache with B_DIRTY.
5264 // commit()/write_log() will do the disk write.
5265 //
5266 // log_write() replaces bwrite(); a typical use is:
5267 //   bp = bread(...)
5268 //   modify bp->data[]
5269 //   log_write(bp)
5270 //   brelse(bp)
5271 void
5272 log_write(struct buf *b)
5273 {
5274   int i;
5275 
5276   if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
5277     panic("too big a transaction");
5278   if (log.outstanding < 1)
5279     panic("log_write outside of trans");
5280 
5281   acquire(&log.lock);
5282   for (i = 0; i < log.lh.n; i++) {
5283     if (log.lh.block[i] == b->blockno)   // log absorbtion
5284       break;
5285   }
5286   log.lh.block[i] = b->blockno;
5287   if (i == log.lh.n)
5288     log.lh.n++;
5289   b->flags |= B_DIRTY; // prevent eviction
5290   release(&log.lock);
5291 }
5292 
5293 
5294 
5295 
5296 
5297 
5298 
5299 
