[BITS 64]

SECTION .text

extern CommonExceptionHandler
extern CommonInterruptHandler
extern KeyboardHandler
extern TimerHandler
extern DeviceNotAvailableHandler
extern HDDHandler
extern MouseHandler
global ISRDivideError, ISRDebug , ISRNMI, ISRBreakPoint , ISROverflow
global ISRBoundRangeExceeded , ISRInvalidOpcode , ISRDeviceNotAvailable , ISRDoubleFault , 
global ISRCoprocessorSegmentOverrun , ISRInvalidTSS , ISRSegmentNotPresent
global ISRStackSegmentFault , ISRGeneralProtection , ISRPageFault , ISR15
global ISRFPUError, ISRAlignmentCheck , ISRMachineCheck , ISRSIMDError , ISRETCException
global ISRTimer , ISRKeyboard , ISRSlavePIC , ISRSerial2 , ISRSerial1 , ISRParallel2
global ISRFloppy , ISRParallel1 , ISRRTC , ISRReserved , ISRNotUsed1 , ISRNotUsed2
global ISRMouse , ISRCoprocessor , ISRHDD1 , ISRHDD2 , ISRETCInterrupt

%macro SAVECONTEXT 0
    push rbp
    mov rbp , rsp
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
    mov ax , 0x10
    mov ds , ax
    mov es , ax
    mov gs , ax
    mov fs , ax
%endmacro

%macro LOADCONTEXT 0
    pop gs
    pop fs
    pop rax
    mov es , ax
    pop rax
    mov ds, ax
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

ISRDivideError:
    SAVECONTEXT
    mov rdi , 0
    call CommonExceptionHandler
    LOADCONTEXT
    iretq
    
ISRDebug:
    SAVECONTEXT
    mov rdi , 1
    call CommonExceptionHandler
    LOADCONTEXT
    iretq
ISRNMI:
    SAVECONTEXT
    mov rdi , 2
    call CommonExceptionHandler

    LOADCONTEXT
    iretq
    
ISRBreakPoint:
    SAVECONTEXT
    mov rdi , 3
    call CommonExceptionHandler
    LOADCONTEXT
    iretq
    
ISROverflow:
    SAVECONTEXT
    mov rdi , 4
    call CommonExceptionHandler
    LOADCONTEXT
    iretq
    
ISRBoundRangeExceeded:
    SAVECONTEXT 
    mov rdi , 5
    call CommonExceptionHandler
    LOADCONTEXT
    iretq
    
ISRInvalidOpcode:
    SAVECONTEXT 
    mov rdi , 6
    call CommonExceptionHandler
    LOADCONTEXT
    iretq
    
ISRDeviceNotAvailable:
    SAVECONTEXT 
    mov rdi , 7
    call DeviceNotAvailableHandler
    LOADCONTEXT
    iretq 
ISRDoubleFault:
    SAVECONTEXT
    mov rdi , 8
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq 
    
ISRCoprocessorSegmentOverrun:
    SAVECONTEXT 
	mov rdi , 9
    call CommonExceptionHandler
    LOADCONTEXT 
    iretq 

ISRInvalidTSS:
    SAVECONTEXT
    mov rdi , 10
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq 
    
ISRSegmentNotPresent:
    SAVECONTEXT
    mov rdi , 11
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq 

ISRStackSegmentFault:
    SAVECONTEXT 
	mov rdi , 12
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq
    
ISRGeneralProtection:
    SAVECONTEXT
	mov rdi , 13
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq 
    
ISRPageFault:
    SAVECONTEXT
    mov rdi , 14
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq
ISR15:
    SAVECONTEXT
	mov rdi , 15
    call CommonExceptionHandler
    LOADCONTEXT 
    iretq 

ISRFPUError:
    SAVECONTEXT 
	mov rdi , 16
    call CommonExceptionHandler
    LOADCONTEXT 
    iretq 

ISRAlignmentCheck:
    SAVECONTEXT 
	mov rdi , 17
    mov rsi , qword[rbp+8]
    call CommonExceptionHandler
    LOADCONTEXT 
    add rsp , 8 
    iretq 

ISRMachineCheck:
    SAVECONTEXT
    mov rdi , 18
    call CommonExceptionHandler
    LOADCONTEXT 
    iretq 

ISRSIMDError:
    SAVECONTEXT 
	mov rdi , 19
    call CommonExceptionHandler
    LOADCONTEXT 
    iretq 

ISRETCException:
    SAVECONTEXT
    mov rdi , 20
    call CommonExceptionHandler
    LOADCONTEXT 
    iretq 

ISRTimer:
    SAVECONTEXT
    mov rdi , 32
    call TimerHandler
    LOADCONTEXT 
    iretq 

ISRKeyboard:
    SAVECONTEXT
    mov rdi , 33
    call KeyboardHandler
    LOADCONTEXT 
    iretq 

ISRSlavePIC:
    SAVECONTEXT 
    mov rdi , 34
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq
    
ISRSerial2:
    SAVECONTEXT
    mov rdi , 35
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 

ISRSerial1:
    SAVECONTEXT 
    mov rdi , 36
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 

ISRParallel2:
    SAVECONTEXT 
    mov rdi , 37
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 
ISRFloppy:
    SAVECONTEXT 
    mov rdi , 38
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 

ISRParallel1:
    SAVECONTEXT
    mov rdi , 39
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 

ISRRTC:
    SAVECONTEXT
    mov rdi , 40
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 

ISRReserved:
    SAVECONTEXT
    mov rdi , 41
    call CommonInterruptHandler
    LOADCONTEXT 
    iretq 

ISRNotUsed1:
    SAVECONTEXT 
    mov rdi , 42
    call CommonInterruptHandler
    LOADCONTEXT
    iretq
    
ISRNotUsed2:
    SAVECONTEXT 
    mov rdi , 43
    call CommonInterruptHandler
    LOADCONTEXT
    iretq
    
ISRMouse:
    SAVECONTEXT
    mov rdi , 44
    call MouseHandler
    LOADCONTEXT
    iretq
ISRCoprocessor:
    SAVECONTEXT
    mov rdi , 45
    call CommonInterruptHandler
    LOADCONTEXT
    iretq
ISRHDD1:
    SAVECONTEXT
    mov rdi , 46
    call HDDHandler
    LOADCONTEXT
    iretq
    
ISRHDD2:
    SAVECONTEXT 
    mov rdi , 47
    call HDDHandler
    LOADCONTEXT
    iretq 
    
ISRETCInterrupt:
    SAVECONTEXT
    mov rdi , 48
    call CommonInterruptHandler
    SAVECONTEXT
    iretq
