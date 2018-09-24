[BITS 64]

SECTION .text

extern Main
extern APICIDAddress
extern WakeUpApplicationProcessorCount

START:
	mov ax , 0x10
	mov ds , ax
	mov es , ax
	mov fs , ax
	mov gs , ax
	mov ss , ax
	mov rsp , 0x6FFFF8
	mov rbp , 0x6FFFF8
	cmp byte[0x7C09] , 0x01
	je .BOOTSTRAPPROCESSORSTARTPOINT
	mov rax , 0
	mov rbx , qword[APICIDAddress]
	mov eax , dword[rbx]
	shr rax , 24
	mov rbx , 0x10000
	mul rbx
	sub rsp , rax
	sub rbp , rax
	lock inc dword[WakeUpApplicationProcessorCount]
.BOOTSTRAPPROCESSORSTARTPOINT:
	call Main
	jmp $
