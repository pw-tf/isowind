#include <components/openmw-mp/TimedLog.hpp>

#include "apps/openmw/mwbase/environment.hpp"
#include "apps/openmw/mwgui/windowmanagerimp.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_Button.h>
#include <MyGUI_ListBox.h>

#include <components/esm/refid.hpp>
#include <components/settings/values.hpp>

#include "GUIDialogList.hpp"
#include "../Main.hpp"
#include "../Networking.hpp"
#include "../LocalPlayer.hpp"

using namespace mwmp;

GUIDialogList::GUIDialogList(const std::string &message, const std::vector<std::string> &list) : WindowModal("tes3mp_dialog_list.layout")
{
    center(); // center window

    getWidget(mListBox, "ListBox");
    getWidget(mMessage, "Message");
    getWidget(mButton, "OkButton");

    mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &GUIDialogList::mousePressed);

    mMessage->setCaptionWithReplacing(message);
    for (size_t i = 0; i < list.size(); i++)
        mListBox->addItem(list[i]);

    // Isowind: with a controller the first entry starts picked, so A always answers with something
    if (Settings::gui().mControllerMenus)
    {
        mDisableGamepadCursor = true;
        mControllerButtons.mA = "#{Interface:Select}";
        mControllerButtons.mDpad = "#{Interface:Move}";
        if (!list.empty())
            mListBox->setIndexSelected(0);
    }
}

bool GUIDialogList::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
{
    if (arg.button == SDL_CONTROLLER_BUTTON_A)
    {
        MWBase::Environment::get().getWindowManager()->playSound(ESM::RefId::stringRefId("Menu Click"));
        mousePressed(mButton);
    }
    else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP || arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
    {
        const size_t count = mListBox->getItemCount();
        if (count == 0)
            return true;
        size_t index = mListBox->getIndexSelected();
        if (index == MyGUI::ITEM_NONE)
            index = 0;
        else
            index = MWGui::wrap(index, count, arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP ? -1 : 1);
        mListBox->setIndexSelected(index);
        mListBox->beginToItemAt(index);
    }
    return true;
}

void GUIDialogList::mousePressed(MyGUI::Widget * /*widget*/)
{
    setVisible(false);
    MWBase::Environment::get().getWindowManager()->popGuiMode();

    size_t id = mListBox->getIndexSelected();

    Main::get().getLocalPlayer()->guiMessageBox.data = MyGUI::utility::toString(id);
    Main::get().getNetworking()->getPlayerPacket(ID_GUI_MESSAGEBOX)->setPlayer(Main::get().getLocalPlayer());
    Main::get().getNetworking()->getPlayerPacket(ID_GUI_MESSAGEBOX)->Send();

    LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Selected id: %d", id);
    if (id == MyGUI::ITEM_NONE)
        return;

    std::string itemName = mListBox->getItemNameAt(mListBox->getIndexSelected()).asUTF8();
    LOG_APPEND(TimedLog::LOG_VERBOSE, "name of item: '%s'", itemName.c_str());
}

void GUIDialogList::onFrame(float frameDuration)
{

}
