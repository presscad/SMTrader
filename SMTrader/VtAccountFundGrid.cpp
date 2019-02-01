#include "stdafx.h"
#include "VtAccountFundGrid.h"
#include "VtGlobal.h"
#include "VtAccountManager.h"
#include "VtAccount.h"
#include "VtFundManager.h"
#include "VtFund.h"
#include "VtUsdStrategyConfigDlg.h"

VtAccountFundGrid::VtAccountFundGrid()
{
	_ConfigDlg = nullptr;
}


VtAccountFundGrid::~VtAccountFundGrid()
{
}

void VtAccountFundGrid::OnSetup()
{
	_defFont.CreateFont(11, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("����"));
	_titleFont.CreateFont(11, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("����"));

	SetDoubleBufferMode(TRUE);
	SetDefColWidth(80);

	SetNumberRows(_RowCount);
	SetNumberCols(_ColCount);

	for (int yIndex = 0; yIndex < _RowCount; yIndex++) {
		for (int xIndex = 0; xIndex < _ColCount; xIndex++) {
			QuickSetAlignment(xIndex, yIndex, UG_ALIGNCENTER | UG_ALIGNVCENTER);
		}
	}

	SetDefFont(&_defFont);
	SetSH_Width(0);
	SetVS_Width(0);
	SetHS_Height(0);
	SetColTitle();
	SetVScrollMode(UG_SCROLLNORMAL);

	InitGrid();

	ResizeWindow();
}

void VtAccountFundGrid::OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed)
{
	auto selItem = _RowToNameMap.find(row);
	if (selItem != _RowToNameMap.end()) {
		if (_ConfigDlg) {
			_ConfigDlg->SetTargetAcntOrFund(selItem->second);
		}
	}
}

void VtAccountFundGrid::OnLClicked(int col, long row, int updn, RECT *rect, POINT *point, int processed)
{
	
}

void VtAccountFundGrid::OnRClicked(int col, long row, int updn, RECT *rect, POINT *point, int processed)
{
	
}

void VtAccountFundGrid::SetColTitle()
{
	CUGCell cell;
	LPCTSTR title[2] = { "���¸�", "�ڵ�" };
	
	_ColWidthVec.push_back(120);
	_ColWidthVec.push_back(100);

	for (int i = 0; i < _ColCount; i++) {
		SetColWidth(i, _ColWidthVec[i]);
		GetCell(i, -1, &cell);
		cell.SetText(title[i]);
		cell.SetBackColor(VtGlobal::GridTitleBackColor);
		cell.SetTextColor(VtGlobal::GridTitleTextColor);
		cell.SetAlignment(UG_ALIGNCENTER | UG_ALIGNVCENTER);
		cell.SetFont(&_titleFont);
		SetCell(i, -1, &cell);
		QuickRedrawCell(i, -1);
	}
}

void VtAccountFundGrid::QuickRedrawCell(int col, long row)
{
	CRect rect;
	GetCellRect(col, row, rect);
	m_CUGGrid->m_drawHint.AddHint(col, row, col, row);

	if (GetCurrentRow() != row || GetCurrentCol() != col)
		TempDisableFocusRect();

	m_CUGGrid->PaintDrawHintsNow(rect);
}

void VtAccountFundGrid::InitGrid()
{
	ClearCells();
	_HeightVec.clear();
	_RowToNameMap.clear();
	if (_Mode == 0) {
		VtAccountManager* acntMgr = VtAccountManager::GetInstance();
		int i = 0;
		for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
			VtAccount* acnt = it->second;
			QuickSetText(0, i, acnt->AccountName.c_str());
			QuickSetText(1, i, acnt->AccountNo.c_str());
			QuickSetBackColor(0, i, RGB(250, 250, 255));
			QuickSetBackColor(1, i, RGB(250, 250, 255));
			QuickRedrawCell(0, i);
			QuickRedrawCell(1, i);
			_HeightVec.push_back(_HeadHeight);
			_RowToNameMap[i] = std::make_tuple(acnt->AccountLevel() == 0 ? 0 : 1, acnt, nullptr);
			i++;
		}

		VtFundManager* fundMgr = VtFundManager::GetInstance();
		std::map<std::string, VtFund*>& fundList = fundMgr->GetFundList();
		for (auto it = fundList.begin(); it != fundList.end(); ++it) {
			VtFund* fund = it->second;
			QuickSetText(0, i, fund->Name.c_str());
			QuickSetBackColor(0, i, RGB(255, 250, 250));
			QuickSetBackColor(1, i, RGB(255, 250, 250));
			_HeightVec.push_back(_HeadHeight);
			_RowToNameMap[i] = std::make_tuple(2, nullptr, fund);
			QuickRedrawCell(0, i);
			i++;
		}
	} else if (_Mode == 1) {
		VtAccountManager* acntMgr = VtAccountManager::GetInstance();
		int i = 0;
		for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
			VtAccount* acnt = it->second;
			QuickSetText(0, i, acnt->AccountName.c_str());
			QuickSetText(1, i, acnt->AccountNo.c_str());
			QuickSetBackColor(0, i, RGB(250, 250, 255));
			QuickSetBackColor(1, i, RGB(250, 250, 255));
			QuickRedrawCell(0, i);
			QuickRedrawCell(1, i);
			_HeightVec.push_back(_HeadHeight);
			_RowToNameMap[i] = std::make_tuple(acnt->AccountLevel() == 0 ? 0 : 1, acnt, nullptr);
			i++;
		}
	} else {
		int i = 0;
		VtFundManager* fundMgr = VtFundManager::GetInstance();
		std::map<std::string, VtFund*>& fundList = fundMgr->GetFundList();
		for (auto it = fundList.begin(); it != fundList.end(); ++it) {
			VtFund* fund = it->second;
			QuickSetText(0, i, fund->Name.c_str());
			QuickSetBackColor(0, i, RGB(255, 250, 250));
			QuickSetBackColor(1, i, RGB(255, 250, 250));
			QuickRedrawCell(0, i);
			_HeightVec.push_back(_HeadHeight);
			_RowToNameMap[i] = std::make_tuple(2, nullptr, fund);
			i++;
		}
	}
}

void VtAccountFundGrid::ClearCells()
{
	CUGCell cell;
	for (int i = 0; i < _RowCount; i++) {
		for (int j = 0; j < _ColCount; j++) {
			QuickSetText(j, i, _T(""));
			QuickSetBackColor(j, i, RGB(255, 255, 255));
			QuickRedrawCell(j, i);
			GetCell(j, i, &cell);
			cell.Tag(nullptr);
			SetCell(j, i, &cell);
		}
	}
}

void VtAccountFundGrid::SetConfigDlg(VtUsdStrategyConfigDlg* ConfigDlg)
{
	_ConfigDlg = ConfigDlg;
}

void VtAccountFundGrid::ResizeWindow()
{
	CRect rcWnd, rcClient;
	GetWindowRect(rcWnd);
	GetClientRect(rcClient);

	int totalHeight = _HeadHeight;
	for (size_t i = 0; i < _HeightVec.size(); ++i) {
		totalHeight += _HeightVec[i];
	}
	totalHeight += 14;
	int totalWidth = 0;
	for (size_t i = 0; i < _ColWidthVec.size(); ++i) {
		totalWidth += _ColWidthVec[i];
	}
	SetWindowPos(nullptr, 0, 0, totalWidth, totalHeight, SWP_NOMOVE | SWP_NOZORDER);
}
