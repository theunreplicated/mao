//
// Copyright 2008 Google Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301, USA.

#ifndef __MAODEFS_H_INCLUDED
#define __MAODEFS_H_INCLUDED

#include <stdio.h>

// Bitmasks for operands

#define  DEF_OP0      (1 << 0)
#define  DEF_OP1      (1 << 1)
#define  DEF_OP2      (1 << 2)
#define  DEF_OP3      (1 << 3)
#define  DEF_OP4      (1 << 4)
#define  DEF_OP5      (1 << 5)

#define  DEF_OP_ALL (DEF_OP0 | DEF_OP1 | DEF_OP2 | DEF_OP3 | DEF_OP4 | DEF_OP5)

// Bitmasks for registers

// 8-bit
#define   REG_AL      (1 <<  0)
#define   REG_CL      (1 <<  1)
#define   REG_DL      (1 <<  2)
#define   REG_BL      (1 <<  3)
#define   REG_AH      (1 <<  4)
#define   REG_CH      (1 <<  5)
#define   REG_DH      (1 <<  6)
#define   REG_BH      (1 <<  7)

// 16-bit
#define   REG_AX      (1 <<  8)
#define   REG_CX      (1 <<  9)
#define   REG_DX      (1 << 10)
#define   REG_BX      (1 << 11)
#define   REG_SP      (1 << 12)
#define   REG_BP      (1 << 13)
#define   REG_SI      (1 << 14)
#define   REG_DI      (1 << 15)

// 32-bit
#define   REG_EAX     (1 << 16)
#define   REG_ECX     (1 << 17)
#define   REG_EDX     (1 << 18)
#define   REG_EBX     (1 << 19)
#define   REG_ESP     (1 << 20)
#define   REG_EBP     (1 << 21)
#define   REG_ESI     (1 << 22)
#define   REG_EDI     (1 << 23)

// 64-bit
#define   REG_RAX     (1 << 24)
#define   REG_RCX     (1 << 25)
#define   REG_RDX     (1 << 26)
#define   REG_RBX     (1 << 27)
#define   REG_RSP     (1 << 28)
#define   REG_RBP     (1 << 29)
#define   REG_RSI     (1 << 30)
#define   REG_RDI     (1 << 31)

#define   REG_R8      (1ULL << 32)
#define   REG_R9      (1ULL << 33)
#define   REG_R10     (1ULL << 34)
#define   REG_R11     (1ULL << 35)
#define   REG_R12     (1ULL << 36)
#define   REG_R13     (1ULL << 37)
#define   REG_R14     (1ULL << 38)
#define   REG_R15     (1ULL << 39)

#define   REG_ALL     (-1ULL)
// more are possible...

typedef struct DefEntry {
  int                opcode;      // matches table gen-opcodes.h
  unsigned int       op_mask;     // if insn defs operand(s)
  unsigned long long reg_mask;    // if insn defs register(s)
  unsigned long long reg_mask8;   //   for  8-bit addressing modes
  unsigned long long reg_mask16;  //   for 16-bit addressing modes
  unsigned long long reg_mask32;  //   for 32-bit addressing modes
  unsigned long long reg_mask64;  //   for 64-bit addressing modes
};
extern DefEntry def_entries[];

// external entry points
class InstructionEntry;

unsigned long long GetRegisterDefMask(InstructionEntry *insn);
void               PrintRegisterDefMask(unsigned long long mask, FILE *f);
unsigned long long GetMaskForRegister(const char *reg);

#endif // __MAODEFS_H_INCLUDED