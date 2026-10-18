#include "maptileskin.hpp"

#include <array>
#include <cmath>

#include <MyGUI_CommonStateInfo.h>
#include <MyGUI_ILayerNode.h>
#include <MyGUI_IRenderTarget.h>
#include <MyGUI_RenderItem.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_TextureUtility.h>
#include <MyGUI_VertexData.h>
#include <MyGUI_Widget.h>

namespace MWGui
{
    namespace
    {
        using Polygon = std::array<MyGUI::FloatPoint, 8>;

        // Sutherland-Hodgman: what of polygon in is on the inner side of one edge of the view
        template <class Inside, class Cross>
        int clipEdge(const Polygon& in, int count, Polygon& out, Inside inside, Cross cross)
        {
            int result = 0;
            for (int i = 0; i < count; ++i)
            {
                const MyGUI::FloatPoint& a = in[i];
                const MyGUI::FloatPoint& b = in[(i + 1) % count];
                const bool aIn = inside(a);
                const bool bIn = inside(b);
                if (aIn && result < 8)
                    out[result++] = a;
                if (aIn != bIn && result < 8)
                    out[result++] = cross(a, b);
            }
            return result;
        }

        int clip(Polygon& polygon, int count, const MyGUI::IntRect& rect)
        {
            Polygon other;
            const auto atX = [](float x) {
                return [x](const MyGUI::FloatPoint& a, const MyGUI::FloatPoint& b) {
                    const float t = (x - a.left) / (b.left - a.left);
                    return MyGUI::FloatPoint(x, a.top + t * (b.top - a.top));
                };
            };
            const auto atY = [](float y) {
                return [y](const MyGUI::FloatPoint& a, const MyGUI::FloatPoint& b) {
                    const float t = (y - a.top) / (b.top - a.top);
                    return MyGUI::FloatPoint(a.left + t * (b.left - a.left), y);
                };
            };
            const float left = static_cast<float>(rect.left), right = static_cast<float>(rect.right);
            const float top = static_cast<float>(rect.top), bottom = static_cast<float>(rect.bottom);
            count = clipEdge(
                polygon, count, other, [&](const MyGUI::FloatPoint& p) { return p.left >= left; }, atX(left));
            count = clipEdge(
                other, count, polygon, [&](const MyGUI::FloatPoint& p) { return p.left <= right; }, atX(right));
            count = clipEdge(
                polygon, count, other, [&](const MyGUI::FloatPoint& p) { return p.top >= top; }, atY(top));
            count = clipEdge(
                other, count, polygon, [&](const MyGUI::FloatPoint& p) { return p.top <= bottom; }, atY(bottom));
            return count;
        }
    }

    MapTileSkin::MapTileSkin()
        : mVertexFormat(MyGUI::RenderManager::getInstance().getVertexFormat())
    {
    }

    void MapTileSkin::setTransform(float angle, MyGUI::Widget* center, MyGUI::Widget* view)
    {
        mAngle = angle;
        mCenter = center;
        mView = view;
        // Even unchanged: the tile, the player and the view may have moved
        outOfDate();
    }

    void MapTileSkin::outOfDate()
    {
        if (mNode)
            mNode->outOfDate(mRenderItem);
    }

    void MapTileSkin::setAlpha(float alpha)
    {
        const MyGUI::uint32 value = static_cast<MyGUI::uint8>(alpha * 255);
        mColour = (mColour & 0x00FFFFFF) | (value << 24);
        outOfDate();
    }

    void MapTileSkin::setVisible(bool visible)
    {
        // MyGUI also hides a skin whose widget is out of its parent's view; whether the widget is visible is checked
        // when drawing instead
        mVisible = visible;
        outOfDate();
    }

    void MapTileSkin::setStateData(MyGUI::IStateInfo* data)
    {
        if (auto* state = data->castType<MyGUI::SubSkinStateInfo>(false))
            _setUVSet(state->getRect());
        else if (auto* rotatingState = data->castType<MyGUI::RotatingSkinStateInfo>(false))
            _setUVSet(rotatingState->getRect());
    }

    void MapTileSkin::createDrawItem(MyGUI::ITexture* texture, MyGUI::ILayerNode* node)
    {
        mNode = node;
        mRenderItem = mNode->addToRenderItem(texture, true, false);
        mRenderItem->addDrawItem(this, (sMaxCorners - 2) * 3);
    }

    void MapTileSkin::destroyDrawItem()
    {
        mNode = nullptr;
        mRenderItem->removeDrawItem(this);
        mRenderItem = nullptr;
    }

