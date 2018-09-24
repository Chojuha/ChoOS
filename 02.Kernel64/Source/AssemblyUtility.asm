[BITS 64]

SECTION .text

global InPortByte
global OutPortByte
global InPortWord
global OutPortWord
global LoadGDTR
global LoadTR
global LoadIDTR
global EnableInterrupt
global DisableInterrupt
global ReadRFLAGS
global ReadTSC
global SwitchContext
global Hlt
global TestAndSet
global InitFPU
global SaveFPUContext
global LoadFPUContext
global SetTS
global ClearTS
global EnableGlobalLocalAPIC
global Pause

InPortByte:
	push rdx
	mov rdx , rdi
	mov rax , 0
	in al , dx
	pop rdx
	ret
OutPortByte:
	push rdx
	push rax
	mov rdx , rdi
	mov rax , rsi
	out dx , al
	pop rax
	pop rdx
	ret
InPortWord:
	push rdx
	mov rdx , rdi
	mov rax , 0
	in ax , dx
	pop rdx
	ret
OutPortWord:
	push rdx
	push rax
	mov rdx , rdi
	mov rax , rsi
	out dx , ax
	pop rax
	pop rdx
	ret
LoadGDTR:
	lgdt[rdi]
	ret
LoadIDTR:
	lidt[rdi]
	ret
LoadTR:
	ltr di
	ret
EnableInterrupt:
	sti
	ret
DisableInterrupt:
	cli
	ret
ReadRFLAGS:
	pushfq
	pop rax
	ret
ReadTSC:
	push rdx
	rdtsc
	shl rdx , 32
	or rax , rdx
	pop rdx
	ret
%macro SAVECONTEXT 0
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov ax , ds
	push rax
	mov ax , es
	push rax
	push fs
	push gs
%endmacro

%macro LOADCONTEXT 0
	pop gs
	pop fs
	pop rax
	mov es , ax
	pop rax
	mov ds , ax
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

SwitchContext:
	push rbp
	mov rbp , rsp
	pushfq
	cmp rdi , 0
	je .LoadContext
	popfq
	push rax
	mov ax , ss
	mov qword[rdi+(23*8)] , rax
	mov rax , rbp
	add rax , 16
	mov qword[rdi+(22*8)] , rax
	pushfq
	pop rax
	mov qword[rdi+(21*8)] , rax
	mov ax , cs
	mov qword[rdi+(20*8)] , rax
	mov rax , qword[rbp+8]
	mov qword[rdi+(19*8)] , rax
	pop rax
	pop rbp
	add rdi , (19*8)
	mov rsp , rdi
	sub rdi , (19*8)
	SAVECONTEXT
.LoadContext:
	mov rsp , rsi
	LOADCONTEXT
	iretq
Hlt:
	hlt
	hlt
	ret
TestAndSet:
	mov rax , rsi
	lock cmpxchg byte[rdi] , dl
	je .SUCCESS
.NOTSAME:
	mov rax , 0x00
	ret
.SUCCESS:
	mov rax , 0x01
	ret
InitFPU:
	finit
	ret
SaveFPUContext:
	fxsave [rdi]
	ret
LoadFPUContext:
	fxrstor [rdi]
SetTS:
	push rax
	mov rax , cr0
	or rax , 0x08
	mov cr0 , rax
	pop rax
	ret
ClearTS:
	clts
	ret
EnableGlobalLocalAPIC:
	push rax
	push rcx
	push rdx
	mov rcx , 27
	rdmsr
	or rax , 0x0800
	wrmsr
	pop rdx
	pop rcx
	pop rax
	ret
Pause:
	pause
	ret
