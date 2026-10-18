// Copied from MWGui::CustomMarkerCollection

#ifndef OPENMW_PLAYERMARKERCOLLECTION_HPP
#define OPENMW_PLAYERMARKERCOLLECTION_HPP

#include <components/esm/refid.hpp>
#include <components/esm3/custommarkerstate.hpp>
#include <map>
#include <MyGUI_Common.h>
#include <MyGUI_Delegate.h>
#include <MyGUI_Colour.h>

namespace mwmp
{
    class PlayerMarkerCollection
    {
    public:

        void addMarker(const ESM::CustomMarker &marker, bool triggerEvent = true);
        void deleteMarker(const ESM::CustomMarker &marker);
        void updateMarker(const ESM::CustomMarker &marker, const std::string &newNote);

        void clear();

        size_t size() const;

        typedef std::multimap <ESM::RefId, ESM::CustomMarker> ContainerType;

        typedef std::pair <ContainerType::const_iterator, ContainerType::const_iterator> RangeType;

        ContainerType::const_iterator begin() const;
        ContainerType::const_iterator end() const;

        RangeType getMarkers(const ESM::RefId &cellId) const;

        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;
        EventHandle_Void eventMarkersChanged;

        bool contains(const ESM::CustomMarker &marker);
    private:
        ContainerType mMarkers;
    };
}

#endif //OPENMW_PLAYERMARKERCOLLECTION_HPP
