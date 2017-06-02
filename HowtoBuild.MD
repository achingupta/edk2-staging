# Introduction

The AArch64StandaloneMm branch can be used in conjunction with ARM Trusted
Firmware to recreate a simple prototype that demonstrates communication between
two UEFI images where one is executing in the normal world and the other is
executing in the secure world.

The normal world image includes a:

1. DXE runtime driver that implements the protocol for communication with the MM
   environment in the secure world
2. Simple application (UefiInfo) that uses this driver to pass a reference to
   the UEFI system table to the MM environment

The secure world image includes:

1. The MM Standalone framework
2. The handler for acknowledging communication initiated by the UefiInfo
   application in normal world

The following sections describe the steps required to build all the components
of this prototype. The instructions have been executed on a host machine with
the following characteristics.

(*) Ubuntu 14.04 LTS system

(*) Preinstalling the following packages:

   acpica-tools
   bc
   bison
   build-essential
   curl
   flex
   g++-multilib
   gcc-multilib
   genext2fs
   git
   gperf
   iasl
   libc6:i386
   libstdc++6:i386
   libncurses5:i386
   libxml2-utils
   make
   openjdk-7-jdk
   python
   python-mako
   uuid-dev
   wget
   zlib1g:i386
   zlib1g-dev:i386
   zip

(*) Configure Git's user.email and user.name attributes:

  git config --global user.email "your.email@address.com"
  git config --global user.name "your name"

# Steps to build MM Standalone images
In user preferred "work" directory, execute the following shell commands
1. mkdir edk2
2. cd edk2
3. git clone https://github.com/tianocore/edk2-staging.git .
4. git checkout AArch64StandaloneMm
5. mkdir OpenPlatformPkg
6. cd OpenPlatformPkg
7. git clone https://git.linaro.org/uefi/OpenPlatformPkg.git .
8. git checkout AArch64StandaloneMm
9. cd ../..
10. mkdir arm-tf
11. cd arm-tf
12. git clone https://github.com/ARM-software/arm-trusted-firmware.git .
13. git checkout prototypes/secure_partitions/rfc_v1
14. cd ..
15. mkdir uefi-tools
16. cd uefi-tools
17. git clone https://git.linaro.org/uefi/uefi-tools.git .
18. git checkout master
19. cd ..
20. cd edk2
21. ../uefi-tools/uefi-build.sh -b DEBUG fvp_mm_standalone
22. ../uefi-tools/uefi-build.sh -a ../arm-tf -b DEBUG fvp_mm_normal

## Build Output
Step 21. will build the MM Standalone mode image to run in the secure
world. Step 22. will build the normal world UEFI image, ARM Trusted Firmware and
a Firmware Image Package (FIP) that includes both the UEFI images.

Boot Loader Stage 1 (BL1) binary and combined arm-tf/uefi firmware image package (fip) binary will be generated at:

1. edk2/Build/ArmVExpress-FVP-AArch64-MM-Normal/DEBUG_GCC49/FV/bl1.bin
2. edk2/Build/ArmVExpress-FVP-AArch64-MM-Normal/DEBUG_GCC49/FV/fip.bin

# Steps to run MM Standalone images
1. Download the ARMv8 Architecture FVP from<br />
https://silver.arm.com/download/download.tm?pv=3744408&p=1424570<br />
For more information, please refer<br />
https://developer.arm.com/products/system-design/fixed-virtual-platforms<br />
2. Install FVP into preferred "work" directory.
3. Create a shell script "run_mm.sh" in the same folder where "FVP_Base_AEMv8A-AEMv8A" is present.
Sample Shell script below:
./FVP_Base_AEMv8A-AEMv8A \
		-C cache_state_modelled=0 \
		-C bp.secure_memory=1 \
		-C bp.tzc_400.diagnostics=1 \
		-C bp.pl011_uart0.untimed_fifos=0 \
		-C cluster1.NUM_CORES=4 \
		-C cluster0.NUM_CORES=4 \
		-C bp.pl011_uart0.out_file=uart0.output \
		-C bp.pl011_uart1.out_file=uart1.output \
		-C bp.pl011_uart2.out_file=uart2.output \
		-C bp.pl011_uart3.out_file=uart3.output \
		-C bp.secureflashloader.fname="<Path to bl1.bin>" \
		-C bp.flashloader0.fname="<Path to fip.bin>" \
		-S -R
