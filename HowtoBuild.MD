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
`StandaloneSmmCoreMemoryAllocationLibConstructor - 0xFC280000
SmramRangeCount - 0x5
SmramRanges[0]: 0x00000000FC000000 - 0x0000000000280000
SmramRanges[1]: 0x00000000FFDF8000 - 0x0000000000008000
SmramRanges[2]: 0x00000000FC280000 - 0x0000000000000240
SmramRanges[3]: 0x00000000FC280240 - 0x0000000003B77DC0
SmramRanges[4]: 0x00000000FBE00000 - 0x0000000000200000
SmmInitializeMemoryServices
SmmAddMemoryRegion 0 : 0x00000000FC000000 - 0x0000000000280000
SmmAddMemoryRegion 1 : 0x00000000FFDF8000 - 0x0000000000008000
SmmAddMemoryRegion 2 : 0x00000000FC280000 - 0x0000000000000240
SmmAddMemoryRegion 3 : 0x00000000FC280240 - 0x0000000003B77DC0
SmmAddMemoryRegion 4 : 0x00000000FBE00000 - 0x0000000000200000
mSmmMemLibInternalMaximumSupportAddress = 0xFFFFFFFFF
SmmMain - 0xFC280000
SmramRangeCount - 0x5
SmramRanges[0]: 0x00000000FC000000 - 0x280000
SmramRanges[1]: 0x00000000FFDF8000 - 0x8000
SmramRanges[2]: 0x00000000FC280000 - 0x240
SmramRanges[3]: 0x00000000FC280240 - 0x3B77DC0
SmramRanges[4]: 0x00000000FBE00000 - 0x200000
mSmramRangeCount - 0x5
mSmramRanges - 0xFFDF6E10
BFV address - 0xFC000000
BFV size    - 0x280000
SmmInstallConfigurationTable For HobList
HobSize - 0x240
SmmHobStart - 0xFFDF6810
SmmInstallProtocolInterface - gEfiMmHandlerStateNotificationProtocolGuid
SmmInstallProtocolInterface: 30C8340F-4C30-41D9-BFAE-444ACB2C1F76 FC014210
SmmRegisterProtocolNotify - SmmConfigurationSmmProtocol
MmRegisterProtocolNotify - MmConfigurationMmProtocol
Smm Dispatch StandaloneBfvAddress - 0xFC000000
SmmCoreFfsFindSmmDriver - 0xFC000000
FvIsBeingProcesssed - 0xFC000000
Check SmmFileTypes - 0xA
Check SmmFileTypes - 0xE
Find PE data - 0xFC016024
SmmAddToDriverList - 58F7A62B-6280-42A7-BC38-10535A64A92C (0xFC016024)
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
Loading SMM driver at 0x000FFDDE000 EntryPoint=0x000FFDDF000 ArmPiMmCpuTpFw.efi
StartImage - 0xFFDDF000 (Standalone Mode)
SmmInstallProtocolInterface: 0C109319-C149-450E-A3E3-B9BADD9DC3A4 FFDE6000
MmConfigurationMmNotify(0C109319-C149-450E-A3E3-B9BADD9DC3A4) - FFDE6000
MM Core registered MM Entry Point address FC001718
SmmInstallProtocolInterface: EB346B97-975F-4A9F-8B22-F8E92BB3D569 FFDE6008
SmramRangeCount - 0x5
SmramRanges[0]: 0x00000000FC000000 - 0x280000 - 0x18
SmramRanges[1]: 0x00000000FFDF8000 - 0x8000 - 0x18
SmramRanges[2]: 0x00000000FC280000 - 0x240 - 0x18
SmramRanges[3]: 0x00000000FC280240 - 0x3B77DC0 - 0x8
SmramRanges[4]: 0x00000000FBE00000 - 0x200000 - 0xCFDFDFDF00000098
mNsCommBuffer SmramRangeindex 4
mNsCommBuffer: 0x00000000FBE00000 - 0x200000
mMpInformationHobData: 0x0000000000000008 - 0x8
mMpInformationHobData[0x80000000]: 0, 0, 0
mMpInformationHobData[0x80000001]: 0, 1, 0
mMpInformationHobData[0x80000002]: 0, 2, 0
mMpInformationHobData[0x80000003]: 0, 3, 0
mMpInformationHobData[0x80000100]: 0, 4, 0
mMpInformationHobData[0x80000101]: 0, 5, 0
mMpInformationHobData[0x80000102]: 0, 6, 0
mMpInformationHobData[0x80000103]: 0, 7, 0
MmiHandlerStateNotifier - GUID A37721E4-8C0B-4BCA-B5E8-E902A025514E, Handler - 0xFC001614
, State - 0SmiHandlerRegister - GUID A37721E4-8C0B-4BCA-B5E8-E902A025514E - Status 0
MmiHandlerStateNotifier - Unknown GUID B65694CC-09E3-4C3B-B5CD-05F44D3CDBFF, Handler - 0xFC006780
, State - 0, Status - 14SmiHandlerRegister - GUID B65694CC-09E3-4C3B-B5CD-05F44D3CDBFF - Status 0
MmiHandlerStateNotifier - Unknown GUID 7081E22F-CAC6-4053-9468-675782CF88E5, Handler - 0xFC0066AC
, State - 0, Status - 14SmiHandlerRegister - GUID 7081E22F-CAC6-4053-9468-675782CF88E5 - Status 0
MmiHandlerStateNotifier - Unknown GUID 60FF8964-E906-41D0-AFED-F241E974E08E, Handler - 0xFC001480
, State - 0, Status - 14SmiHandlerRegister - GUID 60FF8964-E906-41D0-AFED-F241E974E08E - Status 0
MmiHandlerStateNotifier - Unknown GUID 02CE967A-DD7E-4FFC-9EE7-810CF0470880, Handler - 0xFC001598
, State - 0, Status - 14SmiHandlerRegister - GUID 02CE967A-DD7E-4FFC-9EE7-810CF0470880 - Status 0
MmiHandlerStateNotifier - Unknown GUID 2A571201-4966-47F6-8B86-F31E41F32F10, Handler - 0xFC00133C
, State - 0, Status - 14SmiHandlerRegister - GUID 2A571201-4966-47F6-8B86-F31E41F32F10 - Status 0
MmiHandlerStateNotifier - Unknown GUID 27ABF055-B1B8-4C26-8048-748F37BAA2DF, Handler - 0xFC0013A8
, State - 0, Status - 14SmiHandlerRegister - GUID 27ABF055-B1B8-4C26-8048-748F37BAA2DF - Status 0
MmiHandlerStateNotifier - Unknown GUID 7CE88FB3-4BD7-4679-87A8-A8D8DEE50D2B, Handler - 0xFC001414
, State - 0, Status - 14SmiHandlerRegister - GUID 7CE88FB3-4BD7-4679-87A8-A8D8DEE50D2B - Status 0
SmmMain Done!`

