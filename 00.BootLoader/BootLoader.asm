[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START

TOTALSECTORCOUNT: dw 0x02
KERNEL32SECTORCOUNT: dw 0x02
BOOTSTRAPPROCESSOR: db 0x01
STARTGRAPHICMODE: db 0x00

START:
	mov ax , 0x07C0
	mov ds , ax
	mov ax , 0xB800
	mov es , ax

	mov ax , 0x0000
	mov ss , ax
	mov sp , 0xFFFE
	mov bp , 0xFFFE
	mov si , 0
.CLRSCR:
	mov byte[es:si] , 0
	mov byte[es:si+1] , 0x07
	add si , 2
	cmp si , 80*25*2
	jl .CLRSCR
	mov si , 0x00
	mov di , 0x00
	push MESSAGE1
	push 0
	push 0
	call PRINTMESSAGE
	add sp , 6
	RESETDISK:
		mov ax , 0
		mov dl , 0
		int 0x13
		jc DISKERROR
		mov si , 0x1000
		mov es , si
		mov bx , 0x0000
		mov di , WORD[TOTALSECTORCOUNT]
	READDATA:
		cmp di , 0
		je READEND
		sub di , 0x1
		mov ah , 0x02
		mov al , 0x1
		mov ch , byte[TRACKNUMBER]
		mov cl , byte[SECTORNUMBER]
		mov dh , byte[HEADNUMBER]
		mov dl , 0x00
		int 0x13
		jc DISKERROR
		add si , 0x0020
		mov es , si
		mov al , byte[SECTORNUMBER]
		add al , 0x01
		mov byte[SECTORNUMBER] , al
		cmp al , 19
		jl READDATA
		xor byte[HEADNUMBER] , 0x01
		mov byte[SECTORNUMBER] , 0x01
		cmp byte[HEADNUMBER] , 0x00
		jne READDATA
		add byte[TRACKNUMBER] , 0x01
		jmp READDATA
	READEND:
		mov ax , 0x4F01
		mov cx , 0x117
		mov bx , 0x07E0
		mov es , bx
		mov di , 0x00
		int 0x10
		cmp ax , 0x004F
		jne VBEERROR
		cmp byte[STARTGRAPHICMODE] , 0x00
		je JMPPROTECT
		mov ax , 0x4F02
		mov bx , 0x4117
		int 0x10
		cmp ax , 0x004F
		jne VBEERROR
		jmp JMPPROTECT
	VBEERROR:
		push GRAPHICSFAIL
		push 2
		push 0
		call PRINTMESSAGE
		add sp , 6
		jmp $
	JMPPROTECT:
		jmp 0x1000:0x0000
	DISKERROR:
		push OSIMAGELOADINGFAIL
		push 2
		push 0
		call PRINTMESSAGE
		add sp , 6
		jmp $
PRINTMESSAGE:
	push bp
	mov bp , sp
	push es
	push si
	push di
	push ax 
	push cx
	push dx
	mov ax , 0xB800
	mov es , ax
	mov ax , word[bp+6]
	mov si , 160
	mul si
	mov di , ax
	mov ax , word[bp+4]
	mov si , 2
	mul si
	add di , ax
	mov si , word[bp+8]
	.MSGLOOP:
		mov cl , byte[si]
		cmp cl , 0
		je .MSGEND
		mov byte[es:di] , cl
		add si , 1
		add di , 2
		jmp .MSGLOOP
	.MSGEND:
		pop dx
		pop cx
		pop ax
		pop di
		pop si
		pop es
		pop bp
		ret
MESSAGE1: db '<ChoOS Version 1.0>' , 0
OSIMAGELOADINGFAIL: db 'ChoOS Image Loading Failed.(CHOOS_DISKERROR)' , 0
GRAPHICSFAIL: db 'ChoOS Graphics Loading Failed.(CHOOS_GRAPHICLOAD)' , 0
READSUCCESS: db 'ChoOS Image Loading Success.' , 0
SECTORNUMBER: db 0x02
HEADNUMBER: db 0x00
TRACKNUMBER: db 0x00
BOOTDRIVE: db 0x00
LASTSECTOR: db 0x00
LASTHEAD: db 0x00
LASTTRACK: db 0x00
times 510-($-$$) db 0x00
db 0x55
db 0xAA
