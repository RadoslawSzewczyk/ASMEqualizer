;.586
.DATA
ALIGN 16
M_PI REAL4 3.14159265358979323846
INT_MIN DWORD -2147483648, -2147483648, -2147483648, -2147483648 ; 16-byte aligned
INT_MAX DWORD 2147483647, 2147483647, 2147483647, 2147483647     ; 16-byte aligned

lowCutoff REAL4 200.0
highCutoff REAL4 3000.0
sampleRate REAL4 44100.0

.CODE
MyProc1 PROC
    ; Prologue
    push rbp
    mov  rbp, rsp
    sub  rsp, 64                 ; Allocate stack space for local variables

    ; Load parameters
    mov  r10, rcx                ; r10 = buffer
    mov  r11, rdx                ; r11 = length
    movd xmm0, r8d               ; xmm0 = lowGain
    movd xmm1, r9d               ; xmm1 = midGain
    mov  eax, 100                ; Hard-code highGain to 100
    movd xmm2, eax               ; xmm2 = highGain

    cvtdq2ps xmm0, xmm0          ; Convert lowGain to float
    cvtdq2ps xmm1, xmm1          ; Convert midGain to float
    cvtdq2ps xmm2, xmm2          ; Convert highGain to float

    ; Compute coefficients
    movss xmm3, dword ptr [lowCutoff]
    mulss xmm3, dword ptr [M_PI]
    divss xmm3, dword ptr [sampleRate]
    shufps xmm3, xmm3, 0         ; Broadcast lowAlpha

    movss xmm4, dword ptr [highCutoff]
    mulss xmm4, dword ptr [M_PI]
    divss xmm4, dword ptr [sampleRate]
    shufps xmm4, xmm4, 0         ; Broadcast highAlpha

    ; Precompute 1 - coefficients
    movaps xmm5, xmmword ptr [M_PI]
    subps xmm5, xmm3             ; xmm5 = 1 - lowAlpha
    movaps xmm6, xmmword ptr [M_PI]
    subps xmm6, xmm4             ; xmm6 = 1 - highAlpha

    ; Zero out filter state variables
    xorps xmm7, xmm7             ; xmm7 = lowYPrev
    xorps xmm8, xmm8             ; xmm8 = highYPrev

    ; Main processing loop
process_loop:
    cmp  r11, 4                  ; Check if we have at least 4 samples
    jl   remainder_loop          ; Jump to remainder loop if less than 4

    movups xmm9, xmmword ptr [r10]
    cvtdq2ps xmm9, xmm9          ; Convert to float

    ; Low-pass filter
    mulps xmm7, xmm5             ; (1 - lowAlpha) * lowYPrev
    mulps xmm9, xmm3             ; lowAlpha * buffer[i]
    addps xmm7, xmm9             ; Update lowYPrev

    ; High-pass filter
    subps xmm9, xmm7             ; buffer[i] - lowYPrev
    mulps xmm8, xmm6             ; (1 - highAlpha) * highYPrev
    mulps xmm9, xmm4             ; highAlpha * (buffer[i] - lowYPrev)
    addps xmm8, xmm9             ; Update highYPrev

    ; Band-pass filter
    movaps xmm10, xmmword ptr [r10]
    cvtdq2ps xmm10, xmm10        ; Convert to float
    subps xmm10, xmm7            ; Subtract low-pass
    subps xmm10, xmm8            ; Subtract high-pass

    ; Apply gains
    mulps xmm7, xmm0             ; Apply low gain
    mulps xmm8, xmm1             ; Apply mid gain
    mulps xmm10, xmm2            ; Apply high gain

    ; Combine results
    addps xmm7, xmm8
    addps xmm7, xmm10

    ; Clip to int range
    movups xmm11, xmmword ptr [INT_MIN]
    movups xmm12, xmmword ptr [INT_MAX]
    maxps xmm7, xmm11
    minps xmm7, xmm12

    ; Store back
    cvtps2dq xmm7, xmm7
    movups xmmword ptr [r10], xmm7

    ; Advance buffer pointer
    add  r10, 16
    sub  r11, 4
    jmp  process_loop

remainder_loop:
    ; Scalar processing for remaining samples (not shown)

done:
    mov  rsp, rbp
    pop  rbp
    ret
MyProc1 ENDP
END
