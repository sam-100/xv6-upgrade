1550 // Mutual exclusion lock.
1551 struct spinlock {
1552   uint locked;       // Is the lock held?
1553 
1554   // For debugging:
1555   char *name;        // Name of lock.
1556   struct cpu *cpu;   // The cpu holding the lock.
1557   uint pcs[10];      // The call stack (an array of program counters)
1558                      // that locked the lock.
1559 };
1560 
1561 
1562 
1563 
1564 
1565 
1566 
1567 
1568 
1569 
1570 
1571 
1572 
1573 
1574 
1575 
1576 
1577 
1578 
1579 
1580 
1581 
1582 
1583 
1584 
1585 
1586 
1587 
1588 
1589 
1590 
1591 
1592 
1593 
1594 
1595 
1596 
1597 
1598 
1599 
