#pragma once

#include <string>
#include <vector>

#include "BookmarkStore.h"
#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

class BookmarkListActivity final : public Activity {
 public:
  explicit BookmarkListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                const std::vector<Bookmark>& bookmarks, const std::string& bookTitle)
      : Activity("BookmarkList", renderer, mappedInput), bookmarks(bookmarks), bookTitle(bookTitle) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  const std::vector<Bookmark>& bookmarks;
  std::string bookTitle;
  int selectedIndex = 0;
  ButtonNavigator buttonNavigator;
};
