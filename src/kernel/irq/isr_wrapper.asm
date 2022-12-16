extern irq_handler
extern curr_proc
extern next_proc
extern sys_call_isr

struc rf
    ._rax:  resq 1
    ._rbx:  resq 1
    ._rcx:  resq 1
    ._rdx:  resq 1
    ._rdi:  resq 1
    ._rsi:  resq 1
    ._r8:   resq 1
    ._r9:   resq 1
    ._r10:  resq 1
    ._r11:  resq 1
    ._r12:  resq 1
    ._r13:  resq 1
    ._r14:  resq 1
    ._r15:  resq 1
    ._cs:   resw 1
    ._ss:   resw 1
    ._ds:   resw 1
    ._es:   resw 1
    ._fs:   resw 1
    ._gs:   resw 1
    ._pad:  resd 1
    ._rbp:  resq 1
    ._rsp:  resq 1
    ._rip:  resq 1
    ._rflags:   resq 1
endstruc

%macro ISR_WRAPPER 1
global isr_wrapper_%1
isr_wrapper_%1:
    push rdi
    push rsi
    mov dil, %1 ; place irq number in rdi
    mov esi, 0  ; place no error code in rsi
    jmp isr_generic
%endmacro

%macro ISR_WRAPPER_ERR 1
global isr_wrapper_%1
isr_wrapper_%1:
    push rsi
    mov esi, [rsp + 8]  ; place error code in rsi
    mov [rsp + 8], rdi  ; overwrite error code with rdi
    mov dil, %1         ; place irq number in rdi
    jmp isr_generic
%endmacro

%macro CONTEXT_SWITCH 0
; check if next process equals current process
    mov rcx, [curr_proc]
    mov rbx, [next_proc]
    cmp rcx, rbx
    je .no_context_switch

.save:
    ; current process != next process, perform context switch
    ; save current context into current_process
    pop rdx
    mov [rcx + rf._r15], rdx
    pop rdx
    mov [rcx + rf._r14], rdx
    pop rdx
    mov [rcx + rf._r13], rdx
    pop rdx
    mov [rcx + rf._r12], rdx
    pop rdx
    mov [rcx + rf._r11], rdx
    pop rdx
    mov [rcx + rf._r10], rdx
    pop rdx
    mov [rcx + rf._r9], rdx
    pop rdx
    mov [rcx + rf._r8], rdx
    pop rdx
    mov [rcx + rf._rdx], rdx
    pop rdx
    mov [rcx + rf._rcx], rdx
    pop rdx
    mov [rcx + rf._rbx], rdx
    pop rdx
    mov [rcx + rf._rax], rdx
    pop rdx
    mov [rcx + rf._rsi], rdx
    pop rdx
    mov [rcx + rf._rdi], rdx

    ; get data from isr stack
    mov rdx, [rsp]
    mov [rcx + rf._rip], rdx
    mov dx, [rsp + 8]
    mov [rcx + rf._cs], dx
    mov rdx, [rsp + 16]
    mov [rcx + rf._rflags], rdx
    mov rdx, [rsp + 24]
    mov [rcx + rf._rsp], rdx
    mov dx, [rsp + 32]
    mov [rcx + rf._ss], dx

    ; segment registers
    mov [rcx + rf._ds], ds
    mov [rcx + rf._es], es
    mov [rcx + rf._fs], fs
    mov [rcx + rf._gs], gs

    ; set current proc to next proc
    mov [curr_proc], rbx

.load:
    ; load the context from next_process
    ; overwrite isr stack
    mov rdx, [rbx + rf._rip]
    mov [rsp], rdx
    mov dx, [rbx + rf._cs]
    mov [rsp + 8], dx
    mov rdx, [rbx + rf._rflags]
    mov [rsp + 16], rdx
    mov rdx, [rbx + rf._rsp]
    mov [rsp + 24], rdx
    mov dx, [rbx + rf._ss]
    mov [rsp + 32], dx

    ; load registers
    mov rax, [rbx + rf._rax]
    mov rcx, [rbx + rf._rcx]
    mov rdx, [rbx + rf._rdx]
    mov rsi, [rbx + rf._rsi]
    mov rdi, [rbx + rf._rdi]
    mov r8, [rbx + rf._r8]
    mov r9, [rbx + rf._r9]
    mov r10, [rbx + rf._r10]
    mov r11, [rbx + rf._r11]
    mov r12, [rbx + rf._r12]
    mov r13, [rbx + rf._r13]
    mov r14, [rbx + rf._r14]
    mov r15, [rbx + rf._r15]
    mov ds, [rbx + rf._ds]
    mov es, [rbx + rf._es]
    mov fs, [rbx + rf._fs]
    mov gs, [rbx + rf._gs]
    mov rbx, [rbx + rf._rbx]

    ; ready to run next_proc on iretq
.switch:
    iretq

