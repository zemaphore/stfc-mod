#pragma once

#include "GalaxyNode.h"
#include "MonoSingleton.h"

#include <il2cpp/il2cpp_helper.h>

#include <cstdint>

struct GameWorldManager : MonoSingleton<GameWorldManager> {
  friend struct MonoSingleton<GameWorldManager>;

public:
  bool TryGetGalaxyNode(int64_t node_id, GalaxyNode* galaxy_node)
  {
    static auto method =
        get_class_helper().GetMethod<bool(GameWorldManager*, int64_t, GalaxyNode*)>("TryGetGalaxyNode", 2);
    static bool warn = true;

    if (method) {
      return method(this, node_id, galaxy_node);
    }
    if (warn) {
      warn = false;
      ErrorMsg::MissingMethod("GameWorldManager", "TryGetGalaxyNode");
    }
    return false;
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.Navigation", "GameWorldManager");
    return class_helper;
  }
};
