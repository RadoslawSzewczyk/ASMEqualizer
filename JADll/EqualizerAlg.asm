;------------------------------------------------------------------------------
; Program: Multi-Band Audio Filter
; Version: 0.7
; Description:
;   This program applies a three-band filter (low-pass, mid-pass, and high-pass)
;   to an audio sample buffer. Each band is processed separately, then combined.
;
; Parameters:
;   - rcx: Pointer to the audio buffer (32-bit integer samples)
;   - rdx: Number of samples (length of buffer)
;   - r8: Low band gain (percentage)
;   - r9: Mid band gain (percentage)
;   - r10: High band gain (percentage, retrieved from stack)
;
; Registers Used:
;   - rax: General-purpose register used in calculations
;   - rbx: Loop counter (index)
;   - xmm0 - xmm4: Floating point registers for filter calculations
;
; Registers Preserved:
;   - rbp, rbx, rsi, rdi, r12-r15 (saved in prologue, restored in epilogue)
;
; Assumptions:
;   - The audio buffer contains signed 32-bit integer samples.
;   - Uses SSE2 floating-point instructions for calculations.
;   - The filter state variables are aligned and initialized to zero.
;------------------------------------------------------------------------------

.data
ALIGN 16
sampleRate   REAL8  44100.0          ; Sampling rate in Hz
lowCutoff    REAL8  200.0            ; Low cutoff frequency (Hz)
highCutoff   REAL8  5000.0           ; High cutoff frequency (Hz)
pi           REAL8  3.14159          ; Approximation of Pi
hundred      REAL8  100.0            ; Constant 100.0 for gain percentage
upperLimit   REAL8  2147483647.0     ; Maximum 32-bit signed integer value
lowerLimit   REAL8  -2147483648.0    ; Minimum 32-bit signed integer value

.data?
ALIGN 16
lowPrev1    REAL8 ?  ; Low-pass filter previous sample 1
lowPrev2    REAL8 ?  ; Low-pass filter previous sample 2
highPrev1   REAL8 ?  ; High-pass filter previous sample 1
highPrev2   REAL8 ?  ; High-pass filter previous sample 2
midLowPrev1 REAL8 ?  ; Mid band low-pass previous sample
midLowPrev2 REAL8 ?  ; Mid band low-pass previous sample
midHighPrev1 REAL8 ? ; Mid band high-pass previous sample
midHighPrev2 REAL8 ? ; Mid band high-pass previous sample

.code
MyProc1 PROC
    ;---------------------------
    ; Function Prologue - Save registers to preserve function state
    ;---------------------------
    push    rbp                     ; Save base pointer
    push    rbx                     ; Save rbx (loop counter)
    push    rsi                     ; Save rsi
    push    rdi                     ; Save rdi
    push    r12                     ; Save r12
    push    r13                     ; Save r13
    push    r14                     ; Save r14
    push    r15                     ; Save r15

    ;---------------------------
    ; Load function parameters
    ;---------------------------
    mov     rbp, rcx                ; Load buffer pointer into rbp
    mov     rdx, rdx                ; Load number of samples into rdx
    mov     r8, r8                  ; Load low band gain into r8
    mov     r9, r9                  ; Load mid band gain into r9
    mov     r10, qword ptr [rsp + 104] ; Load high band gain from stack into r10

    ;---------------------------
    ; Initialize filter state variables to zero
    ;---------------------------
    xor rax, rax                    ; Zero out RAX register
    mov    qword ptr [lowPrev1], rax ; Reset low-pass previous sample 1
    mov    qword ptr [lowPrev2], rax ; Reset low-pass previous sample 2
    mov    qword ptr [highPrev1], rax ; Reset high-pass previous sample 1
    mov    qword ptr [highPrev2], rax ; Reset high-pass previous sample 2
    mov    qword ptr [midLowPrev1], rax ; Reset mid-low previous sample 1
    mov    qword ptr [midLowPrev2], rax ; Reset mid-low previous sample 2
    mov    qword ptr [midHighPrev1], rax ; Reset mid-high previous sample 1
    mov    qword ptr [midHighPrev2], rax ; Reset mid-high previous sample 2

    xor rbx, rbx                    ; Initialize loop counter (rbx = 0)

ProcessLoop:
    ;---------------------------
    ; Loop Condition: Check if we processed all samples
    ;---------------------------
    cmp rbx, rdx                    ; Compare loop counter (rbx) with length (rdx)
    jge EndLoop                      ; If all samples processed, exit loop

    ;---------------------------
    ; Load current audio sample
    ;---------------------------
    movsxd rax, dword ptr [rbp + rbx * 4] ; Load 32-bit integer sample
    cvtsi2sd xmm0, rax                   ; Convert integer sample to double (SSE2)

    ;---------------------------
    ; Compute Low-Pass Filter
    ;---------------------------
    movsd xmm1, qword ptr [lowCutoff]  ; Load low cutoff frequency
    mulsd xmm1, qword ptr [pi]         ; Multiply by pi
    addsd xmm1, xmm1                   ; Multiply by 2
    divsd xmm1, qword ptr [sampleRate] ; Compute alpha = (2 * pi * fc) / Fs

    mulsd xmm1, xmm0                   ; Compute alpha * x[n]
    movsd xmm2, xmm1                    ; Store in xmm2
    movsd xmm3, qword ptr [lowPrev1]     ; Load previous sample y[n-1]
    mulsd xmm3, hundred                  ; Multiply by 100 for scaling
    subsd xmm2, xmm3                     ; Subtract scaled y[n-1]
    divsd xmm2, hundred                   ; Scale back

    movsd qword ptr [lowPrev2], xmm3     ; Update lowPrev2 = previous lowPrev1
    movsd qword ptr [lowPrev1], xmm2     ; Store new low-pass sample

    ; Apply low band gain
    cvtsi2sd xmm3, r8                   ; Convert lowGain to double
    divsd xmm3, hundred                  ; Divide by 100 (normalize gain)
    mulsd xmm2, xmm3                     ; Apply gain factor
    movsd qword ptr [rsp + 16], xmm2    ; Store low-band output

    ;---------------------------
    ; Compute High-Pass Filter
    ;---------------------------
    movsd xmm1, qword ptr [highCutoff]   ; Load high cutoff frequency
    divsd xmm1, qword ptr [sampleRate]   ; Compute alpha = fc / Fs

    movsd xmm2, qword ptr [highPrev1]    ; Load previous y[n-1]
    addsd xmm2, xmm0                     ; Add current sample x[n]
    subsd xmm2, qword ptr [highPrev2]    ; Subtract y[n-2]
    mulsd xmm1, xmm2                     ; Compute high-pass output

    movsd qword ptr [rsp + 24], xmm1    ; Store high-band output

    ;---------------------------
    ; Combine Filtered Bands
    ;---------------------------
    movsd xmm0, qword ptr [rsp + 16]    ; Load low-band output
    addsd xmm0, qword ptr [rsp + 24]    ; Add high-band output

    maxsd xmm0, lowerLimit              ; Clamp output to lower limit
    minsd xmm0, upperLimit              ; Clamp output to upper limit
    cvttsd2si rax, xmm0                 ; Convert double back to integer
    mov dword ptr [rbp + rbx * 4], eax  ; Store processed sample

    inc rbx                              ; Increment loop counter
    jmp ProcessLoop                      ; Repeat loop

EndLoop:
    ; Restore saved registers (epilogue)
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbx
    pop rbp
    ret
MyProc1 ENDP
END
