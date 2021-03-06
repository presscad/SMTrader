#pragma once
#include "afxwin.h"
#include "BtnST.h"
#include "GradientStatic.h"
#include <vector>
#include <map>
#include "Chart/SmChartDefine.h"

// VtChartTimeToolBar dialog
class VtChartContainer;
class VtSystem;
class VtSymbol;
class VtChartTimeToolBar : public CDialogEx
{
	DECLARE_DYNAMIC(VtChartTimeToolBar)

public:
	VtChartTimeToolBar(CWnd* pParent = NULL);   // standard constructor
	virtual ~VtChartTimeToolBar();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TIME_TOOLBAR };
#endif

	int Mode() const { return _Mode; }
	void Mode(int val) { _Mode = val; }
	VtChartContainer* Container() const { return _Container; }
	void Container(VtChartContainer* val) { _Container = val; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CButtonST _BtnDay;
	CButtonST _BtnMin;
	CButtonST _BtnMonth;
	CButtonST _BtnTick;
	CButtonST _BtnWeek;
	CComboBox _ComboTick;
	CComboBox _ComboTime;
	CButtonST _SearchSymbol;
	CComboBox _ComboSymbol;
	std::vector<CButtonST*> _ButtonVec;

private:
	SmChartType _ChartType = SmChartType::MIN;
	int _Cycle = 1;
	VtSymbol* _Symbol = nullptr;
	void ChangeButtonColor();
	int _Mode;
	VtChartContainer* _Container = nullptr;
	void ChangeChartData();
public:
	void ChangeChartData(VtSymbol* symbol);
	CGradientStatic _StaticTime;
	afx_msg void OnBnClickedBtnMonth();
	afx_msg void OnBnClickedBtnWeek();
	afx_msg void OnBnClickedBtnDay();
	afx_msg void OnBnClickedBtnMin();
	afx_msg void OnBnClickedBtnTick();
	afx_msg void OnCbnSelchangeComboTime();
	CComboBox _ComboSystem;
	CGradientStatic _StaticSystem;
	void InitSystem(std::vector<VtSystem*>& sysVector);
	afx_msg void OnCbnSelchangeComboSystem();
	afx_msg void OnCbnSelchangeComboSymbol();
	afx_msg void OnCbnSelchangeComboTick();
	CComboBox _CombolStyle;
	afx_msg void OnCbnSelchangeComboStyle();
	afx_msg void OnBnClickedButtonSearch();
};
