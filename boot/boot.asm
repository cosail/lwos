org 07c00h
jmp short LABEL_START
nop

%include "fat12.inc"
%include "load.inc"

; constant
LABEL_CONSTANT:
    BaseOfStack         equ 0x7c00  ; 堆栈栈顶值
	ReadFatOffset		equ 0d000h  ; 保存读取FAT段内偏移

; variable
LABEL_VARIABLE:
    MsgBooting  db 'booting'
    MsgBootingLen equ $ - MsgBooting
    MsgError    db 'Can not find loader.bin'
    MsgErrorLen equ $ - MsgError
    StrLoader   db 'LOADER  BIN'
    StrLoaderLen equ $ - StrLoader
	MsgReady	db 'Ready'
	MsgReadyLen equ $ - MsgReady
    
	fat_value dw 0
	bx_value  dw 0

LABEL_START:
    ; initialize segment register
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov sp, BaseOfStack
	
    mov ax, LoaderBase
    mov es, ax
	mov bx, LoaderOffset
	 
    ; clear the screen
    call clear

    ; display the "booting" string
    mov ax, MsgBooting
    mov cx, MsgBootingLen
    mov dh, 0
    call print
    
    ; reset the floppy
    xor ah, ah
    xor dl, dl
    int 13h
    
	;mov ax, 020eh
	;mov cx, 0102h
	;mov dx, 0
	;int 13h
	
    ; read the root entry sectors
    mov ax, RootDirOffset
    mov cl, RootDirSectors
    call read_sector
    
    ; find the boot.bin
	mov cx, [BPB_RootEntCnt] ; 循环次数为目录项的数目
.loop:
	; 比较文件名
	mov ah, StrLoaderLen
	mov si, StrLoader
	mov di, bx
.cmp:
	; 比较字符
	lodsb
	mov dl, [es:di]
	cmp al, dl
	jnz .next
	dec ah
	cmp ah, 0
	jz .FindLoader
	inc di
	jmp .cmp
.next:
	; 下一个目录项
	add bx, 32
	loop .loop
	
	; 查找结束，说明没有找到，打印 no loader 信息
	mov ax, MsgError
	mov cx, MsgErrorLen
	mov dh, 1
	call print
	jmp $
	
.FindLoader:
	;pop bx
	add bx, 26
	mov ax, [es:bx] ; 第一个FAT项
	mov [fat_value], ax ; 保存
	
	; 打印 ready
	mov ax, MsgReady
	mov cx, MsgReadyLen
	mov dh, 1
	call print
	
    ; read the FAT sectors on floppy
	;xchg bx, bx
	mov bx, ReadFatOffset
	mov ax, FAT1Offset
	mov cl, SectorsOfFAT
	call read_sector
    
	mov bx, LoaderOffset
	mov [bx_value], bx
.read:
    ; load boot.bin, 此时可用的通用寄存器（ax, [bx], cx, dx, si, di, bp）
	; 计算FAT值在FAT表中的索引
	mov ax, [fat_value]
	xor dx, dx
	mov bx, 2
	div bx
	mov cx, dx
	mov bx, 3
	mul bx
	add ax, ReadFatOffset
	cmp cx, 0
	jz .non_odd
	inc ax
.non_odd:
	mov si, ax
	mov al, [es:si]
	mov ah, [es:si+1]
	cmp cx, 0
	jnz .odd
	and ax, 0fffh
	jmp .load
.odd:
	shr ax, 4
.load:
	mov dx, ax
	mov ax, [fat_value]
	add ax, FileDataOffset
	mov cl, 1
	mov bx, [bx_value]
	call read_sector
	add bx, 512
	mov [bx_value], bx
	cmp dx, 0ff8h
	jge .execute
	mov [fat_value], dx
	jmp .read
	
.execute:
    ; execute boot.bin
    jmp LoaderBase:LoaderOffset

%include "lib16.inc"

times (510 - ($ - $$)) db 0
dw 0xaa55