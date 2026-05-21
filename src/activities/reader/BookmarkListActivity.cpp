#include "BookmarkListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "activities/ActivityResult.h"
#include "components/UITheme.h"
#include "fontIds.h"

void BookmarkListActivity::onEnter() {
  Activity::onEnter();
  selectedIndex = 0;
  requestUpdate();
}

void BookmarkListActivity::onExit() { Activity::onExit(); }

void BookmarkListActivity::loop() {
  if (bookmarks.empty()) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      ActivityResult result;
      result.isCancelled = true;
      setResult(std::move(result));
      finish();
    }
    return;
  }

  const int total = static_cast<int>(bookmarks.size());
  const int pageItems = UITheme::getInstance().getNumberOfItemsPerPage(renderer, true, false, true, false);

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    setResult(BookmarkListResult{selectedIndex});
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  buttonNavigator.onNextRelease([this, total] {
    selectedIndex = ButtonNavigator::nextIndex(selectedIndex, total);
    requestUpdate();
  });

  buttonNavigator.onPreviousRelease([this, total] {
    selectedIndex = ButtonNavigator::previousIndex(selectedIndex, total);
    requestUpdate();
  });

  buttonNavigator.onNextContinuous([this, total, pageItems] {
    selectedIndex = ButtonNavigator::nextPageIndex(selectedIndex, total, pageItems);
    requestUpdate();
  });

  buttonNavigator.onPreviousContinuous([this, total, pageItems] {
    selectedIndex = ButtonNavigator::previousPageIndex(selectedIndex, total, pageItems);
    requestUpdate();
  });
}

void BookmarkListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  auto metrics = UITheme::getInstance().getMetrics();
  Rect screen = UITheme::getInstance().getScreenSafeArea(renderer, true, false);

  GUI.drawHeader(renderer, Rect{screen.x, screen.y + metrics.topPadding, screen.width, metrics.headerHeight},
                 bookTitle.c_str(), tr(STR_MANAGE_BOOKMARKS));

  const int contentTop = screen.y + metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = screen.height - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  if (bookmarks.empty()) {
    renderer.drawCenteredText(UI_12_FONT_ID, screen.y + screen.height / 2, tr(STR_NO_BOOKMARKS), true);
  } else {
    GUI.drawList(renderer, Rect{screen.x, contentTop, screen.width, contentHeight},
                 static_cast<int>(bookmarks.size()), selectedIndex, [this](int index) {
                   return BookmarkStore::formatName(index, bookmarks[index]);
                 });
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
