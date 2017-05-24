# Changes since commit 86111c5:

1. Rebased on edk2-master commit 00b00cc
2. Changed MM_COMMUNICATION DXE driver to remove unused functionality and align
   SMC FIDs with MM interface specification
3. Renamed PiMmCpuTpFw CPU driver to something more comprehensible
4. Split PiMmStandloneArmTfCpuDriver functionality into separate files
5. Removed unused code from PiMmStandloneArmTfCpuDriver. Event handling
   interface with ARM TF is not needed for synchronous MM communication.
6. Introduced a shared buffer between ARM TF and Standalone MM. The former has
   RW+XN access while the latter has RO+XN access
7. ARM TF maps Standalone MM memory occupied by BFV as RO+X. Heap is marked as
   RW+XN
8. Added an AArch64 specific entry point driver that is responsible for changing
   memory permission attributes using an ARM TF exported SVC/SMC interface
9. Added a new Hob library that allows the entry point driver to create a Hob
   list upon entry during cold boot instead of doing the same in ARM TF
10. Used Ard Biesheuvel's prototype patches to change the permission attributes
    of the MM Standalone Core driver and other drivers when relocated to the
    heap
11. Introduced an event loop in the entry point driver to handle delegated
    events
12. Added rudimentary support for initialising the M environment on secondary
    CPUs
13. Updated instructions to recreate the prototype implementation
