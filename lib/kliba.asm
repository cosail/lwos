%include "sconst.inc"

; import global variable
extern disp_pos

[section .text]

global disp_str
global disp_color_str
global out_byte
global in_byte
global enable_irq
global disable_irq
global enable_int
global disable_int
global port_read
global port_write
global glitter

; void disp_str(char *string)
disp_str:
    push ebp
    mov ebp, esp
    
    push 0fh
	push dword [ebp + 8]
    call disp_color_str
    add esp, 8
    
    pop ebp
    ret
    
; void disp_color_str(char *string, int color)
disp_color_str:
	push ebp
	mov ebp, esp
	
	mov esi, [ebp + 8] ; string
	mov ah, [ebp + 12] ; color
	mov edi, [disp_pos]
.1:
	lodsb
	test al, al
	jz .2
	cmp al, 0ah
	jnz .3
	push eax
	mov eax, edi
	mov bl, 160
	div bl
	and eax, 0ffh
	inc eax
	mov bl, 160
	mul bl
	mov edi, eax
	pop eax
	jmp .1
.3:
	mov [gs:edi], ax
	add edi, 2
	cmp edi, 80 * 25 * 2
	jb .1
	xor edi, edi
	jmp .1
.2:
	mov [disp_pos], edi
	
	pop ebp
	ret
	
; void out_byte(u16 port, u8 value)
out_byte:
	mov edx, [esp + 4] ; port
	mov al, [esp + 8] ; value
	out dx, al
	nop
	nop
	ret
	
; u8 in_byte(u16 port)
in_byte:
	mov edx, [esp + 4] ; port
	xor eax, eax
	in al, dx
	nop
	nop
	ret

; void port_read(u16 port, void *buf, int n)
port_read:
	mov edx, [esp + 4]
	mov edi, [esp + 8]
	mov ecx, [esp + 12]
	shr ecx, 1
	cld
	rep insw
	ret
	
; void port_write(u16 port, void *buf, int n)
port_write:
	mov edx, [esp + 4]
	mov esi, [esp + 8]
	mov ecx, [esp + 12]
	shr ecx, 1
	cld
	rep outsw
	ret

; void disable_irq(int irq)
disable_irq:
	mov ecx, [esp + 4] ; irq
	pushf
	cli
	mov ah, 1
	rol ah, cl
	cmp cl, 8
	jae disable_8
disable_0:
	in al, INT_M_CTLMASK
	test al, ah
	jnz dis_already
	or al, ah
	out INT_M_CTLMASK, al
	popf
	mov eax, 1
	ret
disable_8:
	in al, INT_S_CTLMASK
	test al, ah
	jnz dis_already
	or al, ah
	out INT_S_CTLMASK, al
	popf
	mov eax, 1
	ret
dis_already:
	popf
	xor eax, eax
	ret
	
; void enable_irq(int irq)
enable_irq:
	mov ecx, [esp + 4] ; irq
	pushf
	cli
	mov ah, ~1
	rol ah, cl
	cmp cl, 8
	jae enable_8
enable_0:
	in al, INT_M_CTLMASK
	and al, ah
	out INT_M_CTLMASK, al
	popf
	ret
enable_8:
	in al, INT_S_CTLMASK
	and al, ah
	out INT_S_CTLMASK, al
	popf
	ret

enable_int:
	sti
	ret
	
disable_int:
	cli
	ret

; void glitter(int row, int col)
glitter:
	push eax
	push ebx
	push edx
	
	mov eax, [.current_char]
	inc eax
	cmp eax, .strlen
	je .1
	jmp .2
.1:
	xor eax, eax
.2:
	mov [.current_char], eax
	mov dl, byte [eax + .glitter_str]
	
	xor eax, eax
	mov al, [esp + 16]
	mov bl, .line_width
	mul bl
	mov bx, [esp + 20]
	add ax, bx
	shl ax, 1
	movzx eax, ax
	
	mov [gs:eax], dl
	
	inc eax
	mov byte [gs:eax], 4
	
	jmp .end
	
.current_char:	dd 0
.glitter_str:	db '-\|/'
				db '1234567890'
				db 'abcdefghijklmnopqrstuvwxyz'
				db 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
.strlen			equ $ - .glitter_str
.line_width		equ 80

.end:
	pop edx
	pop ebx
	pop eax
	ret