#### Output after UefiInfo is executed in UEFI Shell
`Received event - 0x10 on cpu 0
SmmEntryPoint ...
CommBuffer - 0xFFDF3A90, CommBufferSize - 0x20
SmmUefiInfoHandler
  SystemTable - 0x00000000EFFE0018
SmmEntryPoint Done`

### ARM TF (FVP UART0)
`NOTICE:  Booting Trusted Firmware
NOTICE:  BL1: v1.3(debug):f15c6ed
NOTICE:  BL1: Built : 18:21:32, Jan 19 2017
INFO:    BL1: RAM 0x4036000 - 0x403c000
INFO:    BL1: Loading BL2
INFO:    Loading image id=1 at address 0x4014000
INFO:    Size of image id=1 at address 0x4014000 is 0x81a0
INFO:    Image id=1 loaded at address 0x4014000, size = 0x81a0
NOTICE:  BL1: Booting BL2
INFO:    Entry point address = 0x4014000
INFO:    SPSR = 0x3c5
NOTICE:  BL2: v1.3(debug):f15c6ed
NOTICE:  BL2: Built : 18:21:32, Jan 19 2017
INFO:    Configuring TrustZone Controller
INFO:    Configuring NS memory - 0x80000000, 0xfbffffff, 0x200000 bytes
INFO:    BL2: Loading BL31
INFO:    Loading image id=3 at address 0x4023000
INFO:    Size of image id=3 at address 0x4023000 is 0xc018
INFO:    Image id=3 loaded at address 0x4023000, size = 0xc018
INFO:    BL2: Loading BL32
INFO:    Loading image id=4 at address 0xffe00000
INFO:    Size of image id=4 at address 0xffe00000 is 0x1000
INFO:    Image id=4 loaded at address 0xffe00000, size = 0x1000
INFO:    BL2: Loading AP_BL3_SFS_PAYLOAD0
INFO:    Loading image id=21 at address 0xfc000000
INFO:    Size of image id=21 at address 0xfc000000 is 0x280000
INFO:    Image id=21 loaded at address 0xfc000000, size = 0x280000
INFO:    BL2: ====SFS_PAYLOAD Information====
INFO:    BL2: image base=0xfc000000
INFO:    BL2: total base=0xfc000000
INFO:    BL2: free base=0xfc280000
INFO:    BL2: stack base=0xffdf8000
INFO:    BL2: image size=0x280000
INFO:    BL2: total size=0x3e00000
INFO:    BL2: free size=0x3b78000
INFO:    BL2: stack size=0x8000
INFO:    BL2: mm_mem_regions=0x401c0d0
INFO:    BL2: mm_mem_regions_size=0xa4
INFO:    BL2: size of mpinfo_hob_data = 0x208
INFO:    BL2: hoblist base=0xfc280000
INFO:    BL2: hoblist size=0x240
INFO:    BL2: free memory base=0xfc280240
INFO:    BL2: free memory size=0x3b77dc0
INFO:    BL2: ===============================
INFO:    BL2: Loading BL33
INFO:    Loading image id=5 at address 0x88000000
INFO:    Size of image id=5 at address 0x88000000 is 0x280000
INFO:    Image id=5 loaded at address 0x88000000, size = 0x280000
NOTICE:  BL1: Booting BL31
INFO:    Entry point address = 0x4023000
INFO:    SPSR = 0x3cd
INFO:    BL31: mm payload ep_info=0
INFO:    BL31: mm payload pc=0xfc000000
INFO:    BL31: mm payload spsr=0x3c0
INFO:    BL31: mm payload arg0=0xfc280000
INFO:    BL31: BL32 free base=0xffe01000
INFO:    BL31: BL32 l1 base=0xffe01000
INFO:    BL31: BL32 l1 end=0xffe01020
INFO:    BL31: BL32 lx base=0xffe02000
INFO:    BL31: BL32 lx end=0xffe06000
INFO:    BL31: BL32 free end=NOTICE:  BL31: v1.3(debug):f15c6ed
NOTICE:  BL31: Built : 18:21:32, Jan 19 2017
INFO:    GICv3 with legacy support detected. ARM GICV3 driver initialized in EL3
INFO:    BL31: Initializing runtime services
INFO:    BL31: Initializing BL32
INFO:    mm_stub: mm payload stack base=0xffdf9000
INFO:    BL31: Preparing for EL3 exit to normal world
INFO:    Entry point address = 0x88000000
INFO:    SPSR = 0x3c9`

