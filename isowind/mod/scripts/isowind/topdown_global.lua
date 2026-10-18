-- Activation must happen in a global script; the player script asks for it with an event.

return {
    eventHandlers = {
        Isowind_Activate = function(e)
            if e.object and e.object:isValid() and e.actor and e.actor:isValid() then
                e.object:activateBy(e.actor)
            end
        end,
    },
}
