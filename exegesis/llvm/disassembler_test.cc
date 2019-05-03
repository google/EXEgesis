// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "exegesis/llvm/disassembler.h"

#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(DisassemblerTest, ComInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; 660F2F0C25FF7F0000; comisd xmm1, qword ptr [0x7fff]; "
      "comisd 0x7fff, %xmm1; COMISDrm",
      disasm.DisassembleHexString("660F2F0C25FF7F0000"));
  EXPECT_EQ(
      "00000000; C5F92F0C25FF7F0000; vcomisd xmm1, qword ptr [0x7fff]; "
      "vcomisd 0x7fff, %xmm1; VCOMISDrm",
      disasm.DisassembleHexString("C5F92F0C25FF7F0000"));
  EXPECT_EQ(
      "00000000; 0F2F0C25FF7F0000; comiss xmm1, dword ptr [0x7fff]; comiss "
      "0x7fff, %xmm1; COMISSrm",
      disasm.DisassembleHexString("0F2F0C25FF7F0000"));
  EXPECT_EQ(
      "00000000; C5F82F0C25FF7F0000; vcomiss xmm1, dword ptr [0x7fff]; "
      "vcomiss 0x7fff, %xmm1; VCOMISSrm",
      disasm.DisassembleHexString("C5F82F0C25FF7F0000"));
}

TEST(DisassemblerTest, VcvtInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; C5FBE60C25FF7F0000; vcvtpd2dq xmm1, xmmword ptr [0x7fff]; "
      "vcvtpd2dqx 0x7fff, %xmm1; VCVTPD2DQrm",
      disasm.DisassembleHexString("C5FBE60C25FF7F0000"));
  EXPECT_EQ(
      "00000000; C5FFE60C25FF7F0000; vcvtpd2dq xmm1, ymmword ptr [0x7fff]; "
      "vcvtpd2dqy 0x7fff, %xmm1; VCVTPD2DQYrm",
      disasm.DisassembleHexString("C5FFE60C25FF7F0000"));
  EXPECT_EQ(
      "00000000; C5F95A0C25FF7F0000; vcvtpd2ps xmm1, xmmword ptr [0x7fff]; "
      "vcvtpd2psx 0x7fff, %xmm1; VCVTPD2PSrm",
      disasm.DisassembleHexString("C5F95A0C25FF7F0000"));
  EXPECT_EQ(
      "00000000; C5F9E60C25FF7F0000; vcvttpd2dq xmm1, xmmword ptr [0x7fff]; "
      "vcvttpd2dqx 0x7fff, %xmm1; VCVTTPD2DQrm",
      disasm.DisassembleHexString("C5F9E60C25FF7F0000"));
}

TEST(DisassemblerTest, Bcd80BitInstructions) {
  Disassembler disasm("");
  // Note(bdb): This is incorrectly disassembled as operations on 32-bit
  // values, whereas the encoding is for 80-bit BCD values.
  // TODO(bdb): file a bug against LLVM.
  EXPECT_EQ(
      "00000000; DF2425FF7F0000; fbld tbyte ptr [0x7fff]; fbld 0x7fff; FBLDm",
      disasm.DisassembleHexString("DF2425FF7F0000"));
  EXPECT_EQ(
      "00000000; DF3425FF7F0000; fbstp tbyte ptr [0x7fff]; fbstp 0x7fff; "
      "FBSTPm",
      disasm.DisassembleHexString("DF3425FF7F0000"));
}

TEST(DisassemblerTest, CompareStringInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; a6; cmpsb byte ptr [rsi], byte ptr es:[rdi]; cmpsb "
      "%es:(%rdi), (%rsi); CMPSB",
      disasm.DisassembleHexString("a6"));
  EXPECT_EQ(
      "00000000; 66a7; cmpsw word ptr [rsi], word ptr es:[rdi]; cmpsw "
      "%es:(%rdi), (%rsi); CMPSW",
      disasm.DisassembleHexString("66a7"));
  EXPECT_EQ(
      "00000000; a7; cmpsd dword ptr [rsi], dword ptr es:[rdi]; cmpsl "
      "%es:(%rdi), (%rsi); CMPSL",
      disasm.DisassembleHexString("a7"));
  EXPECT_EQ(
      "00000000; 48a7; cmpsq qword ptr [rsi], qword ptr es:[rdi]; cmpsq "
      "%es:(%rdi), (%rsi); CMPSQ",
      disasm.DisassembleHexString("48a7"));
}

