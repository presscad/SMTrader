///////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2015 Advanced Software	Engineering	Limited
//
// You may use and modify the code in this file	in your	application, provided the code and
// its modifications are used only in conjunction with ChartDirector. Usage	of this	software
// is subjected	to the terms and condition of the ChartDirector	license.
///////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
// CChartViewer / CViewPortControl Implementation
//
// The CChartViewer	is a subclass of CStatic for displaying	chart images. It extends CStatic 
// to support to support alpha transparency, image maps, clickable hot spots with tool tips, 
// zooming and scrolling and image update rate control. The CViewPortControl is a subclass of 
// CStatic for visualizing and supporting interactive control of the CChartViewer viewport.
//
// To use these controls in a dialog, in the Dialog Editor, drag a Picture control (CStatic
// control) to the form, configure its type as "Bitmap", and give it a unique ID. Then right 
// click on the control and add a variable to represent the control, using CChartViewer or
// CViewPortControl as the variable type.
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ChartViewer.h"
#include <math.h>
#include <stdio.h>
#include "../resource.h"
#include "../MainFrm.h"
#include "../VtChartFrm.h"
#include "../VtIndexDefine.h"
#include "../VtBaseIndex.h"
#include "../VtGlobal.h"
#include "../Global/VtDefine.h"
#include "../VtBaseBand.h"
#include "../VtBaseStress.h"
#include "../VtChartWindow.h"

#ifdef _DEBUG
#define	new	DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
//
// CChartViewer
//
/////////////////////////////////////////////////////////////////////////////

//
// Build in	mouse cursors for zooming and scrolling	support
//
static HCURSOR getZoomInCursor();
static HCURSOR getZoomOutCursor();
static HCURSOR getNoZoomCursor();
static HCURSOR getNoMove2DCursor();
static HCURSOR getNoMoveHorizCursor();
static HCURSOR getNoMoveVertCursor();

//
// Constants used in m_delayChartUpdate
//
enum { NO_DELAY, NEED_DELAY, NEED_UPDATE };
enum { UPDATE_VIEW_PORT_TIMER = 1, DELAYED_MOUSE_MOVE_TIMER = 2 };


//
// Constructor
//
CChartViewer::CChartViewer()
{
	// current chart and hot spot tester
	m_currentChart = 0;
	m_hotSpotTester	= 0;

	// create the tool tip control
	m_ToolTip.Create(this);
	m_ToolTip.Activate(TRUE);
	m_ToolTip.ModifyStyle(0, TTS_NOPREFIX);
	m_toolTipHasAttached = false;

	// initialize chart	configuration
	m_selectBoxLineColor = RGB(0, 0, 0);
	m_selectBoxLineWidth = 2;
	m_mouseUsage = Chart::MouseUsageDefault;
	m_zoomDirection	= Chart::DirectionHorizontal;
	m_zoomInRatio =	2;
	m_zoomOutRatio = 0.5;
	m_mouseWheelZoomRatio = 1;
	m_scrollDirection =	Chart::DirectionHorizontal;
	m_minDragAmount	= 5;
	m_updateInterval = 20;

	// current state of	the	mouse
	m_isOnPlotArea = false;
	m_isPlotAreaMouseDown =	false;
	m_isDragScrolling =	false;
	m_currentHotSpot = -1;
	m_isClickable =	false;
	m_isMouseTracking = false;
	m_isInMouseMove = false;

	// chart update	rate support
	m_needUpdateChart =	false;
	m_needUpdateImageMap = false;
	m_holdTimerActive =	false;
	m_delayUpdateChart = NO_DELAY;
	m_delayedChart = 0;
	m_lastMouseMove = 0;
	m_hasDelayedMouseMove = false;
	m_delayImageMapUpdate = false;

	// track cursor support
	m_autoHideMsg = 0;
	m_currentMouseX = -1;
	m_currentMouseY = -1;
	m_isInMouseMovePlotArea = false;

	// CViewPortControl support
	m_vpControl = 0;
	m_ReentrantGuard = false;
}

BEGIN_MESSAGE_MAP(CChartViewer,	CStatic)
	//{{AFX_MSG_MAP(CChartViewer)
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_TIMER()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CREATE()

	/*
	ON_COMMAND_RANGE(ID_VC_BTC_0, ID_VC_QTUM_0, &CChartViewer::OnProductBithumb)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VC_BTC_0, ID_VC_QTUM_0, &CChartViewer::OnUpdateProductBithumb)

	ON_COMMAND_RANGE(ID_VC_BTC_1, ID_VC_QTUM_1, &CChartViewer::OnProductKorbit)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VC_BTC_1, ID_VC_QTUM_1, &CChartViewer::OnUpdateProductCoinwon)

	ON_COMMAND_RANGE(ID_VC_BTC_2, ID_VC_QTUM_2, &CChartViewer::OnProductBitrex)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VC_BTC_2, ID_VC_QTUM_2, &CChartViewer::OnUpdateProductBitrex)

	ON_COMMAND_RANGE(ID_VC_BTC_3, ID_VC_QTUM_3, &CChartViewer::OnProductKraken)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VC_BTC_3, ID_VC_QTUM_3, &CChartViewer::OnUpdateProductKraken)

	ON_COMMAND_RANGE(ID_VC_BTC_4, ID_VC_QTUM_4, &CChartViewer::OnProductBitfinex)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VC_BTC_4, ID_VC_QTUM_4, &CChartViewer::OnUpdateProductBitfinex)

	ON_COMMAND_RANGE(ID_VC_BTC_5, ID_VC_QTUM_5, &CChartViewer::OnProductPoloniex)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VC_BTC_5, ID_VC_QTUM_5, &CChartViewer::OnUpdateProductPoloniex)

	ON_COMMAND_RANGE(ID_VC_BTC_0_0, ID_VC_QTUM_0_0, &CChartViewer::OnProductCompare0)
	ON_COMMAND_RANGE(ID_VC_BTC_1_0, ID_VC_QTUM_1_0, &CChartViewer::OnProductCompare1)
	ON_COMMAND_RANGE(ID_VC_BTC_2_0, ID_VC_QTUM_2_0, &CChartViewer::OnProductCompare2)
	ON_COMMAND_RANGE(ID_VC_BTC_3_0, ID_VC_QTUM_3_0, &CChartViewer::OnProductCompare3)
	ON_COMMAND_RANGE(ID_VC_BTC_4_0, ID_VC_QTUM_4_0, &CChartViewer::OnProductCompare4)
	ON_COMMAND_RANGE(ID_VC_BTC_5_0, ID_VC_QTUM_5_0, &CChartViewer::OnProductCompare5)

	ON_COMMAND_RANGE(ID_CHART_TYPE_CANDLESTIC, ID_CHART_TYPE_WP, &CChartViewer::OnChartPattern)
	ON_UPDATE_COMMAND_UI_RANGE(ID_CHART_TYPE_CANDLESTIC, ID_CHART_TYPE_WP, &CChartViewer::OnUpdateChartPattern)

	ON_COMMAND_RANGE(ID_CHART_INDEX_MA, ID_CHART_INDEX_VOL, &CChartViewer::OnChartIndex)
	ON_UPDATE_COMMAND_UI_RANGE(ID_CHART_INDEX_MA, ID_CHART_INDEX_VOL, &CChartViewer::OnUpdateChartIndex)
	ON_COMMAND(ID_SHOW_CHART_CTRL, &CChartViewer::OnShowChartCtrl)
	ON_UPDATE_COMMAND_UI(ID_SHOW_CHART_CTRL, &CChartViewer::OnUpdateShowChartCtrl)

	ON_COMMAND_RANGE(ID_DEL_INDEX_0, ID_DEL_INDEX_30, &CChartViewer::OnDelIndex)
	ON_COMMAND(ID_DEL_INDEX_ALL, &CChartViewer::OnDelIndexAll)

	//ID_REMOVEFROMCHART_SYSTEM
	ON_COMMAND(ID_REMOVEFROMCHART_SYSTEM, &CChartViewer::OnDelSystem)

	ON_COMMAND_RANGE(ID_INDEX_0, ID_INDEX_30, &CChartViewer::OnInsertIndex)

	ON_COMMAND_RANGE(ID_SYSTEM_0, ID_SYSTEM_30, &CChartViewer::OnInsertSystem)


	ON_COMMAND_RANGE(ID_INSERT_BAND_0, ID_INSERT_BAND_7, &CChartViewer::OnInsertBand)
	ON_COMMAND_RANGE(ID_REMOVE_BAND_0, ID_REMOVE_BAND_7, &CChartViewer::OnRemoveBand)

	ON_COMMAND_RANGE(ID_INSERT_STRESS_0, ID_INSERT_STRESS_2, &CChartViewer::OnInsertStress)
	ON_COMMAND_RANGE(ID_REMOVE_STRESS_0, ID_REMOVE_STRESS_2, &CChartViewer::OnRemoveStress)
	*/
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChartViewer	message	handlers

//
// Free	resources
//
void CChartViewer::OnDestroy() 
{
	setViewPortControl(0);

	// Free	HBITMAP
	HBITMAP	b =	SetBitmap(0);
	if (0 != b)
		DeleteObject(b);

	// Free	hot	spot tester
	if (0 != m_hotSpotTester)
		delete m_hotSpotTester;
	m_hotSpotTester	= 0;

	CStatic::OnDestroy();
}

