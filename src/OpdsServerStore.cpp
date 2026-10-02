#include "OpdsServerStore.h"

#include <Logging.h>
#include <ObfuscationUtils.h>

#include <algorithm>
#include <cstring>
#include <string_view>

namespace {
constexpr char FLIBUSTA_URL[] = "https://flibusta.is/opds";

bool isFlibustaCatalog(std::string_view url) {
  if (url.starts_with("https://")) {
    url.remove_prefix(8);
  } else if (url.starts_with("http://")) {
    url.remove_prefix(7);
  }
  while (url.ends_with('/')) url.remove_suffix(1);
  return url == "flibusta.is/opds" || url == "flibusta.site/opds";
}
}  // namespace

void OpdsServerStore::toJson(JsonDocument& doc) const {
  doc["flibusta_preset_applied"] = flibustaPresetApplied;
  JsonArray arr = doc["servers"].to<JsonArray>();
  for (const auto& server : servers) {
    JsonObject obj = arr.add<JsonObject>();
    obj["name"] = server.name;
    obj["url"] = server.url;
    obj["username"] = server.username;
    obj["password_obf"] = obfuscation::obfuscateToBase64(server.password);
  }
}

bool OpdsServerStore::fromJson(JsonVariantConst doc) {
  // Tolerate a missing/invalid 'servers' key (treat as empty list); only a
  // JSON parse error is fatal. A null JsonArray iterates zero times.
  servers.clear();
  flibustaPresetApplied = doc["flibusta_preset_applied"] | false;
  JsonArrayConst arr = doc["servers"].as<JsonArrayConst>();
  servers.reserve(std::min(arr.size(), MAX_SERVERS));
  bool needsResave = false;

  for (JsonObjectConst obj : arr) {
    if (servers.size() >= OpdsServerStore::MAX_SERVERS) break;
    OpdsServer server;
    server.name = obj["name"] | "";
    server.url = obj["url"] | "";
    server.username = obj["username"] | "";
    server.password = extractPassword(obj, needsResave);
    servers.push_back(std::move(server));
  }

  LOG_DBG("OPS", "Loaded %zu OPDS servers from file", servers.size());

  if (needsResave) {
    LOG_DBG("OPS", "Resaving JSON with obfuscated passwords");
    requestResave();
  }

  return true;
}

bool OpdsServerStore::ensureFlibustaPreset() {
  if (flibustaPresetApplied) return true;
  const bool alreadyConfigured = std::any_of(servers.begin(), servers.end(),
                                             [](const OpdsServer& server) { return isFlibustaCatalog(server.url); });
  if (!alreadyConfigured && servers.size() >= MAX_SERVERS) {
    LOG_INF("OPS", "Flibusta preset deferred: all catalog slots are in use");
    return false;
  }
  if (!alreadyConfigured) {
    // One cold-path allocation in the existing bounded server store. Owning
    // strings allow the preset to be edited through the normal settings UI.
    servers.reserve(servers.size() + 1);
    servers.push_back({"Flibusta", FLIBUSTA_URL, "", ""});
  }
  flibustaPresetApplied = true;
  if (saveToFile()) return true;
  flibustaPresetApplied = false;
  if (!alreadyConfigured) servers.pop_back();
  LOG_ERR("OPS", "Could not save Flibusta preset");
  return false;
}

bool OpdsServerStore::addServer(const OpdsServer& server) {
  if (servers.size() >= MAX_SERVERS) {
    LOG_DBG("OPS", "Cannot add more servers, limit of %zu reached", MAX_SERVERS);
    return false;
  }

  servers.push_back(server);
  LOG_DBG("OPS", "Added server: %s", server.name.c_str());
  return saveToFile();
}

bool OpdsServerStore::updateServer(size_t index, const OpdsServer& server) {
  if (index >= servers.size()) {
    return false;
  }

  servers[index] = server;
  LOG_DBG("OPS", "Updated server: %s", server.name.c_str());
  return saveToFile();
}

bool OpdsServerStore::removeServer(size_t index) {
  if (index >= servers.size()) {
    return false;
  }

  LOG_DBG("OPS", "Removed server: %s", servers[index].name.c_str());
  servers.erase(servers.begin() + static_cast<ptrdiff_t>(index));
  return saveToFile();
}

const OpdsServer* OpdsServerStore::getServer(size_t index) const {
  if (index >= servers.size()) {
    return nullptr;
  }
  return &servers[index];
}
