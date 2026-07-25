#pragma once

#include <il2cpp/il2cpp_helper.h>

#include <cstdint>

struct NodeAddress {
public:
  __declspec(property(get = __get_System)) int64_t System;

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Models", "NodeAddress");
    return class_helper;
  }

public:
  int64_t __get_System()
  {
    static auto property = get_class_helper().GetProperty("System");
    auto        value    = property.Get<int64_t>(this);
    return value ? *value : -1;
  }
};