//
// MouseMove event handler
//
void CChartViewer::OnMouseMove(UINT	nFlags,	CPoint point) 
{
	// Enable mouse tracking to detect mouse leave events
	if (!m_isMouseTracking)
	{
		TRACKMOUSEEVENT e;		
		e.cbSize = sizeof(e);		
		e.dwFlags = TME_LEAVE;		
		e.hwndTrack = this->m_hWnd;
		m_isMouseTracking = (0 != TrackMouseEvent(&e));
	}

	// On Windows, mouse events can by-pass the event queue. If there are too many mouse events,
	// the event queue may not get processed, preventing other controls from updating. If two mouse
	// events are less than 10ms apart, there is a risk of too many mouse events. So we repost the
	// mouse event as a timer event that is queued up normally, allowing the queue to get processed.
	unsigned int timeBetweenMouseMove = GetTickCount() - m_lastMouseMove;
	if ((m_hasDelayedMouseMove && (timeBetweenMouseMove < 250)) || (timeBetweenMouseMove < 10))
	{
		m_delayedMouseMoveFlag = nFlags;
		m_delayedMouseMovePoint = point;
		if (!m_hasDelayedMouseMove)
		{
			m_hasDelayedMouseMove = true;
			SetTimer(DELAYED_MOUSE_MOVE_TIMER, 1, 0);
		}
	}
	else
		commitMouseMove(nFlags, point);
}

//
// The method that actually performs MouseMove event processing
//
void CChartViewer::commitMouseMove(UINT nFlags, CPoint point)
{
	SetMouseInfo(nFlags, point);
	// Cancel the delayed mouse event if any
	if (m_hasDelayedMouseMove)
	{
		KillTimer(DELAYED_MOUSE_MOVE_TIMER);
		m_hasDelayedMouseMove = false;
	}

	// Remember the mouse coordinates for later use
	m_currentMouseX = point.x;
	m_currentMouseY = point.y;

	// The chart can be updated more than once during mouse move. For example, it can update due to
	// drag to scroll, and also due to drawing track cursor. So we delay updating the display until
	// all all events has occured.
	m_delayUpdateChart = NEED_DELAY;
	m_isInMouseMove = true;

	// Check if mouse is dragging on the plot area
	m_isOnPlotArea = m_isPlotAreaMouseDown || inPlotArea(point.x, point.y);
	if (m_isPlotAreaMouseDown)
		OnPlotAreaMouseDrag(nFlags,	point);
	
	// Send CVN_MouseMoveChart
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_MouseMoveChart), 
		(LPARAM)m_hWnd);

	CString msg;
	msg.Format(_T("Mous x = %d, y = %d\n"), MousePoint.x, MousePoint.y);
	//TRACE(msg);

	if (inExtendedPlotArea(point.x, point.y))
	{
		// Mouse wheel events are only sent to the control in focus. So if mouse wheel zooming is 
		// enabled, we must get the focus in order to receive mouse wheel events.
		if (m_mouseWheelZoomRatio != 1)
			SetFocus();

		// Mouse is in extended plot area, send CVN_MouseMovePlotArea
		m_isInMouseMovePlotArea = true;
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_MouseMovePlotArea), 
			(LPARAM)m_hWnd);
	}
	else if (m_isInMouseMovePlotArea)
	{
		// Mouse was in extended plot area, but is not in it now, so send CVN_MouseLeavePlotArea
		m_isInMouseMovePlotArea = false;
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_MouseLeavePlotArea), 
			(LPARAM)m_hWnd);
		applyAutoHide(CVN_MouseLeavePlotArea);
	}

	// Can update chart now
	commitUpdateChart();
	m_isInMouseMove = false;

	if (m_delayImageMapUpdate)
	{
		m_delayImageMapUpdate = false;
		if (!m_isPlotAreaMouseDown)
			updateViewPort(false, true);
	}

	//
	// Show	hot	spot tool tips if necessary
	//
	if (!m_toolTipHasAttached)
	{
		m_toolTipHasAttached = true;

		// Connects	the	CChartViewer to	the	CToolTipCtrl control
		m_ToolTip.AddTool(this);
		m_ToolTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
	}

	if (nFlags != 0)
	{
		// Hide	tool tips if mouse button is pressed.
		m_ToolTip.UpdateTipText(_T(""),	this);
	}
	else
	{
		// Use the ChartDirector ImageMapHandler to	determine if the mouse is over a hot spot
		int	hotSpotNo =	0;
		if (0 != m_hotSpotTester)
			hotSpotNo =	m_hotSpotTester->getHotSpot(point.x, point.y);

		// If the mouse	is in the same hot spot	since the last mouse move event, there is no need
		// to update the tool tip.
		if (hotSpotNo != m_currentHotSpot)
		{
			// Hot spot	has	changed	- update tool tip text
			m_currentHotSpot = hotSpotNo;

			if (hotSpotNo == 0)
			{
				// Mouse is	not	actually on	hanlder	hot	spot - use default tool	tip	text and reset
				// the clickable flag.
				m_ToolTip.UpdateTipText(m_defaultToolTip, this);
				m_isClickable =	false;
			}
			else
			{
				// Mouse is	on a hot spot. In this implementation, we consider the hot spot	as 
				// clickable if	its	href ("path") parameter	is not empty.
				const char *path = m_hotSpotTester->getValue("path");
				m_isClickable =	((0	!= path) &&	(0 != *path));

				// Use the title attribute as the tool tip.	Note that ChartDirector	uses UTF8, 
				// while MFC uses TCHAR, so	we use the utility UTF8toTCHAR for conversion.
				m_ToolTip.UpdateTipText(UTF8toTCHAR(m_hotSpotTester->getValue("title")), this);
			}
		}
	}

	m_lastMouseMove = GetTickCount();
	CStatic::OnMouseMove(nFlags, point);
}

//
// Delayed MouseMove event handler
//
void CChartViewer::OnDelayedMouseMove() 
{
	if (m_hasDelayedMouseMove)
		commitMouseMove(m_delayedMouseMoveFlag, m_delayedMouseMovePoint);
}

//
// MouseWheel handler
//
BOOL CChartViewer::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// Process the mouse wheel only if the mouse is over the plot area
	if (!isMouseOnPlotArea())
		return FALSE;
	else
	{
		if (_ChartWindow)
			_ChartWindow->OnMouseHWheel(nFlags, zDelta, pt);
		return onMouseWheelZoom(getPlotAreaMouseX(), getPlotAreaMouseY(), zDelta);
	}
}

BOOL CChartViewer::onMouseWheelZoom(int x, int y, short zDelta)
{
	// Zoom ratio = 1 means no zooming
	if (m_mouseWheelZoomRatio == 1)
		return FALSE;

	// X and Y zoom ratios
	double rx = 1;
    double ry = 1;
	if (getZoomDirection() != Chart::DirectionVertical)
		rx = (zDelta > 0) ? m_mouseWheelZoomRatio : 1 / m_mouseWheelZoomRatio;
	if (getZoomDirection() != Chart::DirectionHorizontal)
		ry = (zDelta > 0) ? m_mouseWheelZoomRatio : 1 / m_mouseWheelZoomRatio;

	// Do the zooming
	if (zoomAround(x, y, rx, ry))
	{
		updateViewPort(true, false);
		m_delayImageMapUpdate = true;
	}

	return TRUE;
}

//
// MouseLeave event handler
//
LRESULT CChartViewer::OnMouseLeave(WPARAM wParam, LPARAM lParam) 
{
	// Process delayed mouse move, if any
	OnDelayedMouseMove();

	// Mouse tracking is no longer active
	m_isMouseTracking = false;

	if (m_isInMouseMovePlotArea)
	{
		// Mouse was in extended plot area, but is not in it now, so send CVN_MouseLeavePlotArea
		m_isInMouseMovePlotArea = false;
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_MouseLeavePlotArea), 
			(LPARAM)m_hWnd);
		applyAutoHide(CVN_MouseLeavePlotArea);
	}

	// Send CVN_MouseLeaveChart
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_MouseLeaveChart), 
		(LPARAM)m_hWnd);
	applyAutoHide(CVN_MouseLeaveChart);
 
	return TRUE;
}

//
// Intercept WM_SETCURSOR to change	the	mouse cursor.
//
BOOL CChartViewer::OnSetCursor(CWnd* pWnd, UINT	nHitTest, UINT message)	
{
	if (m_isOnPlotArea)
	{
		switch (m_mouseUsage)
		{
		case Chart::MouseUsageZoomIn:
			if (canZoomIn(m_zoomDirection))
				::SetCursor(getZoomInCursor());
			else
				::SetCursor(getNoZoomCursor());
			return TRUE;
		case Chart::MouseUsageZoomOut:
			if (canZoomOut(m_zoomDirection))
				::SetCursor(getZoomOutCursor());
			else
				::SetCursor(getNoZoomCursor());
			return TRUE;
		}
	}

	if (m_isClickable)
	{
		// Hand	cursor = IDC_HAND =	32649
		HCURSOR	h =	AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32649));
		if (NULL !=	h)
			::SetCursor(h);
		return TRUE;
	}

	return CStatic::OnSetCursor(pWnd, nHitTest,	message);
}

