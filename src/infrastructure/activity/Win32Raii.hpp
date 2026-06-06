#pragma once

#include <oleauto.h>
#include <windows.h>

#include <utility>

namespace chronexa::win32 {

// RAII owner of a kernel HANDLE (process, thread, ...). Closes it with
// CloseHandle on destruction.
class UniqueHandle {
public:
  UniqueHandle() = default;
  explicit UniqueHandle(HANDLE handle) : _handle(handle) {}
  ~UniqueHandle() { reset(); }

  UniqueHandle(const UniqueHandle &) = delete;
  UniqueHandle &operator=(const UniqueHandle &) = delete;

  UniqueHandle(UniqueHandle &&other) noexcept : _handle(std::exchange(other._handle, nullptr)) {}
  UniqueHandle &operator=(UniqueHandle &&other) noexcept {
    if (this != &other) {
      reset();
      _handle = std::exchange(other._handle, nullptr);
    }
    return *this;
  }

  [[nodiscard]] HANDLE get() const { return _handle; }
  explicit operator bool() const { return _handle != nullptr && _handle != INVALID_HANDLE_VALUE; }

  void reset(HANDLE handle = nullptr) {
    if (*this) {
      CloseHandle(_handle);
    }
    _handle = handle;
  }

private:
  HANDLE _handle = nullptr;
};

// RAII owner of a WinEvent hook installed with SetWinEventHook. Removes it
// with UnhookWinEvent on destruction.
class UniqueWinEventHook {
public:
  UniqueWinEventHook() = default;
  explicit UniqueWinEventHook(HWINEVENTHOOK hook) : _hook(hook) {}
  ~UniqueWinEventHook() { reset(); }

  UniqueWinEventHook(const UniqueWinEventHook &) = delete;
  UniqueWinEventHook &operator=(const UniqueWinEventHook &) = delete;

  UniqueWinEventHook(UniqueWinEventHook &&other) noexcept : _hook(std::exchange(other._hook, nullptr)) {}
  UniqueWinEventHook &operator=(UniqueWinEventHook &&other) noexcept {
    if (this != &other) {
      reset();
      _hook = std::exchange(other._hook, nullptr);
    }
    return *this;
  }

  explicit operator bool() const { return _hook != nullptr; }

  void reset(HWINEVENTHOOK hook = nullptr) {
    if (_hook) {
      UnhookWinEvent(_hook);
    }
    _hook = hook;
  }

private:
  HWINEVENTHOOK _hook = nullptr;
};

// RAII owner of a VARIANT. Frees the contained value (including BSTRs)
// with VariantClear on destruction.
class ScopedVariant {
public:
  ScopedVariant() { VariantInit(&_value); }
  ~ScopedVariant() { VariantClear(&_value); }

  ScopedVariant(const ScopedVariant &) = delete;
  ScopedVariant &operator=(const ScopedVariant &) = delete;

  static ScopedVariant fromBstr(const wchar_t *text) {
    ScopedVariant variant;
    variant._value.vt = VT_BSTR;
    variant._value.bstrVal = SysAllocString(text);
    return variant;
  }

  static ScopedVariant fromInt(const long number) {
    ScopedVariant variant;
    variant._value.vt = VT_I4;
    variant._value.lVal = number;
    return variant;
  }

  ScopedVariant(ScopedVariant &&other) noexcept : _value(other._value) { VariantInit(&other._value); }
  ScopedVariant &operator=(ScopedVariant &&other) noexcept {
    if (this != &other) {
      VariantClear(&_value);
      _value = other._value;
      VariantInit(&other._value);
    }
    return *this;
  }

  [[nodiscard]] const VARIANT &get() const { return _value; }
  // Out-parameter access for APIs that fill the VARIANT.
  [[nodiscard]] VARIANT *receive() {
    VariantClear(&_value);
    return &_value;
  }

private:
  VARIANT _value;
};

} // namespace chronexa::win32