    void MapTileSkin::doRender()
    {
        if (!mRenderItem || !mView || !mCenter)
            return;
        const auto* widget = dynamic_cast<MyGUI::Widget*>(mCroppedParent);
        if (!widget || !widget->getInheritedVisible() || !mView->getInheritedVisible())
            return;

        // The image as placed, and the point it turns about, on the screen
        const float left = static_cast<float>(mCroppedParent->getAbsoluteLeft() + mCoord.left);
        const float top = static_cast<float>(mCroppedParent->getAbsoluteTop() + mCoord.top);
        const float width = static_cast<float>(mCoord.width);
        const float height = static_cast<float>(mCoord.height);
        if (width <= 0 || height <= 0)
            return;
        const MyGUI::IntCoord centerCoord = mCenter->getAbsoluteCoord();
        const float cx = centerCoord.left + centerCoord.width / 2.f;
        const float cy = centerCoord.top + centerCoord.height / 2.f;
        const float c = std::cos(mAngle);
        const float s = std::sin(mAngle);

        Polygon polygon;
        int count = 0;
        for (const auto& [x, y] : { std::pair{ left, top }, std::pair{ left + width, top },
                 std::pair{ left + width, top + height }, std::pair{ left, top + height } })
        {
            const float dx = x - cx;
            const float dy = y - cy;
            polygon[count++] = MyGUI::FloatPoint(cx + dx * c - dy * s, cy + dx * s + dy * c);
        }
        count = clip(polygon, count, mView->getAbsoluteRect());
        if (count < 3)
            return;

        const MyGUI::RenderTargetInfo& info = mRenderItem->getRenderTarget()->getInfo();
        const float z = mNode->getNodeDepth();
        std::array<MyGUI::Vertex, sMaxCorners> corners;
        for (int i = 0; i < count; ++i)
        {
            const MyGUI::FloatPoint& p = polygon[i];
            // Where the corner is on the image: turned back
            const float dx = p.left - cx;
            const float dy = p.top - cy;
            const float u = (cx + dx * c + dy * s - left) / width;
            const float v = (cy - dx * s + dy * c - top) / height;
            const float x = ((info.pixScaleX * (p.left - info.leftOffset) + info.hOffset) * 2.f) - 1.f;
            const float y = -(((info.pixScaleY * (p.top - info.topOffset) + info.vOffset) * 2.f) - 1.f);
            corners[i].set(x, y, z, mUV.left + u * (mUV.right - mUV.left), mUV.top + v * (mUV.bottom - mUV.top),
                mColour);
        }

        MyGUI::Vertex* vertices = mRenderItem->getCurrentVertexBuffer();
        int written = 0;
        for (int i = 1; i + 1 < count; ++i)
        {
            vertices[written++] = corners[0];
            vertices[written++] = corners[i];
            vertices[written++] = corners[i + 1];
        }
        mRenderItem->setLastVertexCount(written);
    }

    void MapTileSkin::_updateView()
    {
        outOfDate();
    }

    void MapTileSkin::_correctView()
    {
        outOfDate();
    }

    void MapTileSkin::_setAlign(const MyGUI::IntSize& oldSize)
    {
        // As MyGUI's SubSkin
        if (mAlign.isHStretch())
            mCoord.width += mCroppedParent->getWidth() - oldSize.width;
        else if (mAlign.isRight())
            mCoord.left += mCroppedParent->getWidth() - oldSize.width;
        else if (mAlign.isHCenter())
            mCoord.left = (mCroppedParent->getWidth() - mCoord.width) / 2;

        if (mAlign.isVStretch())
            mCoord.height += mCroppedParent->getHeight() - oldSize.height;
        else if (mAlign.isBottom())
            mCoord.top += mCroppedParent->getHeight() - oldSize.height;
        else if (mAlign.isVCenter())
            mCoord.top = (mCroppedParent->getHeight() - mCoord.height) / 2;

        _updateView();
    }

    void MapTileSkin::_setUVSet(const MyGUI::FloatRect& rect)
    {
        mUV = rect;
        outOfDate();
    }

    void MapTileSkin::_setColour(const MyGUI::Colour& value)
    {
        const MyGUI::uint32 colour = MyGUI::texture_utility::toNativeColour(value, mVertexFormat);
        mColour = (colour & 0x00FFFFFF) | (mColour & 0xFF000000);
        outOfDate();
    }
}