//
// Mouse left button down event.
//
void CChartViewer::OnLButtonDown(UINT nFlags, CPoint point)	
{
	OnDelayedMouseMove();

	SetMouseInfo(nFlags, point);
	if (inPlotArea(point.x,	point.y) &&	(m_mouseUsage != Chart::MouseUsageDefault))
	{
		// Mouse usage is for drag to zoom/scroll. Capture the mouse to	prepare	for	dragging and 
		// save	the	mouse down position	to draw	the	selection rectangle.
		SetCapture();
		m_isPlotAreaMouseDown =	true;
		m_plotAreaMouseDownXPos	= point.x;
		m_plotAreaMouseDownYPos	= point.y;
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_LButtonDownChart),
			(LPARAM)m_hWnd);

		startDrag();
	}
	else
	{
		SetCapture();
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_LButtonDownChart),
			(LPARAM)m_hWnd);

		CStatic::OnLButtonDown(nFlags, point);
	}
}

//
// Mouse left button up	event.
//
void CChartViewer::OnLButtonUp(UINT	nFlags,	CPoint point) 
{
	OnDelayedMouseMove();
	SetMouseInfo(nFlags, point);
	if (m_isPlotAreaMouseDown)
	{
		// Release the mouse capture.
		ReleaseCapture();
		m_isPlotAreaMouseDown =	false;
		setRectVisible(false);
		bool hasUpdate = false;

		switch (m_mouseUsage)
		{
		case Chart::MouseUsageZoomIn :
			if (canZoomIn(m_zoomDirection))
			{
				if (isDrag(m_zoomDirection,	point))
					// Zoom	to the drag	selection rectangle.
					hasUpdate =	zoomTo(m_zoomDirection,	
						m_plotAreaMouseDownXPos, m_plotAreaMouseDownYPos, point.x, point.y);
				else
					// User	just click on a	point. Zoom-in around the mouse	cursor position.
					hasUpdate =	zoomAt(m_zoomDirection,	point.x, point.y, m_zoomInRatio);
			}
			break;
		case Chart::MouseUsageZoomOut:
			// Zoom	out	around the mouse cursor	position.
			if (canZoomOut(m_zoomDirection))
				hasUpdate =	zoomAt(m_zoomDirection,	point.x, point.y, m_zoomOutRatio);
			break;
		default	:
			if (m_isDragScrolling)
				// Drag	to scroll. We can update the image map now as scrolling	has	finished.
				updateViewPort(false, true);
			else
				// Is not zooming or scrolling,	so is just a normal	click event.
				//GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(),	BN_CLICKED), (LPARAM)m_hWnd);
				GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_LButtonUpChart),
				(LPARAM)m_hWnd);
			break;
		}

		m_isDragScrolling =	false;
		if (hasUpdate)
			// View	port has changed - update it.
			updateViewPort(true, true);
	}
	else
	{
		ReleaseCapture();
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_LButtonUpChart), (LPARAM)m_hWnd);
		CStatic::OnLButtonUp(nFlags, point);
	}
}

void CChartViewer::OnRButtonDown(UINT nFlags, CPoint point)
{
	/*
	OnDelayedMouseMove();

	if (inPlotArea(point.x, point.y) && (m_mouseUsage != Chart::MouseUsageDefault))
	{
		// Mouse usage is for drag to zoom/scroll. Capture the mouse to	prepare	for	dragging and 
		// save	the	mouse down position	to draw	the	selection rectangle.
		SetCapture();
		m_isPlotAreaMouseDown = true;
		m_plotAreaMouseDownXPos = point.x;
		m_plotAreaMouseDownYPos = point.y;
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_RButtonDownChart),
			(LPARAM)m_hWnd);

		startDrag();
	}
	else
	{
		m_currentMouseX = point.x;
		m_currentMouseY = point.y;
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_RButtonDownChart),
			(LPARAM)m_hWnd);

		CMenu mnuPopupSubmit;
		mnuPopupSubmit.LoadMenu(IDR_POPUP_CHART_MULTI);

		
		// Get a pointer to the first item of the menu
		CMenu *mnuPopupMenu = mnuPopupSubmit.GetSubMenu(0);
		ASSERT(mnuPopupMenu);
		VtGlobal* global = VtGlobal::GetInstance();
		if (global->IndexList.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(5);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("지표")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("지표"));
			int i = 0;
			for (auto it = global->IndexList.begin(); it != global->IndexList.end(); ++it)
			{
				VtIndexBase* index = *it;
				CString strMenuItem;
				strMenuItem = index->Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_INDEX_0 + i, strMenuItem);
				i++;
			}
			SubPopup.Detach();
		}



		if (global->SystemList.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(5);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("시스템")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("시스템"));
			int i = 0;
			for (auto it = global->SystemList.begin(); it != global->SystemList.end(); ++it)
			{
				VtSystemTemplate sys = *it;
				CString strMenuItem;
				strMenuItem = sys.Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_SYSTEM_0 + i, strMenuItem);
				i++;
			}
			SubPopup.Detach();
		}


		if (global->BandList.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(5);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("VR밴드")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("VR밴드"));
			int i = 0;
			for (auto it = global->BandList.begin(); it != global->BandList.end(); ++it)
			{
				VtBandBase* band = *it;
				CString strMenuItem;
				strMenuItem = band->Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_INSERT_BAND_0 + i, strMenuItem);
				i++;
			}
			SubPopup.Detach();
		}

		if (global->StressList.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(5);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("VR전략구간")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("VR전략구간"));
			int i = 0;
			for (auto it = global->StressList.begin(); it != global->StressList.end(); ++it)
			{
				VtStressBase* band = *it;
				CString strMenuItem;
				strMenuItem = band->Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_INSERT_STRESS_0 + i, strMenuItem);
				i++;
			}
			SubPopup.Detach();
		}


		if (_ChartFrm && _ChartFrm->IndexVector.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(6);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("지표")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("지표"));
			for (int i = 0; i < _ChartFrm->IndexVector.size(); i++)
			{
				CString strMenuItem;
				VtBaseIndex* index = _ChartFrm->IndexVector[i];
				strMenuItem = index->IndexBase.Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_DEL_INDEX_0 + i, strMenuItem);
			}

			InsertMenu(SubPopup.m_hMenu, ID_DEL_INDEX_0 + _ChartFrm->IndexVector.size(), MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
			SubPopup.AppendMenu(MF_STRING, ID_DEL_INDEX_ALL, _T("모두삭제"));
			SubPopup.Detach();
		}
		else
		{
			mnuPopupMenu->EnableMenuItem(ID_REMOVEFROMCHART_INDEX, FALSE ? 0 : MF_DISABLED | MF_GRAYED);
		}

		if (_ChartFrm && _ChartFrm->BandVector.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(6);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("VR밴드")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("VR밴드"));
			for (int i = 0; i < _ChartFrm->BandVector.size(); i++)
			{
				CString strMenuItem;
				VtBaseBand* index = _ChartFrm->BandVector[i];
				strMenuItem = index->BandBase.Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_REMOVE_BAND_0 + i, strMenuItem);
			}

			//InsertMenu(SubPopup.m_hMenu, ID_DEL_INDEX_0 + _ChartFrm->IndexVector.size(), MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
			//SubPopup.AppendMenu(MF_STRING, ID_DEL_INDEX_ALL, _T("모두삭제"));
			//SubPopup.Detach();
		}
		else
		{
			mnuPopupMenu->EnableMenuItem(ID_REMOVE_BAND, FALSE ? 0 : MF_DISABLED | MF_GRAYED);
		}

		
		if (_ChartFrm && _ChartFrm->StressVector.size() > 0)
		{
			CMenu* delIndexMenu = mnuPopupMenu->GetSubMenu(6);
			int itemCount = delIndexMenu->GetMenuItemCount();
			for (int i = 0; i < itemCount; i++)
			{
				CString str;
				delIndexMenu->GetMenuString(i, str, MF_BYPOSITION);
				if (str.Compare(_T("VR전략구간")) == 0)
				{
					delIndexMenu->DeleteMenu(i, MF_BYPOSITION);
					break;
				}
			}
			CMenu SubPopup;
			SubPopup.CreatePopupMenu();
			delIndexMenu->AppendMenu(MF_POPUP, (UINT)SubPopup.m_hMenu, _T("VR전략구간"));
			for (int i = 0; i < _ChartFrm->StressVector.size(); i++)
			{
				CString strMenuItem;
				VtBaseStress* index = _ChartFrm->StressVector[i];
				strMenuItem = index->StressBase.Name.c_str();
				SubPopup.AppendMenu(MF_STRING, ID_REMOVE_STRESS_0 + i, strMenuItem);
			}

			//InsertMenu(SubPopup.m_hMenu, ID_DEL_INDEX_0 + _ChartFrm->IndexVector.size(), MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
			//SubPopup.AppendMenu(MF_STRING, ID_DEL_INDEX_ALL, _T("모두삭제"));
			//SubPopup.Detach();
		}
		else
		{
			mnuPopupMenu->EnableMenuItem(ID_REMOVE_STRESS, FALSE ? 0 : MF_DISABLED | MF_GRAYED);
		}


		if (_ChartFrm)
		{
			if (_ChartFrm->System())
			{
				mnuPopupMenu->EnableMenuItem(ID_REMOVEFROMCHART_SYSTEM, 0);
			}
			else
			{
				mnuPopupMenu->EnableMenuItem(ID_REMOVEFROMCHART_SYSTEM, FALSE ? 0 : MF_DISABLED | MF_GRAYED);
			}
		}

		ClientToScreen(&point);
		// Find out if the user right-clicked the button
		mnuPopupMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}
	*/
}