### EDK2 Output (FVP UART0)
`UEFI firmware (version 2a28be6 built at 18:21:17 on Jan 19 2017)
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/ArmPlatformPkg/PrePi/PeiUniCore/DEBUG/ArmPlatformPrePiUniCore.dll 0x88000800
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/MdeModulePkg/Core/Dxe/DxeMain/DEBUG/DxeCore.dll 0xF3186000
Loading DxeCore at 0x00F3185000 EntryPoint=0x00F3186000
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/MdeModulePkg/Core/Dxe/DxeMain/DEBUG/DxeCore.dll 0xF3186000
HOBLIST address in DXE = 0xEFDBD018
Memory Allocation 0x00000004 0xF3FFB000 - 0xF3FFBFFF
Memory Allocation 0x00000004 0xF3FFA000 - 0xF3FFAFFF
Memory Allocation 0x00000004 0xF3FF9000 - 0xF3FF9FFF
Memory Allocation 0x00000004 0xF3FF8000 - 0xF3FF8FFF
Memory Allocation 0x00000004 0xF3FFC000 - 0xF3FFFFFF
Memory Allocation 0x00000004 0xF3FE8000 - 0xF3FF7FFF
Memory Allocation 0x00000004 0xF38D6000 - 0xF3FE7FFF
Memory Allocation 0x00000004 0xF31C4000 - 0xF38D5FFF
Memory Allocation 0x00000004 0xF3185000 - 0xF31C3FFF
Memory Allocation 0x00000003 0xF3185000 - 0xF31C3FFF
FV Hob            0x88000000 - 0x8827FFFF
FV Hob            0xF31C4000 - 0xF38D49FF
FV2 Hob           0xF31C4000 - 0xF38D49FF
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/MdeModulePkg/Universal/PCD/Dxe/Pcd/DEBUG/PcdDxe.dll 0xEFF41000
Loading driver at 0x000EFF40000 EntryPoint=0x000EFF41048 PcdDxe.efi
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/ArmPkg/Drivers/CpuDxe/CpuDxe/DEBUG/ArmCpuDxe.dll 0xEFF31000
Loading driver at 0x000EFF30000 EntryPoint=0x000EFF31048 ArmCpuDxe.efi
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/MdeModulePkg/Core/RuntimeDxe/RuntimeDxe/DEBUG/RuntimeDxe.dll 0xEAF40000
.
.
.
.
.
.
.
[2J[04D[=3h[2J[09D[2J[04D[=3h[2J[09D[2J[04D[=3h[2J[09D[2J[04D[=3h[2J[09D[2J[04D[=3h[2J[09D[2J[04D[=3h[2J[09D[0m[35m[40m[0m[37m[40mPress ESCAPE for boot options ..........[Bds]Booting UEFI Misc Device
[Bds]Booting UEFI Misc Device 2
[Bds]Booting UEFI Misc Device 3
[Bds]Booting UEFI Misc Device 4
[Bds]Booting UEFI Misc Device 5
[Bds]Booting UEFI Non-Block Boot Device
[Bds]Booting UEFI Non-Block Boot Device 2
[Bds]Booting UEFI Non-Block Boot Device 3
[Bds]Booting UEFI Shell
add-symbol-file /home/supven01/mm/upstream/edk2/Build/ArmVExpress-FVP-AArch64-Normal-MM/DEBUG_GCC49/AARCH64/ShellPkg/Application/Shell/Shell/DEBUG/Shell.dll 0xEA656000
Loading driver at 0x000EA655000 EntryPoint=0x000EA656000 Shell.efi
[2J[44DUEFI Interactive Shell v2.1
EDK II
UEFI v2.60 (EDK II, 0x00010000)
[1m[33m[40mMapping table[0m[37m[40m
[1m[33m[40m      FS2:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40mF5:
          VenHw(C5B9C74A-6D72-4719-99AB-C59F199091EB)
[1m[33m[40m      FS1:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40mF1:
          MemoryMapped(0xB,0x88000000,0x8827FFFF)
[1m[33m[40m      FS0:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40mF0:
          Fv(87940482-FC81-41C3-87E6-399CF85AC8A0)
[1m[33m[40m     BLK4:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40m
          VenHw(E7223039-5836-41E1-B542-D7EC736C5E59)
[1m[33m[40m     BLK0:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40m
          VenHw(02118005-9DA7-443A-92D5-781F022AEDBB)
[1m[33m[40m     BLK2:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40m
          VenHw(1F15DA3C-37FF-4070-B471-BB4AF12A724A)
[1m[33m[40m     BLK3:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40m
          VenHw(CC2CBF29-1498-4CDD-8171-F8B6B41D0909)
[1m[33m[40m     BLK1:[0m[37m[40m [1m[37m[40mAlias(s):[0m[37m[40m
          VenHw(09831032-6FA3-4484-AF4F-0A000A8D3A82)
Press [1m[37m[40mESC[0m[37m[40m in 5 seconds to skip [1m[33m[40mstartup.nsh[0m[37m[40m or any other key to continue.[72DPress [1m[37m[40mESC[0m[37m[40m in 4 seconds to skip [1m[33m[40mstartup.nsh[0m[37m[40m or any other key to continue.[72DPress [1m[37m[40mESC[0m[37m[40m in 3 seconds to skip [1m[33m[40mstartup.nsh[0m[37m[40m or any other key to continue.[72DPress [1m[37m[40mESC[0m[37m[40m in 2 seconds to skip [1m[33m[40mstartup.nsh[0m[37m[40m or any other key to continue.[72DPress [1m[37m[40mESC[0m[37m[40m in 1 seconds to skip [1m[33m[40mstartup.nsh[0m[37m[40m or any other key to continue.
[1m[33m[40mShell> [0m[37m[40m
`
