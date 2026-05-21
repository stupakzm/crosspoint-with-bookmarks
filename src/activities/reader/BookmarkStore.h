#pragma once

#include <string>
#include <vector>

struct Bookmark {
  int spineIndex = 0;
  int pageNumber = 0;
  int pageCount = 0;
};

class BookmarkStore {
 public:
  void load(const std::string& cachePath);
  bool save() const;

  void add(const Bookmark& b);
  void remove(int sortedIndex);

  const std::vector<Bookmark>& getAll() const { return bookmarks; }
  bool hasAt(int spineIndex, int pageNumber) const;
  // Returns 0-based sorted index for the given position, or -1 if not present.
  int sortedIndexAt(int spineIndex, int pageNumber) const;

  // "Bookmark-{1basedIdx}-Chapter {spineIndex+1}-{pageNumber+1}/{pageCount}"
  static std::string formatName(int sortedIndex, const Bookmark& b);

 private:
  std::vector<Bookmark> bookmarks;
  std::string filePath;

  static bool compareBookmarks(const Bookmark& a, const Bookmark& b);
};
