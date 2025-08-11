1250 #include "types.h"
1251 #include "defs.h"
1252 #include "param.h"
1253 #include "memlayout.h"
1254 #include "mmu.h"
1255 #include "proc.h"
1256 #include "x86.h"
1257 
1258 static void startothers(void);
1259 static void mpmain(void)  __attribute__((noreturn));
1260 extern pde_t *kpgdir;
1261 extern char end[]; // first address after kernel loaded from ELF file
1262 
1263 // Bootstrap processor starts running C code here.
1264 // Allocate a real stack and switch to it, first
1265 // doing some setup required for memory allocator to work.
1266 int
1267 main(void)
1268 {
1269   kinit1(end, P2V(4*1024*1024)); // phys page allocator
1270   kvmalloc();      // kernel page table
1271   mpinit();        // detect other processors
1272   lapicinit();     // interrupt controller
1273   seginit();       // segment descriptors
1274   picinit();       // disable pic
1275   ioapicinit();    // another interrupt controller
1276   consoleinit();   // console hardware
1277   uartinit();      // serial port
1278   pinit();         // process table
1279   tvinit();        // trap vectors
1280   binit();         // buffer cache
1281   fileinit();      // file table
1282   ideinit();       // disk
1283   startothers();   // start other processors
1284   kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
1285   userinit();      // first user process
1286   mpmain();        // finish this processor's setup
1287 }
1288 
1289 // Other CPUs jump here from entryother.S.
1290 static void
1291 mpenter(void)
1292 {
1293   switchkvm();
1294   seginit();
1295   lapicinit();
1296   mpmain();
1297 }
1298 
1299 
1300 // Common CPU setup code.
1301 static void
1302 mpmain(void)
1303 {
1304   cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
1305   idtinit();       // load idt register
1306   xchg(&(mycpu()->started), 1); // tell startothers() we're up
1307   scheduler();     // start running processes
1308 }
1309 
1310 pde_t entrypgdir[];  // For entry.S
1311 
1312 // Start the non-boot (AP) processors.
1313 static void
1314 startothers(void)
1315 {
1316   extern uchar _binary_entryother_start[], _binary_entryother_size[];
1317   uchar *code;
1318   struct cpu *c;
1319   char *stack;
1320 
1321   // Write entry code to unused memory at 0x7000.
1322   // The linker has placed the image of entryother.S in
1323   // _binary_entryother_start.
1324   code = P2V(0x7000);
1325   memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);
1326 
1327   for(c = cpus; c < cpus+ncpu; c++){
1328     if(c == mycpu())  // We've started already.
1329       continue;
1330 
1331     // Tell entryother.S what stack to use, where to enter, and what
1332     // pgdir to use. We cannot use kpgdir yet, because the AP processor
1333     // is running in low  memory, so we use entrypgdir for the APs too.
1334     stack = kalloc();
1335     *(void**)(code-4) = stack + KSTACKSIZE;
1336     *(void(**)(void))(code-8) = mpenter;
1337     *(int**)(code-12) = (void *) V2P(entrypgdir);
1338 
1339     lapicstartap(c->apicid, V2P(code));
1340 
1341     // wait for cpu to finish mpmain()
1342     while(c->started == 0)
1343       ;
1344   }
1345 }
1346 
1347 
1348 
1349 
1350 // The boot page table used in entry.S and entryother.S.
1351 // Page directories (and page tables) must start on page boundaries,
1352 // hence the __aligned__ attribute.
1353 // PTE_PS in a page directory entry enables 4Mbyte pages.
1354 
1355 __attribute__((__aligned__(PGSIZE)))
1356 pde_t entrypgdir[NPDENTRIES] = {
1357   // Map VA's [0, 4MB) to PA's [0, 4MB)
1358   [0] = (0) | PTE_P | PTE_W | PTE_PS,
1359   // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
1360   [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
1361 };
1362 
1363 
1364 
1365 
1366 
1367 
1368 
1369 
1370 
1371 
1372 
1373 
1374 
1375 
1376 
1377 
1378 
1379 
1380 
1381 
1382 
1383 
1384 
1385 
1386 
1387 
1388 
1389 
1390 
1391 
1392 
1393 
1394 
1395 
1396 
1397 
1398 
1399 
1400 // Blank page.
1401 
1402 
1403 
1404 
1405 
1406 
1407 
1408 
1409 
1410 
1411 
1412 
1413 
1414 
1415 
1416 
1417 
1418 
1419 
1420 
1421 
1422 
1423 
1424 
1425 
1426 
1427 
1428 
1429 
1430 
1431 
1432 
1433 
1434 
1435 
1436 
1437 
1438 
1439 
1440 
1441 
1442 
1443 
1444 
1445 
1446 
1447 
1448 
1449 
1450 // Blank page.
1451 
1452 
1453 
1454 
1455 
1456 
1457 
1458 
1459 
1460 
1461 
1462 
1463 
1464 
1465 
1466 
1467 
1468 
1469 
1470 
1471 
1472 
1473 
1474 
1475 
1476 
1477 
1478 
1479 
1480 
1481 
1482 
1483 
1484 
1485 
1486 
1487 
1488 
1489 
1490 
1491 
1492 
1493 
1494 
1495 
1496 
1497 
1498 
1499 
1500 // Blank page.
1501 
1502 
1503 
1504 
1505 
1506 
1507 
1508 
1509 
1510 
1511 
1512 
1513 
1514 
1515 
1516 
1517 
1518 
1519 
1520 
1521 
1522 
1523 
1524 
1525 
1526 
1527 
1528 
1529 
1530 
1531 
1532 
1533 
1534 
1535 
1536 
1537 
1538 
1539 
1540 
1541 
1542 
1543 
1544 
1545 
1546 
1547 
1548 
1549 
