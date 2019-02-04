// VtChartTimeToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "VtChartTimeToolBar.h"
#include "afxdialogex.h"
#include "resource.h"
#include "BtnST.h"
#include <string>
#include "VtChartContainer.h"
#include "System/VtSystemManager.h"
#include "System/VtSystem.h"

// VtChartTimeToolBar dialog

IMPLEMENT_DYNAMIC(VtChartTimeToolBar, CDialogEx)

VtChartTimeToolBar::VtChartTimeToolBar(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TIME_TOOLBAR, pParent)
{
	_Mode = 3;
	_Container = nullptr;
}

VtChartTimeToolBar::~VtChartTimeToolBar()
{
}

void VtChartTimeToolBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BTN_DAY, _BtnDay);
	DDX_Control(pDX, IDC_BTN_MIN, _BtnMin);
	DDX_Control(pDX, IDC_BTN_MONTH, _BtnMonth);
	DDX_Control(pDX, IDC_BTN_TICK, _BtnTick);
	DDX_Control(pDX, IDC_BTN_WEEK, _BtnWeek);
	DDX_Control(pDX, IDC_COMBO_TICK, _ComboTick);
	DDX_Control(pDX, IDC_COMBO_TIME, _ComboTime);
	DDX_Control(pDX, IDC_STATIC_TIME, _StaticTime);
	DDX_Control(pDX, IDC_COMBO_SYSTEM, _ComboSystem);
	DDX_Control(pDX, IDC_STATIC_SYSTEM, _StaticSystem);
}


BEGIN_MESSAGE_MAP(VtChartTimeToolBar, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_MONTH, &VtChartTimeToolBar::OnBnClickedBtnMonth)
	ON_BN_CLICKED(IDC_BTN_WEEK, &VtChartTimeToolBar::OnBnClickedBtnWeek)
	ON_BN_CLICKED(IDC_BTN_DAY, &VtChartTimeToolBar::OnBnClickedBtnDay)
	ON_BN_CLICKED(IDC_BTN_MIN, &VtChartTimeToolBar::OnBnClickedBtnMin)
	ON_BN_CLICKED(IDC_BTN_TICK, &VtChartTimeToolBar::OnBnClickedBtnTick)
	ON_CBN_SELCHANGE(IDC_COMBO_TIME, &VtChartTimeToolBar::OnCbnSelchangeComboTime)
	ON_CBN_SELCHANGE(IDC_COMBO_SYSTEM, &VtChartTimeToolBar::OnCbnSelchangeComboSystem)
END_MESSAGE_MAP()


// VtChartTimeToolBar message handlers


BOOL VtChartTimeToolBar::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	_StaticTime.SetTextColor(RGB(255, 255, 255));
	_StaticTime.SetColor(RGB(0, 67, 178));
	_StaticTime.SetGradientColor(RGB(0, 67, 178));
	// Align to center
	_StaticTime.SetTextAlign(1);

	_StaticSystem.SetTextColor(RGB(255, 255, 255));
	_StaticSystem.SetColor(RGB(0, 67, 178));
	_StaticSystem.SetGradientColor(RGB(0, 67, 178));
	// Align to center
	_StaticSystem.SetTextAlign(1);

	_ButtonVec.push_back(&_BtnMonth);
	_ButtonVec.push_back(&_BtnWeek);
	_ButtonVec.push_back(&_BtnDay);
	_ButtonVec.push_back(&_BtnMin);
	_ButtonVec.push_back(&_BtnTick);

	std::vector<std::string> _TooltipVec;
	_TooltipVec.push_back(_T("������"));
	_TooltipVec.push_back(_T("�ּ���"));
	_TooltipVec.push_back(_T("�ϼ���"));
	_TooltipVec.push_back(_T("�м���"));
	_TooltipVec.push_back(_T("ƽ����"));

	int i = 0;
	for (auto btn : _ButtonVec)
	{
		btn->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
		btn->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
		btn->SetColor(BTNST_COLOR_BK_OUT, RGB(255, 255, 255), true);
		btn->SetColor(BTNST_COLOR_FG_OUT, RGB(0, 0, 0), true);
		btn->SetTooltipText(_TooltipVec[i++].c_str());
	}
	for (int i = 1; i < 901; i++)
	{
		_ComboTick.AddString(std::to_string(i).c_str());
	}

	// TODO:  Add extra initialization here
	switch (_Mode)
	{
	case 0: // for month
	{
		for (int i = 1; i < 31; i++)
		{
			_ComboTime.AddString(std::to_string(i).c_str());
		}
		_ComboTime.EnableWindow(TRUE);
		_ComboTick.EnableWindow(FALSE);
	}
		break;
	case 1: // for week
	{
		for (int i = 1; i < 5; i++)
		{
			_ComboTime.AddString(std::to_string(i).c_str());
		}
		_ComboTime.EnableWindow(TRUE);
		_ComboTick.EnableWindow(FALSE);
	}
		break;
	case 2: // day
	{
		for (int i = 1; i < 601; i++)
		{
			_ComboTime.AddString(std::to_string(i).c_str());
		}
		_ComboTime.EnableWindow(TRUE);
		_ComboTick.EnableWindow(FALSE);
	}
		break;
	case 3: // for min
	{
		for (int i = 1; i < 61; i++)
		{
			_ComboTime.AddString(std::to_string(i).c_str());
		}
		_ComboTime.EnableWindow(TRUE);
		_ComboTick.EnableWindow(FALSE);
	}
		break;
	case 4: // for tick
	{
		for (int i = 1; i < 61; i++)
		{
			_ComboTick.AddString(std::to_string(i).c_str());
		}
		_ComboTime.EnableWindow(FALSE);
		_ComboTick.EnableWindow(TRUE);
	}
		break;
	default:
	{
		{
			for (int i = 1; i < 61; i++)
			{
				_ComboTime.AddString(std::to_string(i).c_str());
			}
		}
		_ComboTime.EnableWindow(TRUE);
		_ComboTick.EnableWindow(FALSE);
	}
		break;
	}

	_ButtonVec[_Mode]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
	_ButtonVec[_Mode]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
	_ButtonVec[_Mode]->SetColor(BTNST_COLOR_BK_OUT, RGB(0, 67, 178), true);
	_ButtonVec[_Mode]->SetColor(BTNST_COLOR_FG_OUT, RGB(255, 255, 255), true);

	_ComboTime.SetCurSel(0);
	_ComboTick.SetCurSel(0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


void VtChartTimeToolBar::OnBnClickedBtnMonth()
{
	_ComboTime.EnableWindow(TRUE);
	_ComboTick.EnableWindow(FALSE);
	_Mode = 0;
	for (int i = 0; i < 5; i++)
	{
		if (i == _Mode)
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(0, 67, 178), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(255, 255, 255), true);
		}
		else
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(0, 0, 0), true);
		}
	}
}


