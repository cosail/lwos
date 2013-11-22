; display a string
; parameter:
;     ax: string
;     cx: length
;     dh: line
print:
	pusha
	push es
	mov bp, ax
	mov ax, ds
    mov es, ax
	mov ax, 1301h
	mov bx, 0007h
	mov dl, 0
	int 10h
	pop es
	popa
    ret
	
; clear the screen
; paramter:
;     none
clear:
	pusha
	mov ax, 0600h
    mov bx, 0700h
	mov cx, 0
	mov dx, 184fh
	int 10h
	popa
	ret	
    
; read a sector from floppy
; paramter:
;     ax: the sector be read, from 0
;     cl: count
; data save in ex:bx
; 软盘大小：磁道（80）* 磁头（2）* 扇区（18）

read_sector:
	; 分配局部空间，供局部变量使用
	pusha
	mov bp, sp
	sub sp, 2
	
	; 保存 bx 和读取的扇区数
	push bx
	mov byte [bp - 2], cl
	
	; 计算调用 int 13h 需要的磁道、磁头、起始扇区和扇区数
	mov bl, 18
	div bl ; al 中的商为磁道*磁头的积
		   ; ah 中的余数为起始扇区，因为 int 13h 调用从1开始，需要调整
	inc ah 
	mov cl, ah ; 起始扇区号
	mov dh, al
	and dh, 1 ; 磁头
	shr al, 1
	mov ch, al ; 磁道
.calculate_al:
	; 计算这个磁道可以读取的最大扇区数
	mov al, 18
	sub al, cl
	inc al ; 最大扇区数
	
    ; 在这个磁道上需要读取的实际扇区数
	cmp al, byte [bp - 2]
	jle .sub_al
	mov al, byte [bp - 2]
	mov byte [bp - 2], 0
	jmp .int
.sub_al:
	mov bl, byte [bp - 2]
	sub bl, al ; bl 中保存剩余的扇区数
	mov byte [bp - 2], bl
    
    ; 恢复 bx ,使得保存结果的位置正确
.int:
    pop bx
	mov ah, 2
	mov dl, [BS_DrvNum]
    int 13h
	jc .return ; 读取错误，退出
	cmp byte [bp - 2], 0 ; 是否读取完了
	jz .return ; 读取完毕，退出
	
    ; 移动 bx ,使得下次读取的数据与本次读取的数据连续
	shl ax, 9 ; ax = ax * 512
	add bx, ax
	push bx
    
    ; 调整下一次读取的磁头、磁道和起始扇区，如果还需要读取的话
	cmp dh, 0
	jnz .increase_ch
	inc dh
	jmp .clear_cl
.increase_ch:
	inc ch
	dec dh
.clear_cl:
	mov cl, 0
	jmp .calculate_al
.return:
	mov sp, bp
	popa
    ret

; 关闭软驱马达
KillMotor:
    push dx
    mov dx, 03f2h
    mov al, 0
    out dx, al
    pop dx
    ret
    