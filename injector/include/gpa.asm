BITS 64

; https://dennisbabkin.com/blog/?t=how-to-implement-getprocaddress-in-shellcode
GetKernel32ModuleHandle:
    mov rax, gs:[60h]
    mov rax, [rax + 18h]
    mov rax, [rax + 20h]
    mov rax, [rax]
    mov rax, [rax]
    mov rax, [rax + 20h]
    ret


GetAddressOf_GetProcAddress:
    mov eax, [rcx + 3ch]
    add rax, rcx
    lea rax, [rax + 88h]

    mov edx, [rax]
    lea rax, [rcx + rdx]

    mov edx, [rax + 18h]
    mov r8d, [rax + 20h]
    lea r8, [rcx + r8]

    mov r10, 41636f7250746547h ; "GetProcA"
    mov r11, 0073736572646441h ; "Address\0"

.1:
    mov r9d, [r8]
    lea r9, [rcx + r9]

    ; Function name comparision
    cmp r10, [r9]
    jnz .2
    cmp r11, [r9 + 7]
    jnz .2

    ; Found GetProcAddress
    neg rdx
    mov r10d, [rax + 18h]
    lea rdx, [r10 + rdx]

    mov r10d, [rax + 24h]
    lea r10, [rcx + r10]
    movzx rdx, word [r10 + rdx * 2]

    mov r10d, [rax + 1ch]
    lea r10, [rcx + r10]

    mov r10d, [r10 + rdx * 4]

    lea rax, [rcx + r10] ; Function address
    jmp .end

.2:
    add r8, 4
    dec rdx
    jnz .1

.end:
    ret