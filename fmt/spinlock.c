1600 // Mutual exclusion spin locks.
1601 
1602 #include "types.h"
1603 #include "defs.h"
1604 #include "param.h"
1605 #include "x86.h"
1606 #include "memlayout.h"
1607 #include "mmu.h"
1608 #include "proc.h"
1609 #include "spinlock.h"
1610 
1611 void
1612 initlock(struct spinlock *lk, char *name)
1613 {
1614   lk->name = name;
1615   lk->locked = 0;
1616   lk->cpu = 0;
1617 }
1618 
1619 // Acquire the lock.
1620 // Loops (spins) until the lock is acquired.
1621 // Holding a lock for a long time may cause
1622 // other CPUs to waste time spinning to acquire it.
1623 void
1624 acquire(struct spinlock *lk)
1625 {
1626   pushcli(); // disable interrupts to avoid deadlock.
1627   if(holding(lk))
1628     panic("acquire");
1629 
1630   // The xchg is atomic.
1631   while(xchg(&lk->locked, 1) != 0)
1632     ;
1633 
1634   // Tell the C compiler and the processor to not move loads or stores
1635   // past this point, to ensure that the critical section's memory
1636   // references happen after the lock is acquired.
1637   __sync_synchronize();
1638 
1639   // Record info about lock acquisition for debugging.
1640   lk->cpu = mycpu();
1641   getcallerpcs(&lk, lk->pcs);
1642 }
1643 
1644 
1645 
1646 
1647 
1648 
1649 
1650 // Release the lock.
1651 void
1652 release(struct spinlock *lk)
1653 {
1654   if(!holding(lk))
1655     panic("release");
1656 
1657   lk->pcs[0] = 0;
1658   lk->cpu = 0;
1659 
1660   // Tell the C compiler and the processor to not move loads or stores
1661   // past this point, to ensure that all the stores in the critical
1662   // section are visible to other cores before the lock is released.
1663   // Both the C compiler and the hardware may re-order loads and
1664   // stores; __sync_synchronize() tells them both not to.
1665   __sync_synchronize();
1666 
1667   // Release the lock, equivalent to lk->locked = 0.
1668   // This code can't use a C assignment, since it might
1669   // not be atomic. A real OS would use C atomics here.
1670   asm volatile("movl $0, %0" : "+m" (lk->locked) : );
1671 
1672   popcli();
1673 }
1674 
1675 // Record the current call stack in pcs[] by following the %ebp chain.
1676 void
1677 getcallerpcs(void *v, uint pcs[])
1678 {
1679   uint *ebp;
1680   int i;
1681 
1682   ebp = (uint*)v - 2;
1683   for(i = 0; i < 10; i++){
1684     if(ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
1685       break;
1686     pcs[i] = ebp[1];     // saved %eip
1687     ebp = (uint*)ebp[0]; // saved %ebp
1688   }
1689   for(; i < 10; i++)
1690     pcs[i] = 0;
1691 }
1692 
1693 
1694 
1695 
1696 
1697 
1698 
1699 
1700 // Check whether this cpu is holding the lock.
1701 int
1702 holding(struct spinlock *lock)
1703 {
1704   int r;
1705   pushcli();
1706   r = lock->locked && lock->cpu == mycpu();
1707   popcli();
1708   return r;
1709 }
1710 
1711 
1712 // Pushcli/popcli are like cli/sti except that they are matched:
1713 // it takes two popcli to undo two pushcli.  Also, if interrupts
1714 // are off, then pushcli, popcli leaves them off.
1715 
1716 void
1717 pushcli(void)
1718 {
1719   int eflags;
1720 
1721   eflags = readeflags();
1722   cli();
1723   if(mycpu()->ncli == 0)
1724     mycpu()->intena = eflags & FL_IF;
1725   mycpu()->ncli += 1;
1726 }
1727 
1728 void
1729 popcli(void)
1730 {
1731   if(readeflags()&FL_IF)
1732     panic("popcli - interruptible");
1733   if(--mycpu()->ncli < 0)
1734     panic("popcli");
1735   if(mycpu()->ncli == 0 && mycpu()->intena)
1736     sti();
1737 }
1738 
1739 
1740 
1741 
1742 
1743 
1744 
1745 
1746 
1747 
1748 
1749 
