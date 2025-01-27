.data
ALIGN 16
sampleRate REAL8 44100.0
lowCutoff REAL8 200.0
highCutoff REAL8 5000.0
pi REAL8 3.14159
hundred REAL8 100.0
upperLimit REAL8 2147483647.0
lowerLimit REAL8 -2147483648.0
one REAL8 1.0
zero REAL8 0.0

; Precomputed constants (to be initialized in the prologue)
lowAlpha REAL8 ?
highAlpha REAL8 ?
midLowAlpha REAL8 ?
midHighAlpha REAL8 ?

.data?
ALIGN 16
lowPrev1 REAL8 ?
lowPrev2 REAL8 ?
highPrev1 REAL8 ?
highPrev2 REAL8 ?
midLowPrev1 REAL8 ?
midLowPrev2 REAL8 ?
midHighPrev1 REAL8 ?
midHighPrev2 REAL8 ?
tempGains REAL8 3 dup(?)

.code
MyProc1 PROC
    ; Prologue
    push    rbp
    push    rbx
    push    rsi
    push    rdi
    push    r12
    push    r13
    push    r14
    push    r15


    ;--------------------
    ; Retrieve parameters
    ;--------------------
    mov     rbp, rcx           ; rbp = buffer (1st argument in RCX)
    mov     rdx, rdx           ; rdx = length (2nd argument in RDX)
    mov     r8, r8             ; r8 = lowGain (3rd argument in R8)
    mov     r9, r9             ; r9 = midGain (4th argument in R9)
    mov     r10, qword ptr [rsp + 104]   ; Explicitly specify qword ptr for a 64-bit value

    ;--------------------
    ; Set up variables
    ;--------------------
    cvtsi2sd xmm0, r8               ; Convert lowGain to double
    divsd xmm0, hundred             ; Scale: lowGain / 100
    movsd qword ptr [tempGains], xmm0

    cvtsi2sd xmm0, r9               ; Convert midGain to double
    divsd xmm0, hundred             ; Scale: midGain / 100
    movsd qword ptr [tempGains + 8], xmm0

    cvtsi2sd xmm0, r10              ; Convert highGain to double
    divsd xmm0, hundred             ; Scale: highGain / 100
    movsd qword ptr [tempGains + 16], xmm0

    ; Initialize filter states to zero
    movsd xmm0, zero
    movsd qword ptr [lowPrev1], xmm0
    movsd qword ptr [lowPrev2], xmm0
    movsd qword ptr [highPrev1], xmm0
    movsd qword ptr [highPrev2], xmm0
    movsd qword ptr [midLowPrev1], xmm0
    movsd qword ptr [midLowPrev2], xmm0
    movsd qword ptr [midHighPrev1], xmm0
    movsd qword ptr [midHighPrev2], xmm0

    ; Precompute filter constants
    movsd xmm0, qword ptr [lowCutoff]
    mulsd xmm0, pi
    addsd xmm0, xmm0
    divsd xmm0, qword ptr [sampleRate]
    movsd qword ptr [lowAlpha], xmm0

    movsd xmm0, qword ptr [highCutoff]
    divsd xmm0, qword ptr [sampleRate]
    movsd qword ptr [highAlpha], xmm0

    xor rbx, rbx                    ; Initialize index counter

ProcessLoop:
    cmp rbx, rdx                    ; Compare index with length
    jge EndLoop                     ; Exit loop if index >= length

    ; Process each sample (logic unchanged)
    movsxd rax, dword ptr [rbp + rbx * 4]
    cvtsi2sd xmm0, rax

    ; Low-pass filter (unchanged)
    movsd xmm1, qword ptr [lowAlpha]
    mulsd xmm1, xmm0
    movsd xmm2, one
    subsd xmm2, qword ptr [lowAlpha]
    mulsd xmm2, qword ptr [lowPrev1]
    addsd xmm1, xmm2
    movsd xmm3, qword ptr [lowPrev1]
    movsd qword ptr [lowPrev2], xmm3
    movsd qword ptr [lowPrev1], xmm1
    mulsd xmm1, qword ptr [tempGains]
    movsd qword ptr [rsp + 16], xmm1

    ; High-pass filter (unchanged)
    movsd xmm1, qword ptr [highAlpha]
    movsd xmm2, qword ptr [highPrev1]
    addsd xmm2, xmm0
    subsd xmm2, qword ptr [highPrev2]
    mulsd xmm1, xmm2
    movsd xmm3, qword ptr [highPrev1]
    movsd qword ptr [highPrev2], xmm3
    movsd qword ptr [highPrev1], xmm1
    mulsd xmm1, qword ptr [tempGains + 16]
    movsd qword ptr [rsp + 24], xmm1

    ; Combine bands
    addsd xmm0, qword ptr [rsp + 16]
    addsd xmm0, qword ptr [rsp + 24]
    maxsd xmm0, lowerLimit
    minsd xmm0, upperLimit
    cvttsd2si rax, xmm0
    mov dword ptr [rbp + rbx * 4], eax
    inc rbx
    jmp ProcessLoop

EndLoop:
    ; Epilogue
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    pop     rbp
    ret
MyProc1 ENDP
END
