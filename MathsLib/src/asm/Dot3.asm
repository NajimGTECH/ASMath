; =====================================================================================
; Dot3.asm - Hand-written x64 assembly (MASM) for the dot product of Vector3<float>.
;
; Declared in C++ in AsmFunctions.h with extern "C" (no C++ name mangling).
;
; Microsoft x64 calling convention (the only one used on Windows x64):
;   - the first 4 integer / pointer arguments are passed in RCX, RDX, R8, R9 (in this order);
;   - a float return value is returned in the low lane of XMM0;
;   - RAX, RCX, RDX, R8-R11 and XMM0-XMM5 are "volatile": a function may modify them
;     without saving them;
;   - both functions below are "leaf functions": they call nothing, never touch RSP and
;     do not use any non-volatile register. So they need no prologue, no stack frame and no
;     unwind information (PROC FRAME). A simple PROC ... ret is enough.
;
; Memory layout of a Vector3<float> (12 bytes, no padding):
;   offset 0 = x, offset 4 = y, offset 8 = z     (dword = 4 bytes = one float)
;
; The instructions are scalar SSE instructions ("ss" = scalar single precision): they only
; work on lane 0 of the XMM registers. The operations are done in the same order as
; Vector3::Dot, (a.x*b.x + a.y*b.y) + a.z*b.z, so the result is bit-identical to the C++ one.
; =====================================================================================

.code

; -------------------------------------------------------------------------------------
; float Dot3Asm(const float* a, const float* b)
;   RCX = address of a.x
;   RDX = address of b.x
;   XMM0 = result
; -------------------------------------------------------------------------------------
Dot3Asm PROC
    movss   xmm0, dword ptr [rcx]       ; xmm0 = a.x
    mulss   xmm0, dword ptr [rdx]       ; xmm0 = a.x * b.x
    movss   xmm1, dword ptr [rcx + 4]   ; xmm1 = a.y
    mulss   xmm1, dword ptr [rdx + 4]   ; xmm1 = a.y * b.y
    addss   xmm0, xmm1                  ; xmm0 = a.x*b.x + a.y*b.y
    movss   xmm1, dword ptr [rcx + 8]   ; xmm1 = a.z
    mulss   xmm1, dword ptr [rdx + 8]   ; xmm1 = a.z * b.z
    addss   xmm0, xmm1                  ; xmm0 = (a.x*b.x + a.y*b.y) + a.z*b.z
    ret                                 ; the result is already in xmm0
Dot3Asm ENDP

; -------------------------------------------------------------------------------------
; void DotBatchAsm(const float* a, const float* b, float* out, size_t n)
;   RCX = a   (address of a[0].x, moves forward by 12 bytes per vector)
;   RDX = b   (address of b[0].x, moves forward by 12 bytes per vector)
;   R8  = out (address of out[0], moves forward by 4 bytes per result)
;   R9  = n   (number of vectors still to process, counts down to 0)
;
; Same computation as Dot3Asm, in a loop, without the cost of one call per vector.
; -------------------------------------------------------------------------------------
DotBatchAsm PROC
    test    r9, r9                      ; n == 0 ?
    jz      done                        ; empty batch: nothing is read or written

next_vector:
    movss   xmm0, dword ptr [rcx]       ; a.x
    mulss   xmm0, dword ptr [rdx]       ; a.x * b.x
    movss   xmm1, dword ptr [rcx + 4]   ; a.y
    mulss   xmm1, dword ptr [rdx + 4]   ; a.y * b.y
    addss   xmm0, xmm1
    movss   xmm1, dword ptr [rcx + 8]   ; a.z
    mulss   xmm1, dword ptr [rdx + 8]   ; a.z * b.z
    addss   xmm0, xmm1                  ; (a.x*b.x + a.y*b.y) + a.z*b.z
    movss   dword ptr [r8], xmm0        ; out[i] = result

    add     rcx, 12                     ; next Vector3 of a
    add     rdx, 12                     ; next Vector3 of b
    add     r8, 4                       ; next float of out
    dec     r9                          ; one vector less to process (sets the zero flag)
    jnz     next_vector                 ; loop while r9 != 0

done:
    ret
DotBatchAsm ENDP

END