TEST(DisassemblerTest, InInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; 6c; insb byte ptr es:[rdi], dx; insb %dx, %es:(%rdi); INSB",
      disasm.DisassembleHexString("6c"));
  EXPECT_EQ(
      "00000000; 666d; insw word ptr es:[rdi], dx; insw %dx, %es:(%rdi); INSW",
      disasm.DisassembleHexString("666d"));
  EXPECT_EQ(
      "00000000; 6d; insd dword ptr es:[rdi], dx; insl %dx, %es:(%rdi); INSL",
      disasm.DisassembleHexString("6d"));
  EXPECT_EQ(
      "00000000; f36c; rep  insb byte ptr es:[rdi], dx; "
      "rep  insb %dx, %es:(%rdi); INSB",
      disasm.DisassembleHexString("f36c"));
  EXPECT_EQ(
      "00000000; f3666d; rep  insw word ptr es:[rdi], dx; rep  "
      "insw %dx, %es:(%rdi); INSW",
      disasm.DisassembleHexString("f3666d"));
  EXPECT_EQ(
      "00000000; f36d; rep  insd dword ptr es:[rdi], dx; rep  "
      "insl %dx, %es:(%rdi); INSL",
      disasm.DisassembleHexString("f36d"));
  EXPECT_EQ(
      "00000000; f26c; repne  insb byte ptr es:[rdi], dx; "
      "repne  insb %dx, %es:(%rdi); INSB",
      disasm.DisassembleHexString("f26c"));
  EXPECT_EQ(
      "00000000; f26d; repne  insd dword ptr es:[rdi], dx; "
      "repne  insl %dx, %es:(%rdi); INSL",
      disasm.DisassembleHexString("f26d"));
  EXPECT_EQ(
      "00000000; f2666d; repne  insw word ptr es:[rdi], dx; "
      "repne  insw %dx, %es:(%rdi); INSW",
      disasm.DisassembleHexString("f2666d"));
}

TEST(DisassemblerTest, OutInstructions) {
  Disassembler disasm("");
  EXPECT_EQ("00000000; 6e; outsb dx, byte ptr [rsi]; outsb (%rsi), %dx; OUTSB",
            disasm.DisassembleHexString("6e"));
  EXPECT_EQ(
      "00000000; 666f; outsw dx, word ptr [rsi]; outsw (%rsi), %dx; OUTSW",
      disasm.DisassembleHexString("666f"));
  EXPECT_EQ("00000000; 6f; outsd dx, dword ptr [rsi]; outsl (%rsi), %dx; OUTSL",
            disasm.DisassembleHexString("6f"));
}

TEST(DisassemblerTest, LoadStringInstructions) {
  Disassembler disasm("");
  EXPECT_EQ("00000000; ac; lodsb al, byte ptr [rsi]; lodsb (%rsi), %al; LODSB",
            disasm.DisassembleHexString("ac"));
  EXPECT_EQ(
      "00000000; ad; lodsd eax, dword ptr [rsi]; lodsl (%rsi), %eax; LODSL",
      disasm.DisassembleHexString("ad"));
  EXPECT_EQ(
      "00000000; 66ad; lodsw ax, word ptr [rsi]; lodsw (%rsi), %ax; LODSW",
      disasm.DisassembleHexString("66ad"));
  EXPECT_EQ(
      "00000000; 48ad; lodsq rax, qword ptr [rsi]; lodsq (%rsi), %rax; LODSQ",
      disasm.DisassembleHexString("48ad"));
}

TEST(DisassemblerTest, ScanStringInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; ae; scasb al, byte ptr es:[rdi]; scasb %es:(%rdi), %al; SCASB",
      disasm.DisassembleHexString("ae"));
  EXPECT_EQ(
      "00000000; 66af; scasw ax, word ptr es:[rdi]; scasw %es:(%rdi), %ax; "
      "SCASW",
      disasm.DisassembleHexString("66af"));
  EXPECT_EQ(
      "00000000; af; scasd eax, dword ptr es:[rdi]; scasl %es:(%rdi), %eax; "
      "SCASL",
      disasm.DisassembleHexString("af"));
  EXPECT_EQ(
      "00000000; 48af; scasq rax, qword ptr es:[rdi]; scasq %es:(%rdi), %rax; "
      "SCASQ",
      disasm.DisassembleHexString("48af"));
}

