This package is standalone SMM POC which follows PI1.5 draft.

================================================================================
                      FEATURES
================================================================================
This package is validated on MinnowMax, and boot to UEFI Yacto.
A. New FFS type (EFI_FV_FILETYPE_SMM_STANDALONE) and module types (SMM_STANDALONE,
   SMM_CORE_STANDALONE) are suppported in BaseTool package.
B. New libraries to support standalone SMM module and SMM core are in StandaloneSmmPkg.
C. Standalone SmmCore and PeiIpl/DxeIplGateway are supported in StandaloneSmmPkg.
D. X86 Standalone SmmCpu is supported in StandaloneSmmPkg/X64 dir.

================================================================================
                                 HOW TO BUILD
================================================================================
A. Build Tool
   1. Copy StandaloneSmmPkg\BaseTool to override BaseTool package.
   2. cd BaseTools
      set EDK_TOOLS_PATH=%cd%
      set BASE_TOOLS_PATH=%cd%
      set PYTHON_HOME=C:\Python27
      set PYTHON_FREEZER_PATH=C:\Python27\Scripts
      nmake

B. Build MinnowMax image with SMM_STANDALONE mode.
   1. Add below content in Vlv2TbltDevicePkg\PlatformPkgX64.dsc, as new section.
      
      [LibraryClasses.X64.SMM_CORE_STANDALONE]
        SmmCoreStandaloneEntryPoint|StandaloneSmmPkg/Library/StandaloneSmmCoreEntryPoint/StandaloneSmmCoreEntryPoint.inf
        SmmServicesTableLib|StandaloneSmmPkg/Library/StandaloneSmmCoreSmmServicesTableLib/StandaloneSmmCoreSmmServicesTableLib.inf
        SmmMemLib|StandaloneSmmPkg/Library/StandaloneSmmMemLib/StandaloneSmmMemLib.inf
        MemoryAllocationLib|StandaloneSmmPkg/Library/StandaloneSmmCoreMemoryAllocationLib/StandaloneSmmCoreMemoryAllocationLib.inf
        HobLib|StandaloneSmmPkg/Library/StandaloneSmmCoreHobLib/StandaloneSmmCoreHobLib.inf
        #DebugLib|StandaloneSmmPkg/Override/MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
        ReportStatusCodeLib|StandaloneSmmPkg/Override/MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
        FvLib|StandaloneSmmPkg/Library/FvLib/FvLib.inf
        PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  
      [LibraryClasses.X64.SMM_STANDALONE]
        SmmStandaloneDriverEntryPoint|StandaloneSmmPkg/Library/StandaloneSmmDriverEntryPoint/StandaloneSmmDriverEntryPoint.inf
        SmmServicesTableLib|StandaloneSmmPkg/Library/StandaloneSmmServicesTableLib/StandaloneSmmServicesTableLib.inf
        SmmMemLib|StandaloneSmmPkg/Library/StandaloneSmmMemLib/StandaloneSmmMemLib.inf
        MemoryAllocationLib|StandaloneSmmPkg/Library/StandaloneSmmMemoryAllocationLib/StandaloneSmmMemoryAllocationLib.inf
        HobLib|StandaloneSmmPkg/Library/StandaloneSmmHobLib/StandaloneSmmHobLib.inf
        SmmCpuPlatformHookLib|UefiCpuPkg/Library/SmmCpuPlatformHookLibNull/SmmCpuPlatformHookLibNull.inf
        CpuExceptionHandlerLib|StandaloneSmmPkg/Override/UefiCpuPkg/Library/CpuExceptionHandlerLib/SmmCpuExceptionHandlerLib.inf
        SmmCpuFeaturesLib|StandaloneSmmPkg/Override/UefiCpuPkg/Library/SmmCpuFeaturesLib/StandaloneSmmCpuFeaturesLib.inf
        #DebugLib|StandaloneSmmPkg/Override/MdeModulePkg/Library/PeiDxeDebugLibReportStatusCode/PeiDxeDebugLibReportStatusCode.inf
        ReportStatusCodeLib|StandaloneSmmPkg/Override/MdeModulePkg/Library/SmmReportStatusCodeLib/SmmReportStatusCodeLib.inf
        PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf

   2. Add below content in Vlv2TbltDevicePkg\PlatformPkgX64.dsc, [Components.IA32] section.

        UefiCpuPkg/CpuMpPei/CpuMpPei.inf
        StandaloneSmmPkg/X86/CpuMpInfoPei/CpuMpInfoPei.inf
        StandaloneSmmPkg/StandaloneSmmIplPei/StandaloneSmmIplPei.inf

   3. Add below content in Vlv2TbltDevicePkg\PlatformPkgX64.dsc, [Components.X64] section.
        
        StandaloneSmmPkg/StandaloneSmmIplDxeGateway/StandaloneSmmIplDxeGateway.inf {
          <PcdsPatchableInModule>
            gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
          <PcdsFixedAtBuild>
            gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
          <LibraryClasses>
      !if $(TARGET) != RELEASE
            DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      !endif
        }
        StandaloneSmmPkg/StandaloneSmmCore/StandaloneSmmCore.inf {
          <PcdsPatchableInModule>
            gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
          <PcdsFixedAtBuild>
            gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
          <LibraryClasses>
      !if $(TARGET) != RELEASE
            DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      !endif
        }
        StandaloneSmmPkg/X86/PiSmmCpuStandaloneSmm/PiSmmCpuStandaloneSmm.inf {
          <PcdsPatchableInModule>
            gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000046
          <PcdsFixedAtBuild>
            gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
          <LibraryClasses>
      !if $(TARGET) != RELEASE
            DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
      !endif
        }
        
   4. Add below content in Vlv2TbltDevicePkg\PlatformPkg.fdf, [FV.FVRECOVERY] section

        INF  UefiCpuPkg/CpuMpPei/CpuMpPei.inf
        INF  StandaloneSmmPkg/X86/CpuMpInfoPei/CpuMpInfoPei.inf
        INF  StandaloneSmmPkg/StandaloneSmmIplPei/StandaloneSmmIplPei.inf
        INF  StandaloneSmmPkg/StandaloneSmmCore/StandaloneSmmCore.inf
        INF  StandaloneSmmPkg/X86/PiSmmCpuStandaloneSmm/PiSmmCpuStandaloneSmm.inf
        
   5. Add below content in Vlv2TbltDevicePkg\PlatformPkg.fdf, [FV.FVMAIN] section
   
        INF StandaloneSmmPkg/StandaloneSmmIplDxeGateway/StandaloneSmmIplDxeGateway.inf

   6. Remove below content in Vlv2TbltDevicePkg\PlatformPkg.fdf, [FV.FVMAIN] section
   
        INF MdeModulePkg/Core/PiSmmCore/PiSmmIpl.inf
        INF MdeModulePkg/Core/PiSmmCore/PiSmmCore.inf
        INF RuleOverride = BINARY $(PLATFORM_BINARY_PACKAGE)/$(DXE_ARCHITECTURE)$(TARGET)/$(DXE_ARCHITECTURE)/PiSmmCpuDxeSmm.inf

   7. Add below content in Vlv2TbltDevicePkg\PlatformPkg.fdf, as new section.
   
        [Rule.Common.SMM_CORE_STANDALONE]
          FILE SMM_CORE = $(NAMED_GUID) {
            DXE_DEPEX DXE_DEPEX Optional       $(INF_OUTPUT)/$(MODULE_NAME).depex
            PE32      PE32                     $(INF_OUTPUT)/$(MODULE_NAME).efi
            UI        STRING="$(MODULE_NAME)" Optional
            VERSION   STRING="$(INF_VERSION)" Optional BUILD_NUM=$(BUILD_NUMBER)
          }

        [Rule.Common.SMM_STANDALONE]
          FILE SMM_STANDALONE = $(NAMED_GUID) {
            SMM_DEPEX SMM_DEPEX Optional       $(INF_OUTPUT)/$(MODULE_NAME).depex
            PE32      PE32                     $(INF_OUTPUT)/$(MODULE_NAME).efi
            UI        STRING="$(MODULE_NAME)" Optional
            VERSION   STRING="$(INF_VERSION)" Optional BUILD_NUM=$(BUILD_NUMBER)
          }

   8. Build MinnowMax BIOS accroding to MinnowMax release notes.


