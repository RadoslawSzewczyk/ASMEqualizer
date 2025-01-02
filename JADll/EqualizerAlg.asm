;.586
;INCLUDE C:\masm32\include\windows.inc

;------------------------------------------------------------------------
; This assembly procedure processes a buffer of audio samples and applies a gain factor
; to each sample. The samples are stored as an array of 32-bit integers.
; Version 0.1

; Input Parameters:
; RCX - pointer to the input buffer (array of 32-bit integers)
; RDX - buffer length (number of elements in the array)
; R8  - gain factor (32-bit integer multiplier)

.CODE

MyProc1 proc                      ; Procedure definition
    push rbx                      ; Save the current value of rbx to the stack to preserve it
    xor  rbx, rbx                 ; Clear (set to zero) the rbx register; rbx will serve as our index counter

ProcessLoop:
    cmp  rbx, rdx                 ; Compare index (rbx) with buffer length (rdx)
    jge  EndLoop                  ; If index >= buffer length, jump to EndLoop and exit

    ; Load the current 32-bit sample from the buffer
    mov eax, [rcx + rbx*4]        ; Load the 32-bit integer sample at position [rcx + rbx*4] into eax

    ; Apply the gain (multiplication)
    imul eax, r8d                 ; Multiply the loaded sample (eax) by the gain factor (r8d)
    
    ; Store the modified sample back in the buffer
    mov [rcx + rbx*4], eax        ; Store the result of the multiplication back to the buffer at the same location

    ; Increment index for the next iteration
    inc  rbx                      ; Increment the index counter (rbx)
    jmp  ProcessLoop              ; Jump back to the start of the loop for the next element

EndLoop:
    pop rbx                       ; Restore the original value of rbx from the stack
    ret                           ; Return from the procedure
MyProc1 endp                      ; End of procedure definition

END                               ; Marks the end of the source file
