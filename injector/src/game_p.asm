BITS 64

; Macro definitions

; read dst, pSrc, size
%macro read 3

    mov %1, [%2]
    add %2, %3

%endmacro

; copy pDst, pSrc, temp, tempSize
%macro copy 4

    mov %3, [%2]
    mov [%1], %3
    add %1, %4
    add %2, %4

%endmacro

; unprotect addr, size, fn
%macro unprotect 3

    mov rcx, %1
    mov rdx, %2
    mov r8, 40h  ; PAGE_EXECUTE_READWRITE
    lea r9, [rel oldProtect]

    call %3

%endmacro

; reprotect addr, size, fn
%macro reprotect 3

    mov rcx, %1
    mov rdx, %2
    lea r9, [rel oldProtect]
    mov r8d, [r9]

    call %3

%endmacro


main: ; Replacement entry point
    push rsi
    push rdi
    push r12
    push r13
    push r14


    call GetKernel32ModuleHandle
    mov rsi, rax   ; kernel32.dll

    mov rcx, rax
    call GetAddressOf_GetProcAddress
    mov rdi, rax   ; *GetProcAddress


    mov rcx, rsi          ; kernel32.dll
    lea rdx, [rel s_VirtualProtect]
    call rdi              ; rax = *VirtualProtect
    
    mov rcx, rax
    call RecoverExecutable


    mov rcx, rsi          ; kernel32.dll
    lea rdx, [rel s_LoadLibraryW]
    call rdi              ; rax = *LoadLibraryW

    lea rcx, [rel dllPath]
    call rax              ; LoadLibraryW(dllPath)    


    mov rcx, rax          ; dll from dllPath
    lea rdx, [rel s_get_entry_addr]
    call rdi              ; rax = *get_entry_addr

    call rax              ; rax = *entry


    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    
    jmp rax


RecoverExecutable:  ; expects *VirtualProtect in rcx
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 8

    mov r13, rcx

    ; Find the recovery data structure
    lea rbx, [rel dllPath]

.search:
    read ax, rbx, 2
    test ax, ax
    jnz .search

    ; Recover trampoline bytes (6 + 8 = 14 total)
    read r12, rbx, 8  ; Address
    mov r14, r12

    unprotect r14, 5, r13
    copy r12, rbx, rax, 8
    copy r12, rbx, eax, 4
    copy r12, rbx, ax, 2
    reprotect r14, 5, r13

    ; Recover entry point bytes (5 total)
    read r12, rbx, 8  ; Address
    mov r14, r12

    unprotect r14, 5, r13
    copy r12, rbx, eax, 4
    copy r12, rbx, al, 1
    reprotect r14, 5, r13

    ; Recover import descriptor bytes (20 total)
    read r12, rbx, 8
    mov r14, r12

    unprotect r14, 20, r13
    copy r12, rbx, rax, 8
    copy r12, rbx, rax, 8
    copy r12, rbx, eax, 4
    reprotect r14, 20, r13

    ; Recover import data directory entry size bytes (4 total)
    read r12, rbx, 8
    mov r14, r12

    unprotect r14, 4, r13
    copy r12, rbx, eax, 4
    reprotect r14, 4, r13

    add rsp, 8
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


%include "gpa.asm"

oldProtect: dd 0

; Strings
s_VirtualProtect:  db "VirtualProtect", 0
s_LoadLibraryW:    db "LoadLibraryW", 0
s_get_entry_addr:  db "get_entry_addr", 0 

dllPath:
    ; This will be filled out by the launcher payload dll
    ; Path to the dll to inject into the game
