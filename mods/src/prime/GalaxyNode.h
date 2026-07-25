#pragma once

#include <il2cpp/il2cpp_helper.h>

#include <cstdint>

struct NullableInt64 {
  bool    has_value = false;
  uint8_t padding[7]{};
  int64_t value = 0;
};
static_assert(sizeof(NullableInt64) == 16);

struct GalaxyNode {
  void*   galaxy = nullptr;
  int32_t index  = 0;

  bool TryGetTranslationId(int64_t* translation_id)
  {
    static auto method = get_class_helper().GetMethod<NullableInt64(GalaxyNode*)>("get_TranslationId");
    if (!method || !translation_id) {
      return false;
    }

    const auto result = method(this);
    if (!result.has_value) {
      return false;
    }

    *translation_id = result.value;
    return true;
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Digit.Client.PrimeLib.Runtime", "Digit.PrimeServer.Models", "GalaxyNode");
    return class_helper;
  }
};
static_assert(sizeof(GalaxyNode) == 16);