/*
void CChartViewer::OnProduct(UINT id)
{
	_CurProductID = id;
	CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
	pMain->OnProduct(id);
}

void CChartViewer::OnUpdateProduct(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(_CurProductID == pCmdUI->m_nID);
}

*/
void CChartViewer::OnChartPattern(UINT id)
{
	if (!_ChartWindow)
		return;
	/*
	if (id == ID_CHART_TYPE_CANDLESTIC)
		_ChartFrm->MainChartType(VtMainChartType::CandleStick);
	else if (id == ID_CHART_TYPE_LINE)
		_ChartFrm->MainChartType(VtMainChartType::ClosePrice);
	else if (id == ID_CHART_TYPE_MEIANPRICE)
		_ChartFrm->MainChartType(VtMainChartType::MedianPrice);
	else if (id == ID_CHART_TYPE_OHLC)
		_ChartFrm->MainChartType(VtMainChartType::OHLC);
	else if (id == ID_CHART_TYPE_TP)
		_ChartFrm->MainChartType(VtMainChartType::TypicalPrice);
	else if (id == ID_CHART_TYPE_WP)
		_ChartFrm->MainChartType(VtMainChartType::WeightedClose);
	*/
	updateViewPort(true, false);
}

/*
void CChartViewer::OnUpdateChartPattern(CCmdUI* pCmdUI)
{

}
*/



void CChartViewer::OnProductBithumb(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductBithumb(id);
}

void CChartViewer::OnUpdateProductBithumb(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateProductBithumb(pCmdUI);
}

void CChartViewer::OnProductKorbit(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductKorbit(id);
}

void CChartViewer::OnUpdateProductCoinwon(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateProductCoinwon(pCmdUI);
}

void CChartViewer::OnProductBitrex(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductBitrex(id);
}

void CChartViewer::OnUpdateProductBitrex(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateProductBitrex(pCmdUI);
}

void CChartViewer::OnProductKraken(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductKraken(id);
}

void CChartViewer::OnUpdateProductKraken(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateProductKraken(pCmdUI);
}

void CChartViewer::OnProductBitfinex(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductBitfinex(id);
}

void CChartViewer::OnUpdateProductBitfinex(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateProductBitfinex(pCmdUI);
}

void CChartViewer::OnProductPoloniex(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductPoloniex(id);
}

void CChartViewer::OnUpdateProductPoloniex(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateProductPoloniex(pCmdUI);
}


void CChartViewer::OnProductCompare0(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductCompare0(id);
}

void CChartViewer::OnProductCompare1(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductCompare1(id);
}

void CChartViewer::OnProductCompare2(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductCompare2(id);
}

void CChartViewer::OnProductCompare3(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductCompare3(id);
}

void CChartViewer::OnProductCompare4(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductCompare4(id);
}

void CChartViewer::OnProductCompare5(UINT id)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnProductCompare5(id);
}



void CChartViewer::OnShowChartCtrl()
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnShowChartCtrl();
}

void CChartViewer::OnUpdateShowChartCtrl(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateShowChartCtrl(pCmdUI);
}


void CChartViewer::OnChartIndex(UINT id)
{
	if (!_ChartWindow)
		return;

	//int index = id - ID_CHART_INDEX_MA;
	//_ChartFrm->ShowChartIndex(index);
}


void CChartViewer::OnUpdateChartPattern(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateChartPattern(pCmdUI);
}

void CChartViewer::OnUpdateChartIndex(CCmdUI* pCmdUI)
{
	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	//mainFrm->OnUpdateChartIndex(pCmdUI);
}


void CChartViewer::OnDelIndex(UINT id)
{
// 	int index = id - ID_DEL_INDEX_0;
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->DeleteIndex(index);
// 	}
}

void CChartViewer::OnDelIndexAll()
{
	if (_ChartWindow)
	{
		//_ChartFrm->DeleteAllChartIndex();
	}
}

void CChartViewer::OnInsertIndex(UINT id)
{
// 	int index = id - ID_INDEX_0;
// 	VtGlobal* global = VtGlobal::GetInstance();
// 	VtIndexBase* indexBase = global->IndexList[index];
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->InsertIndex(indexBase);
// 	}
}

void CChartViewer::OnInsertSystem(UINT id)
{
// 	int sysIndex = id - ID_SYSTEM_0;
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->InsertSystem(sysIndex);
// 	}
}

void CChartViewer::OnDelSystem()
{
	if (_ChartWindow)
	{
		//_ChartFrm->DeleteSystem();
	}
}

void CChartViewer::OnInsertBand(UINT id)
{
// 	int index = id - ID_INSERT_BAND_0;
// 	VtGlobal* global = VtGlobal::GetInstance();
// 	VtBandBase* indexBase = global->BandList[index];
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->InsertBand(indexBase);
// 	}
}

void CChartViewer::OnInsertStress(UINT id)
{
// 	int index = id - ID_INSERT_STRESS_0;
// 	VtGlobal* global = VtGlobal::GetInstance();
// 	VtStressBase* indexBase = global->StressList[index];
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->InsertStress(indexBase);
// 	}
}

void CChartViewer::OnRemoveBand(UINT id)
{
// 	int index = id - ID_REMOVE_BAND_0;
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->DeleteBand(index);
// 	}
}

void CChartViewer::OnRemoveStress(UINT id)
{
// 	int index = id - ID_REMOVE_STRESS_0;
// 	if (_ChartFrm)
// 	{
// 		_ChartFrm->DeleteStress(index);
// 	}
}

//
// Custom OnPaint to support alpha transparency
//
void CChartViewer::OnPaint() 
{
	CPaintDC dc(this);
		
	CDC dcMem ;
	if (!dcMem.CreateCompatibleDC(&dc))
		return ;

	HBITMAP h = GetBitmap();
	if (h == NULL)
		return;

	CBitmap cbmp;
	if (!cbmp.Attach(h))
		return;

	BITMAP bmp;
	cbmp.GetBitmap(&bmp);

	CBitmap* oldCbmp = dcMem.SelectObject(&cbmp);

	if (bmp.bmBitsPixel == 32)
	{
		BLENDFUNCTION bfn = {0};
		bfn.BlendOp = AC_SRC_OVER;
		bfn.BlendFlags = 0;
		bfn.SourceConstantAlpha = 255;
		bfn.AlphaFormat = AC_SRC_ALPHA;

		AlphaBlend(dc.GetSafeHdc(), 0, 0, bmp.bmWidth, bmp.bmHeight, dcMem.GetSafeHdc(), 
			0, 0, bmp.bmWidth, bmp.bmHeight, bfn);
	}
	else
		dc.BitBlt(0, 0, bmp.bmWidth, bmp.bmHeight, &dcMem, 0, 0, SRCCOPY);
	
	dcMem.SelectObject(oldCbmp);
	cbmp.Detach();
}

