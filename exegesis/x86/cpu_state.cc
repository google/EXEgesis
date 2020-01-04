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

#include "exegesis/x86/cpu_state.h"

#include <cstdio>
#include <cstring>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "exegesis/util/bits.h"
#include "glog/logging.h"

namespace exegesis {

std::string FPUControlWord::DebugString() const {
  return absl::StrFormat(
      "%i bits precision, rounding: %s, enabled exceptions: %s%s%s%s%s%s",
      GetPrecision(), GetRoundingMode(),
      raw_value_ & 0x0020 ? "Precision|" : "",        //
      raw_value_ & 0x0010 ? "Underflow|" : "",        //
      raw_value_ & 0x0008 ? "Overflow|" : "",         //
      raw_value_ & 0x0004 ? "ZeroDivide|" : "",       //
      raw_value_ & 0x0002 ? "DenormalOperand|" : "",  //
      raw_value_ & 0x0001 ? "InvalidOperation" : "");
}

int FPUControlWord::GetPrecision() const {
  const auto value = GetBitRange(raw_value_, 8, 10);
  switch (value) {
    case 0x00:
      return 24;
    case 0x02:
      return 53;
    case 0x03:
      return 64;
    default:
      return 0;
  }
  return 0;
}

const char* FPUControlWord::GetRoundingMode() const {
  const auto value = GetBitRange(raw_value_, 10, 12);
  switch (value) {
    case 0x00:
      return "nearest";
    case 0x01:
      return "down";
    case 0x02:
      return "up";
    case 0x03:
      return "towards zero";
    default:
      LOG(FATAL) << "Invalid value " << value << " for rounding mode";
  }
  return nullptr;
}

std::string FPUStatusWord::DebugString() const {
  return absl::StrFormat(
      "Busy: %i, Condition Code: 0x%x, top: %i, Err: %i StackFail: %i, "
      "exceptions: %s%s%s%s%s%s",
      !!(raw_value_ & 15),                            //
      GetConditionCode(), GetStackTop(),              //
      !!(raw_value_ & 7),                             //
      !!(raw_value_ & 6),                             //
      raw_value_ & 0x0020 ? "Precision|" : "",        //
      raw_value_ & 0x0010 ? "Underflow|" : "",        //
      raw_value_ & 0x0008 ? "Overflow|" : "",         //
      raw_value_ & 0x0004 ? "ZeroDivide|" : "",       //
      raw_value_ & 0x0002 ? "DenormalOperand|" : "",  //
      raw_value_ & 0x0001 ? "InvalidOperation" : "");
}

int FPUStatusWord::GetStackTop() const {
  return GetBitRange(raw_value_, 11, 14);
}

int FPUStatusWord::GetConditionCode() const {
  return GetBitRange(raw_value_, 14, 15) << 3 | GetBitRange(raw_value_, 8, 11);
}

std::string FPUTagWord::DebugString() const {
  std::string buffer;
  for (int i = 0; i < 8; ++i) {
    absl::StrAppend(&buffer, "ST(", i, "): ", GetStatus(i));
    if (i < 7) {
      absl::StrAppend(&buffer, ", ");
    }
  }
  return buffer;
}

const char* FPUTagWord::GetStatus(int i) const {
  CHECK_GE(i, 0);
  CHECK_LE(i, 7);
  const auto value = GetBitRange(raw_value_, 2 * i, 2 * i + 2);
  switch (value) {
    case 0x00:
      return "valid";
    case 0x01:
      return "zero";
    case 0x02:
      return "special";
    case 0x03:
      return "empty";
    default:
      LOG(FATAL) << "Invalid value " << value << " for FPU tag";
  }
  return nullptr;
}

FXStateBuffer::FXStateBuffer() {
  // MSan does not know that this buffer is filled since this happens directly
  // in assembly. Initializing the buffer to avoid a false positive.
  std::memset(get(), 0, size);
}

std::string FXStateBuffer::DebugString() const {
  const auto control = GetFPUControlWord();
  const auto status = GetFPUStatusWord();
  const auto tag = GetAbridgedFPUTagWord();
  return absl::StrFormat(
      R"(
    FPU Control Word:                                          0x%.4X
        (%s)
    FPU Status Word:                                           0x%.4X
        (%s)
    Abridged FPU Tag Word:                                       0x%.2X
        (%s)
    FPU Opcode:                                                0x%.4X
    FPU Instruction Pointer Selector:              0x%.16X
    FPU Instruction Operand (Data) Pointer Offset: 0x%.16X
    MXCSR Register State:                                  0x%.8X
    MXCSR_MASK:                                            0x%.8X
    )",
      control.raw_value_, control.DebugString(), status.raw_value_,
      status.DebugString(), tag.raw_value_, tag.DebugString(), GetFPUOpCode(),
      GetFPUInstructionPointerOffset(),
      GetFPUInstructionOperandDataPointerOffset(), GetMXCSRRegisterState(),
      GetMXCSR_MASK());
}

}  // namespace exegesis