TEST(DisassemblerTest, StoreStringInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; aa; stosb byte ptr es:[rdi], al; stosb %al, %es:(%rdi); STOSB",
      disasm.DisassembleHexString("aa"));
  EXPECT_EQ(
      "00000000; ab; stosd dword ptr es:[rdi], eax; stosl %eax, %es:(%rdi); "
      "STOSL",
      disasm.DisassembleHexString("ab"));
  EXPECT_EQ(
      "00000000; 66ab; stosw word ptr es:[rdi], ax; stosw %ax, %es:(%rdi); "
      "STOSW",
      disasm.DisassembleHexString("66ab"));
}

TEST(DisassemblerTest, MovsAndMovsd) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; a5; movsd dword ptr es:[rdi], dword ptr [rsi]; movsl (%rsi), "
      "%es:(%rdi); MOVSL",
      disasm.DisassembleHexString("a5"));
}

TEST(DisassemblerTest, FpComparisonInstructions) {
  Disassembler disasm("");
  EXPECT_EQ("00000000; DFF3; fcompi st, st(3); fcompi %st(3), %st; COM_FIPr",
            disasm.DisassembleHexString("DFF3"));
  EXPECT_EQ("00000000; DFEB; fucompi st, st(3); fucompi %st(3), %st; UCOM_FIPr",
            disasm.DisassembleHexString("DFEB"));
}

TEST(DisassemblerTest, LoadSegmentLimitInstruction) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; 440F030C25FF7F0000; lsl r9d, word ptr [0x7fff]; lsll 0x7fff, "
      "%r9d; LSL32rm",
      disasm.DisassembleHexString("440F030C25FF7F0000"));
  EXPECT_EQ("00000000; 4D0F03C9; lsl r9, r9d; lslq %r9d, %r9; LSL64rr",
            disasm.DisassembleHexString("4D0F03C9"));
  EXPECT_EQ(
      "00000000; 4C0F030C25FF7F0000; lsl r9, word ptr [0x7fff];"
      " lslq 0x7fff, %r9; LSL64rm",
      disasm.DisassembleHexString("4C0F030C25FF7F0000"));
}

TEST(DisassemblerTest, PUnpackInstructions) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; 0F603425FF7F0000; punpcklbw mm6, dword ptr [0x7fff]; "
      "punpcklbw 0x7fff, %mm6; MMX_PUNPCKLBWirm",
      disasm.DisassembleHexString("0F603425FF7F0000"));
  EXPECT_EQ(
      "00000000; 0F623425FEFFFF7F; punpckldq mm6, dword ptr [0x7ffffffe]; "
      "punpckldq 0x7ffffffe, %mm6; MMX_PUNPCKLDQirm",
      disasm.DisassembleHexString("0F623425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; 0F613425FEFFFF7F; punpcklwd mm6, dword ptr [0x7ffffffe]; "
      "punpcklwd 0x7ffffffe, %mm6; MMX_PUNPCKLWDirm",
      disasm.DisassembleHexString("0F613425FEFFFF7F"));
}

TEST(DisassemblerTest, XlatInstruction) {
  Disassembler disasm("");
  EXPECT_EQ("00000000; D7; xlatb; xlatb; XLAT",
            disasm.DisassembleHexString("D7"));
}

TEST(DisassemblerTest, Mov64BitImmediate) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; 48B85634129078563412; movabs rax, 0x1234567890123456; movabsq "
      "$0x1234567890123456, %rax; MOV64ri",
      disasm.DisassembleHexString("48B85634129078563412"));
  EXPECT_EQ(
      "00000000; b8FEFFFF7F; mov eax, 0x7ffffffe; movl $0x7ffffffe, %eax; "
      "MOV32ri",
      disasm.DisassembleHexString("b8FEFFFF7F"));
  EXPECT_EQ(
      "00000000; 48c7c0FEFFFF7F; mov rax, 0x7ffffffe; movq $0x7ffffffe, %rax; "
      "MOV64ri32",
      disasm.DisassembleHexString("48c7c0FEFFFF7F"));
}

