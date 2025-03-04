#pragma once

#include "../../defines.hpp"
#include <list>
#include "../../helpers/WLClasses.hpp"
#include "../../Window.hpp"
#include "../../helpers/Timer.hpp"
#include "InputMethodRelay.hpp"

enum eClickBehaviorMode {
    CLICKMODE_DEFAULT = 0,
    CLICKMODE_KILL
};

enum eMouseBindMode {
    MBIND_INVALID = -1,
    MBIND_MOVE = 0,
    MBIND_RESIZE
};

struct STouchData {
    CWindow* touchFocusWindow = nullptr;
    SLayerSurface* touchFocusLS = nullptr;
    wlr_surface* touchFocusSurface = nullptr;
    Vector2D touchSurfaceOrigin;
};

class CInputManager {
public:

    void            onMouseMoved(wlr_pointer_motion_event*);
    void            onMouseWarp(wlr_pointer_motion_absolute_event*);
    void            onMouseButton(wlr_pointer_button_event*);
    void            onMouseWheel(wlr_pointer_axis_event*);
    void            onKeyboardKey(wlr_keyboard_key_event*, SKeyboard*);
    void            onKeyboardMod(void*, SKeyboard*);

    void            newKeyboard(wlr_input_device*);
    void            newVirtualKeyboard(wlr_input_device*);
    void            newMouse(wlr_input_device*, bool virt = false);
    void            newTouchDevice(wlr_input_device*);
    void            newSwitch(wlr_input_device*);
    void            destroyTouchDevice(STouchDevice*);
    void            destroyKeyboard(SKeyboard*);
    void            destroyMouse(wlr_input_device*);
    void            destroySwitch(SSwitchDevice*);

    void            constrainMouse(SMouse*, wlr_pointer_constraint_v1*);
    void            recheckConstraint(SMouse*);
    void            unconstrainMouse();
    std::string     getActiveLayoutForKeyboard(SKeyboard*);

    Vector2D        getMouseCoordsInternal();
    void            refocus();

    void            setKeyboardLayout();
    void            setPointerConfigs();
    void            setTouchDeviceConfigs();

    void            updateDragIcon();
    void            updateCapabilities(wlr_input_device*);

    void            setClickMode(eClickBehaviorMode);
    eClickBehaviorMode getClickMode();
    void            processMouseRequest(wlr_seat_pointer_request_set_cursor_event*);

    void            onTouchDown(wlr_touch_down_event*);
    void            onTouchUp(wlr_touch_up_event*);
    void            onTouchMove(wlr_touch_motion_event*);

    void            onPointerHoldBegin(wlr_pointer_hold_begin_event*);
    void            onPointerHoldEnd(wlr_pointer_hold_end_event*);

    STouchData      m_sTouchData;

    // for dragging floating windows
    CWindow*        currentlyDraggedWindow = nullptr;
    eMouseBindMode  dragMode = MBIND_INVALID;

    SDrag           m_sDrag;

    std::list<SConstraint>  m_lConstraints;
    std::list<SKeyboard>    m_lKeyboards;
    std::list<SMouse>       m_lMice;

    // tablets
    std::list<STablet>      m_lTablets;
    std::list<STabletTool>  m_lTabletTools;
    std::list<STabletPad>   m_lTabletPads;

    // idle inhibitors
    std::list<SIdleInhibitor> m_lIdleInhibitors;

    // Touch devices
    std::list<STouchDevice> m_lTouchDevices;

    // Switches
    std::list<SSwitchDevice> m_lSwitches;

    void            newTabletTool(wlr_input_device*);
    void            newTabletPad(wlr_input_device*);
    void            focusTablet(STablet*, wlr_tablet_tool*, bool motion = false);
    void            newIdleInhibitor(wlr_idle_inhibitor_v1*);
    void            recheckIdleInhibitorStatus();

    void            onSwipeBegin(wlr_pointer_swipe_begin_event*);
    void            onSwipeEnd(wlr_pointer_swipe_end_event*);
    void            onSwipeUpdate(wlr_pointer_swipe_update_event*);

    SSwipeGesture   m_sActiveSwipe;

    SKeyboard*      m_pActiveKeyboard = nullptr;

    CTimer          m_tmrLastCursorMovement;

    CInputMethodRelay m_sIMERelay;

    // for shared mods
    uint32_t        accumulateModsFromAllKBs();

    CWindow*        m_pFollowOnDnDBegin = nullptr;

    // for virtual keyboards: whether we should respect them as normal ones
    bool            shouldIgnoreVirtualKeyboard(SKeyboard*);

private:

    // for click behavior override
    eClickBehaviorMode m_ecbClickBehavior = CLICKMODE_DEFAULT;
    bool            m_bEmptyFocusCursorSet = false;
    Vector2D        m_vLastCursorPosFloored = Vector2D();

    // for some bugs in follow mouse 0
    bool            m_bLastFocusOnLS = false;

    void            processMouseDownNormal(wlr_pointer_button_event* e);
    void            processMouseDownKill(wlr_pointer_button_event* e);

    void            disableAllKeyboards(bool virt = false);

    uint32_t        m_uiCapabilities = 0;

    void            mouseMoveUnified(uint32_t, bool refocus = false);

    STabletTool*    ensureTabletToolPresent(wlr_tablet_tool*);

    void            applyConfigToKeyboard(SKeyboard*);

    // this will be set after a refocus()
    wlr_surface*    m_pFoundSurfaceToFocus = nullptr;
    SLayerSurface*  m_pFoundLSToFocus = nullptr;
    CWindow*        m_pFoundWindowToFocus = nullptr;

    // swipe
    void            beginWorkspaceSwipe();
};

inline std::unique_ptr<CInputManager> g_pInputManager;
