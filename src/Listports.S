.syntax         unified
.arch           armv7-a

@   .data section
.section        .data
dev_path:       .asciz      "/dev"
prefix:         .ascii      "/dev/"
newline:        .ascii      "\n"


@   .bss section
.section        .bss
.align          4
buff:           .space      4096        @ 4096 bytes of buffer


@   .text section

.section        .text
.global         _start

_start:
	@ open (/dev, 0_RDONLY)
	mov         r7,#5                  	@ syscall(open)
	ldr         r0,=dev_path			@ filename
	mov			r1,#0					@ 0_RDONLY
	mov			r2,#0					@ not applicable for 0_RDONLY
	svc			0						@ invoke kernal

	@ check if open fail
	cmp			r0,#0					@ check if read done
	blt			exit					@ if error exit
	mov			r4,r0					@ save FD on f4

read_dir:
	@ read directory entries getdents
	mov			r7,#141					@ syscall(getdents)
	mov			r0,r4					@ FD
	ldr			r1,=buff				@ store list here
	mov			r2,#4096				@ size
	svc			0						@ invoke kernal

	@ check error, end of directory
	cmp			r0,#0
	ble			close_and_exit
	mov			r5,r0					@ save FD

	mov 		r6,#0

loop:
	cmp			r6,r5					
	bge			read_dir

	ldr			r9,=buff				@ load buffer address
	add			r9,r9,r6				@ go to first entry point
	ldrh		r8,[r9,#8]				@ load 2 byte on r8
	cmp			r8,#0
	beq			close_and_exit
	add			r10,r9,#10				@ directory name start from here

	@ check "tty"
	ldrb		r0,[r10,#0]
	cmp			r0,#'t'
	bne			next_entry				@ if not equal
	ldrb		r0,[r10,#1]
	cmp			r0,#'t'
	bne			next_entry
	ldrb		r0,[r10,#2]
	cmp			r0,#'y'
	bne			next_entry

	@ branch to check 'U' (USB) or 'A' (ACM)
	ldrb		r0,[r10,#3]
	cmp			r0,#'U'
	beq			chech_usb
	cmp			r0,#'A'
	beq			chech_acm
	b			next_entry

chech_usb:
	ldrb		r0,[r10,#4]
	cmp			r0,#'S'
	bne			next_entry
	ldrb		r0,[r10,#5]
	cmp			r0,#'B'
	bne			next_entry
	b			match_found

chech_acm:
	ldrb		r0,[r10,#4]
	cmp			r0,#'C'
	bne			next_entry
	ldrb		r0,[r10,#5]
	cmp			r0,#'M'
	bne			next_entry
	b			match_found

match_found:
	@ print "/dev/"
	mov			r7,#4					@ syscall(write)
	mov			r0,#1					@ stdout
	ldr			r1,=prefix				@ load address
	mov			r2,#5					@ size
	svc			0						@ invoke kernal

	@ count name length
	mov			r11,r10
	mov			r3,#0

count_len:
	ldrb		r0,[r11],#1				@ load and increment the pointer
	cmp			r0,#0
	beq			print_name
	add			r3,r3,#1
	b			count_len

print_name:
	mov			r7,#4
	mov			r0,#1
	mov			r1,r10
	mov			r2,r3
	svc			0

	@ print newline
	mov			r7,#4
	mov			r0,#1
	ldr			r1,=newline
	mov			r2,#1
	svc			0

next_entry:
	add			r6,r6,r8				@ current entry address + previous entry size
	b			loop

close_and_exit:
	mov			r7,#6					@ syscall(close)
	mov			r0,r4					@ FD
	svc			0

exit:
	mov			r7,#1					@ syscall(exit)
	mov			r0,#0
	svc			0
