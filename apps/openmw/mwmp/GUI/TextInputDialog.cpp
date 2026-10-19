#include "TextInputDialog.hpp"

#include "apps/openmw/mwbase/windowmanager.hpp"
#include "apps/openmw/mwbase/environment.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#include <components/esm/refid.hpp>

namespace mwmp
{
    TextInputDialog::TextInputDialog()
            : MWGui::WindowModal("tes3mp_text_input.layout")
    {
        // Centre dialog
        center();

        getWidget(mTextEdit, "TextEdit");
        mTextEdit->eventEditSelectAccept += newDelegate(this, &TextInputDialog::onTextAccepted);

        MyGUI::Button *okButton;
        getWidget(okButton, "OKButton");
        okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &TextInputDialog::onOkClicked);

        // Make sure the edit box has focus
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);

        mControllerButtons.mA = "#{Interface:OK}";
    }

    void TextInputDialog::setNextButtonShow(bool shown)
    {
        MyGUI::Button *okButton;
        getWidget(okButton, "OKButton");

        if (shown)
            okButton->setCaption(MyGUI::UString(std::string(MWBase::Environment::get().getWindowManager()->getGameSettingString("sNext", ""))));
        else
            okButton->setCaption(MyGUI::UString(std::string(MWBase::Environment::get().getWindowManager()->getGameSettingString("sOK", ""))));
    }

    void TextInputDialog::setEditPassword(bool value)
    {
        mTextEdit->setEditPassword(value);
    }

    void TextInputDialog::setTextLabel(const std::string &label)
    {
        setText("LabelT", label);
    }

    void TextInputDialog::setTextNote(const std::string &note)
    {
        setText("TextNote", note);
    }

    void TextInputDialog::onOpen()
    {
        WindowModal::onOpen();
        // Make sure the edit box has focus
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);
    }

    bool TextInputDialog::exit()
    {
        return false;
    }

// widget controls

    void TextInputDialog::onOkClicked(MyGUI::Widget *_sender)
    {
        if (mTextEdit->getCaption() == "")
        {
            //MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage37}");
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);
        }
        else
            eventDone(this);
    }

    void TextInputDialog::onTextAccepted(MyGUI::EditBox *_sender)
    {
        onOkClicked(_sender);
    }

    std::string TextInputDialog::getTextInput() const
    {
        return mTextEdit->getCaption();
    }

    void TextInputDialog::setTextInput(const std::string &text)
    {
        mTextEdit->setCaption(text);
    }

    bool TextInputDialog::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_A)
        {
            onOkClicked(nullptr);
            MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
            return true;
        }
        return false;
    }
}
