This branch will be used to develop support for ACPI Platform Error Interfaces
(APEI) on ARMv8-A standard platforms.

The branch owners: Achin Gupta <achin.gupta@arm.com>, James Morse <james.morse@arm.com>

# Feature Introduction
The ACPI specification describes APEI which provide a mechanism for the firmware
to convey error information to OSPM. This branch will be used to implement
support for these interfaces in EDK2 and test them on ARM platforms.

# Aims
Upstream support for APEI to EDK2

# Related Modules
* MdeModulePkg
* ArmPkg
