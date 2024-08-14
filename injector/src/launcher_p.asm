BITS 64

main: ; Replacement entry point
    push rsi


    call GetKernel32ModuleHandle
    mov rsi, rax          ; kernel32.dll

    mov rcx, rax
    call GetAddressOf_GetProcAddress


    mov rcx, rsi          ; kernel32.dll
    lea rdx, [rel s_LoadLibraryW]
    call rax              ; rax = *LoadLibraryW

    lea rcx, [rel dllPath]
    call rax              ; LoadLibraryA(dllPath)


    pop rsi
    ret


%include "gpa.asm"


; Strings
s_LoadLibraryW:  db "LoadLibraryW", 0

dllPath:
    ; This will be filled out by the injector
    ; Path to the dll to inject into the launcher