.no_context_switch:
%endmacro

section .text
bits 64

global isr_generic
isr_generic:
    ; expects rdi and rsi already pushed;
    ; rsi at top of stack

    ; push rest of registers
    push rax
    push rbx
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; place a pointer to the stack frame stack in 3rd arg
    mov rdx, rsp
    add rdx, 112

    call irq_handler
    
    CONTEXT_SWITCH

    ; no context switch
    ; restore scratch registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rsi
    pop rdi
    iretq

global isr_wrapper_206
isr_wrapper_206:
    push rdi
    push rsi
    push rax
    push rbx
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    call sys_call_isr
    
    CONTEXT_SWITCH

    ; no context switch
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rbx
    add rsp, 8  ; don't restore return val (rax)
    pop rsi
    pop rdi
    iretq

ISR_WRAPPER 0
ISR_WRAPPER 1
ISR_WRAPPER 2
ISR_WRAPPER 3
ISR_WRAPPER 4
ISR_WRAPPER 5
ISR_WRAPPER 6
ISR_WRAPPER 7
ISR_WRAPPER_ERR 8
ISR_WRAPPER 9
ISR_WRAPPER_ERR 10
ISR_WRAPPER_ERR 11
ISR_WRAPPER_ERR 12
ISR_WRAPPER_ERR 13
ISR_WRAPPER_ERR 14
ISR_WRAPPER 15
ISR_WRAPPER 16
ISR_WRAPPER_ERR 17
ISR_WRAPPER 18
ISR_WRAPPER 19
ISR_WRAPPER 20
ISR_WRAPPER_ERR 21
ISR_WRAPPER 22
ISR_WRAPPER 23
ISR_WRAPPER 24
ISR_WRAPPER 25
ISR_WRAPPER 26
ISR_WRAPPER 27
ISR_WRAPPER 28
ISR_WRAPPER_ERR 29
ISR_WRAPPER_ERR 30
ISR_WRAPPER 31
ISR_WRAPPER 32
ISR_WRAPPER 33
ISR_WRAPPER 34
ISR_WRAPPER 35
ISR_WRAPPER 36
ISR_WRAPPER 37
ISR_WRAPPER 38
ISR_WRAPPER 39
ISR_WRAPPER 40
ISR_WRAPPER 41
ISR_WRAPPER 42
ISR_WRAPPER 43
ISR_WRAPPER 44
ISR_WRAPPER 45
ISR_WRAPPER 46
ISR_WRAPPER 47
ISR_WRAPPER 48
ISR_WRAPPER 49
ISR_WRAPPER 50
ISR_WRAPPER 51
ISR_WRAPPER 52
ISR_WRAPPER 53
ISR_WRAPPER 54
ISR_WRAPPER 55
ISR_WRAPPER 56
ISR_WRAPPER 57
ISR_WRAPPER 58
ISR_WRAPPER 59
ISR_WRAPPER 60
ISR_WRAPPER 61
ISR_WRAPPER 62
ISR_WRAPPER 63
ISR_WRAPPER 64
ISR_WRAPPER 65
ISR_WRAPPER 66
ISR_WRAPPER 67
ISR_WRAPPER 68
ISR_WRAPPER 69
ISR_WRAPPER 70
ISR_WRAPPER 71
ISR_WRAPPER 72
ISR_WRAPPER 73
ISR_WRAPPER 74
ISR_WRAPPER 75
ISR_WRAPPER 76
ISR_WRAPPER 77
ISR_WRAPPER 78
ISR_WRAPPER 79
ISR_WRAPPER 80
ISR_WRAPPER 81
ISR_WRAPPER 82
ISR_WRAPPER 83
ISR_WRAPPER 84
ISR_WRAPPER 85
ISR_WRAPPER 86
ISR_WRAPPER 87
ISR_WRAPPER 88
ISR_WRAPPER 89
ISR_WRAPPER 90
ISR_WRAPPER 91
ISR_WRAPPER 92
ISR_WRAPPER 93
ISR_WRAPPER 94
ISR_WRAPPER 95
ISR_WRAPPER 96
ISR_WRAPPER 97
ISR_WRAPPER 98
ISR_WRAPPER 99
ISR_WRAPPER 100
ISR_WRAPPER 101
ISR_WRAPPER 102
ISR_WRAPPER 103
ISR_WRAPPER 104
ISR_WRAPPER 105
ISR_WRAPPER 106
ISR_WRAPPER 107
ISR_WRAPPER 108
ISR_WRAPPER 109
ISR_WRAPPER 110
ISR_WRAPPER 111
ISR_WRAPPER 112
ISR_WRAPPER 113
ISR_WRAPPER 114
ISR_WRAPPER 115
ISR_WRAPPER 116
ISR_WRAPPER 117
ISR_WRAPPER 118
ISR_WRAPPER 119
ISR_WRAPPER 120
ISR_WRAPPER 121
ISR_WRAPPER 122
ISR_WRAPPER 123
ISR_WRAPPER 124
ISR_WRAPPER 125
ISR_WRAPPER 126
ISR_WRAPPER 127
ISR_WRAPPER 128
ISR_WRAPPER 129
ISR_WRAPPER 130
ISR_WRAPPER 131
ISR_WRAPPER 132
ISR_WRAPPER 133
ISR_WRAPPER 134
ISR_WRAPPER 135
ISR_WRAPPER 136
ISR_WRAPPER 137
ISR_WRAPPER 138
ISR_WRAPPER 139
ISR_WRAPPER 140
ISR_WRAPPER 141
ISR_WRAPPER 142
ISR_WRAPPER 143
ISR_WRAPPER 144
ISR_WRAPPER 145
ISR_WRAPPER 146
ISR_WRAPPER 147
ISR_WRAPPER 148
ISR_WRAPPER 149
ISR_WRAPPER 150
ISR_WRAPPER 151
ISR_WRAPPER 152
ISR_WRAPPER 153
ISR_WRAPPER 154
ISR_WRAPPER 155
ISR_WRAPPER 156
ISR_WRAPPER 157
ISR_WRAPPER 158
ISR_WRAPPER 159
ISR_WRAPPER 160
ISR_WRAPPER 161
ISR_WRAPPER 162
ISR_WRAPPER 163
ISR_WRAPPER 164
ISR_WRAPPER 165
ISR_WRAPPER 166
ISR_WRAPPER 167
ISR_WRAPPER 168
ISR_WRAPPER 169
ISR_WRAPPER 170
ISR_WRAPPER 171
ISR_WRAPPER 172
ISR_WRAPPER 173
ISR_WRAPPER 174
ISR_WRAPPER 175
ISR_WRAPPER 176
ISR_WRAPPER 177
ISR_WRAPPER 178
ISR_WRAPPER 179
ISR_WRAPPER 180
ISR_WRAPPER 181
ISR_WRAPPER 182
ISR_WRAPPER 183
ISR_WRAPPER 184
ISR_WRAPPER 185
ISR_WRAPPER 186
ISR_WRAPPER 187
ISR_WRAPPER 188
ISR_WRAPPER 189
ISR_WRAPPER 190
ISR_WRAPPER 191
ISR_WRAPPER 192
ISR_WRAPPER 193
ISR_WRAPPER 194
ISR_WRAPPER 195
ISR_WRAPPER 196
ISR_WRAPPER 197
ISR_WRAPPER 198
ISR_WRAPPER 199
ISR_WRAPPER 200
ISR_WRAPPER 201
ISR_WRAPPER 202
ISR_WRAPPER 203
ISR_WRAPPER 204
ISR_WRAPPER 205
; ISR_WRAPPER 206
ISR_WRAPPER 207
ISR_WRAPPER 208
ISR_WRAPPER 209
ISR_WRAPPER 210
ISR_WRAPPER 211
ISR_WRAPPER 212
ISR_WRAPPER 213
ISR_WRAPPER 214
ISR_WRAPPER 215
ISR_WRAPPER 216
ISR_WRAPPER 217
ISR_WRAPPER 218
ISR_WRAPPER 219
ISR_WRAPPER 220
ISR_WRAPPER 221
ISR_WRAPPER 222
ISR_WRAPPER 223
ISR_WRAPPER 224
ISR_WRAPPER 225
ISR_WRAPPER 226
ISR_WRAPPER 227
ISR_WRAPPER 228
ISR_WRAPPER 229
ISR_WRAPPER 230
ISR_WRAPPER 231
ISR_WRAPPER 232
ISR_WRAPPER 233
ISR_WRAPPER 234
ISR_WRAPPER 235
ISR_WRAPPER 236
ISR_WRAPPER 237
ISR_WRAPPER 238
ISR_WRAPPER 239
ISR_WRAPPER 240
ISR_WRAPPER 241
ISR_WRAPPER 242
ISR_WRAPPER 243
ISR_WRAPPER 244
ISR_WRAPPER 245
ISR_WRAPPER 246
ISR_WRAPPER 247
ISR_WRAPPER 248
ISR_WRAPPER 249
ISR_WRAPPER 250
ISR_WRAPPER 251
ISR_WRAPPER 252
ISR_WRAPPER 253
ISR_WRAPPER 254
ISR_WRAPPER 255

global ist_stack1_bottom, ist_stack1_top, ist_stack2_bottom
global ist_stack2_top, ist_stack3_bottom, ist_stack3_top

section .bss
align 0x1000
ist_stack1_bottom:
    resb 4096 * 2
ist_stack1_top:

ist_stack2_bottom:
    resb 4096 * 2
ist_stack2_top:

ist_stack3_bottom:
    resb 4096 * 2
ist_stack3_top: