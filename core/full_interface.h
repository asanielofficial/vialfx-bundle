// VialFX - Licensed under GPL-3.0-or-later.
// Derived from Vital's FullInterface by Matt Tytel (GPL-3.0): same OpenGL host, popups and skin
// handling, but hosting a single effect section under a slim top bar instead of the whole synth.
#pragma once

#include "JuceHeader.h"

#include "effect_header.h"
#include "memory.h"
#include "open_gl_background.h"
#include "popup_browser.h"
#include "shaders.h"
#include "synth_section.h"
#include "wavetable_creator.h"

class AboutSection;
class DeleteSection;
class PresetBrowser;
class SaveSection;
class SettingsSection;
struct SynthGuiData;

class FullInterface : public SynthSection, public EffectHeader::Listener,
                      public OpenGLRenderer, DragAndDropContainer, private Timer {
  public:
    static constexpr double kMinOpenGlVersion = 1.4;
    static constexpr int kTopHeight = 48;
    static constexpr float kDesignWidth = 600.0f;

    explicit FullInterface(SynthGuiData* synth_gui_data);
    ~FullInterface() override;

    // Window size (at scale 1.0) that fits the hosted effect section exactly.
    static Point<int> getDesignSize();

    void setOscilloscopeMemory(const vital::poly_float*) { }
    void setAudioMemory(const vital::StereoMemory* memory) { if (header_) header_->setAudioMemory(memory); }

    void paintBackground(Graphics& g) override;
    void copySkinValues(const Skin& skin);
    void reloadSkin(const Skin& skin);
    // Called after this plugin publishes its skin to the bundle, so it doesn't reload its own change.
    void markBundleSkinApplied();

    void repaintChildBackground(SynthSection* child);
    void repaintOpenGlBackground(OpenGlComponent* component);
    void redoBackground();
    void checkShouldReposition(bool resize = true);
    void parentHierarchyChanged() override {
      SynthSection::parentHierarchyChanged();
      checkShouldReposition();
    }
    void resized() override;
    void animate(bool animate) override;
    void reset() override;
    void setAllValues(vital::control_map& controls) override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    // EffectHeader::Listener
    void showAboutSection() override;
    void deleteRequested(File preset) override;
    void setPresetBrowserVisibility(bool visible) override;
    void bankImported() override;
    void showSettings(Component* anchor) override;

    void setFocus() { }
    void notifyChange();
    void notifyFresh();
    void externalPresetLoaded(const File& preset);
    void openSaveDialog();

    // Popups used by Vital's sliders, buttons and selectors.
    void popupBrowser(SynthSection* owner, Rectangle<int> bounds, std::vector<File> directories,
                      String extensions, std::string passthrough_name, std::string additional_folders_name);
    void popupBrowserUpdate(SynthSection* owner);
    void popupSelector(Component* source, Point<int> position, const PopupItems& options,
                       std::function<void(int)> callback, std::function<void()> cancel);
    void dualPopupSelector(Component* source, Point<int> position, int width,
                           const PopupItems& options, std::function<void(int)> callback);
    void popupDisplay(Component* source, const std::string& text,
                      BubbleComponent::BubblePlacement placement, bool primary);
    void hideDisplay(bool primary);

    // Modulation lives in Vital's synth UI only; these exist so shared code keeps working.
    void modulationChanged() { }
    void modulationValueChanged(int) { }

    // Vital features that need the full synth (accounts, wavetables, LFO files): not available here.
    void startDownload() { }  // content downloads need a Vital account; not offered here
    std::string getSignedInName() { return ""; }
    void signIn() { }
    void signOut() { }
    void saveLfo(const json&) { }

    void enableRedoBackground(bool enable) {
      enable_redo_background_ = enable;
      if (enable)
        resized();
    }

    float getResizingScale() const { return width_ * 1.0f / std::max(1, resized_width_); }
    float getPixelScaling() const { return display_scale_; }
    int getPixelMultiple() const override { return pixel_multiple_; }

  private:
    void timerCallback() override;   // picks up skins synced from other VialFX plugins
    Time bundle_skin_applied_;
    std::unique_ptr<EffectHeader> header_;
    std::unique_ptr<SynthSection> effect_section_;
    std::unique_ptr<SettingsSection> settings_section_;
    std::unique_ptr<AboutSection> about_section_;
    std::unique_ptr<PresetBrowser> preset_browser_;
    std::unique_ptr<PopupBrowser> popup_browser_;
    std::unique_ptr<SinglePopupSelector> popup_selector_;
    std::unique_ptr<DualPopupSelector> dual_popup_selector_;
    std::unique_ptr<PopupDisplay> popup_display_1_;
    std::unique_ptr<PopupDisplay> popup_display_2_;
    std::unique_ptr<SaveSection> save_section_;
    std::unique_ptr<DeleteSection> delete_section_;

    int width_ = 0;
    int resized_width_ = 0;
    float last_render_scale_ = 0.0f;
    float display_scale_ = 1.0f;
    int pixel_multiple_ = 1;
    bool setting_all_values_ = false;
    bool unsupported_ = false;
    bool animate_ = true;
    bool enable_redo_background_ = true;
    CriticalSection open_gl_critical_section_;
    OpenGLContext open_gl_context_;
    std::unique_ptr<Shaders> shaders_;
    OpenGlWrapper open_gl_;
    Image background_image_;
    OpenGlBackground background_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FullInterface)
};