void VtChartTimeToolBar::OnBnClickedBtnWeek()
{
	_ComboTime.EnableWindow(TRUE);
	_ComboTick.EnableWindow(FALSE);
	_Mode = 1;
	for (int i = 0; i < 5; i++)
	{
		if (i == _Mode)
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(0, 67, 178), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(255, 255, 255), true);
		}
		else
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(0, 0, 0), true);
		}
	}
}


void VtChartTimeToolBar::OnBnClickedBtnDay()
{
	_ComboTime.EnableWindow(TRUE);
	_ComboTick.EnableWindow(FALSE);
	_Mode = 2;
	for (int i = 0; i < 5; i++)
	{
		if (i == _Mode)
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(0, 67, 178), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(255, 255, 255), true);
		}
		else
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(0, 0, 0), true);
		}
	}
}


void VtChartTimeToolBar::OnBnClickedBtnMin()
{
	_ComboTime.EnableWindow(TRUE);
	_ComboTick.EnableWindow(FALSE);
	_Mode = 3;
	for (int i = 0; i < 5; i++)
	{
		if (i == _Mode)
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(0, 67, 178), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(255, 255, 255), true);
		}
		else
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(0, 0, 0), true);
		}
	}
}


void VtChartTimeToolBar::OnBnClickedBtnTick()
{
	_ComboTime.EnableWindow(FALSE);
	_ComboTick.EnableWindow(TRUE);
	_Mode = 4;
	for (int i = 0; i < 5; i++)
	{
		if (i == _Mode)
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(0, 67, 178), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(255, 255, 255), true);
		}
		else
		{
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_IN, RGB(255, 67, 0), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_IN, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_BK_OUT, RGB(255, 255, 255), true);
			_ButtonVec[i]->SetColor(BTNST_COLOR_FG_OUT, RGB(0, 0, 0), true);
		}
	}
}


void VtChartTimeToolBar::OnCbnSelchangeComboTime()
{
	int curSel = _ComboTime.GetCurSel();
	if (curSel != -1 && _Container)
		_Container->ChangeChartTime(curSel + 1);
}

void VtChartTimeToolBar::InitSystem(std::vector<VtSystem*>& sysVector)
{
	_ComboSystem.ResetContent();
	for (auto it = sysVector.begin(); it != sysVector.end(); ++it)
	{
		VtSystem* system = *it;
		int index = _ComboSystem.AddString(system->Name().c_str());
		_ComboSystem.SetItemDataPtr(index, system);
	}

	if (_ComboSystem.GetCount() > 0)
		_ComboSystem.SetCurSel(0);
}


void VtChartTimeToolBar::OnCbnSelchangeComboSystem()
{
	int curSel = _ComboSystem.GetCurSel();
	if (_Container && curSel != -1)
	{
		VtSystem* system = (VtSystem*)_ComboSystem.GetItemDataPtr(curSel);
		_Container->ChangeSystem(system);
	}
}