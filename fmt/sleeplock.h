4250 // Long-term locks for processes
4251 struct sleeplock {
4252   uint locked;       // Is the lock held?
4253   struct spinlock lk; // spinlock protecting this sleep lock
4254 
4255   // For debugging:
4256   char *name;        // Name of lock.
4257   int pid;           // Process holding lock
4258 };
4259 
4260 
4261 
4262 
4263 
4264 
4265 
4266 
4267 
4268 
4269 
4270 
4271 
4272 
4273 
4274 
4275 
4276 
4277 
4278 
4279 
4280 
4281 
4282 
4283 
4284 
4285 
4286 
4287 
4288 
4289 
4290 
4291 
4292 
4293 
4294 
4295 
4296 
4297 
4298 
4299 
