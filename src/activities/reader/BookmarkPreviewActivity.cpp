#include "BookmarkPreviewActivity.h"

#include <CrossPointSettings.h>
#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "MappedInputManager.h"
#include "ReaderUtils.h"
#include "activities/ActivityResult.h"
#include "components/UITheme.h"
#include "fontIds.h"

void BookmarkPreviewActivity::onEnter() {
  Activity::onEnter();
  ReaderUtils::applyOrientation(renderer, orientation);
  requestUpdate();
}

void BookmarkPreviewActivity::onExit() {
  Activity::onExit();
  renderer.setOrientation(GfxRenderer::Orientation::Portrait);
  section.reset();
}

void BookmarkPreviewActivity::loop() {
  const auto& bookmarks = store.getAll();

  if (overlayVisible) {
    // Back always exits to bookmark list
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      ActivityResult result;
      result.isCancelled = true;
      setResult(std::move(result));
      finish();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      switch (overlayOption) {
        case 0:  // Hide overlay
          overlayVisible = false;
          overlayOption = 0;
          requestUpdate();
          break;
        case 1:  // Go to position
          setResult(BookmarkPreviewResult{BookmarkPreviewResult::Action::GOTO, viewIndex});
          finish();
          break;
        case 2:  // Remove
          setResult(BookmarkPreviewResult{BookmarkPreviewResult::Action::REMOVE, viewIndex});
          finish();
          break;
      }
      return;
    }

    // Prev/Next cycle overlay options
    if (mappedInput.wasReleased(MappedInputManager::Button::PageBack) ||
        mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      overlayOption = (overlayOption - 1 + OVERLAY_OPTION_COUNT) % OVERLAY_OPTION_COUNT;
      requestUpdate();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      overlayOption = (overlayOption + 1) % OVERLAY_OPTION_COUNT;
      requestUpdate();
      return;
    }

  } else {
    // Overlay hidden: Back exits
    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      ActivityResult result;
      result.isCancelled = true;
      setResult(std::move(result));
      finish();
      return;
    }

    // Confirm shows overlay
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      overlayVisible = true;
      overlayOption = 0;
      requestUpdate();
      return;
    }

    // Prev/Next navigate between bookmarks
    if (mappedInput.wasReleased(MappedInputManager::Button::PageBack) ||
        mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      if (!bookmarks.empty()) {
        viewIndex = (viewIndex - 1 + static_cast<int>(bookmarks.size())) % static_cast<int>(bookmarks.size());
        section.reset();
        requestUpdate();
      }
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::PageForward) ||
        mappedInput.wasReleased(MappedInputManager::Button::Right)) {
      if (!bookmarks.empty()) {
        viewIndex = (viewIndex + 1) % static_cast<int>(bookmarks.size());
        section.reset();
        requestUpdate();
      }
      return;
    }
  }
}

void BookmarkPreviewActivity::loadSectionForView() {
  const auto& bookmarks = store.getAll();
  if (bookmarks.empty() || viewIndex < 0 || viewIndex >= static_cast<int>(bookmarks.size())) {
    return;
  }

  const int targetSpine = bookmarks[viewIndex].spineIndex;
  if (section && loadedSpineIndex == targetSpine) {
    return;
  }

  section.reset();
  section = std::unique_ptr<Section>(new Section(epub, targetSpine, renderer));

  // Compute viewport for current settings
  int marginTop = SETTINGS.screenMargin;
  int marginRight = SETTINGS.screenMargin;
  int marginBottom = SETTINGS.screenMargin;
  int marginLeft = SETTINGS.screenMargin;
  const uint16_t vpW = renderer.getScreenWidth() - marginLeft - marginRight;
  const uint16_t vpH = renderer.getScreenHeight() - marginTop - marginBottom;

  if (!section->loadSectionFile(SETTINGS.getReaderFontId(), SETTINGS.getReaderLineCompression(),
                                SETTINGS.extraParagraphSpacing, SETTINGS.paragraphAlignment, vpW, vpH,
                                SETTINGS.hyphenationEnabled, SETTINGS.embeddedStyle, SETTINGS.imageRendering,
                                SETTINGS.focusReadingEnabled)) {
    LOG_DBG("BKP", "Section not cached, building for spine %d", targetSpine);
    GUI.drawPopup(renderer, tr(STR_INDEXING));
    if (!section->createSectionFile(SETTINGS.getReaderFontId(), SETTINGS.getReaderLineCompression(),
                                    SETTINGS.extraParagraphSpacing, SETTINGS.paragraphAlignment, vpW, vpH,
                                    SETTINGS.hyphenationEnabled, SETTINGS.embeddedStyle, SETTINGS.imageRendering,
                                    SETTINGS.focusReadingEnabled, nullptr)) {
      LOG_ERR("BKP", "Failed to build section cache for spine %d", targetSpine);
      section.reset();
      return;
    }
  }

  loadedSpineIndex = targetSpine;
}