//
// Chart hold timer.
//
void CChartViewer::OnTimer(UINT_PTR nIDEvent)	
{
	if (nIDEvent == DELAYED_MOUSE_MOVE_TIMER)
	{
		// Is a delayed mouse move event
		OnDelayedMouseMove();
	}
	else
	{	
		// Is a delayed view port update
		m_holdTimerActive =	false;

		// Reset the timer
		KillTimer(nIDEvent);
		
		// If has pending chart	view port update request, handles them now.
		if (m_needUpdateChart || m_needUpdateImageMap)
			updateViewPort(m_needUpdateChart, m_needUpdateImageMap);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CChartViewer	overrides

BOOL CChartViewer::PreTranslateMessage(MSG*	pMsg) 
{
	// Remember	to forward mouse messages to the CToolTipCtrl
	BOOL res = CStatic::PreTranslateMessage(pMsg);
	/*
	if (pMsg->message == WM_MOUSEMOVE)
	{
		CPoint point;
		GetCursorPos(&point);
		CWnd* pWnd = WindowFromPoint(point);
		if (pWnd == this)
		{
			pWnd->SetFocus();
		}
	}
	*/
	m_ToolTip.RelayEvent(pMsg);
	return res;
}

/////////////////////////////////////////////////////////////////////////////
// CChartViewer	properties

//
// Set the chart to	the	control
//
void CChartViewer::setChart(BaseChart *c)
{
	// In case the user	forgets	to check the "Notify" check	box	in the Dialog editor, we set it
	// ourselves so	the	CChartViewer control can receive mouse events.
	if ((GetStyle()	& SS_NOTIFY) ==	0)
		ModifyStyle(0, SS_NOTIFY);

	m_currentChart = c;
	setImageMap(0);

	if (0 != c)
	{
		commitPendingSyncAxis(c);
		if (m_delayUpdateChart != NO_DELAY)
			c->makeChart();
	}

	updateDisplay();
}

//
// Get back	the	same BaseChart pointer provided	by the previous	setChart call.
//
BaseChart *CChartViewer::getChart()
{
	return m_currentChart;
}

//
// Set the CViewPortControl to be associated with this control
//
void CChartViewer::setViewPortControl(CViewPortControl *vpc)
{
	if (m_ReentrantGuard)
		return;
	m_ReentrantGuard = true;
	
	if (0 != m_vpControl)
		m_vpControl->setViewer(0);
	m_vpControl = vpc;
	if (0 != m_vpControl)
		m_vpControl->setViewer(this);

	m_ReentrantGuard = false;
}


//
// Get the CViewPortControl that is associated with this control
//
CViewPortControl *CChartViewer::getViewPortControl()
{
	return m_vpControl;
}

//
// Set image map used by the chart
//
void CChartViewer::setImageMap(const char *imageMap)
{
	//delete the existing ImageMapHandler
	if (0 != m_hotSpotTester)
		delete m_hotSpotTester;
	m_currentHotSpot = -1;
	m_isClickable =	false;
	
	//create a new ImageMapHandler to represent	the	image map
	if ((0 == imageMap)	|| (0 == *imageMap))
		m_hotSpotTester	= 0;
	else
		m_hotSpotTester	= new ImageMapHandler(imageMap);
}

//
// Get the image map handler for the chart
//
ImageMapHandler	*CChartViewer::getImageMapHandler()
{
	return m_hotSpotTester;
}

//
// Set the default tool	tip	to use
//
void CChartViewer::setDefaultToolTip(LPCTSTR text)
{
	m_defaultToolTip = text;
}

//
// Get the CToolTipCtrl	for	managing tool tips.
//
CToolTipCtrl *CChartViewer::getToolTipCtrl()
{
	return &m_ToolTip;
}

//
// Set the border width	of the selection box
//
void CChartViewer::setSelectionBorderWidth(int width)
{
	m_selectBoxLineWidth = width;
}

//
// Get the border with of the selection	box.
//
int	CChartViewer::getSelectionBorderWidth()
{
	return m_selectBoxLineWidth;
}

//
// Set the border color	of the selection box
//
void CChartViewer::setSelectionBorderColor(COLORREF	c)
{
	m_selectBoxLineColor = c;
	if (m_TopLine.m_hWnd !=	0)
	{
		m_TopLine.SetColor(c);
		m_LeftLine.SetColor(c);
		m_BottomLine.SetColor(c);
		m_RightLine.SetColor(c);
	}
}

//
// Get the border color	of the selection box.
//
COLORREF CChartViewer::getSelectionBorderColor()
{
	return m_selectBoxLineColor;
}

//
// Set the mouse usage mode
//
void CChartViewer::setMouseUsage(int mouseUsage)
{
	m_mouseUsage = mouseUsage;
}

//
// Get the mouse usage mode
//
int	CChartViewer::getMouseUsage()
{
	return m_mouseUsage;
}

//
// Set the zoom	direction
//
void CChartViewer::setZoomDirection(int	direction)
{
	m_zoomDirection	= direction;
}

//
// Get the zoom	direction
//
int	CChartViewer::getZoomDirection()
{
	return m_zoomDirection;
}

//
// Set the scroll direction
//
void CChartViewer::setScrollDirection(int direction)
{
	m_scrollDirection =	direction;
}

//
// Get the scroll direction
//
int	CChartViewer::getScrollDirection()
{
	return m_scrollDirection;
}

//
// Set the zoom-in ratio for mouse click zoom-in
//
void CChartViewer::setZoomInRatio(double ratio)
{
	m_zoomInRatio =	ratio;
}

//
// Get the zoom-in ratio for mouse click zoom-in
//
double CChartViewer::getZoomInRatio()
{
	return m_zoomInRatio;
}

//
// Set the zoom-out	ratio
//
void CChartViewer::setZoomOutRatio(double ratio)
{
	m_zoomOutRatio = ratio;
}

//
// Get the zoom-out	ratio
//
double CChartViewer::getZoomOutRatio()
{
	return m_zoomOutRatio;	
}

//
// Set the mouse wheel zoom ratio 
//
void CChartViewer::setMouseWheelZoomRatio(double ratio)
{
	m_mouseWheelZoomRatio = ratio;
}

//
// Get the mouse wheel zoom ratio 
//
double CChartViewer::getMouseWheelZoomRatio()
{
	return m_mouseWheelZoomRatio;	
}

//
// Set the minimum mouse drag before the dragging is considered	as real. This is to	avoid small	
// mouse vibrations	triggering a mouse drag.
//
void CChartViewer::setMinimumDrag(int offset)
{
	m_minDragAmount	= offset;
}

//
// Get the minimum mouse drag before the dragging is considered	as real.
//
int	CChartViewer::getMinimumDrag()
{
	return m_minDragAmount;
}

//
// Set the minimum interval	between	ViewPortChanged	events.	This is	to avoid the chart being 
// updated too frequently. (Default	is 20ms	between	chart updates.)	Multiple update	events
// arrived during the interval will	be merged into one chart update	and	executed at	the	end
// of the interval.
//
void CChartViewer::setUpdateInterval(int interval)
{
	m_updateInterval = interval;
}

//
// Get the minimum interval	between	ViewPortChanged	events.	
//
int	CChartViewer::getUpdateInterval()
{
	return m_updateInterval;
}

//
// Check if	there is a pending chart update	request. 
//
bool CChartViewer::needUpdateChart()
{
	return m_needUpdateChart;
}

//
// Check if	there is a pending image map update	request. 
//
bool CChartViewer::needUpdateImageMap()
{
	return m_needUpdateImageMap;
}

//
// Get the current mouse x coordinate when used in a mouse move event handler
//
int CChartViewer::getChartMouseX()
{
	int ret = m_currentMouseX;
	if (ret < 0)
		ret = getPlotAreaLeft() + getPlotAreaWidth();
	return ret;
}

//
// Get the current mouse y coordinate when used in a mouse move event handler
//
int CChartViewer::getChartMouseY()
{
	int ret = m_currentMouseY;
	if (ret < 0)
		ret = getPlotAreaTop() + getPlotAreaHeight();
	return ret;
}

//
// Get the current mouse x coordinate bounded to the plot area when used in a mouse event handler
//
int CChartViewer::getPlotAreaMouseX()
{
	int ret = getChartMouseX();
	if (ret < getPlotAreaLeft())
		ret = getPlotAreaLeft();
	if (ret > getPlotAreaLeft() + getPlotAreaWidth())
		ret = getPlotAreaLeft() + getPlotAreaWidth();
	return ret;
}

//
// Get the current mouse y coordinate bounded to the plot area when used in a mouse event handler
//
int CChartViewer::getPlotAreaMouseY()
{
	int ret = getChartMouseY();
	if (ret < getPlotAreaTop())
		ret = getPlotAreaTop();
	if (ret > getPlotAreaTop() + getPlotAreaHeight())
		ret = getPlotAreaTop() + getPlotAreaHeight();
	return ret;
}

//
// Check if mouse is on the extended plot area
//
bool CChartViewer::isMouseOnPlotArea()
{
	if (this->m_isMouseTracking)
		return inExtendedPlotArea(getChartMouseX(), getChartMouseY());
	else
		return false;
}

//
// Check if mouse is dragging to scroll or to select the zoom rectangle
//
bool CChartViewer::isMouseDragging()
{
	return m_isPlotAreaMouseDown;
}

//
// Check if is currently processing a mouse move event
//
bool CChartViewer::isInMouseMoveEvent()
{
	return m_isInMouseMove;
}

/////////////////////////////////////////////////////////////////////////////
// CChartViewer	methods

//
// Update the display
//
void CChartViewer::updateDisplay()
{
	if (m_delayUpdateChart == NO_DELAY)
		commitUpdateChart();
	else
	{
		m_delayUpdateChart = NEED_UPDATE;
		delete m_delayedChart;
		m_delayedChart = (0 != m_currentChart) ? new BaseChart(m_currentChart) : 0;
	}
}

//
// Commit chart to display
//
void CChartViewer::commitUpdateChart()
{
	try
	{
		if (m_delayUpdateChart == NEED_DELAY)
		{
			// No actual update occur
			m_delayUpdateChart = NO_DELAY;
			return;
		}


		// Get the HBITMAP and metrics for the chart
		BaseChart *c = (m_delayUpdateChart == NEED_UPDATE) ? m_delayedChart : m_currentChart;
		HBITMAP	chartBMP = 0;
		const char *chartMetrics = 0;

		if (0 != c)
		{
			// Output chart	as Device Indpendent Bitmap	with file headers
			c->setTransparentColor(-2);
			MemBlock m = c->makeChart(Chart::BMP);

			const BITMAPINFO *bmp = (const BITMAPINFO *)(m.data + 14);

			// MFC expects HBITMAP,	so convert from	DIB	to HBITMAP.	
			CDC	*cdc = GetDC();
			void *buffer;
			chartBMP = CreateDIBSection(cdc->m_hDC, bmp, DIB_RGB_COLORS, &buffer, NULL, 0x0);
			if (buffer)
			{
				memcpy(buffer, m.data + *(int *)(m.data + 10), bmp->bmiHeader.biSizeImage);
			}

			ReleaseDC(cdc);

			// Get chart metrics
			chartMetrics = c->getChartMetrics();
		}

		// Set the HBITMAP for display
		if (0 != (chartBMP = SetBitmap(chartBMP)))
			DeleteObject(chartBMP);

		// Set the chart metrics and clear the image map
		setChartMetrics(chartMetrics);

		// Remember to update the selection rectangle
		if ((0 != m_TopLine.m_hWnd) && m_TopLine.IsWindowVisible())
		{
			m_TopLine.Invalidate();
			m_LeftLine.Invalidate();
			m_RightLine.Invalidate();
			m_BottomLine.Invalidate();
			m_TopLine.UpdateWindow();
			m_LeftLine.UpdateWindow();
			m_RightLine.UpdateWindow();
			m_BottomLine.UpdateWindow();
		}

		// Any delayed chart has been committed
		m_delayUpdateChart = NO_DELAY;
		delete m_delayedChart;
		m_delayedChart = 0;
	}
	catch (std::exception e)
	{
		std::string msg = e.what();
	}
}

//
// Set the message used to remove the dynamic layer
//
void CChartViewer::removeDynamicLayer(int msg)
{
	m_autoHideMsg = msg;
	if (msg == -1)
		applyAutoHide(msg);
}

//
// Attempt to hide the dynamic layer using the specified message
//
void CChartViewer::applyAutoHide(int msg)
{
	if (m_autoHideMsg == msg)
	{
		if (0 != m_currentChart)
			m_currentChart->removeDynamicLayer();
		m_autoHideMsg = 0;

	    updateDisplay();
	}
}

void CChartViewer::SetMouseInfo(UINT a_flags, CPoint a_Point)
{
	nFlags = a_flags;
	MousePoint = a_Point;
}

//
// Create the edges	for	the	selection rectangle
//
void CChartViewer::initRect()
{
	m_TopLine.Create(GetParent(), m_selectBoxLineColor);
	m_LeftLine.Create(GetParent(), m_selectBoxLineColor);
	m_BottomLine.Create(GetParent(), m_selectBoxLineColor);
	m_RightLine.Create(GetParent(),	m_selectBoxLineColor);
}

//
// Set selection rectangle position	and	size
//
void CChartViewer::drawRect(int	x, int y, int width, int height)
{
	// Create the edges	of the rectangle if	not	already	created
	if (m_TopLine.m_hWnd ==	0)
		initRect();

	// width < 0 is	interpreted	as the rectangle extends to	the	left or	x.
	// height <0 is	interpreted	as the rectangle extends to	above y.
	if (width <	0)
		x -= (width	= -width);
	if (height < 0)
		y -= (height = -height);

	// Compute the position	of the selection rectangle as relative to the parent window
	RECT rect;
	rect.left =	x;
	rect.top = y;
	rect.right = x + width;
	rect.bottom	= y	+ height;
	MapWindowPoints(m_TopLine.GetParent(), &rect);

	// Put the edges along the sides of	the	rectangle
	m_TopLine.MoveWindow(rect.left,	rect.top, rect.right - rect.left, m_selectBoxLineWidth);
	m_LeftLine.MoveWindow(rect.left, rect.top, m_selectBoxLineWidth, rect.bottom - rect.top);
	m_BottomLine.MoveWindow(rect.left, rect.bottom - m_selectBoxLineWidth +	1, 
		rect.right - rect.left,	m_selectBoxLineWidth);
	m_RightLine.MoveWindow(rect.right -	m_selectBoxLineWidth + 1, rect.top,	
		m_selectBoxLineWidth, rect.bottom -	rect.top);
}

//
// Show/hide selection rectangle
//
void CChartViewer::setRectVisible(bool b)
{
	// Create the edges	of the rectangle if	not	already	created
	if (b && (m_TopLine.m_hWnd == 0)) 
		initRect();

	// Show/hide the edges
	if (m_TopLine.m_hWnd != 0)
	{
		int	state =	b ?	SW_SHOW	: SW_HIDE;
		m_TopLine.ShowWindow(state);
		m_LeftLine.ShowWindow(state);
		m_BottomLine.ShowWindow(state);
		m_RightLine.ShowWindow(state);
	}
}

//
// Determines if the mouse is dragging.
//
bool CChartViewer::isDrag(int direction, CPoint	point)
{
	// We only consider	the	mouse is dragging it is	has	dragged	more than m_minDragAmount. This	is
	// to avoid	small mouse	vibrations triggering a	mouse drag.
	int	spanX =	abs(point.x	- m_plotAreaMouseDownXPos);
	int	spanY =	abs(point.y	- m_plotAreaMouseDownYPos);
	return ((direction != Chart::DirectionVertical)	&& (spanX >= m_minDragAmount)) ||
		((direction	!= Chart::DirectionHorizontal) && (spanY >=	m_minDragAmount));
}

//
// Process mouse dragging over the plot	area
//
void CChartViewer::OnPlotAreaMouseDrag(UINT	/* nFlags */, CPoint point)
{
	switch (m_mouseUsage)
	{
		case Chart::MouseUsageZoomIn :
		{
			//
			// Mouse is	used for zoom in. Draw the selection rectangle if necessary.
			//

			bool isDragZoom	= canZoomIn(m_zoomDirection) &&	isDrag(m_zoomDirection,	point);
			if (isDragZoom)
			{
				int	spanX =	m_plotAreaMouseDownXPos	- point.x;
				int	spanY =	m_plotAreaMouseDownYPos	- point.y;

				switch (m_zoomDirection)
				{
				case Chart::DirectionHorizontal:
					drawRect(point.x, getPlotAreaTop(),	spanX, getPlotAreaHeight());
					break;
				case Chart::DirectionVertical:
					drawRect(getPlotAreaLeft(),	point.y, getPlotAreaWidth(), spanY);
					break;
				default:
					drawRect(point.x, point.y, spanX, spanY);
					break;
				}
			}
			setRectVisible(isDragZoom);
			break;
		}
		case Chart::MouseUsageScroll :
		{
			//
			// Mouse is	used for drag scrolling. Scroll	and	update the view	port.
			//

			if (m_isDragScrolling || isDrag(m_scrollDirection, point))
			{
				m_isDragScrolling =	true;
				switch (m_scrollDirection)
				{
				case Chart::DirectionHorizontal:
					::SetCursor(getNoMoveHorizCursor());
					break;
				case Chart::DirectionVertical:
					::SetCursor(getNoMoveVertCursor());
					break;
				default	:
					::SetCursor(getNoMove2DCursor());
					break;
				}
								
				if (dragTo(m_scrollDirection, 
					point.x	- m_plotAreaMouseDownXPos, point.y - m_plotAreaMouseDownYPos))
					updateViewPort(true, false);
			}
		}
	}
}

//
// Trigger the ViewPortChanged event
//
void CChartViewer::updateViewPort(bool needUpdateChart,	bool needUpdateImageMap)
{
	// Merge the current update	requests with any pending requests.
	m_needUpdateChart =	m_needUpdateChart || needUpdateChart;
	m_needUpdateImageMap = needUpdateImageMap;

	// Hold	timer has not expired, so do not update	chart immediately. Keep	the	requests pending.
	if (m_holdTimerActive)
		return;

	// The chart can be updated more than once during mouse move. For example, it can update due to
	// drag to scroll, and also due to drawing track cursor. So we delay updating the display until
	// all all updates has occured.
	int hasDelayUpdate = (m_delayUpdateChart != NO_DELAY);
	if (!hasDelayUpdate)
		m_delayUpdateChart = NEED_DELAY;
	
	// Can trigger the ViewPortChanged event.
	validateViewPort();
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(),	CVN_ViewPortChanged), 
		(LPARAM)m_hWnd);
	
	if (m_needUpdateChart && (0 != m_vpControl))
		m_vpControl->updateDisplay();

	// Can update chart now
	if (!hasDelayUpdate)
		commitUpdateChart();

	// Clear any pending updates.
	m_needUpdateChart =	false;
	m_needUpdateImageMap = false;

	// Set hold	timer to prevent multiple chart	updates	within a short period.
	if (m_updateInterval > 0)
	{
		m_holdTimerActive = true;
		SetTimer(UPDATE_VIEW_PORT_TIMER, m_updateInterval, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRectCtrl
//
// A rectangle with	a background color.	Use	as thick edges for the selection
// rectangle.
//

//
// Create control with a given color
//
BOOL CRectCtrl::Create(CWnd	*pParentWnd, COLORREF c)
{
	SetColor(c);

	RECT r;
	r.left = r.top = r.right = r.bottom	= 0;
	return CStatic::Create(0, WS_CHILD,	r, pParentWnd);
}

BEGIN_MESSAGE_MAP(CRectCtrl, CStatic)
	//{{AFX_MSG_MAP(CRectCtrl)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRectCtrl message handlers

HBRUSH CRectCtrl::CtlColor(CDC*	/* pDC */, UINT	/* nCtlColor */) 
{
	return (HBRUSH)m_Color.m_hObject;
}

/////////////////////////////////////////////////////////////////////////////
// CRectCtrl proporties

//
// Set the background color
//
void CRectCtrl::SetColor(COLORREF c)
{
	m_Color.CreateSolidBrush(c);
}

/////////////////////////////////////////////////////////////////////////////
// Build in	mouse cursors for zooming and scrolling	support
//

static const unsigned int zoomInCursorA[] = 
{ 
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xff3ff8ff,
0xff0fe0ff,
0xff07c0ff,
0xff07c0ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff07c0ff,
0xff07c0ff,
0xff01e0ff,
0xff30f8ff,
0x7ff0ffff,
0x3ff8ffff,
0x1ffcffff,
0x0ffeffff,
0x0fffffff,
0x9fffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff
};

static const unsigned int zoomInCursorB[] = 
{
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00c00700,
0x00f01f00,
0x00f01e00,
0x00f83e00,
0x00f83e00,
0x00183000,
0x00f83e00,
0x00f83e00,
0x00f01e00,
0x00f01f00,
0x00c00700,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000
};

static const unsigned int zoomOutCursorA[] =	
{ 
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xff3ff8ff,
0xff0fe0ff,
0xff07c0ff,
0xff07c0ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff07c0ff,
0xff07c0ff,
0xff01e0ff,
0xff30f8ff,
0x7ff0ffff,
0x3ff8ffff,
0x1ffcffff,
0x0ffeffff,
0x0fffffff,
0x9fffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff
};

static const unsigned int zoomOutCursorB[] =	
{
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00c00700,
0x00f01f00,
0x00f01f00,
0x00f83f00,
0x00f83f00,
0x00183000,
0x00f83f00,
0x00f83f00,
0x00f01f00,
0x00f01f00,
0x00c00700,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000
};

static const unsigned int noZoomCursorA[] = 
{ 
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xff3ff8ff,
0xff0fe0ff,
0xff07c0ff,
0xff07c0ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff0380ff,
0xff07c0ff,
0xff07c0ff,
0xff01e0ff,
0xff30f8ff,
0x7ff0ffff,
0x3ff8ffff,
0x1ffcffff,
0x0ffeffff,
0x0fffffff,
0x9fffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff
};

static const unsigned int noZoomCursorB[] = 
{
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00c00700,
0x00f01f00,
0x00f01f00,
0x00f83f00,
0x00f83f00,
0x00f83f00,
0x00f83f00,
0x00f83f00,
0x00f01f00,
0x00f01f00,
0x00c00700,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000
};

static const unsigned int noMove2DCursorA[] =
{
0xffffffff,
0xffffffff,
0xffffffff,
0xff7ffeff,
0xff3ffcff,
0xff1ff8ff,
0xff0ff0ff,
0xff07e0ff,
0xff03c0ff,
0xff03c0ff,
0xfffc3fff,
0x7ffc3ffe,
0x3ffc3ffc,
0x1f7c3ef8,
0x0f3c3cf0,
0x071c38e0,
0x071c38e0,
0x0f3c3cf0,
0x1f7c3ef8,
0x3ffc3ffc,
0x7ffc3ffe,
0xfffc3fff,
0xff03c0ff,
0xff03c0ff,
0xff07e0ff,
0xff0ff0ff,
0xff1ff8ff,
0xff3ffcff,
0xff7ffeff,
0xffffffff,
0xffffffff,
0xffffffff
};

static const unsigned int noMove2DCursorB[] =
{
0x00000000,
0x00000000,
0x00000000,
0x00800100,
0x00400200,
0x00200400,
0x00100800,
0x00081000,
0x00042000,
0x00fc3f00,
0x0003c000,
0x80024001,
0x40024002,
0x20824104,
0x10424208,
0x08224410,
0x08224410,
0x10424208,
0x20824104,
0x40024002,
0x80024001,
0x0003c000,
0x00fc3f00,
0x00042000,
0x00081000,
0x00100800,
0x00200400,
0x00400200,
0x00800100,
0x00000000,
0x00000000,
0x00000000
};

static const unsigned int noMoveHorizCursorA[]	=
{
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xfffc3fff,
0x7ffc3ffe,
0x3ffc3ffc,
0x1f7c3ef8,
0x0f3c3cf0,
0x071c38e0,
0x071c38e0,
0x0f3c3cf0,
0x1f7c3ef8,
0x3ffc3ffc,
0x7ffc3ffe,
0xfffc3fff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff
};

static const unsigned int noMoveHorizCursorB[]	=
{
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x0003c000,
0x80024001,
0x40024002,
0x20824104,
0x10424208,
0x08224410,
0x08224410,
0x10424208,
0x20824104,
0x40024002,
0x80024001,
0x0003c000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000
};

static const unsigned int noMoveVertCursorA[] =
{
0xffffffff,
0xffffffff,
0xffffffff,
0xff7ffeff,
0xff3ffcff,
0xff1ff8ff,
0xff0ff0ff,
0xff07e0ff,
0xff03c0ff,
0xff03c0ff,
0xffffffff,
0xffffffff,
0xffffffff,
0xff7ffeff,
0xff3ffcff,
0xff1ff8ff,
0xff1ff8ff,
0xff3ffcff,
0xff7ffeff,
0xffffffff,
0xffffffff,
0xffffffff,
0xff03c0ff,
0xff03c0ff,
0xff07e0ff,
0xff0ff0ff,
0xff1ff8ff,
0xff3ffcff,
0xff7ffeff,
0xffffffff,
0xffffffff,
0xffffffff
};

static const unsigned int noMoveVertCursorB[] =
{
0x00000000,
0x00000000,
0x00000000,
0x00800100,
0x00400200,
0x00200400,
0x00100800,
0x00081000,
0x00042000,
0x00fc3f00,
0x00000000,
0x00000000,
0x00000000,
0x00800100,
0x00400200,
0x00200400,
0x00200400,
0x00400200,
0x00800100,
0x00000000,
0x00000000,
0x00000000,
0x00fc3f00,
0x00042000,
0x00081000,
0x00100800,
0x00200400,
0x00400200,
0x00800100,
0x00000000,
0x00000000,
0x00000000
};

static HCURSOR hZoomInCursor = 0;
static HCURSOR hZoomOutCursor =	0;
static HCURSOR hNoZoomCursor = 0;
static HCURSOR hNoMove2DCursor = 0;
static HCURSOR hNoMoveHorizCursor =	0;
static HCURSOR hNoMoveVertCursor = 0;

class FreeCursors
{
public:
	~FreeCursors()
	{
		if (0 != hZoomInCursor)
			DestroyCursor(hZoomInCursor);
		if (0 != hZoomOutCursor)
			DestroyCursor(hZoomOutCursor);
		if (0 != hNoZoomCursor)
			DestroyCursor(hNoZoomCursor);
		if (0 != hNoMove2DCursor)
			DestroyCursor(hNoMove2DCursor);
		if (0 != hNoMoveHorizCursor)
			DestroyCursor(hNoMoveHorizCursor);
		if (0 != hNoMoveVertCursor)
			DestroyCursor(hNoMoveVertCursor);
	}
} dummyFreeCursorObj;

static HCURSOR getZoomInCursor()
{
	if (0 == hZoomInCursor)
	{
		hZoomInCursor =	CreateCursor(AfxGetApp()->m_hInstance, 15, 15, 32, 32, 
			zoomInCursorA, zoomInCursorB);
	}
	return hZoomInCursor;
}

static HCURSOR getZoomOutCursor()
{
	if (0 == hZoomOutCursor)
	{
		hZoomOutCursor = CreateCursor(AfxGetApp()->m_hInstance,	15,	15,	32,	32,	
			zoomOutCursorA,	zoomOutCursorB);
	}
	return hZoomOutCursor;
}

static HCURSOR getNoZoomCursor()
{
	if (0 == hNoZoomCursor)
	{
		hNoZoomCursor =	CreateCursor(AfxGetApp()->m_hInstance, 15, 15, 32, 32, 
			noZoomCursorA, noZoomCursorB);
	}
	return hNoZoomCursor;
}

static HCURSOR getNoMove2DCursor()
{
	if (0 == hNoMove2DCursor)
	{
		hNoMove2DCursor	= CreateCursor(AfxGetApp()->m_hInstance, 15, 15, 32, 32, 
			noMove2DCursorA, noMove2DCursorB);
	}
	return hNoMove2DCursor;
}

static HCURSOR getNoMoveHorizCursor()
{
	if (0 == hNoMoveHorizCursor)
	{
		hNoMoveHorizCursor = CreateCursor(AfxGetApp()->m_hInstance,	15,	15,	32,	32,	
			noMoveHorizCursorA,	noMoveHorizCursorB);
	}
	return hNoMoveHorizCursor;
}

static HCURSOR getNoMoveVertCursor()
{
	if (0 == hNoMoveVertCursor)
	{
		hNoMoveVertCursor =	CreateCursor(AfxGetApp()->m_hInstance, 15, 15, 32, 32, 
			noMoveVertCursorA, noMoveVertCursorB);
	}
	return hNoMoveVertCursor;
}


/////////////////////////////////////////////////////////////////////////////
//
// CViewPortControl
//
/////////////////////////////////////////////////////////////////////////////

//
// Constructor
//
CViewPortControl::CViewPortControl()
{
	m_ChartViewer = 0;
	m_Chart = 0;
	m_ReentrantGuard = false;

	m_mouseDownX = 0;
	m_mouseDownY = 0;
}

BEGIN_MESSAGE_MAP(CViewPortControl,	CStatic)
	//{{AFX_MSG_MAP(CChartViewer)
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETCURSOR()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//
// Free	resources
//
void CViewPortControl::OnDestroy() 
{
	setViewer(0);
	
	// Free	HBITMAP
	HBITMAP	b =	SetBitmap(0);
	if (0 != b)
		DeleteObject(b);

	CStatic::OnDestroy();
}

//
// Set the CChartViewer to be associated with this control
//
void CViewPortControl::setViewer(CChartViewer *viewer)
{
	if (m_ReentrantGuard)
		return;
	m_ReentrantGuard = true;

	if (0 != m_ChartViewer)
		m_ChartViewer->setViewPortControl(0);
	m_ChartViewer = viewer;
	setViewPortManager(viewer);
	if (0 != m_ChartViewer)
		m_ChartViewer->setViewPortControl(this);

	m_ReentrantGuard = false;
	
	updateDisplay();
}

//
// Get back	the	same CChartViewer pointer provided	by the previous	setViewer call.
//
CChartViewer *CViewPortControl::getViewer()
{
	return m_ChartViewer;
}

//
// Set the chart to	be displayed in the	control
//
void CViewPortControl::setChart(BaseChart *c)
{
	// In case the user	forgets	to check the "Notify" check	box	in the Dialog editor, we set it
	// ourselves so	the	CChartViewer control can receive mouse events.
	if ((GetStyle()	& SS_NOTIFY) ==	0)
		ModifyStyle(0, SS_NOTIFY);

	m_Chart = c;
	ViewPortControlBase::setChart(c);
	updateDisplay();
}

//
// Get back	the	same BaseChart pointer provided	by the previous	setChart call.
//
BaseChart *CViewPortControl::getChart()
{
	return m_Chart;
}

//
// Convert from mouse x-coordinate to image x-coordinate
//
double CViewPortControl::toImageX(int x)
{
	// In this version, no conversion is done. It is assumed the control does not stretch or shrink
	// the image and does not provide any additional margin to offset the image.
	return x;
}

//
// Convert from mouse x-coordinate to image x-coordinate
//
double CViewPortControl::toImageY(int y)
{
	// In this version, no conversion is done. It is assumed the control does not stretch or shrink
	// the image and does not provide any additional margin to offset the image.
	return y;
}

//
// Display the chart
//
void CViewPortControl::paintDisplay()
{
	BaseChart *c = getChart();
	
	// Get the HBITMAP for the chart
	HBITMAP	chartBMP = 0;
	if (0 != c)
	{
		// Output chart	as Device Indpendent Bitmap	with file headers
		c->setTransparentColor(-2);
		MemBlock m = c->makeChart(Chart::BMP);
		const BITMAPINFO *bmp = (const BITMAPINFO *)(m.data + 14);

		// MFC expects HBITMAP,	so convert from	DIB	to HBITMAP.	
		CDC	*cdc = GetDC();
		void *buffer;
		chartBMP = CreateDIBSection(cdc->m_hDC, bmp, DIB_RGB_COLORS, &buffer, NULL, 0x0);
		memcpy(buffer, m.data + *(int *)(m.data + 10), bmp->bmiHeader.biSizeImage);
		ReleaseDC(cdc);
	}

	// Set the HBITMAP for display
	if (0 != (chartBMP = SetBitmap(chartBMP)))
		DeleteObject(chartBMP);
}

//
// Custom OnPaint to support alpha transparency
//
void CViewPortControl::OnPaint() 
{
	CPaintDC dc(this);
		
	CDC dcMem ;
	if (!dcMem.CreateCompatibleDC(&dc))
		return ;

	HBITMAP h = GetBitmap();
	if (h == NULL)
		return;

	CBitmap cbmp;
	if (!cbmp.Attach(h))
		return;

	BITMAP bmp;
	cbmp.GetBitmap(&bmp);

	CBitmap* oldCbmp = dcMem.SelectObject(&cbmp);

	if (bmp.bmBitsPixel == 32)
	{
		BLENDFUNCTION bfn = {0};
		bfn.BlendOp = AC_SRC_OVER;
		bfn.BlendFlags = 0;
		bfn.SourceConstantAlpha = 255;
		bfn.AlphaFormat = AC_SRC_ALPHA;

		AlphaBlend(dc.GetSafeHdc(), 0, 0, bmp.bmWidth, bmp.bmHeight, dcMem.GetSafeHdc(), 
			0, 0, bmp.bmWidth, bmp.bmHeight, bfn);
	}
	else
		dc.BitBlt(0, 0, bmp.bmWidth, bmp.bmHeight, &dcMem, 0, 0, SRCCOPY);
	
	dcMem.SelectObject(oldCbmp);
	cbmp.Detach();
}

//
// Mouse left button down event.
//
void CViewPortControl::OnLButtonDown(UINT nFlags, CPoint point)
{
	CStatic::OnLButtonDown(nFlags, point);

	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_LButtonDownChart),
		(LPARAM)m_hWnd);

	// Enable mouse capture for drag support
	SetCapture();

	// Remember current mouse position
	m_mouseDownX = point.x;
	m_mouseDownY = point.y;

	// Get the CChartViewer zoom/scroll state to determine which type of mouse action is allowed
	syncState();

	// Handle the mouse down event
	handleMouseDown(toImageX(point.x), toImageY(point.y));

	// Update the chart viewer if the viewport has changed
	updateChartViewerIfNecessary();
}

//
// MouseMove event handler
//
void CViewPortControl::OnMouseMove(UINT nFlags, CPoint point)
{
	// Get the CChartViewer zoom/scroll state to determine which type of mouse action is allowed
	syncState();

	// Handle the mouse move event
	handleMouseMove(toImageX(point.x), toImageY(point.y), isDrag(point));

	// Update the chart viewer if the viewport has changed
	updateChartViewerIfNecessary();

	// Mouse wheel events are only sent to the control in focus. So if mouse wheel zooming is 
	// enabled, we must get the focus in order to receive mouse wheel events.
	if ((0 != m_ChartViewer) && (m_ChartViewer->getMouseWheelZoomRatio() != 1)
		&& isOnPlotArea(toImageX(point.x), toImageY(point.y)))
		SetFocus();

	// Update the mouse cursor
	HCURSOR cursor = getHCursor(getCursor());
	if (cursor != NULL)
		::SetCursor(cursor);

	// Update the display
	if (needUpdateDisplay())
		paintDisplay();
}

//
// Mouse left button up event.
//
void CViewPortControl::OnLButtonUp(UINT nFlags, CPoint point)
{
	CStatic::OnLButtonUp(nFlags, point);
	ReleaseCapture();

	// Get the CChartViewer zoom/scroll state to determine which type of mouse action is allowed
	syncState();

	// Handle the mouse up event
	handleMouseUp(toImageX(point.x), toImageY(point.y));

	// Update the chart viewer if the viewport has changed
	updateChartViewerIfNecessary();
}

//
// MouseWheel handler
//
BOOL CViewPortControl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// Process the mouse wheel only if the mouse is over the plot area
	ScreenToClient(&pt);
	if ((0 == m_ChartViewer) || (!isOnPlotArea(toImageX(pt.x), toImageY(pt.y))))
		return FALSE;

	// Ask the CChartViewer to zoom around the center of the chart
	int x = m_ChartViewer->getPlotAreaLeft() + m_ChartViewer->getPlotAreaWidth() / 2;
	int y = m_ChartViewer->getPlotAreaTop() + m_ChartViewer->getPlotAreaHeight() / 2;
	return m_ChartViewer->onMouseWheelZoom(x, y, zDelta);
}

