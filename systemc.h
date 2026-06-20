#pragma once
#include <cstdint>
#include <cstring>
#include <cassert>

template<int W> struct sc_bv {
  uint32_t words[(W + 31) / 32] = {0};
  sc_bv() {}
  sc_bv(uint64_t v) { words[0] = static_cast<uint32_t>(v); if ((W + 31)/32 > 1) words[1] = static_cast<uint32_t>(v >> 32); }
  uint64_t to_uint64() const { uint64_t res = words[0]; if ((W + 31)/32 > 1) res |= (static_cast<uint64_t>(words[1]) << 32); return res; }
  uint32_t to_uint() const { return words[0]; }
  uint32_t get_word(int idx) const { if (idx < (W + 31) / 32) return words[idx]; return 0; }
  void set_word(int idx, uint32_t v) { if (idx < (W + 31) / 32) words[idx] = v; }
};

template<typename T> struct sc_signal {
  T val = T();
  void write(T v) { val = v; }
  T read() const { return val; }
  void operator=(T v) { val = v; }
  operator T() const { return val; }
};

struct sc_clock {
  sc_clock(const char*, int, int) {}
  sc_clock() {}
  bool pos() const { return true; }
  bool neg() const { return false; }
  bool posedge() const { return true; }
  bool negedge() const { return false; }
  operator bool() const { return true; }
  sc_clock* operator->() { return this; }
};

template<typename T> struct sc_in {
  T val = T();
  T read() const { return val; }
  bool pos() const { return true; }
  bool neg() const { return false; }
  void bind(T& v) { val = v; }
  void bind(sc_signal<T>& s) { val = s.val; }
  void operator()(T& v) { val = v; }
  void operator()(sc_signal<T>& s) { val = s.val; }
  void operator()(sc_clock& c) { }
  operator T() const { return val; }
  bool operator!() const { return !static_cast<bool>(val); }
  sc_clock* operator->() { static sc_clock c; return &c; }
};

template<typename T> struct sc_out {
  T val = T();
  void write(T v) { val = v; }
  T read() const { return val; }
  void bind(T& v) { val = v; }
  void bind(sc_signal<T>& s) { s.val = val; }
  void operator()(T& v) { val = v; }
  void operator()(sc_signal<T>& s) { s.val = val; }
  void operator=(T v) { val = v; }
  operator T() const { return val; }
  bool operator!() const { return !static_cast<bool>(val); }
};

typedef sc_in<bool> sc_in_clk;

struct sc_module_name {
  const char* name_;
  sc_module_name(const char* name) : name_(name) {}
  operator const char*() const { return name_; }
};

struct sc_module {
  const char* name_;
  sc_module(sc_module_name name) : name_(name.name_) {}
  sc_module(const char* name) : name_(name) {}
  const char* name() const { return name_; }
  void trace(void*, int) {}
};

struct DummySensitive {
  template<typename T> DummySensitive& operator<<(const T&) { return *this; }
};

#define SC_MODULE(name) struct name : public sc_module
#define SC_CTOR(name) name(const char* nm) : sc_module(nm)
#define SC_METHOD(func)
#define sensitive DummySensitive()

#define SC_HAS_PROCESS(name)

#define SC_NS 1
#define SC_ZERO_TIME 0
inline void sc_start(...) {}
inline void sc_stop() {}

namespace sc_core {
}
namespace sc_dt {
  using ::sc_bv;
}

#ifndef SC_NO_MAIN
// removed main wrapper
#endif
