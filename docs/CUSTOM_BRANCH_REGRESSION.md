# Custom Branch Feature and Regression Guide

This document records the behavior added by the `custom` branch before it is merged with upstream. It is the acceptance
checklist for confirming that the fork-specific behavior survives the merge.

Comparison baseline:

- Common upstream base: `ff1e5aa`
- Pre-merge custom head: `05f7cd8`
- Range containing the custom work: `ff1e5aa..05f7cd8`

The merge commit `ad9999b` only brought an earlier upstream `dev` into the branch; it does not define an additional custom
feature.

## Required behavior at a glance

| Area | Behavior that must survive the merge |
| --- | --- |
| Dock shortcuts | Dock selection is state-based and deterministic, not dependent on the `select_timer` double-tap window. |
| Cargo export | Every valid dock shortcut press rewrites `community_patch_cargo.csv` with that fleet's cargo, ship, system, and position. |
| Gifts shortcut | The default binding is `K` and opens the Gifts/Chests shop tab through the game's Gifts deep link. |
| Object tracking | Tracked UI objects use weak IL2CPP GC handles without GC-finalizer, liveness-finalizer, or inherited `OnDestroy` detours. |

## 1. Deterministic dock shortcuts

The `select_ship1` through `select_ship8` shortcuts default to `1` through `8`. When keyboard input is not focused in a text
field, their required behavior is:

| Input | Expected result |
| --- | --- |
| Dock key for a dock that is not selected | Select the dock and open its drawer. |
| Same dock key again while that dock is selected | Load its system if needed and center on the ship. |
| `Shift` + dock key | Select the dock/open its drawer only, even if it was already selected. |

There is no time limit between the first and second presses. Whether to locate is determined only by the dock's current
selection state. The `select_timer` setting remains in the inherited configuration but must not affect this custom dock-key
behavior.

`Shift` is allowed in addition to an otherwise unmodified dock binding. Other unexpected modifiers must not make a plain
dock binding match. Explicitly configured modifiers must continue to follow the normal shortcut matching rules.

This behavior intentionally replaces the old `Shift` + dock Discovery-tow path. A regression test must verify that
`Shift` + dock selects only and does not initiate a tow.

The existing preview guard remains applicable. With `disable_preview_locate = true` and a hideable preview visible, a
repeated plain dock press must not locate the fleet. It may locate again once the preview no longer blocks the action.

### Regression checklist

- [ ] Press a different dock key once: the dock is selected without changing to its system view.
- [ ] Wait longer than the configured `select_timer`, then press it again: the ship is still located.
- [ ] Press an already-selected dock with `Shift`: the drawer remains selected and no locate occurs.
- [ ] Confirm that `Shift` + dock does not initiate a Discovery tow.
- [ ] Focus chat or another text input and press a dock key: no dock action or cargo export occurs.
- [ ] Exercise the `disable_preview_locate` behavior with a target preview open.
- [ ] Verify custom dock bindings, including one with an explicit modifier.

## 2. Cargo CSV export

Every valid dock shortcut interaction calls the cargo exporter before selecting or locating the fleet. This includes the
first selection press, a repeated locate press, and `Shift` + dock. The file is truncated and rewritten on every successful
export, so it always describes one fleet rather than accumulating history.

Filename:

```text
community_patch_cargo.csv
```

On macOS it is written under:

```text
~/Library/Preferences/com.stfcmod.startrekpatch/
```

On Windows `File::MakePath` leaves the filename relative to the game process's working directory.

The file is semicolon-delimited and contains exactly this header:

```text
dock;currentCargo;protectedCargo;totalCargo;shipName;systemName;systemId;x;y
```

Field definitions:

| Field | Meaning and format |
| --- | --- |
| `dock` | One-based dock number (`1` through `8`). |
| `currentCargo` | Current value from `UnprotectedCargoProgress`, written with zero decimal places. |
| `protectedCargo` | Maximum protected-cargo value, written with zero decimal places. |
| `totalCargo` | Maximum unprotected/total cargo value, written with zero decimal places. |
| `shipName` | The fleet hull's displayed name; empty when unavailable. |
| `systemName` | Localized system name in the current client language; empty when lookup or translation fails. |
| `systemId` | Numeric system ID, or `-1` when no address is available. |
| `x` | `SystemPosition.x`, written with two decimal places. |
| `y` | `SystemPosition.z`, written with two decimal places because the game world uses the XZ plane. |

A docked fleet reports its station position, so the coordinate columns are expected to remain populated while docked. The
export currently writes values directly and does not implement CSV quoting or escaping.

If the fleet, cargo hold, or either cargo progress object cannot be resolved, the exporter returns without rewriting the
existing file. A file-open failure is logged as a warning and must not disrupt the dock action.

### Regression checklist

- [ ] Delete or move the old CSV, press each populated dock key, and confirm the file is created in the expected directory.
- [ ] Confirm each press replaces the previous row instead of appending another row.
- [ ] Verify all nine columns and the exact header/order shown above.
- [ ] Compare current, protected, and total cargo values with the selected fleet in game.
- [ ] Check a named ship and confirm `shipName` matches.
- [ ] Check fleets in two systems and confirm localized `systemName` and numeric `systemId` change correctly.
- [ ] Compare `x`/`y` with an in-game or bookmark position, including a docked fleet.
- [ ] Confirm both a repeated plain dock press and `Shift` + dock refresh the file.
- [ ] Press an empty/unavailable dock and confirm there is no crash or corrupt partial rewrite.