//
// Get the CChartViewer zoom/scroll state
//
void CViewPortControl::syncState()
{
	CChartViewer *viewer = getViewer();
	if (0 != viewer)
		setZoomScrollDirection(viewer->getZoomDirection(), viewer->getScrollDirection());
}

//
// Determines if the mouse is dragging.
//
bool CViewPortControl::isDrag(CPoint point)
{
	CChartViewer *viewer = getViewer();
	if (0 == viewer)
		return false;

	int minimumDrag = viewer->getMinimumDrag();
	int moveX = abs(m_mouseDownX - point.x);
	int moveY = abs(m_mouseDownY - point.y);
	return (moveX >= minimumDrag) || (moveY >= minimumDrag);
}

//
// Update the display
//
void CViewPortControl::updateDisplay()
{
	paintViewPort();
	paintDisplay();
}

//
// Update the CChartViewer if the viewport has changed
//
void CViewPortControl::updateChartViewerIfNecessary()
{
	CChartViewer *viewer = getViewer();
	if (0 == viewer)
		return;
	
	if (needUpdateChart() || needUpdateImageMap())
		viewer->updateViewPort(needUpdateChart(), needUpdateImageMap());
}

//
// The the mouse cursor
//
HCURSOR CViewPortControl::getHCursor(int position)
{
	switch (position)
	{
	case Chart::Left:
	case Chart::Right:
		//IDC_SIZEWE = 32644
		return AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32644));
	case Chart::Top:
	case Chart::Bottom:
		//IDC_SIZENS = 32645
		return AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32645));
	case Chart::TopLeft:
	case Chart::BottomRight:
		//IDC_SIZENWSE = 32642
		return AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32642));
	case Chart::TopRight:
	case Chart::BottomLeft:
		//IDC_SIZENESW = 32643
		return AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32643));
	}

	//IDC_ARROW = 32512
	return AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32512));
}

