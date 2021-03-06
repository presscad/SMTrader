// VtStrategyToolWnd.cpp : implementation file
//

#include "stdafx.h"
#include "VtStrategyToolWnd.h"
#include "afxdialogex.h"
#include "resource.h"
#include "HdWindowManager.h"
#include "Poco/Delegate.h"
#include "VtGlobal.h"
#include "VtSystemGroupManager.h"
using Poco::Delegate;

// VtStrategyToolWnd dialog

IMPLEMENT_DYNAMIC(VtStrategyToolWnd, CDialogEx)

VtStrategyToolWnd::VtStrategyToolWnd(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_STRATEGE_TOOLS, pParent)
{
	if (!VtSystemGroupManager::GetInstance()->SystemGroupLoaded()) {
		VtSystemGroupManager::GetInstance()->InitSystemGroup();
	}
}

VtStrategyToolWnd::~VtStrategyToolWnd()
{
	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	_WindowEvent -= delegate(wndMgr, &HdWindowManager::OnWindowEvent);
}

void VtStrategyToolWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(VtStrategyToolWnd, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// VtStrategyToolWnd message handlers


void VtStrategyToolWnd::SaveToXml(pugi::xml_node& window_node)
{
	CRect rcWnd;
	GetWindowRect(rcWnd);
	pugi::xml_node window_child = window_node.append_child("window_pos");
	window_child.append_attribute("left") = rcWnd.left;
	window_child.append_attribute("top") = rcWnd.top;
	window_child.append_attribute("right") = rcWnd.right;
	window_child.append_attribute("bottom") = rcWnd.bottom;
}

void VtStrategyToolWnd::LoadFromXml(pugi::xml_node& window_node)
{
	pugi::xml_node window_pos_node = window_node.child("window_pos");
	CRect rcWnd;
	rcWnd.left = window_pos_node.attribute("left").as_int();
	rcWnd.top = window_pos_node.attribute("top").as_int();
	rcWnd.right = window_pos_node.attribute("right").as_int();
	rcWnd.bottom = window_pos_node.attribute("bottom").as_int();
	MoveWindow(rcWnd);
	ShowWindow(SW_SHOW);
}

void VtStrategyToolWnd::Save(simple::file_ostream<same_endian_type>& ss)
{
	CRect rcWnd;
	GetWindowRect(rcWnd);
	ss << rcWnd.left << rcWnd.top << rcWnd.right << rcWnd.bottom;
}

void VtStrategyToolWnd::Load(simple::file_istream<same_endian_type>& ss)
{
	CRect rcWnd;
	ss >> rcWnd.left >> rcWnd.top >> rcWnd.right >> rcWnd.bottom;
	MoveWindow(rcWnd);
	ShowWindow(SW_SHOW);
}

void VtStrategyToolWnd::UpdateSystem(VtSystem* sys, bool enable)
{
	if (!sys)
		return;
	_GridCtrl.UpdateSystem(sys, enable);
}

BOOL VtStrategyToolWnd::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	this->SetIcon(hIcon, FALSE);

	_GridCtrl.ToolWnd(this);
	_GridCtrl.AttachGrid(this, IDC_STATIC_STRATEGY);

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	_WindowEvent += delegate(wndMgr, &HdWindowManager::OnWindowEvent);

	HdWindowEventArgs arg;
	arg.pWnd = this;
	arg.wndType = HdWindowType::StrategyToolWindow;
	arg.eventType = HdWindowEventType::Created;
	FireWindowEvent(std::move(arg));

	VtGlobal::StrategyToolWnd = this;

	SetTimer(PosiTimer, 100, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void VtStrategyToolWnd::OnClose()
{
	KillTimer(PosiTimer);
	_GridCtrl.DisableAllSystems();
	// TODO: Add your message handler code here and/or call default
	CDialogEx::OnClose();

	HdWindowEventArgs arg;
	arg.pWnd = this;
	arg.wndType = HdWindowType::StrategyToolWindow;
	arg.eventType = HdWindowEventType::Closed;
	FireWindowEvent(std::move(arg));
	VtGlobal::StrategyToolWnd = nullptr;
}


void VtStrategyToolWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == PosiTimer) {
		_GridCtrl.UpdatePosition();
	}

	CDialogEx::OnTimer(nIDEvent);
}
