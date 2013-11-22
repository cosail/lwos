; FAT12
BS_OEMName      db '12345678'
BPB_BytePerSec  dw 512
BPB_SecPerClus  db 1
BPB_RsvdSecCnt  dw 1
BPB_NumFATs     db 2
BPB_RootEntCnt  dw 224
BPB_TotSec16    dw 2880
BPB_Media       db 0xf0
BPB_SecPerFAT   dw 9
BPB_SecPerTrk   dw 18
BPB_NumHeads    dw 2
BPB_HiddenSec   dd 0
BPB_TotSec32    dd 0

BS_DrvNum       db 0
BS_Reserved1    db 0
BS_BootSig      db 0x29
BS_VolId        dd 0
BS_VolLabel     db '01234567890'
BS_FileSysType  db 'FAT12   '

; constant
; FAT1 的第一个扇区号 = BPB_RsvdSecCnt
FAT1Offset      equ	1

; 每个FAT占用的扇区数
SectorsOfFAT    equ	9

; 根目录的第一个扇区号 = BPB_RsvdSecCnt + (BPB_NumFATs * SectorsOfFAT)
RootDirOffset   equ 19

; 根目录占用扇区数
; RootDirSectors = ((BPB_RootEntCnt*32)+(BPB_BytsPerSec–1))/BPB_BytsPerSec
RootDirSectors  equ 14

; 文件数据起始扇区号 = BPB_RsvdSecCnt + (BPB_NumFATs * SectorsOfFAT) + RootDirSectors- 2
; 文件的开始Sector号 = DirEntry中的开始Sector号 + FileDataOffset
FileDataOffset  equ 31