//
// Intercept WM_SETCURSOR to change	the	mouse cursor.
//
BOOL CViewPortControl::OnSetCursor(CWnd* pWnd, UINT	nHitTest, UINT message)	
{
	HCURSOR cursor = getHCursor(getCursor());
	if (cursor != NULL)
	{
		::SetCursor(cursor);
		return TRUE;
	}

	return CStatic::OnSetCursor(pWnd, nHitTest,	message);
}


void CChartViewer::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_currentMouseX = point.x;
	m_currentMouseY = point.y;
	GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), CVN_LButtonDblClk),
		(LPARAM)m_hWnd);

	if (_ChartWindow)
	{
		//_ChartFrm->OnLButtonDblClk(nFlags, point);
	}

	__super::OnLButtonDblClk(nFlags, point);
}


int CChartViewer::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	_ComboChartType = new CComboBox();
	_ComboChartType->Create(WS_CHILD | WS_VISIBLE,
		CRect(10, 50, 100, 150), this, 0x1448);
	_ComboChartType->AddString(_T("Candle"));
	_ComboChartType->AddString(_T("Line"));
	return 0;
}


void CChartViewer::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	if (nChar == VK_DELETE)
	{
		AfxMessageBox(_T("Del Clicked!"));
	}
	__super::OnKeyDown(nChar, nRepCnt, nFlags);
}
