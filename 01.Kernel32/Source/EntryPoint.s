[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax , 0x1000
	mov ds , ax
	mov es , ax
	mov ax , 0x0000
	mov es , ax
	cmp byte[es:0x7C09] , 0x00
	je .APPLICATIONPROCESSORSTARTPOINT
	mov ax , 0x2401
	int 0x15
	jc .A20GATEERROR
	jmp .A20GATESUCCESS
.A20GATEERROR:
	in al , 0x92
	or al , 0x02
	and al , 0xFE
	out 0x92 , al
.A20GATESUCCESS:
.APPLICATIONPROCESSORSTARTPOINT:
	cli
	lgdt[GDTR]
	mov eax , 0x4000003B
	mov cr0 , eax
	jmp dword 0x18: (PROTECTEDMODE - $$ + 0x10000)
	
[BITS 32]
PROTECTEDMODE:
	mov ax , 0x20
	mov ds , ax
	mov es , ax
	mov fs , ax
	mov gs , ax
	mov ss , ax
	mov esp , 0xFFFE
	mov ebp , 0xFFFE
	cmp byte[0x7C09] , 0x00
	je .APPLICATIONPROCESSORSTARTPOINT
.APPLICATIONPROCESSORSTARTPOINT:
	jmp dword 0x18: 0x10200
PRINTMESSAGE:
	push ebp
	mov ebp , esp
	push esi
	push edi
	push eax
	push ecx
	push edx
	mov eax , dword[ebp+12]
	mov esi , 160
	mul esi
	mov edi , eax
	mov eax , dword[ebp+8]
	mov esi , 2
	mul esi
	add edi , eax
	mov esi , dword[ebp+16]
	.MSGLOOP:
		mov cl , byte[esi]
		cmp cl , 0
		je .MSGEND
		mov byte[edi+0xB8000] , cl
		add esi , 1
		add edi , 2
		jmp .MSGLOOP
	.MSGEND:
		pop edx
		pop ecx
		pop eax
		pop edi
		pop esi
		pop ebp
		ret	
align 8 , db 0x00
dw 0x0000
GDTR:
	dw GDTEND-GDT-1
	dd (GDT-$$+0x10000)
GDT:
	NULLDescriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
	IA_32eCodeDescriptor:
		dw 0xFFFF
		dw 0x0000
		db 0x00
		db 0x9A
		db 0xAF
		db 0x00
	IA_32eDataDescriptor:
		dw 0xFFFF
		dw 0x0000
		db 0x00
		db 0x92
		db 0xAF
		db 0x00
	CodeDescriptor:
		dw 0xFFFF
		dw 0x0000
		db 0x00
		db 0x9A
		db 0xCF
		db 0x00
	DataDescriptor:
		dw 0xFFFF
		dw 0x0000
		db 0x00
		db 0x92
		db 0xCF
		db 0x00
GDTEND:

times 512-($-$$) db 0x00
