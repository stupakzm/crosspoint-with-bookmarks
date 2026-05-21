#pragma once

#include <Epub.h>
#include <Epub/Section.h>

#include <memory>
#include <vector>

#include "BookmarkStore.h"
#include "activities/Activity.h"

class BookmarkPreviewActivity final : public Activity {
 public:
  explicit BookmarkPreviewActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                   std::shared_ptr<Epub> epub, BookmarkStore& store, int startIndex,
                                   uint8_t orientation)
      : Activity("BookmarkPreview", renderer, mappedInput),
        epub(std::move(epub)),
        store(store),
        viewIndex(startIndex),
        orientation(orientation) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  std::shared_ptr<Epub> epub;
  BookmarkStore& store;
  int viewIndex;
  uint8_t orientation;

  bool overlayVisible = false;
  int overlayOption = 0;  // 0=hide overlay, 1=go to position, 2=remove
  static constexpr int OVERLAY_OPTION_COUNT = 3;

  std::unique_ptr<Section> section = nullptr;
  int loadedSpineIndex = -1;

  void loadSectionForView();
  const char* overlayOptionLabel(int option) const;
};