4. ./run_mm.sh
5. Output can be seen on FVP console.
6. The normal world will boot to the UEFI shell. MM communication can be
   demonstrated by executing the UefiInfo application.

## Sample Output
The following subsections describe output that signifies correct establishment
of MM communication between the secure and normal worlds.

### MM Standalone Output (FVP UART2)
NumSpMemRegions - 0x6  
SpMemBase       - 0xFF200000  
SpMemLimit      - 0x100000000  
SpImageBase     - 0xFF200000  
SpStackBase     - 0xFF610000  
SpHeapBase      - 0xFF620000  
SpNsCommBufBase - 0xFF600000  
SpSharedBufBase - 0xFF500000  
SpImageSize     - 0x300000  
SpPcpuStackSize - 0x2000  
SpHeapSize      - 0x9E0000  
SpNsCommBufSize - 0x10000  
SpPcpuSharedBufSize - 0x20000  
NumCpus         - 0x8  
CpuInfo         - 0xFF500680  
Mpidr           - 0x80000000  
LinearId        - 0x0  
Flags           - 0x1  
Mpidr           - 0x80000001  
LinearId        - 0x1  
Flags           - 0x0  
Mpidr           - 0x80000002  
LinearId        - 0x2  
Flags           - 0x0  
Mpidr           - 0x80000003  
LinearId        - 0x3  
Flags           - 0x0  
Mpidr           - 0x80000100  
LinearId        - 0x4  
Flags           - 0x0  
Mpidr           - 0x80000101  
LinearId        - 0x5  
Flags           - 0x0  
Mpidr           - 0x80000102  
LinearId        - 0x6  
Flags           - 0x0  
Mpidr           - 0x80000103  
LinearId        - 0x7  
Flags           - 0x0  
Found Standalone MM PE data - 0xFF201000  
Found Standalone MM PE data - 0xFF201000  
Standalone MM Core PE-COFF SectionHeaderOffset - 0xF60, NumberOfSections - 3  
UpdateMmFoundationPeCoffPermissions: Section 0 of image at 0xFF201000 has 0x60000020 permissions  
UpdateMmFoundationPeCoffPermissions: Section 0 of image at 0xFF201000 has .et name  
UpdateMmFoundationPeCoffPermissions: Section 0 of image at 0xFF201000 has 0xFF202000 address  
UpdateMmFoundationPeCoffPermissions: Section 0 of image at 0xFF201000 has 0x1000 data  
UpdateMmFoundationPeCoffPermissions: Ignoring section 0 of image at 0xFF201000 with 0x60000020 permissions  
UpdateMmFoundationPeCoffPermissions: Section 1 of image at 0xFF201000 has 0xC0000040 permissions  
UpdateMmFoundationPeCoffPermissions: Section 1 of image at 0xFF201000 has .aa name  
UpdateMmFoundationPeCoffPermissions: Section 1 of image at 0xFF201000 has 0xFF219000 address  
UpdateMmFoundationPeCoffPermissions: Section 1 of image at 0xFF201000 has 0x18000 data  
UpdateMmFoundationPeCoffPermissions: Mapping section 1 of image at 0xFF201000 with RW-XN permissions  
UpdateMmFoundationPeCoffPermissions: Section 2 of image at 0xFF201000 has 0x42000040 permissions  
UpdateMmFoundationPeCoffPermissions: Section 2 of image at 0xFF201000 has .eo name  
UpdateMmFoundationPeCoffPermissions: Section 2 of image at 0xFF201000 has 0xFF21A000 address  
UpdateMmFoundationPeCoffPermissions: Section 2 of image at 0xFF201000 has 0x19000 data  
UpdateMmFoundationPeCoffPermissions: Mapping section 2 of image at 0xFF201000 with RO-XN permissions  
StandaloneSmmCoreMemoryAllocationLibConstructor - 0xFF620000  
SmramRangeCount - 0x6  
SmramRanges[0]: 0x00000000FF200000 - 0x0000000000300000  
SmramRanges[1]: 0x00000000FF500000 - 0x0000000000100000  
SmramRanges[2]: 0x00000000FF600000 - 0x0000000000010000  
SmramRanges[3]: 0x00000000FF610000 - 0x0000000000010000  
SmramRanges[4]: 0x00000000FF620000 - 0x00000000000002C8  
SmramRanges[5]: 0x00000000FF6202C8 - 0x00000000009DFD38  
SmmInitializeMemoryServices  
SmmAddMemoryRegion 0 : 0x00000000FF200000 - 0x0000000000300000  
SmmAddMemoryRegion 1 : 0x00000000FF500000 - 0x0000000000100000  
SmmAddMemoryRegion 2 : 0x00000000FF600000 - 0x0000000000010000  
SmmAddMemoryRegion 3 : 0x00000000FF610000 - 0x0000000000010000  
SmmAddMemoryRegion 4 : 0x00000000FF620000 - 0x00000000000002C8  
SmmAddMemoryRegion 5 : 0x00000000FF6202C8 - 0x00000000009DFD38  
mSmmMemLibInternalMaximumSupportAddress = 0xFFFFFFFFF  
SmmMain - 0xFF620000  
SmramRangeCount - 0x6  
SmramRanges[0]: 0x00000000FF200000 - 0x300000  
SmramRanges[1]: 0x00000000FF500000 - 0x100000  
SmramRanges[2]: 0x00000000FF600000 - 0x10000  
SmramRanges[3]: 0x00000000FF610000 - 0x10000  
SmramRanges[4]: 0x00000000FF620000 - 0x2C8  
SmramRanges[5]: 0x00000000FF6202C8 - 0x9DFD38  
mSmramRangeCount - 0x6  
mSmramRanges - 0xFFFFEE10  
BFV address - 0xFF200000  
BFV size    - 0x300000  
SmmInstallConfigurationTable For HobList  
HobSize - 0x2C8  
SmmHobStart - 0xFFFFE810  
SmmInstallProtocolInterface - gEfiMmHandlerStateNotificationProtocolGuid  
SmmInstallProtocolInterface: 30C8340F-4C30-41D9-BFAE-444ACB2C1F76 FF219218  
SmmRegisterProtocolNotify - SmmConfigurationSmmProtocol  
MmRegisterProtocolNotify - MmConfigurationMmProtocol  
Smm Dispatch StandaloneBfvAddress - 0xFF200000  
SmmCoreFfsFindSmmDriver - 0xFF200000  
FvIsBeingProcesssed - 0xFF200000  
Check SmmFileTypes - 0xA  
Check SmmFileTypes - 0xE  
Find PE data - 0xFF21B024  
SmmAddToDriverList - 58F7A62B-6280-42A7-BC38-10535A64A92C (0xFF21B024)  
SmmDispatcher  
  Drain the Scheduled Queue  
  Search DriverList for items to place on Scheduled Queue  
  DriverEntry (Discovered) - 58F7A62B-6280-42A7-BC38-10535A64A92C  
