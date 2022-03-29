; global init_ps2_controller
; global init_keyboard

section .text
bits 64

; Initializes the ps/2 controller
; Returns 1 if successful, bad code if error
old_init_ps2_controller:
    mov ah, 0xAD
    call write_command      ; disable port 1
    mov ah, 0xA7
    call write_command      ; disable port 2
    call read_data          ; flush output buffer
    
    ; controller configuration
    mov ah, 0x20
    call write_command      ; tell controller to read configuration byte
    call read_data          ; read the data port
    and ah, 0xBC             ; disable IRQs and translation (clear bits 0, 1, 6)
    mov ch, ah
    mov ah, 0x60
    call write_command      ; tell controller to write next byte to configuration
    mov ah, ch
    call write_data         ; write configuration byte

    ; test ps/2 controller
    mov ah, 0xAA
    call write_command
    call read_data
    cmp ah, 0x55            ; check for correct test response
    jz .self_test_passed
    mov rax, -1
    ret
.self_test_passed:

    ; check if there are two channels
    ; if last read configuration bit 5 is clear, it is single channel
    test ch, 0x10
    jz .test_first_port
    ; if it was not clear, we need to double check
    ; enable port 2
    mov ah, 0xA8
    call write_command
    ; read configuration byte again
    mov ah, 0x20
    call write_command
    call read_data
    mov ch, ah
    ; disable port 2 again
    mov ah, 0xA7
    call write_command
    test ch, 0x10
    jz .test_first_port ; if the bit was set, there is no 2nd port

    ; test port two
    mov ah, 0xA9
    call write_command
    call read_data
    cmp ah, 0
    jz .port_two_pass
    mov rax, -2         ; test failed
    ret
.port_two_pass:
    mov ah, 0xA8
    call write_command  ; enable port two

    ; test port one
.test_first_port:
    mov ah, 0xAB
    call write_command
    call read_data
    cmp ah, 0
    jz .port_one_pass
    mov rax, -3         ; test failed
    ret
.port_one_pass:
    mov ah, 0xAE
    call write_command  ; enable port one

; TODO: enable interrupts for the keyboards
    mov rax, 1
    ret

; Initializes the keyboard
; Returns 1 on success, negative on error
old_init_keyboard:
    mov ah, 0xFF
    call write_data     ; reset port one
    call read_data
    cmp ah, 0xAA
    jz .keyboard_test_passed
    mov rax, -1
    ret
.keyboard_test_passed:
    mov rax, 1
    ret

; Reads from data port when controller is ready into ah
read_data:
    mov byte dh, [0x64]
    test dh, 0x1
    jz read_data        ; status bit must be set before reading
    mov byte ah, [0x60]
    ret

; Writes data in ah to data port when ready
write_data:
    mov byte dh, [0x64]
    test dh, 0x2
    jnz write_data      ; status bit must be clear before writing
    mov byte [0x60], ah
    ret

; Writes data in ah to command register when ready
write_command:
    mov byte dh, [0x64]
    test dh, 0x2
    jnz write_command   ; status bit must be clear before writing
    mov byte [0x64], ah
    ret
