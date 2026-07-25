#pragma once

#include <il2cpp/il2cpp_helper.h>

#include <cstdint>

struct LocaleTextContext {
public:
  static LocaleTextContext* Create(const char* identifier, const char* category)
  {
    auto&       helper  = get_class_helper();
    static auto ctor    = helper.GetMethod<void(LocaleTextContext*, Il2CppString*, Il2CppString*)>(".ctor", 2);
    auto        context = helper.New<LocaleTextContext>();
    if (!ctor || !context) {
      return nullptr;
    }

    ctor(context, il2cpp_string_new(identifier), il2cpp_string_new(category));
    return context;
  }

  bool ApplyIdentifierParameter(int64_t value)
  {
    static auto method =
        get_class_helper().GetMethod<void(LocaleTextContext*, Il2CppArray*)>("ApplyIdentifierParameters", 1);
    static auto object_class = il2cpp_get_class_helper("mscorlib", "System", "Object").get_cls();
    static auto int64_class  = il2cpp_get_class_helper("mscorlib", "System", "Int64").get_cls();
    static auto array_class  = object_class ? il2cpp_array_class_get(object_class, 1) : nullptr;

    if (!method || !int64_class || !array_class) {
      return false;
    }

    auto parameters = il2cpp_array_new_specific(array_class, 1);
    auto boxed      = il2cpp_value_box(int64_class, &value);
    if (!parameters || !boxed) {
      return false;
    }

    reinterpret_cast<Il2CppObject**>(reinterpret_cast<Il2CppArraySize*>(parameters)->vector)[0] = boxed;
    method(this, parameters);
    return true;
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper = il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.UI", "LocaleTextContext");
    return class_helper;
  }
};

struct LocaleUtilities {
public:
  static bool HasTranslation(LocaleTextContext* context)
  {
    static auto method = get_class_helper().GetMethod<bool(LocaleTextContext*)>("HasTranslation", 1);
    return method && context && method(context);
  }

  static Il2CppString* Localize(LocaleTextContext* context)
  {
    static auto method = get_class_helper().GetMethod<Il2CppString*(LocaleTextContext*, bool, bool)>("Localize", 3);
    return method && context ? method(context, false, false) : nullptr;
  }

private:
  static IL2CppClassHelper& get_class_helper()
  {
    static auto class_helper =
        il2cpp_get_class_helper("Assembly-CSharp", "Digit.Client.Localization", "LocaleUtilities");
    return class_helper;
  }
};
