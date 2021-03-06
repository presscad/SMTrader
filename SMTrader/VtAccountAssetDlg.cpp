// VtAccountAssetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SMTrader.h"
#include "VtAccountAssetDlg.h"
#include "afxdialogex.h"
#include "VtAccountManager.h"
#include "HdWindowManager.h"
#include "VtAccount.h"
#include "Poco/Delegate.h"
using Poco::Delegate;

// VtAccountAssetDlg dialog

IMPLEMENT_DYNAMIC(VtAccountAssetDlg, CDialogEx)

VtAccountAssetDlg::VtAccountAssetDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ACCOUNT_ASSET, pParent)
{

}

VtAccountAssetDlg::~VtAccountAssetDlg()
{
	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	_WindowEvent -= delegate(wndMgr, &HdWindowManager::OnWindowEvent);
}

void VtAccountAssetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_ACCOUNT_ASSET, _TabCtrl);
}


BEGIN_MESSAGE_MAP(VtAccountAssetDlg, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_ACCOUNT_ASSET, &VtAccountAssetDlg::OnTcnSelchangeTabAccountAsset)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// VtAccountAssetDlg message handlers


void VtAccountAssetDlg::SaveToXml(pugi::xml_node& window_node)
{
	CRect rcWnd;
	GetWindowRect(rcWnd);
	pugi::xml_node window_child = window_node.append_child("window_pos");
	window_child.append_attribute("left") = rcWnd.left;
	window_child.append_attribute("top") = rcWnd.top;
	window_child.append_attribute("right") = rcWnd.right;
	window_child.append_attribute("bottom") = rcWnd.bottom;
	if (_AssetPage.GetSafeHwnd() && _AssetPage.Account()) {
		window_child = window_node.append_child("account_no");
		window_child.append_child(pugi::node_pcdata).set_value(_AssetPage.Account()->AccountNo.c_str());
	}
}

void VtAccountAssetDlg::LoadFromXml(pugi::xml_node& window_node)
{
	pugi::xml_node window_pos_node = window_node.child("window_pos");
	CRect rcWnd;
	rcWnd.left = window_pos_node.attribute("left").as_int();
	rcWnd.top = window_pos_node.attribute("top").as_int();
	rcWnd.right = window_pos_node.attribute("right").as_int();
	rcWnd.bottom = window_pos_node.attribute("bottom").as_int();
	pugi::xml_node account_no_node = window_node.child("account_no");
	if (account_no_node) {
		std::string account_no = window_node.child_value("account_no");
		SetAccount(account_no);
	}
	MoveWindow(rcWnd);
	ShowWindow(SW_SHOW);
}

void VtAccountAssetDlg::Save(simple::file_ostream<same_endian_type>& ss)
{
	CRect rcWnd;
	GetWindowRect(rcWnd);
	ss << rcWnd.left << rcWnd.top << rcWnd.right << rcWnd.bottom;
	if (_AssetPage.GetSafeHwnd() && _AssetPage.Account()) {
		ss << _AssetPage.Account()->AccountNo;
	}
	else {
		ss << _T("untitled");
	}
}

void VtAccountAssetDlg::Load(simple::file_istream<same_endian_type>& ss)
{
	CRect rcWnd;
	ss >> rcWnd.left >> rcWnd.top >> rcWnd.right >> rcWnd.bottom;
	std::string acntNo;
	ss >> acntNo;
	SetAccount(acntNo);
	MoveWindow(rcWnd);
	ShowWindow(SW_SHOW);
}

void VtAccountAssetDlg::InitTabCtrl()
{
	_TabCtrl.InsertItem(0, _T("예탁잔고 및 증거금"));
	_TabCtrl.InsertItem(1, _T("주문가능수량"));

	CRect rect;
	_TabCtrl.GetClientRect(rect);

	_AssetPage.Create(IDD_ASSET, &_TabCtrl);
	//_AssetPage.OrderConfigMgr(_OrderConfigMgr);
	_AssetPage.SetWindowPos(nullptr, 0, 25, rect.Width(), rect.Height() - 30, SWP_NOZORDER);
	_CurrentPage = &_AssetPage;

	_OrderPage.Create(IDD_ORDER_AVAILABLE, &_TabCtrl);
	//_OrderPage.OrderConfigMgr(_OrderConfigMgr);
	_OrderPage.SetWindowPos(nullptr, 0, 25, rect.Width(), rect.Height() - 30, SWP_NOZORDER);
	_OrderPage.ShowWindow(SW_HIDE);

	_CurrentPage->ShowWindow(SW_SHOW);
}

BOOL VtAccountAssetDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
	this->SetIcon(hIcon, FALSE);

	InitTabCtrl();
	// TODO:  Add extra initialization here

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	_WindowEvent += delegate(wndMgr, &HdWindowManager::OnWindowEvent);

	HdWindowEventArgs arg;
	arg.pWnd = this;
	arg.wndType = HdWindowType::AssetWindow;
	arg.eventType = HdWindowEventType::Created;
	FireWindowEvent(std::move(arg));

	CRect rect;
	GetWindowRect(rect);
	SetWindowPos(nullptr, 0, 0, 328, rect.Height(), SWP_NOMOVE);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void VtAccountAssetDlg::OnTcnSelchangeTabAccountAsset(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	if (_CurrentPage != nullptr)
	{
		_CurrentPage->ShowWindow(SW_HIDE);
		_CurrentPage = nullptr;
	}

	int curIndex = _TabCtrl.GetCurSel();
	switch (curIndex)
	{
	case 0:
		_AssetPage.ShowWindow(SW_SHOW);
		_CurrentPage = &_AssetPage;
		break;
	case 1:
		_OrderPage.ShowWindow(SW_SHOW);
		_CurrentPage = &_OrderPage;
		break;
	}
	*pResult = 0;
}


void VtAccountAssetDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnClose();

	HdWindowEventArgs arg;
	arg.pWnd = this;
	arg.wndType = HdWindowType::AssetWindow;
	arg.eventType = HdWindowEventType::Closed;
	FireWindowEvent(std::move(arg));
}

void VtAccountAssetDlg::OnReceiveAccountInfo()
{
	if (_AssetPage.GetSafeHwnd())
	{
		_AssetPage.OnReceiveAccountInfo();
	}
}

void VtAccountAssetDlg::SetAccount(std::string acntNo)
{
	if (_AssetPage.GetSafeHwnd()) {
		_AssetPage.SetAccount(acntNo);
	}
}
