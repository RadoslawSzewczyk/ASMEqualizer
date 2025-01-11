.DATA
M_PI REAL8 3.14159265358979323846
lowCutoff REAL4 200.0
highCutoff REAL4 3000.0
sampleRate REAL4 44100.0

.CODE
MyProc1 PROC
    ; Prologue
    push rbp
    mov rbp, rsp
    sub rsp, 512                      ; Reserve more space on the stack

    ; Save non-volatile registers
    push rbx
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15

    ; Load parameters
    mov r12, rcx                     ; r12 = buffer
    mov r13, rdx                     ; r13 = length
    mov r14d, r8d                    ; r14 = lowGain
    mov r15d, r9d                    ; r15 = midGain
    mov r10d, r10d                   ; r10 = highGain

    ; Constants for filter coefficients
    vmovss xmm0, DWORD PTR [lowCutoff]    ; xmm0 = lowCutoff
    vmulss xmm0, xmm0, DWORD PTR [M_PI]  ; xmm0 = 2 * pi * lowCutoff
    vdivss xmm0, xmm0, DWORD PTR [sampleRate] ; xmm0 = lowAlpha
    vmovss DWORD PTR [rbp-16], xmm0      ; Store lowAlpha on stack

    vmovss xmm1, DWORD PTR [highCutoff]  ; xmm1 = highCutoff
    vmulss xmm1, xmm1, DWORD PTR [M_PI]  ; xmm1 = 2 * pi * highCutoff
    vdivss xmm1, xmm1, DWORD PTR [sampleRate] ; xmm1 = highAlpha
    vmovss DWORD PTR [rbp-20], xmm1      ; Store highAlpha on stack

    ; Initialize filter states
    xorps xmm2, xmm2                    ; lowYPrev = 0.0
    xorps xmm3, xmm3                    ; lowXPrev = 0.0
    xorps xmm4, xmm4                    ; highYPrev = 0.0
    xorps xmm5, xmm5                    ; highXPrev = 0.0

    ; Main loop
    xor rdi, rdi                       ; rdi = loop counter
LoopStart:
    cmp rdi, r13                       ; Compare counter with length
    jge LoopEnd                        ; Exit if counter >= length

    ; Load current sample
    mov eax, DWORD PTR [r12 + rdi*4]   ; eax = buffer[rdi]
    cvtsi2ss xmm6, eax                 ; xmm6 = (float)buffer[rdi]

    ; Low-pass filter
    vmovss xmm7, DWORD PTR [rbp-16]    ; xmm7 = lowAlpha
    vmulss xmm7, xmm7, xmm6            ; xmm7 = lowAlpha * buffer[rdi]
    vsubss xmm8, xmm2, xmm7            ; xmm8 = (1 - lowAlpha) * lowYPrev
    vaddss xmm2, xmm7, xmm8            ; lowYPrev = lowAlpha * input + (1 - lowAlpha) * lowYPrev
    cvtss2si eax, xmm2                 ; Convert lowYPrev to int

    ; Calculate offset for lowPassBuffer
    lea rbx, [rdi + rdi*2]             ; rbx = rdi * 3
    shl rbx, 2                         ; rbx = rdi * 12

    ; Store in lowPassBuffer
    mov DWORD PTR [rsp + rbx], eax     ; Store in lowPassBuffer

    ; High-pass filter
    vmovss xmm7, DWORD PTR [rbp-20]    ; xmm7 = highAlpha
    vmulss xmm7, xmm7, xmm6            ; xmm7 = highAlpha * buffer[rdi]
    vsubss xmm8, xmm4, xmm7            ; xmm8 = highAlpha * (highYPrev + input - highXPrev)
    vaddss xmm4, xmm7, xmm8            ; highYPrev = highAlpha * input + highAlpha * (highYPrev + input - highXPrev)
    cvtss2si eax, xmm4                 ; Convert highYPrev to int

    ; Calculate offset for highPassBuffer
    lea rbx, [rdi + rdi*2]             ; rbx = rdi * 3
    shl rbx, 2                         ; rbx = rdi * 12

    ; Ensure the offset is within the stack bounds
    cmp rbx, 508                       ; Compare with stack space (512 - 4)
    jge LoopEnd                        ; Exit if offset exceeds stack space

    ; Store in highPassBuffer
    mov DWORD PTR [rsp + rbx + 4], eax ; Store in highPassBuffer

    ; Band-pass filter
    mov eax, DWORD PTR [r12 + rdi*4]   ; Load original sample
    sub eax, DWORD PTR [rsp + rbx]     ; Subtract low-pass sample
    sub eax, DWORD PTR [rsp + rbx + 4] ; Subtract high-pass sample

    ; Store in bandPassBuffer
    mov DWORD PTR [rsp + rbx + 8], eax ; Store in bandPassBuffer

    ; Apply gains
    mov eax, DWORD PTR [rsp + rbx]     ; Load low-pass sample
    imul eax, r14d                     ; Multiply by lowGain

    mov ebx, DWORD PTR [rsp + rbx + 8] ; Load band-pass sample
    imul ebx, r15d                     ; Multiply by midGain
    add eax, ebx                       ; Add to low-pass result

    mov ebx, DWORD PTR [rsp + rbx + 4] ; Load high-pass sample
    imul ebx, r10d                     ; Multiply by highGain
    add eax, ebx                       ; Add to combined result

    ; Clip to 32-bit integer range
    cmp eax, 7FFFFFFFh                 ; Compare with INT_MAX
    jle NoClipHigh                    ; If <= INT_MAX, skip clipping
    mov eax, 7FFFFFFFh                 ; Set to INT_MAX
NoClipHigh:
    cmp eax, 80000000h                 ; Compare with INT_MIN
    jge NoClipLow                     ; If >= INT_MIN, skip clipping
    mov eax, 80000000h                 ; Set to INT_MIN
NoClipLow:

    ; Store result back in buffer
    mov DWORD PTR [r12 + rdi*4], eax   ; buffer[rdi] = result

    ; Increment loop counter
    inc rdi
    jmp LoopStart                     ; Repeat loop

LoopEnd:
    ; Epilogue
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

MyProc1 ENDP
END
