#ifndef MWGUI_SETTINGS_H
#define MWGUI_SETTINGS_H

#include <components/files/configurationmanager.hpp>
#include <components/lua_ui/adapter.hpp>

#include <vector>

#include "windowbase.hpp"

namespace MWGui
{
    class SettingsWindow : public WindowBase
    {
    public:
        SettingsWindow(Files::ConfigurationManager& cfgMgr);

        void onOpen() override;

        void onClose() override;

        void onFrame(float duration) override;

        void updateControlsBox();

        void updateLightSettings();

        void updateVSyncModeSettings();

        void updateWindowModeSettings();

        void onResChange(int, int) override;

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        bool stepTab(int delta) override;

    protected:
        MyGUI::TabControl* mSettingsTab;
        MyGUI::Button* mOkButton;

        // graphics
        MyGUI::ListBox* mResolutionList;
        MyGUI::ComboBox* mWindowModeList;
        MyGUI::ComboBox* mVSyncModeList;
        MyGUI::Button* mWindowBorderButton;
        MyGUI::ComboBox* mTextureFilteringButton;

        MyGUI::Button* mWaterRefractionButton;
        MyGUI::Button* mSunlightScatteringButton;
        MyGUI::Button* mWobblyShoresButton;
        MyGUI::ComboBox* mWaterTextureSize;
        MyGUI::ComboBox* mWaterReflectionDetail;
        MyGUI::ComboBox* mWaterRainRippleDetail;

        MyGUI::ComboBox* mMaxLights;
        MyGUI::ComboBox* mLightingMethodButton;
        MyGUI::Button* mLightsResetButton;

        MyGUI::ComboBox* mPrimaryLanguage;
        MyGUI::ComboBox* mSecondaryLanguage;
        MyGUI::Button* mGmstOverridesL10n;

        MyGUI::Widget* mWindowModeHint;

        // controls
        MyGUI::ScrollView* mControlsBox;
        MyGUI::Button* mResetControlsButton;
        MyGUI::Button* mKeyboardSwitch;
        MyGUI::Button* mControllerSwitch;
        bool mKeyboardMode; // if true, setting up the keyboard. Otherwise, it's controller

        MyGUI::EditBox* mScriptFilter;
        MyGUI::ListBox* mScriptList;
        MyGUI::Widget* mScriptBox;
        MyGUI::Widget* mScriptDisabled;
        MyGUI::ScrollView* mScriptView;
        LuaUi::LuaAdapter* mScriptAdapter;
        size_t mCurrentPage;

        void onTabChanged(MyGUI::TabControl* sender, size_t index);
        void onOkButtonClicked(MyGUI::Widget* sender);
        void onTextureFilteringChanged(MyGUI::ComboBox* sender, size_t pos);
        void onSliderChangePosition(MyGUI::ScrollBar* scroller, size_t pos);
        void onButtonToggled(MyGUI::Widget* sender);
        void onResolutionSelected(MyGUI::ListBox* sender, size_t index);
        void onResolutionAccept();
        void onResolutionCancel();
        void highlightCurrentResolution();

        void onRefractionButtonClicked(MyGUI::Widget* sender);
        void onWaterTextureSizeChanged(MyGUI::ComboBox* sender, size_t pos);
        void onWaterReflectionDetailChanged(MyGUI::ComboBox* sender, size_t pos);
        void onWaterRainRippleDetailChanged(MyGUI::ComboBox* sender, size_t pos);

        void onLightingMethodButtonChanged(MyGUI::ComboBox* sender, size_t pos);
        void onLightsResetButtonClicked(MyGUI::Widget* sender);
        void onMaxLightsChanged(MyGUI::ComboBox* sender, size_t pos);

        void onPrimaryLanguageChanged(MyGUI::ComboBox* sender, size_t pos) { onLanguageChanged(0, sender, pos); }
        void onSecondaryLanguageChanged(MyGUI::ComboBox* sender, size_t pos) { onLanguageChanged(1, sender, pos); }
        void onLanguageChanged(size_t langPriority, MyGUI::ComboBox* sender, size_t pos);
        void onGmstOverridesL10nChanged(MyGUI::Widget* sender);

        void onWindowModeChanged(MyGUI::ComboBox* sender, size_t pos);
        void onVSyncModeChanged(MyGUI::ComboBox* sender, size_t pos);

        void onRebindAction(MyGUI::Widget* sender);
        void onInputTabMouseWheel(MyGUI::Widget* sender, int rel);
        void onResetDefaultBindings(MyGUI::Widget* sender);
        void onResetDefaultBindingsAccept();
        void onKeyboardSwitchClicked(MyGUI::Widget* sender);
        void onControllerSwitchClicked(MyGUI::Widget* sender);

        void onWindowResize(MyGUI::Window* sender);

        void onScriptFilterChange(MyGUI::EditBox*);
        void onScriptListSelection(MyGUI::ListBox*, size_t index);

        void apply();

        void configureWidgets(MyGUI::Widget* widget, bool init);
        MyGUI::TextBox* getSliderLabel(MyGUI::ScrollBar* scroller) const;

        void layoutControlsBox();
        void renderScriptSettings();

        void computeMinimumWindowSize();

        /// Isowind: the D-pad steps the pointer through the page's controls, A works the one it is on and left /
        /// right adjust it; the left stick still moves the pointer freely
        bool handlesPointerSelect() const override { return true; }
        std::vector<MyGUI::Widget*> collectControls() const;
        void collectControls(MyGUI::Widget* parent, std::vector<MyGUI::Widget*>& controls) const;
        MyGUI::Widget* controlUnderPointer(const std::vector<MyGUI::Widget*>& controls) const;
        void pointAt(MyGUI::Widget* control);
        void pointAtFirstControl();
        void stepControl(int delta);
        void adjustControl(int delta);
        void selectControl();
        /// A frame around the control the pointer is on (nullptr: none)
        void frameControl(MyGUI::Widget* control);
        MyGUI::Widget* mControllerFrame = nullptr;
        MyGUI::Widget* mFramedControl = nullptr;

    private:
        void resetScrollbars();
        Files::ConfigurationManager& mCfgMgr;
    };
}

#endif