TEST(DisassemblerTest, FpStatusAndEnvironment) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; D92425FEFFFF7F; fldenv dword ptr [0x7ffffffe]; fldenv "
      "0x7ffffffe; FLDENVm",
      disasm.DisassembleHexString("D92425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; DD3425FEFFFF7F; fnsave dword ptr [0x7ffffffe]; fnsave "
      "0x7ffffffe; FSAVEm",
      disasm.DisassembleHexString("DD3425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; D93425FEFFFF7F; fnstenv dword ptr [0x7ffffffe]; fnstenv "
      "0x7ffffffe; FSTENVm",
      disasm.DisassembleHexString("D93425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; DD3C25FEFFFF7F; fnstsw word ptr [0x7ffffffe]; fnstsw "
      "0x7ffffffe; FNSTSWm",
      disasm.DisassembleHexString("DD3C25FEFFFF7F"));
  EXPECT_EQ(
      "00000000; DD2425FEFFFF7F; frstor dword ptr [0x7ffffffe]; frstor "
      "0x7ffffffe; FRSTORm",
      disasm.DisassembleHexString("DD2425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; DD3425FEFFFF7F; fnsave dword ptr [0x7ffffffe]; fnsave "
      "0x7ffffffe; FSAVEm",
      disasm.DisassembleHexString("DD3425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; D93425FEFFFF7F; fnstenv dword ptr [0x7ffffffe]; fnstenv "
      "0x7ffffffe; FSTENVm",
      disasm.DisassembleHexString("D93425FEFFFF7F"));
  EXPECT_EQ(
      "00000000; DD3C25FEFFFF7F; fnstsw word ptr [0x7ffffffe]; fnstsw "
      "0x7ffffffe; FNSTSWm",
      disasm.DisassembleHexString("DD3C25FEFFFF7F"));
  EXPECT_EQ(
      "00000000; c7c0FEFFFF7F; mov eax, 0x7ffffffe; movl $0x7ffffffe, %eax; "
      "MOV32ri_alt",
      disasm.DisassembleHexString("c7c0FEFFFF7F"));
}

TEST(DisassemblerTest, InvalidateTLBEntry) {
  Disassembler disasm("");
  EXPECT_EQ(
      "00000000; 0F013C25FEFFFF7F; invlpg byte ptr [0x7ffffffe]; invlpg "
      "0x7ffffffe; INVLPG",
      disasm.DisassembleHexString("0F013C25FEFFFF7F"));
}

TEST(DisassemblerTest, TooShortABuffer) {
  Disassembler disasm("");
  unsigned llvm_opcode;
  std::string llvm_mnemonic;
  std::vector<std::string> llvm_operands;
  std::string intel_instruction;
  std::string att_instruction;
  std::vector<uint8_t> full = {0x66, 0x0F, 0x2F, 0x0C, 0x25,
                               0xFF, 0x7F, 0x00, 0x00};

  // Sanity check.
  EXPECT_EQ(full.size(), disasm.Disassemble(full, &llvm_opcode, &llvm_mnemonic,
                                            &llvm_operands, &intel_instruction,
                                            &att_instruction));
  EXPECT_EQ("COMISDrm", llvm_mnemonic);

  // This one is empty.
  EXPECT_EQ(0,
            disasm.Disassemble({}, &llvm_opcode, &llvm_mnemonic, &llvm_operands,
                               &intel_instruction, &att_instruction));

  // This one is a Length-Changing Prefix.
  EXPECT_EQ(1, disasm.Disassemble(
                   std::vector<uint8_t>(full.begin(), full.begin() + 1),
                   &llvm_opcode, &llvm_mnemonic, &llvm_operands,
                   &intel_instruction, &att_instruction));
  EXPECT_EQ("DATA16_PREFIX", llvm_mnemonic);

  for (auto end = full.begin() + 2; end != full.rbegin().base(); ++end) {
    EXPECT_EQ(0,
              disasm.Disassemble(std::vector<uint8_t>(full.begin(), end),
                                 &llvm_opcode, &llvm_mnemonic, &llvm_operands,
                                 &intel_instruction, &att_instruction));
  }
}

}  // namespace
}  // namespace exegesis