Evaluate SMM DEPEX for FFS(58F7A62B-6280-42A7-BC38-10535A64A92C)  
  TRUE  
  END  
  RESULT = TRUE  
  Drain the Scheduled Queue  
  DriverEntry (Scheduled) - 58F7A62B-6280-42A7-BC38-10535A64A92C  
SmmLoadImage - 58F7A62B-6280-42A7-BC38-10535A64A92C  
UpdatePeCoffPermissions: Mapping section 0 of image at 0xFFFE7000 with RO-XN permissions and size 0x7000  
UpdatePeCoffPermissions: Mapping section 0 of image at 0xFFFE7000 with RO-X permissions and size 0x7000  
UpdatePeCoffPermissions: Mapping section 1 of image at 0xFFFEE000 with RW-XN permissions and size 0x1000  
UpdatePeCoffPermissions: Mapping section 2 of image at 0xFFFEF000 with RO-XN permissions and size 0x1000  
Loading SMM driver at 0x000FFFE6000 EntryPoint=0x000FFFE7000 PiMmStandloneArmTfCpuDriver.efi  
StartImage - 0xFFFE7000 (Standalone Mode)  
SmmInstallProtocolInterface: 0C109319-C149-450E-A3E3-B9BADD9DC3A4 FFFEE008  
MmConfigurationMmNotify(0C109319-C149-450E-A3E3-B9BADD9DC3A4) - FFFEE008  
MM Core registered MM Entry Point address FF204DE4  
SmmInstallProtocolInterface: EB346B97-975F-4A9F-8B22-F8E92BB3D569 FFFEE010  
Sharing Cpu Driver EP *0xFF2192C0 = 0xFFFE7A80  
mNsCommBuffer.PhysicalStart - 0xFF600000  
mNsCommBuffer.PhysicalSize - 0x10000  
mNsCommBuffer: 0x00000000FF600000 - 0x10000  
mMpInformationHobData: 0x0000000000000008 - 0x8  
mMpInformationHobData[0x80000000]: 0, 0, 0  
mMpInformationHobData[0x80000001]: 0, 1, 1  
mMpInformationHobData[0x80000002]: 0, 2, 2  
mMpInformationHobData[0x80000003]: 0, 3, 3  
mMpInformationHobData[0x80000100]: 1, 0, 0  
mMpInformationHobData[0x80000101]: 1, 1, 1  
mMpInformationHobData[0x80000102]: 1, 2, 2  
mMpInformationHobData[0x80000103]: 1, 3, 3  
SmiHandlerRegister - GUID A37721E4-8C0B-4BCA-B5E8-E902A025514E - Status 0  
SmiHandlerRegister - GUID B65694CC-09E3-4C3B-B5CD-05F44D3CDBFF - Status 0  
SmiHandlerRegister - GUID 7081E22F-CAC6-4053-9468-675782CF88E5 - Status 0  
SmiHandlerRegister - GUID 60FF8964-E906-41D0-AFED-F241E974E08E - Status 0  
SmiHandlerRegister - GUID 02CE967A-DD7E-4FFC-9EE7-810CF0470880 - Status 0  
SmiHandlerRegister - GUID 2A571201-4966-47F6-8B86-F31E41F32F10 - Status 0  
SmiHandlerRegister - GUID 27ABF055-B1B8-4C26-8048-748F37BAA2DF - Status 0  
SmiHandlerRegister - GUID 7CE88FB3-4BD7-4679-87A8-A8D8DEE50D2B - Status 0  
SmmMain Done!  
Shared Cpu Driver EP 0xFFFE7A80  
SpStackBase         - 0xFF614000  
SpSharedBufBase     - 0xFF520000  
SpPcpuStackSize     - 0x2000  
SpPcpuSharedBufSize - 0x20000  
CpuInfo             - 0x80000001  
Mpidr               - 0x80000001  
LinearId            - 0x1  
Flags               - 0x0  
SpStackBase         - 0xFF616000  
SpSharedBufBase     - 0xFF540000  
SpPcpuStackSize     - 0x2000  
SpPcpuSharedBufSize - 0x20000  
CpuInfo             - 0x80000002  
Mpidr               - 0x80000002  
LinearId            - 0x2  
Flags               - 0x0  
SpStackBase         - 0xFF618000  
SpSharedBufBase     - 0xFF560000  
SpPcpuStackSize     - 0x2000  
SpPcpuSharedBufSize - 0x20000  
CpuInfo             - 0x80000003  
Mpidr               - 0x80000003  
LinearId            - 0x3  
Flags               - 0x0  
SpStackBase         - 0xFF61A000  
SpSharedBufBase     - 0xFF580000  
SpPcpuStackSize     - 0x2000  
SpPcpuSharedBufSize - 0x20000  
CpuInfo             - 0x80000100  
Mpidr               - 0x80000100  
LinearId            - 0x4  
Flags               - 0x0  
SpStackBase         - 0xFF61C000  
SpSharedBufBase     - 0xFF5A0000  
SpPcpuStackSize     - 0x2000  
SpPcpuSharedBufSize - 0x20000  
CpuInfo             - 0x80000101  
Mpidr  

