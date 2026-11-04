#pragma once

#include <cstdio>
#include <cstdlib>

namespace pro_buzzer_test {
inline unsigned checks = 0;
inline bool optical_pass = true;

// Only optical I/O is stubbed; playback and the alarm automations are real.
struct OpticalStub {
  bool perform_self_test() { return optical_pass; }
  bool status_has_error() { return false; }
  bool is_failed() { return false; }
};

inline void check(bool condition, const char *expression, int line) {
  if (!condition) {
    std::fprintf(stderr, "Check failed at line %d: %s\n", line, expression);
    std::exit(1);
  }
  ++checks;
}
inline void finish() {
  std::printf("PRO_BUZZER_TESTS_PASSED: %u checks\n", checks);
  std::fflush(stdout);
  std::exit(0);
}
}  // namespace pro_buzzer_test

#define PRO_CHECK(condition) pro_buzzer_test::check(bool(condition), #condition, __LINE__)
