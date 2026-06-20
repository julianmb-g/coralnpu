#pragma once
#define ABSL_FLAG(type, name, default_val, help) type FLAGS_##name = default_val;

#ifndef MOCK_GETFLAG
#define MOCK_GETFLAG
namespace absl { template <typename T> T GetFlag(const T& f) { return f; } }
#endif
