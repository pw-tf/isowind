#ifndef OPENMW_MWGUI_MAPTILESKIN_H
#define OPENMW_MWGUI_MAPTILESKIN_H

#include <MyGUI_ISubWidgetRect.h>
#include <MyGUI_RenderFormat.h>

namespace MyGUI
{
    class ILayerNode;
    class RenderItem;
    class Widget;
}

namespace MWGui
{
    /// Isowind: draws its widget's image turned about a point and cut to another widget, for the minimap that turns
    /// with the camera. Unlike MyGUI's RotatingSkin it doesn't rely on the widget being inside its parent: a map tile
    /// that is out of view unturned may well be in view turned, so it is drawn wherever it is placed, cut only to the
    /// view it was given (and not at all before it is given one).
    class MapTileSkin final : public MyGUI::ISubWidgetRect
    {
        MYGUI_RTTI_DERIVED(MapTileSkin)

    public:
        MapTileSkin();

        /// Turns the image by angle (radians, clockwise) about the middle of center, and cuts it to view
        void setTransform(float angle, MyGUI::Widget* center, MyGUI::Widget* view);

        void setAlpha(float alpha) override;
        void setVisible(bool visible) override;
        void setStateData(MyGUI::IStateInfo* data) override;

        void createDrawItem(MyGUI::ITexture* texture, MyGUI::ILayerNode* node) override;
        void destroyDrawItem() override;

        void doRender() override;

        void _updateView() override;
        void _correctView() override;
        void _setAlign(const MyGUI::IntSize& oldSize) override;
        void _setUVSet(const MyGUI::FloatRect& rect) override;
        void _setColour(const MyGUI::Colour& value) override;

    private:
        void outOfDate();

        // A rectangle cut by a rectangle has up to 8 corners: 6 triangles
        static constexpr int sMaxCorners = 8;

        float mAngle = 0.f;
        MyGUI::Widget* mCenter = nullptr;
        MyGUI::Widget* mView = nullptr;

        MyGUI::FloatRect mUV{ 0.f, 0.f, 1.f, 1.f };
        MyGUI::VertexColourType mVertexFormat;
        MyGUI::uint32 mColour = 0xFFFFFFFF;

        MyGUI::ILayerNode* mNode = nullptr;
        MyGUI::RenderItem* mRenderItem = nullptr;
    };
}

#endif
