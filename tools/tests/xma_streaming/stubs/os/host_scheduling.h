#pragma once
// Fixture stand-ins: thread placement does not affect decoding.
namespace os::scheduling
{
inline int g_preferPerformanceCoresCalls = 0;
inline int g_raiseAudioPriorityCalls = 0;
inline void PreferPerformanceCores(bool = false) { ++g_preferPerformanceCoresCalls; }
inline void RaiseAudioThreadPriority() { ++g_raiseAudioPriorityCalls; }
}
