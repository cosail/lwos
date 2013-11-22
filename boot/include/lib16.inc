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
; ���̴�С���ŵ���80��* ��ͷ��2��* ������18��

read_sector:
	; ����ֲ��ռ䣬���ֲ�����ʹ��
	pusha
	mov bp, sp
	sub sp, 2
	
	; ���� bx �Ͷ�ȡ��������
	push bx
	mov byte [bp - 2], cl
	
	; ������� int 13h ��Ҫ�Ĵŵ�����ͷ����ʼ������������
	mov bl, 18
	div bl ; al �е���Ϊ�ŵ�*��ͷ�Ļ�
		   ; ah �е�����Ϊ��ʼ��������Ϊ int 13h ���ô�1��ʼ����Ҫ����
	inc ah 
	mov cl, ah ; ��ʼ������
	mov dh, al
	and dh, 1 ; ��ͷ
	shr al, 1
	mov ch, al ; �ŵ�
.calculate_al:
	; ��������ŵ����Զ�ȡ�����������
	mov al, 18
	sub al, cl
	inc al ; ���������
	
    ; ������ŵ�����Ҫ��ȡ��ʵ��������
	cmp al, byte [bp - 2]
	jle .sub_al
	mov al, byte [bp - 2]
	mov byte [bp - 2], 0
	jmp .int
.sub_al:
	mov bl, byte [bp - 2]
	sub bl, al ; bl �б���ʣ���������
	mov byte [bp - 2], bl
    
    ; �ָ� bx ,ʹ�ñ�������λ����ȷ
.int:
    pop bx
	mov ah, 2
	mov dl, [BS_DrvNum]
    int 13h
	jc .return ; ��ȡ�����˳�
	cmp byte [bp - 2], 0 ; �Ƿ��ȡ����
	jz .return ; ��ȡ��ϣ��˳�
	
    ; �ƶ� bx ,ʹ���´ζ�ȡ�������뱾�ζ�ȡ����������
	shl ax, 9 ; ax = ax * 512
	add bx, ax
	push bx
    
    ; ������һ�ζ�ȡ�Ĵ�ͷ���ŵ�����ʼ�������������Ҫ��ȡ�Ļ�
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

; �ر���������
KillMotor:
    push dx
    mov dx, 03f2h
    mov al, 0
    out dx, al
    pop dx
    ret
    