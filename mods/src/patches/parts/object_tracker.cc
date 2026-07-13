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

#include <mutex>

std::mutex                                                   tracked_objects_mutex;
eastl::unordered_map<Il2CppClass*, eastl::vector<uintptr_t>> tracked_objects;

void add_to_tracking_recursive(Il2CppClass* klass, void* _this)
{
  if (!klass) {
    return;
  }

  auto& tracked_object_vector = tracked_objects[klass];
  tracked_object_vector.emplace_back(uintptr_t(_this));

  return add_to_tracking_recursive(klass->parent, _this);
}

void remove_from_tracking_all(void* _this)
{
#define GET_CLASS(obj) ((Il2CppClass*)(((size_t)obj) & ~(size_t)1))
  for (auto& [klass, tracked_object_vector] : tracked_objects) {
    tracked_object_vector.erase_first(uintptr_t(_this));
  }
#undef GET_CLASS
}

void remove_from_tracking_recursive(Il2CppClass* klass, void* _this)
{
#define GET_CLASS(obj) ((Il2CppClass*)(((size_t)obj) & ~(size_t)1))
  if (!GET_CLASS(klass)) {
    return;
  }

  if (tracked_objects.find(klass) == tracked_objects.end()) {
    return;
  }

  auto& tracked_object_vector = tracked_objects[GET_CLASS(klass->parent)];
  tracked_object_vector.erase_first(uintptr_t(_this));
  return remove_from_tracking_recursive(GET_CLASS(klass->parent), _this);
#undef GET_CLASS
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
  // Object death is caught by the il2cpp liveness sweep (calc_liveness_hook). We do
  // NOT register a Boehm GC finalizer (it runs during GC_finish_collection and would
  // have to mutate tracked_objects without holding tracked_objects_mutex — locking
  // there deadlocks against the stop-the-world collector — which races and corrupts
  // the container), nor detour OnDestroy (a shared inherited base method that macOS
  // will not tolerate being hooked).
  add_to_tracking_recursive(cls->klass, _this);
  return obj;
}

void track_free(auto original, void* _this)
{
#define GET_CLASS(obj) ((Il2CppClass*)(((size_t)obj) & ~(size_t)1))
  if (_this != nullptr) {
    std::scoped_lock lk{tracked_objects_mutex};
    auto             cls = (Il2CppObject*)_this;
    remove_from_tracking_all(_this);
    return original(_this);
  }
#undef GET_CLASS
}

void calc_liveness_hook(auto original, void* state)
{
  original(state);

  std::scoped_lock                                    lk{tracked_objects_mutex};
  eastl::vector<eastl::pair<Il2CppClass*, uintptr_t>> objects_to_free;
  eastl::unordered_set<uintptr_t>                     objects_seen;
#define IS_MARKED(obj) (((size_t)(obj)->klass) & (size_t)1)
  for (auto& [klass, objects] : tracked_objects) {
    for (auto object : objects) {
      if (IS_MARKED((Il2CppObject*)object) && objects_seen.find(object) == objects_seen.end()) {
        objects_to_free.emplace_back(klass, object);
        objects_seen.emplace(object);
      }
    }
  }

#undef IS_MARKED

#define GET_CLASS(obj) ((Il2CppClass*)(((size_t)obj) & ~(size_t)1))
  for (auto& [klass, object] : objects_to_free) {
    spdlog::trace("Clearing {}({})", (void*)object, GET_CLASS(klass)->name);
    remove_from_tracking_all((void*)object);
  }
#undef GET_CLASS

  tracked_objects = tracked_objects;
}

static eastl::unordered_set<void*> seen_ctor;

template <typename T> void TrackObject()
{
  auto& object_class = T::get_class_helper();
  auto  ctor         = object_class.GetMethod(".ctor");
  if (seen_ctor.find(ctor) == seen_ctor.end()) {
    SPUD_STATIC_DETOUR(ctor, track_ctor);
    seen_ctor.emplace(ctor);
  }

  // NOTE: OnDestroy is intentionally not detoured. Most tracked types do not declare
  // their own OnDestroy; GetMethod resolves it to a shared inherited base method, and
  // detouring that (repeatedly / with a fragile trampoline) crashes on macOS. Object
  // death is instead caught by the il2cpp liveness sweep (calc_liveness_hook).
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

  SPUD_STATIC_DETOUR(il2cpp_unity_liveness_finalize, calc_liveness_hook);
}
