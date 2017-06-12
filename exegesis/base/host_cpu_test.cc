#include "exegesis/base/host_cpu.h"

#include "exegesis/base/cpu_info.h"
#include "exegesis/proto/cpuid.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

#ifdef __x86_64__
TEST(HostCpuIdDumpTest, X86) {
  const CpuIdDumpProto& cpuid_dump = HostCpuIdDumpOrDie();
  EXPECT_TRUE(cpuid_dump.has_x86_cpuid_dump());
}

TEST(HostCpuInfoTest, X86) {
  const CpuInfo& cpu_info = HostCpuInfoOrDie();
  EXPECT_FALSE(cpu_info.cpu_model_id().empty());
  EXPECT_FALSE(cpu_info.supported_features().empty());
}
#endif  // __x86_64__

}  // namespace
}  // namespace exegesis
