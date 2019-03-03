#include "stdafx.h"
#include "VtSignalDefinitionGrid.h"
#include "VtGlobal.h"
#include "VtOutSignalDefManager.h"
#include "VtOutSignalDef.h"

VtSignalDefinitionGrid::VtSignalDefinitionGrid()
{
}


VtSignalDefinitionGrid::~VtSignalDefinitionGrid()
{
}

void VtSignalDefinitionGrid::OnSetup()
{
	_defFont.CreateFont(11, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("굴림"));
	_titleFont.CreateFont(11, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("굴림"));

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
}

void VtSignalDefinitionGrid::OnDClicked(int col, long row, RECT *rect, POINT *point, BOOL processed)
{

}

int VtSignalDefinitionGrid::OnCanViewMove(int oldcol, long oldrow, int newcol, long newrow)
{
	return 0;
}

void VtSignalDefinitionGrid::SetColTitle()
{
	CUGCell cell;
	LPCTSTR title[4] = { "신호", "포지션", "발생장소", "설명" };
	int colWidth[4] = { 80, 110, 50, 80 };


	for (int i = 0; i < _ColCount; i++) {
		SetColWidth(i, colWidth[i]);
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

void VtSignalDefinitionGrid::QuickRedrawCell(int col, long row)
{
	CRect rect;
	GetCellRect(col, row, rect);
	m_CUGGrid->m_drawHint.AddHint(col, row, col, row);

	if (GetCurrentRow() != row || GetCurrentCol() != col)
		TempDisableFocusRect();

	m_CUGGrid->PaintDrawHintsNow(rect);
}

void VtSignalDefinitionGrid::InitGrid()
{
	VtOutSignalDefManager* outSigDefMgr = VtOutSignalDefManager::GetInstance();
	int i = 0;
	OutSigDefVec& sigDefVec = outSigDefMgr->GetSignalDefVec();
	for (auto it = sigDefVec.begin(); it != sigDefVec.end(); ++it) {
		SharedOutSigDef sig = *it;
		QuickSetText(0, i, sig->Name.c_str());
		QuickSetText(3, i, sig->Desc.c_str());
		QuickRedrawCell(0, i);
		QuickRedrawCell(0, 3);
		i++;
	}
}

void VtSignalDefinitionGrid::ClearCells()
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

void VtSignalDefinitionGrid::AddOutSigDef(SharedOutSigDef sig)
{
	VtOutSignalDefManager* outSigDefMgr = VtOutSignalDefManager::GetInstance();
	OutSigDefVec& sigDefVec = outSigDefMgr->GetSignalDefVec();
	int yIndex = sigDefVec.size() - 1;
	QuickSetText(0, yIndex, sig->Name.c_str());
	QuickSetText(1, yIndex, sig->SymbolCode.c_str());
	QuickSetText(2, yIndex, sig->StrategyName.c_str());
	QuickSetText(3, yIndex, sig->Desc.c_str());
	QuickRedrawCell(0, yIndex);
	QuickRedrawCell(1, yIndex);
	QuickRedrawCell(2, yIndex);
	QuickRedrawCell(3, yIndex);
}