const char* BookmarkPreviewActivity::overlayOptionLabel(int option) const {
  switch (option) {
    case 0: return tr(STR_HIDE_OVERLAY);
    case 1: return tr(STR_GOTO_POSITION);
    case 2: return tr(STR_REMOVE_BOOKMARK);
    default: return "";
  }
}

void BookmarkPreviewActivity::render(RenderLock&&) {
  const auto& bookmarks = store.getAll();

  if (bookmarks.empty() || viewIndex < 0 || viewIndex >= static_cast<int>(bookmarks.size())) {
    renderer.clearScreen();
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, tr(STR_NO_BOOKMARKS), true);
    renderer.displayBuffer();
    return;
  }

  loadSectionForView();

  if (!section) {
    renderer.clearScreen();
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, tr(STR_PAGE_LOAD_ERROR), true);
    renderer.displayBuffer();
    return;
  }

  const auto& bm = bookmarks[viewIndex];
  const int targetPage = (bm.pageNumber < section->pageCount) ? bm.pageNumber : 0;
  section->currentPage = targetPage;

  auto metrics = UITheme::getInstance().getMetrics();

  // Reserve bottom space for overlay when visible
  const int overlayHeight = overlayVisible ? (metrics.tabBarHeight + metrics.buttonHintsHeight) : 0;

  int marginTop = SETTINGS.screenMargin;
  int marginRight = SETTINGS.screenMargin;
  int marginBottom = SETTINGS.screenMargin + overlayHeight;
  int marginLeft = SETTINGS.screenMargin;

  renderer.clearScreen();

  auto p = section->loadPageFromSectionFile();
  if (p) {
    auto* fcm = renderer.getFontCacheManager();
    auto scope = fcm->createPrewarmScope();
    p->render(renderer, SETTINGS.getReaderFontId(), marginLeft, marginTop);
    scope.endScanAndPrewarm();
    p->render(renderer, SETTINGS.getReaderFontId(), marginLeft, marginTop);
  }

  if (overlayVisible) {
    const int screenW = renderer.getScreenWidth();
    const int screenH = renderer.getScreenHeight();

    const std::string name = BookmarkStore::formatName(viewIndex, bm);
    const int nameBarY = screenH - overlayHeight;

    // Name bar
    GUI.drawSubHeader(renderer, Rect{0, nameBarY, screenW, metrics.tabBarHeight}, name.c_str());

    // Button hints: Back=exit, Confirm=activate selected option, Prev/Next=cycle options
    const char* prevOptionLabel = overlayOptionLabel((overlayOption - 1 + OVERLAY_OPTION_COUNT) % OVERLAY_OPTION_COUNT);
    const char* nextOptionLabel = overlayOptionLabel((overlayOption + 1) % OVERLAY_OPTION_COUNT);
    (void)prevOptionLabel;
    (void)nextOptionLabel;
    const auto labels =
        mappedInput.mapLabels(tr(STR_BACK), overlayOptionLabel(overlayOption), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  renderer.displayBuffer();
}
