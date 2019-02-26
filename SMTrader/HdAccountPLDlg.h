#pragma once
#include "HdAccountPLGrid.h"
#include "HdProductRemainGrid.h"
#include "afxwin.h"
#include "HdWindowEvent.h"
#include "Poco/BasicEvent.h"
//#include "TTComboBox.h"

using Poco::BasicEvent;
// HdAccountPLDlg dialog
class VtAccount;
class VtSymbol;
struct VtOrder;

class HdAccountPLDlg : public CDialogEx
{
	DECLARE_DYNAMIC(HdAccountPLDlg)

public:
	HdAccountPLDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~HdAccountPLDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MINI_JANGO };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();

private:
	HdAccountPLGrid _AccountGrid;
	VtAccount* _Account;
	HdProductRemainGrid _ProductGrid;

	BasicEvent<HdWindowEventArgs> _WindowEvent;
	void FireWindowEvent(HdWindowEventArgs&& arg)
	{
		_WindowEvent(this, arg);
	}
public:
	std::string _DefaultAccount;
	CComboBox _ComboAccount;
	void InitAccount();
	VtAccount* Account() const { return _Account; }
	void Account(VtAccount* val);
	void SetAccount(VtAccount* acnt);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	void OnReceiveQuote(VtSymbol* sym);
	void OnOrderFilledHd(VtOrder* order);
	void OnSymbolMaster(VtSymbol* sym);
	void OnReceiveAccountInfo();
	afx_msg void OnCbnSelchangeComboAccount();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
