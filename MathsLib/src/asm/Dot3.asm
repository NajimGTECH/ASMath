.code

; float Dot3Asm(const float* a, const float* b)
; rcx = a, rdx = b, result in xmm0
Dot3Asm PROC
    movss xmm0, dword ptr [rcx]        ; xmm0 = a.x
    mulss xmm0, dword ptr [rdx]        ; xmm0 = a.x * b.x

    movss xmm1, dword ptr [rcx + 4]    ; xmm1 = a.y
    mulss xmm1, dword ptr [rdx + 4]    ; xmm1 = a.y * b.y
    addss xmm0, xmm1                   ; xmm0 = a.x*b.x + a.y*b.y

    movss xmm1, dword ptr [rcx + 8]    ; xmm1 = a.z
    mulss xmm1, dword ptr [rdx + 8]    ; xmm1 = a.z * b.z
    addss xmm0, xmm1                   ; xmm0 = a.x*b.x + a.y*b.y + a.z*b.z

    ret                                ; the result is in xmm0
Dot3Asm ENDP


; void DotBatchAsm(const float* a, const float* b, float* out, size_t n)
; rcx = a, rdx = b, r8 = out, r9 = n
; Does out[i] = dot(a[i], b[i]) for the n vectors.
DotBatchAsm PROC
    cmp r9, 0                          ; if n == 0
    je  finished                       ; then there is nothing to do

loop_start:
    ; same code as Dot3Asm
    movss xmm0, dword ptr [rcx]        ; a.x
    mulss xmm0, dword ptr [rdx]        ; a.x * b.x
    movss xmm1, dword ptr [rcx + 4]    ; a.y
    mulss xmm1, dword ptr [rdx + 4]    ; a.y * b.y
    addss xmm0, xmm1
    movss xmm1, dword ptr [rcx + 8]    ; a.z
    mulss xmm1, dword ptr [rdx + 8]    ; a.z * b.z
    addss xmm0, xmm1

    movss dword ptr [r8], xmm0         ; out[i] = result

    add rcx, 12                        ; a points to the next vector (3 floats = 12 bytes)
    add rdx, 12                        ; b points to the next vector
    add r8, 4                          ; out points to the next float (4 bytes)
    sub r9, 1                          ; one vector less to do
    jnz loop_start                     ; if r9 is not 0, do the next vector

finished:
    ret
DotBatchAsm ENDP

END
