7400 // See MultiProcessor Specification Version 1.[14]
7401 
7402 struct mp {             // floating pointer
7403   uchar signature[4];           // "_MP_"
7404   void *physaddr;               // phys addr of MP config table
7405   uchar length;                 // 1
7406   uchar specrev;                // [14]
7407   uchar checksum;               // all bytes must add up to 0
7408   uchar type;                   // MP system config type
7409   uchar imcrp;
7410   uchar reserved[3];
7411 };
7412 
7413 struct mpconf {         // configuration table header
7414   uchar signature[4];           // "PCMP"
7415   ushort length;                // total table length
7416   uchar version;                // [14]
7417   uchar checksum;               // all bytes must add up to 0
7418   uchar product[20];            // product id
7419   uint *oemtable;               // OEM table pointer
7420   ushort oemlength;             // OEM table length
7421   ushort entry;                 // entry count
7422   uint *lapicaddr;              // address of local APIC
7423   ushort xlength;               // extended table length
7424   uchar xchecksum;              // extended table checksum
7425   uchar reserved;
7426 };
7427 
7428 struct mpproc {         // processor table entry
7429   uchar type;                   // entry type (0)
7430   uchar apicid;                 // local APIC id
7431   uchar version;                // local APIC verison
7432   uchar flags;                  // CPU flags
7433     #define MPBOOT 0x02           // This proc is the bootstrap processor.
7434   uchar signature[4];           // CPU signature
7435   uint feature;                 // feature flags from CPUID instruction
7436   uchar reserved[8];
7437 };
7438 
7439 struct mpioapic {       // I/O APIC table entry
7440   uchar type;                   // entry type (2)
7441   uchar apicno;                 // I/O APIC id
7442   uchar version;                // I/O APIC version
7443   uchar flags;                  // I/O APIC flags
7444   uint *addr;                  // I/O APIC address
7445 };
7446 
7447 
7448 
7449 
7450 // Table entry types
7451 #define MPPROC    0x00  // One per processor
7452 #define MPBUS     0x01  // One per bus
7453 #define MPIOAPIC  0x02  // One per I/O APIC
7454 #define MPIOINTR  0x03  // One per bus interrupt source
7455 #define MPLINTR   0x04  // One per system interrupt source
7456 
7457 
7458 
7459 
7460 
7461 
7462 
7463 
7464 
7465 
7466 
7467 
7468 
7469 
7470 
7471 
7472 
7473 
7474 
7475 
7476 
7477 
7478 
7479 
7480 
7481 
7482 
7483 
7484 
7485 
7486 
7487 
7488 
7489 
7490 
7491 
7492 
7493 
7494 
7495 
7496 
7497 
7498 
7499 
7500 // Blank page.
7501 
7502 
7503 
7504 
7505 
7506 
7507 
7508 
7509 
7510 
7511 
7512 
7513 
7514 
7515 
7516 
7517 
7518 
7519 
7520 
7521 
7522 
7523 
7524 
7525 
7526 
7527 
7528 
7529 
7530 
7531 
7532 
7533 
7534 
7535 
7536 
7537 
7538 
7539 
7540 
7541 
7542 
7543 
7544 
7545 
7546 
7547 
7548 
7549 
