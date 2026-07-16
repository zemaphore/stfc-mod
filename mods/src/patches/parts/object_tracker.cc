#include <il2cpp/il2cpp_helper.h>

#include "prime/AllianceStarbaseObjectViewerWidget.h"
#include "prime/AnimatedRewardsScreenViewController.h"
#include "prime/ArmadaObjectViewerWidget.h"
#include "prime/AssignShipsWidget.h"
#include "prime/CelestialObjectViewerWidget.h"
#include "prime/EmbassyObjectViewer.h"
#include "prime/FleetBarViewController.h"
#include "prime/FullScreenChatViewController.h"
#include "prime/HousingObjectViewerWidget.h"
#include "prime/InventoryListViewController.h"
#include "prime/MiningObjectViewerWidget.h"
#include "prime/OfficerAssignmentViewController.h"
#include "prime/MissionsObjectViewerWidget.h"
#include "prime/NavigationInteractionUIViewController.h"
#include "prime/PreScanTargetWidget.h"
#include "prime/ElementSelectorViewController.h"
#include "prime/StarNodeObjectViewerWidget.h"

#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/vector.h>
#include <spdlog/spdlog.h>
#include <spud/detour.h>

#include <algorithm>
#include <mutex>

std::mutex                                                   tracked_objects_mutex;
eastl::unordered_map<Il2CppClass*, eastl::vector<uintptr_t>> tracked_objects;
eastl::unordered_map<uintptr_t, Il2CppGCHandle>               tracked_object_handles;

void add_to_tracking_recursive(Il2CppClass* klass, void* _this)
{
  if (!klass) {
    return;
  }

  auto& tracked_object_vector = tracked_objects[klass];
  const auto object            = uintptr_t(_this);
  if (std::find(tracked_object_vector.begin(), tracked_object_vector.end(), object)
      == tracked_object_vector.end()) {
    tracked_object_vector.emplace_back(object);
  }

  return add_to_tracking_recursive(klass->parent, _this);
}

void* track_ctor(auto original, void* _this)
{
  auto obj = original(_this);
  if (_this == nullptr) {
    return _this;
  }

  auto cls = (Il2CppObject*)_this;
  if (cls->klass == nullptr) {
    return obj;
  }

  std::scoped_lock lk{tracked_objects_mutex};
  spdlog::trace("Tracking {}({})", _this, cls->klass->name);

  const auto object    = uintptr_t(_this);
  const auto handle_it = tracked_object_handles.find(object);
  if (handle_it == tracked_object_handles.end()) {
    auto handle = il2cpp_gchandle_new_weakref(cls, false);
    if (!handle) {
      spdlog::warn("Unable to create weak GC handle for {}({})", _this, cls->klass->name);
      return obj;
    }
    tracked_object_handles.emplace(object, handle);
  } else if (il2cpp_gchandle_get_target(handle_it->second) != cls) {
    il2cpp_gchandle_free(handle_it->second);
    handle_it->second = il2cpp_gchandle_new_weakref(cls, false);
    if (!handle_it->second) {
      tracked_object_handles.erase(handle_it);
      spdlog::warn("Unable to replace weak GC handle for {}({})", _this, cls->klass->name);
      return obj;
    }
  }

  add_to_tracking_recursive(cls->klass, _this);
  return obj;
}

static eastl::unordered_set<void*> seen_ctor;

template <typename T> void TrackObject()
{
  auto& object_class = T::get_class_helper();
  auto  klass        = object_class.get_cls();
  if (!klass) {
    spdlog::warn("Unable to track an unresolved IL2CPP class");
    return;
  }

  auto  ctor         = object_class.GetMethod(".ctor");
  if (!ctor) {
    spdlog::warn("Unable to track {}: constructor not found", klass->name);
    return;
  }

  if (seen_ctor.find(ctor) == seen_ctor.end()) {
    SPUD_STATIC_DETOUR(ctor, track_ctor);
    seen_ctor.emplace(ctor);
  }

  // Object lifetime is observed through a weak IL2CPP GC handle. Avoid detouring
  // inherited OnDestroy methods or the process-wide liveness finalizer.
}

void InstallObjectTrackers()
{
  TrackObject<PreScanTargetWidget>();
  TrackObject<FleetBarViewController>();
  TrackObject<AllianceStarbaseObjectViewerWidget>();
  TrackObject<AnimatedRewardsScreenViewController>();
  TrackObject<ArmadaObjectViewerWidget>();
  TrackObject<AssignShipsWidget>();
  TrackObject<CelestialObjectViewerWidget>();
  TrackObject<EmbassyObjectViewer>();
  TrackObject<FullScreenChatViewController>();
  TrackObject<HousingObjectViewerWidget>();
  TrackObject<InventoryListViewController>();
  TrackObject<MiningObjectViewerWidget>();
  TrackObject<MissionsObjectViewerWidget>();
  TrackObject<NavigationInteractionUIViewController>();
  TrackObject<OfficerAssignmentViewController>();
  TrackObject<ElementSelectorViewController>();
  TrackObject<StarNodeObjectViewerWidget>();
}