#### Output after UefiInfo is executed in UEFI Shell (FVP UART0)
FS0:\> UefiInfo.efi  
add-symbol-file /home/achgup01/work/genfw/uefi/edk2/Build/ArmVExpress-FVP-AArch64-MM-Normal/DEBUG_GCC49/AARCH64/MdeModulePkg/Application/UefiInfoApp/UefiInfo/DEBUG/UefiInfo.dll 0xF5F99000  
Loading driver at 0x000F5F98000 EntryPoint=0x000F5F99000 UefiInfo.efi  
0xFF600000 - 0xFEFFF550 - 0x20  
ARM_SMC_ID_MM_COMMUNICATE_AARCH64 0xC4000041 - 0xFF600000 - 0x0  
remove-symbol-file /home/achgup01/work/genfw/uefi/edk2/Build/ArmVExpress-FVP-AArch64-MM-Normal/DEBUG_GCC49/AARCH64/MdeModulePkg/Application/UefiInfoApp/UefiInfo/DEBUG/UefiInfo.dll 0xF5F99000  

### ARM TF (FVP UART0)
NOTICE:  Booting Trusted Firmware  
NOTICE:  BL1: v1.3(debug):v1.3-659-g6b2b576  
NOTICE:  BL1: Built : 10:08:00, May 31 2017  
INFO:    BL1: RAM 0x4036000 - 0x403c000  
INFO:    BL1: Loading BL2  
INFO:    Loading image id=1 at address 0x4027000  
INFO:    Image id=1 loaded: 0x4027000 - 0x402e2a8  
NOTICE:  BL1: Booting BL2  
INFO:    Entry point address = 0x4027000  
INFO:    SPSR = 0x3c5  
NOTICE:  BL2: v1.3(debug):v1.3-659-g6b2b576  
NOTICE:  BL2: Built : 10:08:00, May 31 2017  
INFO:    BL2: Doing platform setup  
INFO:    Configuring TrustZone Controller  
INFO:    BL2: Loading image id 3  
INFO:    Loading image id=3 at address 0xff000000  
INFO:    Image id=3 loaded: 0xff000000 - 0xff00e218  
INFO:    BL2: Loading image id 4  
INFO:    Loading image id=4 at address 0xff200000  
INFO:    Image id=4 loaded: 0xff200000 - 0xff480000  
INFO:    BL2: Loading image id 5  
INFO:    Loading image id=5 at address 0x88000000  
INFO:    Image id=5 loaded: 0x88000000 - 0x88280000  
NOTICE:  BL1: Booting BL31  
INFO:    Entry point address = 0xff000000  
INFO:    SPSR = 0x3cd  
NOTICE:  BL31: v1.3(debug):v1.3-659-g6b2b576  
NOTICE:  BL31: Built : 10:08:00, May 31 2017  
INFO:    GICv3 with legacy support detected. ARM GICV3 driver initialized in EL3  
INFO:    BL31: Initializing runtime services  
INFO:    BL31: Initializing BL32  
NOTICE:  Received SPM_MEMORY_ATTRIBUTES_SET SMC  
NOTICE:    Start address  : 0xff219000  
NOTICE:    Number of pages: 1 (4096 bytes)  
NOTICE:    Attributes     : 0x7  
NOTICE:    (Equivalent TF attributes: 0x22)  
NOTICE:  Received SPM_MEMORY_ATTRIBUTES_SET SMC  
NOTICE:    Start address  : 0xff219000  
NOTICE:    Number of pages: 1 (4096 bytes)  
NOTICE:    Attributes     : 0x5  
NOTICE:    (Equivalent TF attributes: 0x2a)  
NOTICE:  Received SPM_MEMORY_ATTRIBUTES_SET SMC  
NOTICE:    Start address  : 0xff21a000  
NOTICE:    Number of pages: 1 (4096 bytes)  
NOTICE:    Attributes     : 0x7  
NOTICE:    (Equivalent TF attributes: 0x22)  
NOTICE:  Received SPM_MEMORY_ATTRIBUTES_SET SMC  
NOTICE:    Start address  : 0xfffe7000  
NOTICE:    Number of pages: 7 (28672 bytes)  
NOTICE:    Attributes     : 0x7  
NOTICE:    (Equivalent TF attributes: 0x22)  
NOTICE:  Received SPM_MEMORY_ATTRIBUTES_SET SMC  
NOTICE:    Start address  : 0xfffe7000  
NOTICE:    Number of pages: 7 (28672 bytes)  
NOTICE:    Attributes     : 0x3  
NOTICE:    (Equivalent TF attributes: 0x2)  
NOTICE:  Received SPM_MEMORY_ATTRIBUTES_SET SMC  
NOTICE:    Start address  : 0xfffef000  
NOTICE:    Number of pages: 1 (4096 bytes)  
NOTICE:    Attributes     : 0x7  
NOTICE:    (Equivalent TF attributes: 0x22)  
INFO:    BL31: Preparing for EL3 exit to normal world  
INFO:    Entry point address = 0x88000000  
INFO:    SPSR = 0x3c9  
