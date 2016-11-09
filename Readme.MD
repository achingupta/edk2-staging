This branch will be used to develop support for MM Standalone mode on ARMv8-A
platforms in the AArch64 execution state.

The branch owner: Achin Gupta <achin.gupta@arm.com>

# Feature Introduction
The "Volume 4: Platform Initialization Specification, Management Mode Core
Interface" introduces the concept of MM Standalone Mode. Initialization of this
mode can be done during the SEC phase (Section 1.5.2). On ARMv8-A systems,
[ARM Trusted Firmware](https://github.com/ARM-software/arm-trusted-firmware) is
responsible for launching the normal world firmware e.g. UEFI. It can be seen as
running in the PI SEC phase. The crux of the work in this branch is to implement
and standardize support for MM Standalone mode in conjunction with ARM Trusted
Firmware.

# Aims
## Upstream support for MM Standalone mode on ARM to EDK2
The aim of this branch is to upstream support to EDK2 for:

1. Initialisation of the Standalone MM environment during the SEC phase in
   collaboration with ARM Trusted Firmware
2. Accessing MM services instantiated in the secure world from the normal world
   i.e. UEFI, OS in collaboration with ARM Trusted Firmware.

## Contribute standard protocols to PI Volume 4 specification
While adding support for MM Standalone mode, new protocols will be introduced
that could be standardized in an architecture agnostic manner. Hence, the intent
will be to propose addition of such protocols to the "Volume 4: Platform
Initialization Specification, Management Mode Core Interface".

Candidate content for a PI specification is bound by the UEFI confidentiality
agreement. In order to migrate any publicly visible protocols from this
edk2-staging branch to a UEFI forum (PIWG in this case), the following steps
will be taken:

1. Discussions around a protocol will take place on edk2-devel
2. Feedback from UEFI forum members in edk2-devel will be included until there
   is general agreement regarding the protocol specification
3. Further discussions around the protocol will then take place in the PIWG
   forum and no further changes will be made to the protocol in the staging
   branch. The expectation will be that the protocol is added to the next
   release of the PI specifications without any further changes
4. Protocol could be upstreamed to EDK2 and later updated in case any further
   changes are made to it before it appears in the next PI specification.

# Details
The Standalone MM environment is instantiated in Secure EL0 as a separate
firmware volume. It executes under control of ARM TF which is instantiated in
EL3. Both components execute in the AArch64 execution state. This branch will
build upon the StandaloneSmmPkg module originally contributed by Intel. ARM
partners are expected to collaborate for further development.

Code should be upstreamed to EDK2 only after it has been reviewed by the
relevant stakeholders and tested on either an ARM partner or an ARM development
platform. Testing should demonstrate that both synchronous and asynchronous MM
events can be handled in the MM Standalone mode.

This branch will be initially supported on the platform and toolchain specified
below:

* Platform: FVPv7.7 FVP_Base_AEMv8A-AEMv8A
* Toolchain: aarch64-linux-gnu-gcc (Linaro GCC 2014.11) 4.9.3 20141031 (prerelease)

# Related Modules
The following modules are related to Standalone MM:

* StandaloneSmmPkg - Standalone MM package. This package provides the MM Foundation, MM CPU driver and related libraries/modules required for instantiating a MM environment in the secure world
* MdePkg
* MdeModulePkg
* ArmPkg
* ArmPlatformPkg
