#pragma once
#include "afxcmn.h"
#include "VtAssetPage.h"
#include "VtOrderAvailablePage.h"
#include "HdWindowEvent.h"
#include "Poco/BasicEvent.h"

using Poco::BasicEvent;

// VtAccountAssetDlg dialog

class VtAccountAssetDlg : public CDialogEx
{
	DECLARE_DYNAMIC(VtAccountAssetDlg)

public:
	VtAccountAssetDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~VtAccountAssetDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ACCOUNT_ASSET };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BasicEvent<HdWindowEventArgs> _WindowEvent;
	void FireWindowEvent(HdWindowEventArgs&& arg)
	{
		_WindowEvent(this, arg);
	}

	VtAssetPage _AssetPage;
	VtOrderAvailablePage _OrderPage;
	CWnd* _CurrentPage = nullptr;
	void InitTabCtrl();
	CTabCtrl _TabCtrl;
	virtual BOOL OnInitDialog();
	afx_msg void OnTcnSelchangeTabAccountAsset(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClose();
	void OnReceiveAccountInfo();
};