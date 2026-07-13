#pragma once

#include "errormsg.h"
#include <il2cpp/il2cpp_helper.h>

#include "MonoSingleton.h"

// Wraps Digit.Prime.GameInput.ShortcutsManager, the game's keybind dispatcher.
// OpenGifts() reproduces the HUD "CLAIM" button / OnGiftsAction handler by
// following the game's own pre-configured gifts deep link straight to the
// GIFTS shop tab, rather than a bare section change that lands on the wrong tab.
struct ShortcutsManager : MonoSingleton<ShortcutsManager> {
  friend struct MonoSingleton<ShortcutsManager>;

public:
  // ShopCategory.Values.Chests — selects the GIFTS tab inside the shop.
  static constexpr int kShopCategoryChests = 2;

  // Returns true if the gifts shop was opened via the game's deep-link path.
  static bool OpenGifts()
  {
    auto instance = Instance();
    if (instance == nullptr) {
      return false;
    }

    auto gifts_deep_link = instance->GiftsDeepLink();
    if (gifts_deep_link == nullptr) {
      return false;
    }

    // private static void OpenShop(ShopDeepLink shopDeepLink, ShopCategory shopCategory)
    // ShopCategory is a 4-byte IDataEnum struct passed by value == its int value.
    static auto OpenShopMethod = get_class_helper().GetMethod<void(void*, int)>("OpenShop", 2);
    static auto OpenShopWarn   = true;
    if (OpenShopMethod == nullptr) {
      if (OpenShopWarn) {
        OpenShopWarn = false;
        ErrorMsg::MissingStaticMethod("ShortcutsManager", "OpenShop");
      }
      return false;
    }

    OpenShopMethod(gifts_deep_link, kShopCategoryChests);
    return true;
  }

private:
  // ShopDeepLink _giftsDeepLink; // serialized asset already targeting gifts
  void* GiftsDeepLink()
  {
    static auto field    = get_class_helper().GetField("_giftsDeepLink");
    static auto fieldWarn = true;
    if (!field.isValidHelper()) {
      if (fieldWarn) {
        fieldWarn = false;
        ErrorMsg::MissingMethod("ShortcutsManager", "_giftsDeepLink");
      }
      return nullptr;
    }
    return *(void**)((uintptr_t)this + field.offset());
  }

  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Assembly-CSharp", "Digit.Prime.GameInput", "ShortcutsManager");
    return class_helper;
  }
};