The bindings most vulnerable to IL2CPP drift are `FleetPlayerData._cargoHoldData`, `Address`, `SystemPosition`,
`GameWorldManager.TryGetGalaxyNode`, `GalaxyNode.TranslationId`, and the locale utility methods. A blank name or unchanged
CSV after the merge should be investigated against those bindings first.

## 3. Gifts shortcut and navigation

The default `show_gifts` shortcut changes from `/` to `K`. User-supplied TOML overrides still take precedence over the
default.

The shortcut follows `ShortcutsManager`'s configured Gifts deep link and calls the shop-opening path with the Chests shop
category. It should land directly on the Gifts tab, matching the HUD `CLAIM` button, rather than opening the generic Offers
tab. If the deep-link instance, field, or method is unavailable, it falls back to the inherited generic shop-section
navigation and must not crash.

### Regression checklist

- [ ] With a newly generated/default configuration, press `K` and confirm it opens the Gifts tab directly.
- [ ] Confirm `/` is no longer the default Gifts binding.
- [ ] Set a custom `show_gifts` binding and confirm the override still works.
- [ ] Open Gifts repeatedly from several sections and confirm navigation remains responsive.
- [ ] Check logs for missing `_giftsDeepLink` or `OpenShop` bindings after a game update.

## 4. Weak-handle object tracking and macOS stability

The custom object tracker exists to support hotkeys that need to find live Unity view controllers and widgets. The inherited
implementation could crash macOS during startup or garbage collection because it mutated tracking state from a Boehm GC
finalizer and installed repeated detours on shared inherited `OnDestroy` methods.

The custom lifetime model is:

- Hook each distinct tracked constructor once.
- Create one weak IL2CPP GC handle per object address.
- Store the object under its concrete class and tracked parent classes without duplicate entries.
- Resolve a weak handle before returning an object from `ObjectFinder::Get` or `GetAll`.
- Remove dead handles and stale addresses while holding `tracked_objects_mutex`.
- Replace a stale handle safely if IL2CPP reuses an object address.
- Do not install a GC finalizer, `il2cpp_unity_liveness_finalize` detour, or tracked-type `OnDestroy` detours.

### Regression checklist

- [ ] Launch and quit the game repeatedly on macOS ARM64 without `SIGILL`, `SIGSEGV`, or `SIGBUS` during bootstrap/GC.
- [ ] Move repeatedly between galaxy, system, station, chat, inventory, officer, and reward views.
- [ ] Open and close target previews repeatedly, then exercise primary/secondary/recall actions.
- [ ] Verify dock selection and locate continue working after several screen transitions.
- [ ] Verify `Ctrl-F` focus search works in supported views after opening and closing them repeatedly.
- [ ] Verify officer left/right navigation and chat focus still find the currently live controller.
- [ ] Dismiss the animated rewards screen with the configured action/Escape shortcut.
- [ ] Leave the client running through multiple natural GC cycles and confirm stale views are not acted upon.
- [ ] Check logs for unresolved tracked classes, missing constructors, or weak-handle creation failures.
- [ ] Perform at least a launch and hotkey smoke test on Windows because the tracking code is shared.

## Merge review map

These files carry the fork-specific intent and deserve explicit review after resolving upstream changes:

| Files | Intent to preserve |
| --- | --- |
| `mods/src/patches/parts/hotkeys.cc` | Deterministic dock state machine, export on every dock press, and Gifts deep-link navigation. |
| `mods/src/patches/mapkey.h`, `mapkey.cc` | Allow `Shift` with plain dock bindings without accepting unrelated modifiers. |
| `mods/src/patches/parts/object_tracker.cc`, `mods/src/il2cpp/il2cpp_helper.h` | Mutex-protected weak-handle lifetime tracking and stale-object cleanup. |
| `mods/src/prime/FleetPlayerData.h` | Cargo, address, hull, and system-position accessors. |
| `mods/src/prime/GalaxyNode.h`, `GameWorldManager.h`, `LocaleUtilities.h`, `NodeAddress.h` | System ID/name/coordinate lookup used by CSV export. |
| `mods/src/prime/ShortcutsManager.h` | Direct Gifts deep-link wrapper and safe fallback signal. |
| `mods/src/defaultconfig.h`, `example_community_patch_settings.toml` | Default Gifts binding of `K`. |

The predicted upstream merge conflicts in `hotkeys.cc`, `object_tracker.cc`, `defaultconfig.h`, and the example TOML overlap
directly with this behavior. Conflict resolution is not complete until each corresponding checklist section passes.

## Custom commit inventory

| Commit | Intent |
| --- | --- |
| `f123626` | Deterministic dock behavior, Shift-aware dock matching, and initial cargo export. |
| `877dbd9` | Direct Gifts deep link and default binding change to `K`. |
| `388ed6c` | Remove crash-prone GC-finalizer and inherited `OnDestroy` tracking paths. |
| `4718e17` | Ensure cargo export runs on every dock shortcut press. |
| `40a729a` | Stabilize object lifetime tracking with weak IL2CPP GC handles. |
| `cddcbb3` | Add ship name to the cargo CSV. |
| `059087b` | Add localized system name to the cargo CSV. |
| `05f7cd8` | Add system ID and XZ-plane coordinates to the cargo CSV. |
