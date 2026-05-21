#include "BookmarkStore.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>

static constexpr char BOOKMARKS_FILENAME[] = "/bookmarks.json";

bool BookmarkStore::compareBookmarks(const Bookmark& a, const Bookmark& b) {
  if (a.spineIndex != b.spineIndex) return a.spineIndex < b.spineIndex;
  return a.pageNumber < b.pageNumber;
}

void BookmarkStore::load(const std::string& cachePath) {
  bookmarks.clear();
  filePath = cachePath + BOOKMARKS_FILENAME;

  if (!Storage.exists(filePath.c_str())) {
    return;
  }

  String raw = Storage.readFile(filePath.c_str());
  if (raw.isEmpty()) {
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, raw.c_str()) != DeserializationError::Ok) {
    LOG_ERR("BKM", "Failed to parse bookmarks JSON");
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    Bookmark bm;
    bm.spineIndex = obj["s"] | 0;
    bm.pageNumber = obj["p"] | 0;
    bm.pageCount = obj["c"] | 0;
    bookmarks.push_back(bm);
  }

  std::sort(bookmarks.begin(), bookmarks.end(), compareBookmarks);
  LOG_DBG("BKM", "Loaded %u bookmarks", (unsigned)bookmarks.size());
}

bool BookmarkStore::save() const {
  if (filePath.empty()) {
    LOG_ERR("BKM", "Cannot save: filePath not set (load() not called)");
    return false;
  }

  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& bm : bookmarks) {
    JsonObject obj = arr.add<JsonObject>();
    obj["s"] = bm.spineIndex;
    obj["p"] = bm.pageNumber;
    obj["c"] = bm.pageCount;
  }

  String json;
  serializeJson(doc, json);
  const bool ok = Storage.writeFile(filePath.c_str(), json);
  if (!ok) {
    LOG_ERR("BKM", "Failed to write bookmarks file");
  }
  return ok;
}

void BookmarkStore::add(const Bookmark& b) {
  if (hasAt(b.spineIndex, b.pageNumber)) {
    return;
  }
  bookmarks.push_back(b);
  std::sort(bookmarks.begin(), bookmarks.end(), compareBookmarks);
  save();
}

void BookmarkStore::remove(int sortedIndex) {
  if (sortedIndex < 0 || sortedIndex >= static_cast<int>(bookmarks.size())) {
    return;
  }
  bookmarks.erase(bookmarks.begin() + sortedIndex);
  save();
}

bool BookmarkStore::hasAt(int spineIndex, int pageNumber) const {
  return sortedIndexAt(spineIndex, pageNumber) != -1;
}

int BookmarkStore::sortedIndexAt(int spineIndex, int pageNumber) const {
  for (int i = 0; i < static_cast<int>(bookmarks.size()); i++) {
    if (bookmarks[i].spineIndex == spineIndex && bookmarks[i].pageNumber == pageNumber) {
      return i;
    }
  }
  return -1;
}

std::string BookmarkStore::formatName(int sortedIndex, const Bookmark& b) {
  return "Bookmark-" + std::to_string(sortedIndex + 1) + "-Chapter " + std::to_string(b.spineIndex + 1) + "-" +
         std::to_string(b.pageNumber + 1) + "/" + std::to_string(b.pageCount);
}
