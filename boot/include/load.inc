LoaderBase 		equ 9000h
LoaderOffset	equ  100h
PhyLoaderBase	equ LoaderBase * 10h

KernelBase		equ 8000h
KernelOffset	equ    0h
PhyKernelBase	equ KernelBase * 10h

PhyKernelEntry	equ 30400h

BOOT_PARAM_ADDR equ 0x900
BOOT_PARAM_MAGIC equ 0xb007
KERNEL_VALID_SIZE equ 0x1d000 ; 0x8000~0x9d000, 116K

PageDirBase equ 100000h ; page dirctory table begin at 1M
PageTabBase equ 101000h ; page entry table begin at 1M + 4K
