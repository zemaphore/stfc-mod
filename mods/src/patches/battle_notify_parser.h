#pragma once

#include <cstdint>
#include <optional>
#include <string>

struct Toast;

struct BattleExportData {
  int64_t     id;
  std::string result;
  std::string receivedAtUtc;
};

// Cache completed fleet-battle metadata as soon as its toast reaches the client.
// receivedAtUtc is the local machine's UTC clock at receipt, with millisecond precision.
void battle_notify_capture(Toast* toast);

// Return the most recently received completed battle for the requested player fleet.
std::optional<BattleExportData> battle_notify_latest_for_fleet(int64_t fleetId);

// Attempt to build a detailed notification body from a battle toast's Data.
// Returns empty string if the toast has no battle data or parsing fails.
std::string battle_notify_parse(Toast* toast);
