#include <os/performance_hint_policy.h>
#include <cassert>
#include <cstdio>

int main()
{
    using os::performance_hint::Policy;
    Policy policy;                     // 50% of the frame by default
    assert(policy.TargetNs() == int64_t(1000.0 / 30.0 * 1e6 * 0.5));
    for (int i = 0; i < 60; ++i) policy.ObserveInterval(16.7);
    assert(policy.Period() < 17.0 && policy.TargetNs() == int64_t(1000.0 / 60.0 * 1e6 * 0.5));
    for (int i = 0; i < 60; ++i) policy.ObserveInterval(33.3);
    assert(policy.Period() > 33.0);   // back to the 30 FPS class
    policy.ObserveInterval(5000.0);    // a loading hitch does not move the class
    policy.ObserveInterval(-1.0);
    assert(policy.Period() > 33.0);
    // The busiest thread is the frame's work; stalls are capped at four periods.
    assert(policy.WorkNs(9'000'000, 12'000'000) == 12'000'000);
    assert(policy.WorkNs(-5, 0) == 0);
    assert(policy.WorkNs(2'000'000'000, 0) == int64_t(policy.Period() * 4e6));
    assert(Policy(5).Percent() == 20 && Policy(99).Percent() == 90);
    std::printf("PASS performance hint policy: frame class, target, work and caps\n");
}
