1750 #include "param.h"
1751 #include "types.h"
1752 #include "defs.h"
1753 #include "x86.h"
1754 #include "memlayout.h"
1755 #include "mmu.h"
1756 #include "proc.h"
1757 #include "elf.h"
1758 
1759 extern char data[];  // defined by kernel.ld
1760 pde_t *kpgdir;  // for use in scheduler()
1761 
1762 // Set up CPU's kernel segment descriptors.
1763 // Run once on entry on each CPU.
1764 void
1765 seginit(void)
1766 {
1767   struct cpu *c;
1768 
1769   // Map "logical" addresses to virtual addresses using identity map.
1770   // Cannot share a CODE descriptor for both kernel and user
1771   // because it would have to have DPL_USR, but the CPU forbids
1772   // an interrupt from CPL=0 to DPL=3.
1773   c = &cpus[cpuid()];
1774   c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
1775   c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
1776   c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
1777   c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
1778   lgdt(c->gdt, sizeof(c->gdt));
1779 }
1780 
1781 // Return the address of the PTE in page table pgdir
1782 // that corresponds to virtual address va.  If alloc!=0,
1783 // create any required page table pages.
1784 static pte_t *
1785 walkpgdir(pde_t *pgdir, const void *va, int alloc)
1786 {
1787   pde_t *pde;
1788   pte_t *pgtab;
1789 
1790   pde = &pgdir[PDX(va)];
1791   if(*pde & PTE_P){
1792     pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
1793   } else {
1794     if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
1795       return 0;
1796     // Make sure all those PTE_P bits are zero.
1797     memset(pgtab, 0, PGSIZE);
1798     // The permissions here are overly generous, but they can
1799     // be further restricted by the permissions in the page table
1800     // entries, if necessary.
1801     *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
1802   }
1803   return &pgtab[PTX(va)];
1804 }
1805 
1806 // Create PTEs for virtual addresses starting at va that refer to
1807 // physical addresses starting at pa. va and size might not
1808 // be page-aligned.
1809 static int
1810 mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
1811 {
1812   char *a, *last;
1813   pte_t *pte;
1814 
1815   a = (char*)PGROUNDDOWN((uint)va);
1816   last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
1817   for(;;){
1818     if((pte = walkpgdir(pgdir, a, 1)) == 0)
1819       return -1;
1820     if(*pte & PTE_P)
1821       panic("remap");
1822     *pte = pa | perm | PTE_P;
1823     if(a == last)
1824       break;
1825     a += PGSIZE;
1826     pa += PGSIZE;
1827   }
1828   return 0;
1829 }
1830 
1831 // There is one page table per process, plus one that's used when
1832 // a CPU is not running any process (kpgdir). The kernel uses the
1833 // current process's page table during system calls and interrupts;
1834 // page protection bits prevent user code from using the kernel's
1835 // mappings.
1836 //
1837 // setupkvm() and exec() set up every page table like this:
1838 //
1839 //   0..KERNBASE: user memory (text+data+stack+heap), mapped to
1840 //                phys memory allocated by the kernel
1841 //   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
1842 //   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
1843 //                for the kernel's instructions and r/o data
1844 //   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
1845 //                                  rw data + free physical memory
1846 //   0xfe000000..0: mapped direct (devices such as ioapic)
1847 //
1848 // The kernel allocates physical memory for its heap and for user memory
1849 // between V2P(end) and the end of physical memory (PHYSTOP)
1850 // (directly addressable from end..P2V(PHYSTOP)).
1851 
1852 // This table defines the kernel's mappings, which are present in
1853 // every process's page table.
1854 static struct kmap {
1855   void *virt;
1856   uint phys_start;
1857   uint phys_end;
1858   int perm;
1859 } kmap[] = {
1860  { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
1861  { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
1862  { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
1863  { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
1864 };
1865 
1866 // Set up kernel part of a page table.
1867 pde_t*
1868 setupkvm(void)
1869 {
1870   pde_t *pgdir;
1871   struct kmap *k;
1872 
1873   if((pgdir = (pde_t*)kalloc()) == 0)
1874     return 0;
1875   memset(pgdir, 0, PGSIZE);
1876   if (P2V(PHYSTOP) > (void*)DEVSPACE)
1877     panic("PHYSTOP too high");
1878   for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
1879     if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
1880                 (uint)k->phys_start, k->perm) < 0) {
1881       freevm(pgdir);
1882       return 0;
1883     }
1884   return pgdir;
1885 }
1886 
1887 // Allocate one page table for the machine for the kernel address
1888 // space for scheduler processes.
1889 void
1890 kvmalloc(void)
1891 {
1892   kpgdir = setupkvm();
1893   switchkvm();
1894 }
1895 
1896 
1897 
1898 
1899 
1900 // Switch h/w page table register to the kernel-only page table,
1901 // for when no process is running.
1902 void
1903 switchkvm(void)
1904 {
1905   lcr3(V2P(kpgdir));   // switch to the kernel page table
1906 }
1907 
1908 // Switch TSS and h/w page table to correspond to process p.
1909 void
1910 switchuvm(struct proc *p)
1911 {
1912   if(p == 0)
1913     panic("switchuvm: no process");
1914   if(p->kstack == 0)
1915     panic("switchuvm: no kstack");
1916   if(p->pgdir == 0)
1917     panic("switchuvm: no pgdir");
1918 
1919   pushcli();
1920   mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
1921                                 sizeof(mycpu()->ts)-1, 0);
1922   mycpu()->gdt[SEG_TSS].s = 0;
1923   mycpu()->ts.ss0 = SEG_KDATA << 3;
1924   mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
1925   // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
1926   // forbids I/O instructions (e.g., inb and outb) from user space
1927   mycpu()->ts.iomb = (ushort) 0xFFFF;
1928   ltr(SEG_TSS << 3);
1929   lcr3(V2P(p->pgdir));  // switch to process's address space
1930   popcli();
1931 }
1932 
1933 // Load the initcode into address 0 of pgdir.
1934 // sz must be less than a page.
1935 void
1936 inituvm(pde_t *pgdir, char *init, uint sz)
1937 {
1938   char *mem;
1939 
1940   if(sz >= PGSIZE)
1941     panic("inituvm: more than a page");
1942   mem = kalloc();
1943   memset(mem, 0, PGSIZE);
1944   mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
1945   memmove(mem, init, sz);
1946 }
1947 
1948 
1949 
1950 // Load a program segment into pgdir.  addr must be page-aligned
1951 // and the pages from addr to addr+sz must already be mapped.
1952 int
1953 loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
1954 {
1955   uint i, pa, n;
1956   pte_t *pte;
1957 
1958   if((uint) addr % PGSIZE != 0)
1959     panic("loaduvm: addr must be page aligned");
1960   for(i = 0; i < sz; i += PGSIZE){
1961     if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
1962       panic("loaduvm: address should exist");
1963     pa = PTE_ADDR(*pte);
1964     if(sz - i < PGSIZE)
1965       n = sz - i;
1966     else
1967       n = PGSIZE;
1968     if(readi(ip, P2V(pa), offset+i, n) != n)
1969       return -1;
1970   }
1971   return 0;
1972 }
1973 
1974 // Allocate page tables and physical memory to grow process from oldsz to
1975 // newsz, which need not be page aligned.  Returns new size or 0 on error.
1976 int
1977 allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
1978 {
1979   char *mem;
1980   uint a;
1981 
1982   if(newsz >= KERNBASE)
1983     return 0;
1984   if(newsz < oldsz)
1985     return oldsz;
1986 
1987   a = PGROUNDUP(oldsz);
1988   for(; a < newsz; a += PGSIZE){
1989     mem = kalloc();
1990     if(mem == 0){
1991       cprintf("allocuvm out of memory\n");
1992       deallocuvm(pgdir, newsz, oldsz);
1993       return 0;
1994     }
1995     memset(mem, 0, PGSIZE);
1996     if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
1997       cprintf("allocuvm out of memory (2)\n");
1998       deallocuvm(pgdir, newsz, oldsz);
1999       kfree(mem);
2000       return 0;
2001     }
2002   }
2003   return newsz;
2004 }
2005 
2006 // Lazy version of allocuvm
2007 int
2008 allocuvm_lazy(pde_t *pgdir, uint oldsz, uint newsz) {
2009   char *mem;
2010   uint a;
2011 
2012   if(newsz >= KERNBASE)
2013     return 0;
2014   if(newsz < oldsz)
2015     return oldsz;
2016 
2017   a = PGROUNDUP(oldsz);
2018   for(; a < newsz; a += PGSIZE){
2019     if((pgdir[PDX(a)] & PTE_P) == 0) {
2020       // allocate new page table here
2021       char *mem = kalloc();
2022       if(mem == 0)
2023         panic("allocuvm_lazy");
2024       memset(mem, 0, PGSIZE);
2025       pgdir[PDX(a)] = V2P(mem) | PTE_P | PTE_W | PTE_U;
2026     }
2027 
2028     pte_t *pgtable = P2V(PTE_ADDR(pgdir[PDX(a)]));
2029     pgtable[PTX(a)] = 0;
2030     pgtable[PTX(a)] = PTE_W | PTE_U | PTE_LAZY;
2031   }
2032   return newsz;
2033 }
2034 
2035 // Deallocate user pages to bring the process size from oldsz to
2036 // newsz.  oldsz and newsz need not be page-aligned, nor does newsz
2037 // need to be less than oldsz.  oldsz can be larger than the actual
2038 // process size.  Returns the new process size.
2039 int
2040 deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
2041 {
2042   pte_t *pte;
2043   uint a, pa;
2044 
2045   if(newsz >= oldsz)
2046     return oldsz;
2047 
2048 
2049 
2050   a = PGROUNDUP(newsz);
2051   for(; a  < oldsz; a += PGSIZE){
2052     pte = walkpgdir(pgdir, (char*)a, 0);
2053     if(!pte)
2054       a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
2055     else if((*pte & PTE_P) != 0){
2056       pa = PTE_ADDR(*pte);
2057       if(pa == 0)
2058         panic("kfree");
2059       char *v = P2V(pa);
2060       kfree(v);
2061       *pte = 0;
2062     }
2063   }
2064   return newsz;
2065 }
2066 
2067 // Free a page table and all the physical memory pages
2068 // in the user part.
2069 void
2070 freevm(pde_t *pgdir)
2071 {
2072   uint i;
2073 
2074   if(pgdir == 0)
2075     panic("freevm: no pgdir");
2076   deallocuvm(pgdir, KERNBASE, 0);
2077   for(i = 0; i < NPDENTRIES; i++){
2078     if(pgdir[i] & PTE_P){
2079       char * v = P2V(PTE_ADDR(pgdir[i]));
2080       kfree(v);
2081     }
2082   }
2083   kfree((char*)pgdir);
2084 }
2085 
2086 // Clear PTE_U on a page. Used to create an inaccessible
2087 // page beneath the user stack.
2088 void
2089 clearpteu(pde_t *pgdir, char *uva)
2090 {
2091   pte_t *pte;
2092 
2093   pte = walkpgdir(pgdir, uva, 0);
2094   if(pte == 0)
2095     panic("clearpteu");
2096   *pte &= ~PTE_U;
2097 }
2098 
2099 
2100 // Given a parent process's page table, create a copy
2101 // of it for a child.
2102 pde_t*
2103 copyuvm(pde_t *pgdir, uint sz)
2104 {
2105   pde_t *new_pgdir;
2106   if((new_pgdir = setupkvm()) == 0)
2107     return 0;
2108 
2109   for(int i=0; i<KERNBASE/(PGSIZE*NPTENTRIES); i++) {
2110     if((pgdir[i] & PTE_P) == 0)
2111       continue;
2112 
2113     pte_t *pgtable = P2V(PTE_ADDR(pgdir[i]));
2114     pte_t *new_pgtable = kalloc();
2115     if(new_pgtable == 0)
2116       goto bad;
2117     memmove(new_pgtable, pgtable, PGSIZE);
2118     for(int j=0; j<NPTENTRIES; j++) {
2119       pte_t pte = pgtable[j];
2120       uint flags = (pte & 0xfff);
2121 
2122       if((flags & (PTE_P | PTE_LAZY)) == 0)
2123         continue;
2124       if(flags & PTE_LAZY) {
2125         new_pgtable[j] = flags;
2126         continue;
2127       }
2128 
2129       char *mem = kalloc();
2130       if(mem == 0)
2131         goto bad;
2132       memmove(mem, (void*)(i*PGSIZE*NPTENTRIES+j*PGSIZE), PGSIZE);
2133       new_pgtable[j] = (V2P(mem) | flags);
2134     }
2135     new_pgdir[i] = ((uint)V2P(new_pgtable) | PTE_P | PTE_W | PTE_U);
2136   }
2137 
2138   return new_pgdir;
2139 bad:
2140   freevm(new_pgdir);
2141   return 0;
2142 }
2143 
2144 
2145 
2146 
2147 
2148 
2149 
2150 // Map user virtual address to kernel address.
2151 char*
2152 uva2ka(pde_t *pgdir, char *uva)
2153 {
2154   pte_t *pte;
2155 
2156   pte = walkpgdir(pgdir, uva, 0);
2157   if((*pte & PTE_P) == 0)
2158     return 0;
2159   if((*pte & PTE_U) == 0)
2160     return 0;
2161   return (char*)P2V(PTE_ADDR(*pte));
2162 }
2163 
2164 // Copy len bytes from p to user address va in page table pgdir.
2165 // Most useful when pgdir is not the current page table.
2166 // uva2ka ensures this only works for PTE_U pages.
2167 int
2168 copyout(pde_t *pgdir, uint va, void *p, uint len)
2169 {
2170   char *buf, *pa0;
2171   uint n, va0;
2172 
2173   buf = (char*)p;
2174   while(len > 0){
2175     va0 = (uint)PGROUNDDOWN(va);
2176     pa0 = uva2ka(pgdir, (char*)va0);
2177     if(pa0 == 0)
2178       return -1;
2179     n = PGSIZE - (va - va0);
2180     if(n > len)
2181       n = len;
2182     memmove(pa0 + (va - va0), buf, n);
2183     len -= n;
2184     buf += n;
2185     va = va0 + PGSIZE;
2186   }
2187   return 0;
2188 }
2189 
2190 
2191 
2192 
2193 
2194 
2195 
2196 
2197 
2198 
2199 
2200 // Blank page.
2201 
2202 
2203 
2204 
2205 
2206 
2207 
2208 
2209 
2210 
2211 
2212 
2213 
2214 
2215 
2216 
2217 
2218 
2219 
2220 
2221 
2222 
2223 
2224 
2225 
2226 
2227 
2228 
2229 
2230 
2231 
2232 
2233 
2234 
2235 
2236 
2237 
2238 
2239 
2240 
2241 
2242 
2243 
2244 
2245 
2246 
2247 
2248 
2249 
2250 // Blank page.
2251 
2252 
2253 
2254 
2255 
2256 
2257 
2258 
2259 
2260 
2261 
2262 
2263 
2264 
2265 
2266 
2267 
2268 
2269 
2270 
2271 
2272 
2273 
2274 
2275 
2276 
2277 
2278 
2279 
2280 
2281 
2282 
2283 
2284 
2285 
2286 
2287 
2288 
2289 
2290 
2291 
2292 
2293 
2294 
2295 
2296 
2297 
2298 
2299 
2300 // Blank page.
2301 
2302 
2303 // Walk the page directory and count the total number of valid user page entries from virtual address 0 to 2GB
2304 int sys_numvp(void) {
2305   struct proc *p = myproc();
2306 
2307   pde_t *pgdir = p->pgdir;
2308   int cnt = 0;
2309 
2310   for(int i=0; i<NPDENTRIES/2; i++) {
2311     if((pgdir[i] & PTE_P) == 0)
2312       continue;
2313 
2314     pte_t *pgtable = (pte_t*)P2V(PTE_ADDR(pgdir[i]));
2315     for(int j=0; j<NPTENTRIES; j++) {
2316       if((pgtable[j] & (PTE_P | PTE_LAZY)) == 0)
2317         continue;
2318       cnt++;
2319     }
2320   }
2321   return cnt + 1;     // 1 for stack guard page
2322   // return myproc()->sz/PGSIZE + 1;
2323 }
2324 
2325 int sys_numpp(void) {
2326   struct proc *p = myproc();
2327 
2328   pde_t *pgdir = p->pgdir;
2329   int cnt = 0;
2330 
2331   for(int i=0; i<NPDENTRIES/2; i++) {
2332     if((pgdir[i] & PTE_P) == 0)
2333       continue;
2334 
2335     pte_t *pgtable = (pte_t*)P2V(PTE_ADDR(pgdir[i]));
2336     for(int j=0; j<NPTENTRIES; j++) {
2337       if((pgtable[j] & PTE_P) == 0)
2338         continue;
2339       cnt++;
2340     }
2341   }
2342   return cnt + 1;     // 1 for stack guard page
2343 }
2344 
2345 
2346 
2347 
2348 
2349 
2350 int sys_getptsize(void) {
2351   int cnt = 0;
2352 
2353   pde_t *pgdir = myproc()->pgdir;
2354   for(int i=0; i<NPDENTRIES; i++) {
2355     if(pgdir[i] & PTE_P)
2356       cnt++;
2357   }
2358   return cnt + 1;     // 1 for the page directory page
2359 }
2360 
2361 
2362 int sys_mmap(void) {
2363   int n;
2364   char *start;
2365   if(argint(0, (int*)(&start)))
2366     return -1;
2367   if(argint(1, &n) < 0)
2368     return -1;
2369 
2370   struct proc *curr_proc = myproc();
2371   // Perform some checks for valid arguments
2372   if(n < 0)
2373     return -1;
2374   if(((uint)start & 0xfff) != 0) {
2375     cprintf("mmap: start address is not page aligned.\n");
2376     return -1;
2377   }
2378   if(start + PGROUNDUP(n) > KERNBASE) {
2379     cprintf("mmap: Invalid inputs to mmap(%x, %d).\n", start, n);
2380     return -1;
2381   }
2382 
2383 
2384   // Allocate the pages here
2385   for(uint p=start; p < start + PGROUNDUP(n); p += PGSIZE) {
2386     pte_t *pte = walkpgdir(curr_proc->pgdir, p, 1);
2387     if(*pte & PTE_P) {
2388       cprintf("mmap: given range of pages are not free.\n");
2389       return -1;
2390     }
2391     *pte = 0 | PTE_LAZY;
2392   }
2393 
2394   switchuvm(curr_proc);
2395   return 0;
2396 }
2397 
2398 
2399 
2400 int sys_munmap(void) {
2401   char *start;
2402   int n;    // size
2403   if(argint(0, (int*)&start) < 0)
2404     return -1;
2405   if(argint(1, &n) < 0)
2406     return -1;
2407   cprintf("sys_munmap called with arguments: {start = %x, n = %d}\n", start, n);
2408 
2409   // Perform some checks for valid arguments
2410   if(n < 0)
2411     return -1;
2412   if(((uint)start & 0xfff) != 0) {
2413     cprintf("munmap: start address is not page aligned.\n");
2414     return -1;
2415   }
2416   if(start + PGROUNDUP(n) > KERNBASE) {
2417     cprintf("munmap: Invalid inputs to mmap(%x, %d).\n", start, n);
2418     return -1;
2419   }
2420 
2421 
2422   // Iterate over the addresses and de-allocate the pages
2423   struct proc *curr_proc = myproc();
2424   pde_t *pgdir = curr_proc->pgdir;
2425   for(uint addr=start; addr < start+PGROUNDUP(n); addr += PGSIZE) {
2426 
2427 
2428     pte_t *pte = walkpgdir(pgdir, addr, 0);
2429     if(pte == 0) {
2430       cprintf("munmap: Page doesn't exist at address %x\n", addr);
2431       return -1;
2432     }
2433 
2434     // de-allocate the page here
2435     if(*pte & PTE_P)
2436       kfree(P2V(PTE_ADDR(*pte)));
2437     *pte = 0;
2438   }
2439 
2440   switchuvm(curr_proc);
2441 
2442 }
2443 
2444 
2445 
2446 
2447 
2448 
2449 
