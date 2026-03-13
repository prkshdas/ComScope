.syntax         unified
.arch           armv7-a

@ --- Data Section ---
.section        .data
port_path:      .asciz      "/dev/ttyUSB0"      @ Hardcoded for now. Update this if yours is ttyACM0!
err_msg:        .ascii      "Error: Could not open port. Run with sudo?\n"
err_len = . - err_msg

@ --- BSS Section ---
.section        .bss
.align          4
buff:           .space      256                 @ Buffer to hold incoming serial data

@ --- Text Section ---
.section        .text
.global         _start

_start:
    @ 1. Open the serial port
    mov         r7, #5                  @ syscall(open)
    ldr         r0, =port_path          @ filename (/dev/ttyUSB0)
    @ Flags: O_RDWR (2) | O_NOCTTY (256) = 258
    @ O_NOCTTY prevents the serial port from becoming the controlling terminal
    mov         r1, #258                
    mov         r2, #0                  @ mode (not needed here)
    svc         0                       @ invoke kernel

    @ Check if open failed
    cmp         r0, #0
    blt         open_error              @ Branch to error if FD < 0
    mov         r4, r0                  @ Save the serial port File Descriptor (FD) in r4

read_loop:
    @ 2. Read from the serial port
    mov         r7, #3                  @ syscall(read)
    mov         r0, r4                  @ FD (our serial port)
    ldr         r1, =buff               @ where to store incoming data
    mov         r2, #256                @ max bytes to read at once
    svc         0                       @ invoke kernel

    @ Check if read was successful
    cmp         r0, #0
    ble         read_loop               @ If 0 or error, just loop and try reading again
    mov         r5, r0                  @ Save the actual number of bytes read in r5

    @ 3. Write incoming data to standard output (screen)
    mov         r7, #4                  @ syscall(write)
    mov         r0, #1                  @ FD 1 = stdout
    ldr         r1, =buff               @ address of our data
    mov         r2, r5                  @ number of bytes we just read
    svc         0                       @ invoke kernel

    b           read_loop               @ Jump back to top and wait for more data!

open_error:
    @ Print error message
    mov         r7, #4                  @ syscall(write)
    mov         r0, #1                  @ stdout
    ldr         r1, =err_msg
    mov         r2, #err_len
    svc         0

exit:
    mov         r7, #1                  @ syscall(exit)
    mov         r0, #0
    svc         0
    