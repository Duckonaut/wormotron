#pragma once
#include "types.h"

// Burrow: A 16-bit instruction set.
//
// Each instruction is 32 bits in size:
// The first 8 bits are the opcode, the following 24 can be split as 3 schemes:
// | name       | 0000 0000 | 0000 0000 | 0000 0000 |
// | unused     |                                   |
// | reg-imm    | register  | 16-bit value          |
// | regs       | dest reg  | src reg a | src reg b |
//
// There are 32 registers:
// - a-z: general-purpose registers 0 through 25
//   - registers a-d have special purpose for syscalls:
//     - %a holds the syscall number
//     - b-d are arguments.
// - fl: flags register: register 31
// - ip: instruction pointer: register 30
// - sp: stack pointer: register 29
// 26-28 are currently reserved.
//
// The flags register has 4 flags:
// - zero: set if the last operation resulted in 0
// - carry: set if the last operation resulted in a carry
// - overflow: set if the last operation resulted in an overflow
// - sign: set if the last operation resulted in a negative number
//
// The instruction listing:
// | op name | opcode | argument scheme   | comment                                     |
// | ------- | ------ | ----------------- | ------------------------------------------- |
// | nop     | 00     | unused            | nothing                                     |
// | ldi     | 01     | reg-imm           | load immediate value                        |
// | ldr     | 02     | regs              | load value from address in src reg a to dest|
// | ldrb    | 03     | regs              | load low byte of value from address in src a|
// | add     | 04     | regs              | add src regs together                       |
// | sub     | 05     | regs              | sub src reg b from a                        |
// | mul     | 06     | regs              | multiply src reg a by b                     |
// | div     | 07     | regs              | divide src reg a by b                       |
// | mod     | 08     | regs              | modulo src reg a by b                       |
// | and     | 09     | regs              | bitwise and a and b                         |
// | or      | 0a     | regs              | bitwise or a and b                          |
// | xor     | 0b     | regs              | bitwise xor a and b                         |
// | shl     | 0c     | regs              | shift a left by b bits                      |
// | shr     | 0d     | regs              | shift a right by b bits                     |
// | jmp     | 0e     | reg-imm           | jump to immediate address                   |
// | jz      | 0f     | reg-imm           | jump to immediate address if zero flag set  |
// | jd      | 10     | reg-imm           | jump to value in register. value is ignored |
// | sti     | 11     | reg-imm           | store value in register to memory           |
// | stib    | 12     | reg-imm           | store low byte of value in register to mem  |
// | str     | 13     | regs              | store value in src reg a to address in dest |
// | strb    | 14     | regs              | store low byte of value in src reg a to mem |
// | sys     | 15     | unused            | syscall                                     |
//
// Memory layout:
// | address        | description |
// | -------------- | ----------- |
// | 0x0000-0x3fff  | code        |
// | 0x4000-0x5fff  | stack       |
// | 0x6000-0x9fff  | heap        |
// | 0xa000-0xdfff  | graphics    |
// | 0xe000-0xffff  | system      |

#define BURROW_OP_BIT_SIZE 8
#define BURROW_OP_NOP 0x00
#define BURROW_OP_LDI 0x01
#define BURROW_OP_LDR 0x02
#define BURROW_OP_LDRB 0x03
#define BURROW_OP_ADD 0x04
#define BURROW_OP_SUB 0x05
#define BURROW_OP_MUL 0x06
#define BURROW_OP_DIV 0x07
#define BURROW_OP_MOD 0x08
#define BURROW_OP_AND 0x09
#define BURROW_OP_OR 0x0a
#define BURROW_OP_XOR 0x0b
#define BURROW_OP_SHL 0x0c
#define BURROW_OP_SHR 0x0d
#define BURROW_OP_JMP 0x0e
#define BURROW_OP_JZ 0x0f
#define BURROW_OP_JD 0x10
#define BURROW_OP_STI 0x11
#define BURROW_OP_STIB 0x12
#define BURROW_OP_STR 0x13
#define BURROW_OP_STRB 0x14
#define BURROW_OP_SYS 0x15

#define BURROW_OP_COUNT 22

#define BURROW_REG_BIT_SIZE 16
#define BURROW_REG_COUNT 32
#define BURROW_REG_REPR_BIT_SIZE 8
#define BURROW_REG_A 0x00
#define BURROW_REG_B 0x01
#define BURROW_REG_C 0x02
#define BURROW_REG_D 0x03
#define BURROW_REG_E 0x04
#define BURROW_REG_F 0x05
#define BURROW_REG_G 0x06
#define BURROW_REG_H 0x07
#define BURROW_REG_I 0x08
#define BURROW_REG_J 0x09
#define BURROW_REG_K 0x0a
#define BURROW_REG_L 0x0b
#define BURROW_REG_M 0x0c
#define BURROW_REG_N 0x0d
#define BURROW_REG_O 0x0e
#define BURROW_REG_P 0x0f
#define BURROW_REG_Q 0x10
#define BURROW_REG_R 0x11
#define BURROW_REG_S 0x12
#define BURROW_REG_T 0x13
#define BURROW_REG_U 0x14
#define BURROW_REG_V 0x15
#define BURROW_REG_W 0x16
#define BURROW_REG_X 0x17
#define BURROW_REG_Y 0x18
#define BURROW_REG_Z 0x19
// special registers
#define BURROW_REG_FL 0x1f
#define BURROW_REG_IP 0x1e
#define BURROW_REG_SP 0x1d

#define BURROW_FL_NONE 0x00
#define BURROW_FL_ZERO 0x01     // 0000 0001
#define BURROW_FL_CARRY 0x02    // 0000 0010
#define BURROW_FL_OVERFLOW 0x04 // 0000 0100
#define BURROW_FL_SIGN 0x08     // 0000 1000
#define BURROW_FL_FIN 0x80      // 1000 0000

#define BURROW_MEM_SIZE 0x10000
#define BURROW_MEM_CODE_START 0x0000
#define BURROW_MEM_CODE_END 0x3fff
#define BURROW_MEM_STACK_START 0x4000
#define BURROW_MEM_STACK_END 0x5fff
#define BURROW_MEM_HEAP_START 0x6000
#define BURROW_MEM_HEAP_END 0x9fff
#define BURROW_MEM_GRAPHICS_START 0xa000
#define BURROW_MEM_GRAPHICS_END 0xdfff
#define BURROW_MEM_SYSTEM_START 0xe000
#define BURROW_MEM_SYSTEM_END 0xffff

#define BURROW_SYS_EXIT 0x00
#define BURROW_SYS_PRINT 0x01
#define BURROW_SYS_MAX_SYSCALLS 0x20
