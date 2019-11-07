#include "stdafx.h"
#include "SmOrderGrid.h"
#include "Global/VtDefine.h"
#include "VtGlobal.h"
#include "VtOrderCenterWndHd.h"
#include "VtSymbol.h"
#include "VtOrderPanelGrid.h"
#include "VtSymbolManager.h"
#include "VtQuoteManager.h"
#include "VtQuote.h"
#include "VtSymbolMaster.h"
#include "Poco/String.h"
#include "VtHogaManager.h"
#include "VtHoga.h"
#include "VtCellLabel.h"
#include "VtAccountManager.h"
#include "VtOrder.h"
#include "VtOrderManager.h"
#include "VtOrderManagerSelector.h"
#include "VtAccount.h"
#include "VtOrderManagerSelector.h"
#include "VtProductOrderManagerSelector.h"
#include "VtProductOrderManager.h"
#include "Poco/NumberFormatter.h"
#include <chrono>
#include <algorithm>
#include <vector>
#include <iterator>
#include <map>
#include "resource.h"
#include "VtStopOrderManager.h"
#include "VtPosition.h"
#include "VtHdClient.h"
#include "VtOrderWndHd.h"
#include "VtOrderConfigManager.h"
#include "VtFundOrderManager.h"
#include "VtFund.h"
#include "VtSubAccountManager.h"
#include "Format/format.h"
#include "VtCutManager.h"
#include "XFormatNumber.h"
#include <algorithm> 
#include <functional>
#include "SmCallbackManager.h"
#include "VtSymbol.h"


using Poco::trim;
using Poco::NumberFormatter;
using namespace std;
using namespace std::placeholders;

constexpr int Round(float x) { return static_cast<int>(x + 0.5f); }

SmOrderGrid::SmOrderGrid()
{
	BuyColor.push_back(RGB(253, 173, 176));
	BuyColor.push_back(RGB(255, 196, 199));
	BuyColor.push_back(RGB(255, 214, 212));
	BuyColor.push_back(RGB(255, 224, 225));
	BuyColor.push_back(RGB(255, 232, 232));

	SellColor.push_back(RGB(159, 214, 255));
	SellColor.push_back(RGB(185, 224, 255));
	SellColor.push_back(RGB(204, 230, 250));
	SellColor.push_back(RGB(221, 243, 255));
	SellColor.push_back(RGB(230, 247, 255));

	_SymbolCode = "101PC000";

	RegisterQuoteCallback();
	RegisterHogaCallback();
	RegisterOrderallback();

	m_bMouseTracking = FALSE;
}


SmOrderGrid::~SmOrderGrid()
{
}


void SmOrderGrid::Symbol(VtSymbol* val)
{
	_Symbol = val;
	_SymbolCode = _Symbol->ShortCode;
}

void SmOrderGrid::RegisterQuoteCallback()
{
	SmCallbackManager::GetInstance()->SubscribeQuoteCallback((long)this, std::bind(&SmOrderGrid::OnUpdateSise, this, _1));
}

void SmOrderGrid::UnregisterQuoteCallback()
{
	SmCallbackManager::GetInstance()->UnsubscribeQuoteCallback((long)this);
}

void SmOrderGrid::OnUpdateSise(const VtSymbol* symbol)
{
	if (!symbol || _SymbolCode.compare(symbol->ShortCode) != 0)
		return;
	std::set<std::pair<int, int>> refreshSet;
	SetQuoteColor(symbol, refreshSet);
	RefreshCells(refreshSet);
}

void SmOrderGrid::RegisterHogaCallback()
{
	SmCallbackManager::GetInstance()->SubscribeHogaCallback((long)this, std::bind(&SmOrderGrid::OnUpdateHoga, this, _1));
}

void SmOrderGrid::UnregisterHogaCallback()
{
	SmCallbackManager::GetInstance()->UnsubscribeHogaCallback((long)this);
}

void SmOrderGrid::OnUpdateHoga(const VtSymbol* symbol)
{
	if (!symbol || _SymbolCode.compare(symbol->ShortCode) != 0)
		return;
	std::set<std::pair<int, int>> refreshSet;
	ClearHogas(refreshSet);
	SetHogaInfo(symbol, refreshSet);
	RefreshCells(refreshSet);
	Invalidate();
}

void SmOrderGrid::RegisterOrderallback()
{
	SmCallbackManager::GetInstance()->SubscribeOrderCallback((long)this, std::bind(&SmOrderGrid::OnOrderEvent, this, _1));
}

void SmOrderGrid::UnregisterOrderCallback()
{
	SmCallbackManager::GetInstance()->UnsubscribeOrderCallback((long)this);
}

void SmOrderGrid::OnOrderEvent(const VtOrder* order)
{
	if (!order || _SymbolCode.compare(order->shortCode) != 0)
		return;
	std::set<std::pair<int, int>> refreshSet;
	ClearOldOrders(refreshSet);
	SetOrderInfo(refreshSet);
	RefreshCells(refreshSet);
}

void SmOrderGrid::Init()
{
	_defFont.CreateFont(12, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("굴림"));
	_titleFont.CreateFont(11, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("굴림"));
	_CenterFont.CreateFont(11, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("굴림"));
	_CursorFont.CreateFont(10, 0, 0, 0, 500, 0, 0, 0, 0, 0, 0, 0, 0, _T("굴림"));
	_Init = false;

	_GridColMap[SmOrderGridCol::STOP] = 48;
	_GridColMap[SmOrderGridCol::ORDER] = 60;
	_GridColMap[SmOrderGridCol::COUNT] = 45;
	_GridColMap[SmOrderGridCol::QUANTITY] = 45;
	_GridColMap[SmOrderGridCol::CENTER] = 80;

	_HeadHeight = 20;

	_RowCount = GetMaxRow();
	_CloseRow = FindCenterRow();
	_EndRowForValue = _RowCount - 3;
	SetRowCount(_RowCount);
	SetColumnCount(_ColCount);
	SetFixedRowCount(_FixedRow);
	SetFixedColumnCount(_FixedCol);
	SetColTitle(_Init);

	SetFixedColumnSelection(FALSE);
	SetFixedRowSelection(FALSE);
	EnableColumnHide();
	SetCompareFunction(CGridCtrl::pfnCellNumericCompare);
	SetDoubleBuffering(TRUE);
	EnableSelection(FALSE);
	SetEditable(FALSE);

	_Account = VtAccountManager::GetInstance()->FindAddAccount("00162001");
	_Symbol = VtSymbolManager::GetInstance()->FindSymbol(_SymbolCode);

	for (int i = 1; i < m_nRows; ++i) {
		SetRowHeight(i, _CellHeight);
	}

	_StopOrderMgr = new VtStopOrderManager();
	SetFont(&_defFont);
	SetOrderArea();
	SetCenterValue();
}

void SmOrderGrid::SetColTitle(bool init)
{
	const int ColCount = _ColCount;
	LPCTSTR title[9] = { "STOP", "주문", "건수", "잔량", "정렬", "잔량", "건수", "주문", "STOP" };
	int colWidth[9] = {
		_GridColMap[SmOrderGridCol::STOP],
		_GridColMap[SmOrderGridCol::ORDER],
		_GridColMap[SmOrderGridCol::COUNT],
		_GridColMap[SmOrderGridCol::QUANTITY],
		_GridColMap[SmOrderGridCol::CENTER],
		_GridColMap[SmOrderGridCol::QUANTITY],
		_GridColMap[SmOrderGridCol::COUNT],
		_GridColMap[SmOrderGridCol::ORDER],
		_GridColMap[SmOrderGridCol::STOP]
	};

	SetRowHeight(0, _HeadHeight);

	for (int i = 0; i < _ColCount; i++)
	{
		if (i == CenterCol)
			continue;
		if (!init)
			SetColumnWidth(i, colWidth[i]);

		CGridCellBase* pCell = GetCell(0, i);
		pCell->SetText(title[i]);
		// 텍스트 정렬
		pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		// 셀 배경 색 설정
		pCell->SetBackClr(VtGlobal::GridTitleBackColor);
		// 셀 글자색 설정
		pCell->SetTextClr(VtGlobal::GridTitleTextColor);
		LOGFONT lf;
		_titleFont.GetLogFont(&lf);
		// 셀 폰트 설정
		pCell->SetFont(&lf);
		RedrawCell(0, i);
	}
}

void SmOrderGrid::SetCenterValue(const VtSymbol* symbol, std::set<std::pair<int, int>>& refreshSet)
{
	if (!symbol)
		return;
	ClearQuotes(refreshSet);

	std::string code = symbol->ShortCode.substr(0, 1);
	if ((code.compare(_T("2")) == 0) || (code.compare(_T("3")) == 0)) {
		SetCenterValueForOption(symbol, refreshSet);
	}
	else
	{
		SetCenterValueForFuture(symbol, refreshSet);
	}

	SetQuoteColor(symbol, refreshSet);
}

void SmOrderGrid::SetCenterValue()
{
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* symbol = symMgr->FindSymbol(_SymbolCode);
	if (symbol) {
		std::set<std::pair<int, int>> refreshSet;
		SetCenterValue(symbol, refreshSet);

		ClearHogas(refreshSet);
		SetHogaInfo(symbol, refreshSet);
		ClearOldOrders(refreshSet);
		SetOrderInfo(refreshSet);
		
		ClearOldStopOrders(refreshSet);
		SetStopOrderInfo(refreshSet);
		CalcPosStopOrders(refreshSet);

		RefreshCells(refreshSet);
	}
}

void SmOrderGrid::SetCenterValueByFixed(const VtSymbol* symbol, std::set<std::pair<int, int>>& refreshSet)
{
	if (!symbol)
		return;

	std::string code = symbol->ShortCode.substr(0, 1);
	if ((code.compare(_T("2")) == 0) || (code.compare(_T("3")) == 0)){
		SetCenterValueByFixedForOption(symbol, refreshSet);
	}
	else {
		SetCenterValueByFixedForFuture(symbol, refreshSet);
	}
}

void SmOrderGrid::ClearQuotes(std::set<std::pair<int, int>>& refreshSet)
{
	for (auto it = _QuotePos.begin(); it != _QuotePos.end(); ++it) {
		std::pair<int, int> pos = *it;
		if (pos.first < _StartRowForValue ||
			pos.first > _EndRowForValue)
			continue;
		refreshSet.insert(std::make_pair(pos.first, pos.second));
		CGridCellBase* pCell = GetCell(pos.first, pos.second);
		if (pCell) {
			pCell->SetText("");
			pCell->SetLabel("");
			pCell->SetBackClr(RGB(255, 255, 255));
		}
	}
}

void SmOrderGrid::SetCenterValueForFuture(const VtSymbol* sym, std::set<std::pair<int, int>>& refreshSet)
{
	if (!sym)
		return;

	// Set the close value on the center of the center grid
	int startValue = sym->Quote.intClose + (sym->intTickSize * (_CloseRow - _StartRowForValue));

	ValueToRowMap.clear();
	RowToValueMap.clear();
	for (int i = _StartRowForValue; i <= _EndRowForValue; i++) {
		int curValue = startValue - sym->intTickSize * (i - 1);
		CGridCellBase* pCell = GetCell(i, CenterCol);
		std::string strVal = fmt::format("{:.{}f}", curValue / std::pow(10, sym->IntDecimal), sym->IntDecimal);
		pCell->SetText(strVal.c_str());
		pCell->SetLongValue(curValue);
		pCell->SetTextClr(RGB(0, 0, 0));
		// 텍스트 정렬
		pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		//RedrawCell(i, CenterCol);
		refreshSet.insert(std::make_pair(i, CenterCol));
		ValueToRowMap[curValue] = i;
		RowToValueMap[i] = curValue;
	}
}

void SmOrderGrid::SetCenterValueForOption(const VtSymbol* sym, std::set<std::pair<int, int>>& refreshSet)
{
	if (!sym)
		return;

	int endValue = sym->Quote.intClose;
	int endRow = _EndRowForValue - 1;
	int zeroRow = _EndRowForValue;
	if (_CloseRow < endRow) {
		for (int r = _CloseRow; r < endRow; ++r) {
			if (endValue == 0) {
				zeroRow = r;
				break;
			}
			if (endValue <= 1000)
				endValue -= sym->intTickSize;
			else
				endValue -= 5;
		}

		if (zeroRow < endRow) {
			_CloseRow = endRow - (zeroRow - _CloseRow);
			//SetCenterRow(_CloseRow);
		}
	}

	int startValue = sym->Quote.intClose;
	if (_CloseRow > _StartRowForValue) {
		for (int r = _CloseRow; r > _StartRowForValue; --r) {
			if (startValue < 1000)
				startValue += sym->intTickSize;
			else
				startValue += 5;
		}
	}
	else if (_CloseRow <= _StartRowForValue) {
		for (int r = _CloseRow; r < _StartRowForValue; ++r) {
			if (startValue <= 1000)
				startValue -= sym->intTickSize;
			else
				startValue -= 5;
		}
	}
	ValueToRowMap.clear();
	RowToValueMap.clear();
	int curVal = startValue;
	for (int i = _StartRowForValue; i <= _EndRowForValue; ++i) {
		CGridCellBase* pCell = GetCell(i, CenterCol);
		ValueToRowMap[curVal] = i;
		RowToValueMap[i] = curVal;
		std::string strVal = NumberFormatter::format(curVal / std::pow(10, sym->IntDecimal), sym->IntDecimal);
		pCell->SetText(strVal.c_str());
		pCell->SetLongValue(curVal);
		pCell->SetTextClr(RGB(0, 0, 0));
		// 텍스트 정렬
		pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		if (curVal > 1000)
			curVal = curVal - 5;
		else
			curVal = curVal - sym->intTickSize;
		//RedrawCell(i, CenterCol);
		refreshSet.insert(std::make_pair(i, CenterCol));
	}
}

void SmOrderGrid::SetCenterValueByFixedForFuture(const VtSymbol* sym, std::set<std::pair<int, int>>& refreshSet)
{
	if (!sym)
		return;

	_CloseRow = FindCenterRow();
	// Set the close value on the center of the center grid
	int startValue = sym->Quote.intClose + (sym->intTickSize * (_CloseRow - _StartRowForValue));

	int rangeStart = std::max(_HighRow - 1, _StartRowForValue);
	int rangeEnd = std::min(_LowRow + 1, _EndRowForValue);
	ValueToRowMap.clear();
	RowToValueMap.clear();
	for (int i = _StartRowForValue; i <= _EndRowForValue; i++) {
		int curValue = startValue - sym->intTickSize * (i - 1);
		CGridCellBase* pCell = GetCell(i, CenterCol);
		std::string strVal = NumberFormatter::format(curValue / std::pow(10, sym->IntDecimal), sym->IntDecimal);
		pCell->SetText(strVal.c_str());
		pCell->SetLongValue(curValue);
		pCell->SetTextClr(RGB(0, 0, 0));
		// 텍스트 정렬
		pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		//RedrawCell(i, CenterCol);
		refreshSet.insert(std::make_pair(i, CenterCol));
		ValueToRowMap[curValue] = i;
		RowToValueMap[i] = curValue;
	}
}

void SmOrderGrid::SetCenterValueByFixedForOption(const VtSymbol* sym, std::set<std::pair<int, int>>& refreshSet)
{
	if (!sym)
		return;

	_CloseRow = FindCenterRow();
	_CloseRow = -1 * _CloseRow;

	int Row10 = 0;
	if (1000 <= sym->Quote.intClose) {
		int delta = sym->Quote.intClose - 1000;
		int deltaRow = delta / 5;
		Row10 = _CloseRow - deltaRow;
	}
	else {
		int delta = 1000 - sym->Quote.intClose;
		int deltaRow = delta / sym->intTickSize;
		Row10 = _CloseRow + deltaRow;
	}

	int startValue = 0;
	if (_StartRowForValue >= Row10) {
		startValue = (_StartRowForValue - Row10) * 5 + 1000;
	}
	else {
		startValue = 1000 - (Row10 - _StartRowForValue) * sym->intTickSize;
	}

	ValueToRowMap.clear();
	RowToValueMap.clear();
	int curVal = startValue;
	for (int i = _StartRowForValue; i <= _EndRowForValue; ++i) {
		ValueToRowMap[curVal] = i;
		RowToValueMap[i] = curVal;
		CGridCellBase* pCell = GetCell(i, CenterCol);
		std::string strVal = NumberFormatter::format(curVal / std::pow(10, sym->IntDecimal), sym->IntDecimal);
		pCell->SetText(strVal.c_str());
		pCell->SetLongValue(curVal);
		pCell->SetTextClr(RGB(0, 0, 0));
		// 텍스트 정렬
		pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		//RedrawCell(i, CenterCol);
		refreshSet.insert(std::make_pair(i, CenterCol));
		if (curVal > 1000)
			curVal = curVal - 5;
		else
			curVal = curVal - sym->intTickSize;
	}
}

int SmOrderGrid::FindCenterRow()
{
	int count = GetMaxRow();

	return (count - 2) / 2;
}

int SmOrderGrid::GetMaxRow()
{
	RECT rect;
	GetWindowRect(&rect);


	double pureHeight = rect.bottom - rect.top;

	int count = (int)(pureHeight / _CellHeight);

	return count;
}


void SmOrderGrid::SetHogaInfo(const VtSymbol* sym, std::set<std::pair<int, int>>& refreshSet)
{
	if (!sym)
		return;

	for (int i = 0; i < 5; i++) {
		int pos = FindRowFromCenterValue(sym->Hoga.Ary[i].IntBuyPrice);
		if (pos >= _StartRowForValue && pos <= _EndRowForValue) {
			CGridCellBase* pCell = GetCell(pos, CenterCol + 2);
			std::string strVal = fmt::format("{}", sym->Hoga.Ary[i].BuyNo);
			pCell->SetText(strVal.c_str());
			pCell->SetBackClr(BuyColor[i]);
			pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			_HogaPos.insert(std::make_pair(pos, CenterCol + 2));
			refreshSet.insert(std::make_pair(pos, CenterCol + 2));

			pCell = GetCell(pos, CenterCol + 1);
			strVal = fmt::format("{}", sym->Hoga.Ary[i].BuyQty);
			pCell->SetText(strVal.c_str());
			pCell->SetBackClr(BuyColor[i]);
			pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			_HogaPos.insert(std::make_pair(pos, CenterCol + 1));
			refreshSet.insert(std::make_pair(pos, CenterCol + 1));
		}

		pos = FindRowFromCenterValue(sym->Hoga.Ary[i].IntSellPrice);
		if (i == 0) {
			_CloseLineRow = pos;
			for (int k = 0; k < m_nCols; ++k) {
				CGridCellBase* pCell = GetCell(pos, k);
				if (pCell)
					pCell->Style(1);
			}
		}
		if (pos >= _StartRowForValue && pos <= _EndRowForValue) {
			CGridCellBase* pCell = GetCell(pos, CenterCol - 2);
			std::string strVal = fmt::format("{}", sym->Hoga.Ary[i].SellNo);
			pCell->SetText(strVal.c_str());
			pCell->SetBackClr(SellColor[i]);
			pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			_HogaPos.insert(std::make_pair(pos, CenterCol - 2));
			refreshSet.insert(std::make_pair(pos, CenterCol - 2));

			pCell = GetCell(pos, CenterCol - 1);
			strVal = fmt::format("{}", sym->Hoga.Ary[i].SellQty);
			pCell->SetText(strVal.c_str());
			pCell->SetBackClr(SellColor[i]);
			pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
			_HogaPos.insert(std::make_pair(pos, CenterCol - 1));
			refreshSet.insert(std::make_pair(pos, CenterCol - 1));
		}
	}
}

int SmOrderGrid::FindRowFromCenterValue(int value)
{
	auto it = ValueToRowMap.find(value);
	if (it != ValueToRowMap.end()) {
		return it->second;
	}
	else {
		return -2;
	}
}

int SmOrderGrid::FindRowFromCenterValue(const VtSymbol* sym, int value)
{
	if (!sym)
		return 0;
	auto it = ValueToRowMap.find(value);
	if (it != ValueToRowMap.end()) { // 값이 보이는 범위 안에 있을 때
		return it->second;
	}
	else { // 값이 보이는 범위 밖에 있을 때
		auto itr = ValueToRowMap.rbegin();
		int bigVal = itr->first;
		int bigRow = itr->second;
		int thousandRow = 0;
		// 가격이 10인 행을 찾는 과정 - 10이상이면 승수가 변한다.
		if (bigVal >= 1000) { // 최상위 값이 10보다 이상일 경우
			int delta = bigVal - 1000;
			int deltaRow = delta / sym->intTickSize;
			thousandRow = bigRow + deltaRow;
		}
		else { // 최상위 값이 10미만인 경우
			int delta = 1000 - bigVal;
			thousandRow = bigRow - delta;
		}

		if (value >= 1000) { // 가격이 10 이상인 있는 경우 - 종목의 틱크기 만큼 변함
			int delta = value - 1000;
			int deltaRow = delta / sym->intTickSize;
			return thousandRow - deltaRow;
		}
		else { // 가격이 10 미만인 경우 - 종목에 관계없이 1씩 변함
			int delta = 1000 - value;
			return thousandRow + delta;
		}
	}
}

void SmOrderGrid::SetQuoteColor(const VtSymbol* sym, std::set<std::pair<int, int>>& refreshSet)
{
	if (!sym)
		return;

	if (ValueToRowMap.size() == 0)
		return;

	CUGCell cell;
	
	_LowRow = FindRowFromCenterValue(sym, sym->Quote.intLow);
	_HighRow = FindRowFromCenterValue(sym, sym->Quote.intHigh);
	_CloseRow = FindRowFromCenterValue(sym, sym->Quote.intClose);
	_OpenRow = FindRowFromCenterValue(sym, sym->Quote.intOpen);
	_PreCloseRow = FindRowFromCenterValue(sym, sym->Quote.intPreClose);

	if (sym->Quote.intClose > sym->Quote.intOpen) { // 양봉
		for (auto it = ValueToRowMap.rbegin(); it != ValueToRowMap.rend(); ++it) {
			if (it->second < _HighRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(242, 242, 242));
			} 
			else if (it->second < _CloseRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(255, 255, 255));
			}
			else if (it->second <= _OpenRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(252, 226, 228));
			}
			else if (it->second < _LowRow + 1) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(255, 255, 255));
			}
			else {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(242, 242, 242));
			}
		}

	}
	else if (sym->Quote.intClose < sym->Quote.intOpen) { // 음봉
		for (auto it = ValueToRowMap.rbegin(); it != ValueToRowMap.rend(); ++it) {
			if (it->second < _HighRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(242, 242, 242));
			}
			else if (it->second < _OpenRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(255, 255, 255));
			}
			else if (it->second <= _CloseRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(218, 226, 245));
			}
			else if (it->second < _LowRow + 1) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(255, 255, 255));
			}
			else {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(242, 242, 242));
			}
		}
	}
	else { // 도지
		for (auto it = ValueToRowMap.rbegin(); it != ValueToRowMap.rend(); ++it) {
			if (it->second < _HighRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(242, 242, 242));
			}
			else if (it->second < _CloseRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(255, 255, 255));
			}
			else if (it->second <= _OpenRow) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(252, 226, 228));
			}
			else if (it->second < _LowRow + 1) {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(255, 255, 255));
			}
			else {
				CGridCellBase* pCell = GetCell(it->second, CenterCol);
				pCell->SetBackClr(RGB(242, 242, 242));
			}
		}
	}

	// 종가 색상 입히기
	CGridCellBase* pCell = GetCell(_CloseRow, CenterCol);
	if (pCell && _CloseRow >= _StartRowForValue && _CloseRow <= _EndRowForValue)
		pCell->SetBackClr(RGB(255, 255, 0));

	// 고가 텍스트 색상 설정
	SetSiseLabel(_HighRow, "H", RGB(255, 0, 0));
	
	// 저가 텍스트 색상 설정
	SetSiseLabel(_LowRow, "L", RGB(0, 0, 255));

	// 시작가 레이블 설정
	SetSiseLabel(_OpenRow, "O", RGB(0, 0, 0));
	
	// 이전 종가 레이블 설정
	SetSiseLabel(_PreCloseRow, "C", RGB(0, 0, 0));
	
	for (auto it = ValueToRowMap.begin(); it != ValueToRowMap.end(); ++it) {
		refreshSet.insert(std::make_pair(it->second, CenterCol));
	}
}

void SmOrderGrid::SetSiseCellBackColor(int minRow, int maxRow, int start, int end, COLORREF color)
{
	if (end < minRow || start > maxRow) {
		return;
	}

	start = max(minRow, start);
	end = min(maxRow, end);

	for (int i = start; i < end; ++i) {
		CGridCellBase* pCell = GetCell(i, CenterCol);
		pCell->SetBackClr(color);
	}
}

void SmOrderGrid::SetSiseLabel(int row, LPCTSTR text, COLORREF textColor)
{
	if (row < _StartRowForValue || row > _EndRowForValue)
		return;
	CGridCellBase* pCell = GetCell(row, CenterCol);
	if (pCell) {
		pCell->SetLabel(text);
		pCell->SetTextClr(textColor);
		_QuotePos.insert(std::make_pair(row, CenterCol));
	}
}

void SmOrderGrid::ClearHogas(std::set<std::pair<int, int>>& refreshSet)
{
	// 호가 표시된 기존 셀을 초기화 시킨다.
	for (auto it = _HogaPos.begin(); it != _HogaPos.end(); ++it){
		std::pair<int, int> pos = *it;
		CGridCellBase* pCell = GetCell(pos.first, pos.second);
		pCell->SetText("");
		pCell->SetBackClr(RGB(255, 255, 255));
		refreshSet.insert(std::make_pair(pos.first, pos.second));
	}

	// 호가 라인을 다시 정상으로 되돌려 놓는다.
	for (int k = 0; k < m_nCols; ++k) {
		CGridCellBase* pCell = GetCell(_CloseLineRow, k);
		if (pCell)
			pCell->Style(0);
	}
}

void SmOrderGrid::SetOrderInfo(std::set<std::pair<int, int>>& refreshSet)
{
	if (!_Symbol || !_Account)
		return;

	VtOrderManager* orderMgr = VtOrderManagerSelector::GetInstance()->FindAddOrderManager(_Account->AccountNo);

	_OrderPos.clear();

	std::vector<VtOrder*> acptOrderList = orderMgr->GetAcceptedOrders(_Symbol->ShortCode);

	for (auto it = acptOrderList.begin(); it != acptOrderList.end(); ++it) {
		VtOrder* order = *it;
		if (order->amount == 0)
			continue;

		int row = FindRowFromCenterValue(_Symbol, order->intOrderPrice);

		CGridCellBase* pCell = nullptr;
		if (row >= _StartRowForValue && row <= _EndRowForValue) {
			
			if (order->orderPosition == VtPositionType::Sell) {
				pCell = GetCell(row, CenterCol - 3);
				pCell->SetFormat(DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				pCell->AddOrder(order);
				_OrderPos.insert(std::make_pair(row, CenterCol - 3));
				refreshSet.insert(std::make_pair(row, CenterCol - 3));
			}
			else if (order->orderPosition == VtPositionType::Buy) {
				pCell = GetCell(row, CenterCol + 3);
				pCell->SetFormat(DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				pCell->AddOrder(order);
				_OrderPos.insert(std::make_pair(row, CenterCol + 3));
				refreshSet.insert(std::make_pair(row, CenterCol + 3));
			}
			// 주문 갯수 설정
			std::string strVal = fmt::format("{}", pCell->GetOrderCount());
			pCell->SetText(strVal.c_str());	
		}
	}
}

void SmOrderGrid::ClearOldOrders(std::set<std::pair<int, int>>& refreshSet)
{
	for (auto it = _OrderPos.begin(); it != _OrderPos.end(); ++it) {
		std::pair<int, int> pos = *it;
		CGridCellBase* pCell = GetCell(pos.first, pos.second);
		pCell->SetText("");
		pCell->ClearOrders();
		refreshSet.insert(std::make_pair(pos.first, pos.second));
	}
}

void SmOrderGrid::SetStopOrderInfo(std::set<std::pair<int, int>>& refreshSet)
{
	if (!_Symbol)
		return;

	_StopOrderPos.clear();
	for (auto it = _StopOrderMgr->StopOrderMapHd.begin(); it != _StopOrderMgr->StopOrderMapHd.end(); ++it) {
		HdOrderRequest* order = it->second;
		int row = FindRowFromCenterValue(order->Price);
		CGridCellBase* pCell = nullptr;
		if (row >= _StartRowForValue && row <= _EndRowForValue) {

			if (order->Position == VtPositionType::Sell) {
				pCell = GetCell(row, CenterCol - 4);
				pCell->AddStopOrder(order);
				pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				_StopOrderPos.insert(std::make_pair(row, CenterCol - 4));
				refreshSet.insert(std::make_pair(row, CenterCol - 4));
			}
			else if (order->Position == VtPositionType::Buy) {
				pCell = GetCell(row, CenterCol + 4);
				pCell->AddStopOrder(order);
				pCell->SetFormat(DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
				_StopOrderPos.insert(std::make_pair(row, CenterCol + 4));
				refreshSet.insert(std::make_pair(row, CenterCol + 4));
			}
			// 스탑 주문 갯수 설정
			std::string strVal = fmt::format("{}({})", pCell->GetStopOrderCount(), _StopOrderMgr->Slip());
			pCell->SetText(strVal.c_str());
		}
	}
}

void SmOrderGrid::ClearOldStopOrders(std::set<std::pair<int, int>>& refreshSet)
{
	for (auto it = _StopOrderPos.begin(); it != _StopOrderPos.end(); ++it) {
		std::pair<int, int> pos = *it;
		CGridCellBase* pCell = GetCell(pos.first, pos.second);
		pCell->SetText("");
		pCell->ClearStopOrders();
		refreshSet.insert(std::make_pair(pos.first, pos.second));
	}
}

void SmOrderGrid::CalcPosStopOrders(std::set<std::pair<int, int>>& refreshSet)
{
	if (!_Symbol)
		return;

	ClearStopOrders();
	for (auto it = _StopOrderMgr->StopOrderMapHd.begin(); it != _StopOrderMgr->StopOrderMapHd.end(); ++it) {
		HdOrderRequest* order = it->second;
		int order_row = FindRowFromCenterValue(order->Price);
		CCellID order_cell, slip_cell;
		order_cell.row = order_row;
		if (order->Position == VtPositionType::Buy) {
			order_cell.col = CenterCol + 4;
			slip_cell.col = CenterCol + 3;
			order->slip > 0 ? slip_cell.row = order_row - order->slip : slip_cell.row = order_row + order->slip;
		}
		else if (order->Position == VtPositionType::Sell) {
			order_cell.col = CenterCol - 4;
			slip_cell.col = CenterCol - 3;
			order->slip > 0 ? slip_cell.row = order_row + order->slip : slip_cell.row = order_row - order->slip;
		}

		if (order_cell.row < _StartRowForValue || order_cell.row > _EndRowForValue ||
			slip_cell.row  < _StartRowForValue || slip_cell.row > _EndRowForValue)
			continue;
	
		PutStopOrder(std::make_pair(order_cell, slip_cell));

		int min_row = min(order_cell.row, slip_cell.row);
		int max_row = max(order_cell.row, slip_cell.row);
		int min_col = min(order_cell.col, slip_cell.col);
		int max_col = max(order_cell.col, slip_cell.col);
		for (int r = min_row; r <= max_row; ++r) {
			for (int c = min_col; c <= max_col; ++c) {
				refreshSet.insert(std::make_pair(r, c));
			}
		}
	}

	Invalidate();
}

void SmOrderGrid::RefreshCells(std::set<std::pair<int, int>>& refreshSet)
{
	for (auto it = refreshSet.begin(); it != refreshSet.end(); ++it) {
		std::pair<int, int> pos = *it;
		CGridCellBase* pCell = GetCell(pos.first, pos.second);
		RedrawCell(pos.first, pos.second);
	}
}

void SmOrderGrid::PutOrder(int price, VtPositionType position, VtPriceType priceType /*= VtPriceType::Price*/)
{
	if (!_Symbol || !_Account)
		return;

	VtOrderManager* orderMgr = VtOrderManagerSelector::GetInstance()->FindAddOrderManager(_Account->AccountNo);
	
	HdOrderRequest request;
	request.Price = price;
	request.Position = position;
	request.Amount = 1;
	request.AccountNo = _Account->AccountNo;
	request.Password = _Account->Password;
	request.SymbolCode = _Symbol->ShortCode;
	request.FillCondition = VtFilledCondition::Fas;
	request.PriceType = priceType;

	request.RequestId = orderMgr->GetOrderRequestID();
	request.SourceId = (long)this;
	request.SubAccountNo = _T("");
	request.FundName = _T("");

	request.AccountLevel = 0;
	request.orderType = VtOrderType::New;
	orderMgr->PutOrder(std::move(request));
}

void SmOrderGrid::SetOrderArea()
{
	for (int i = 1; i < m_nRows; ++i) {
		CGridCellBase* pCell = GetCell(i, CenterCol - 3);
		pCell->SetBackClr(RGB(218, 226, 245));
		pCell = GetCell(i, CenterCol + 3);
		pCell->SetBackClr(RGB(252, 226, 228));
	}
}

BEGIN_MESSAGE_MAP(SmOrderGrid, CGridCtrl)
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()


BOOL SmOrderGrid::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	int distance = zDelta / 60;
	if (abs(zDelta) > 120)
		distance = zDelta / 120;
	else
		distance = zDelta / 40;
	_CloseRow =_CloseRow + (distance);
	SetCenterValue();
	return CGridCtrl::OnMouseWheel(nFlags, zDelta, pt);
}


void SmOrderGrid::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CCellID cell = GetCellFromPt(point);
	if (!IsValid(cell))
	{
		//ASSERT(FALSE);
		return;
	}

	//CGridCtrl::OnLButtonDblClk(nFlags, point);

	CGridCellBase* pCell = GetCell(cell.row, cell.col);
	if (!pCell)
		return;

	//SendMessageToParent(cell.row, cell.col, NM_DBLCLK);

	if (cell.row < _StartRowForValue || cell.row >= _EndRowForValue)
		return;
	auto it = RowToValueMap.find(cell.row);
	int price = it->second;

	if (cell.col == CenterCol - 3) // For Sell
	{
		PutOrder(price, VtPositionType::Sell, VtPriceType::Price);
	}
	else if (cell.col == CenterCol + 3) // For Buy
	{
		PutOrder(price, VtPositionType::Buy, VtPriceType::Price);
	}
	else if (cell.col == CenterCol + 4)
	{
		AddStopOrder(price, VtPositionType::Buy);
	}
	else if (cell.col == CenterCol - 4)
	{
		AddStopOrder(price, VtPositionType::Sell);
	}

	CWnd::OnLButtonDblClk(nFlags, point);
}


void SmOrderGrid::OnLButtonDown(UINT nFlags, CPoint point)
{
	CCellID cell = GetCellFromPt(point);
	if (IsValid(cell) == TRUE) {
		CGridCellBase* pCell = GetCell(cell.row, cell.col);
		if (pCell->GetOrderCount() > 0 || pCell->GetStopOrderCount() > 0) {
			GetCellRect(cell, _DragStartRect);
			_DragStartCol = cell.col;
			_DragStartRow = cell.row;
			_OrderDragStarted = true;
			
		}
	}

	SetCapture();
	CGridCtrl::OnLButtonDown(nFlags, point);
}


void SmOrderGrid::OnLButtonUp(UINT nFlags, CPoint point)
{
	CCellID cell = GetCellFromPt(point);
	if (IsValid(cell) == TRUE) {
		if (_OrderDragStarted) {
			CleanOldOrderTrackLine(cell);
		}
	}
	
	if (_OrderDragStarted) {
		_OrderDragStarted = false;
	}
	Invalidate();
	ReleaseCapture();
	CGridCtrl::OnLButtonUp(nFlags, point);
}


void SmOrderGrid::OnMove(int x, int y)
{
	CGridCtrl::OnMove(x, y);
}


void SmOrderGrid::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CGridCtrl::OnRButtonDown(nFlags, point);
}


void SmOrderGrid::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CGridCtrl::OnRButtonUp(nFlags, point);
}

void SmOrderGrid::DrawArrow(int direction, POINT *point, CDC* pdc, POINT p0, POINT p1, int head_length, int head_width)
{
	CBrush brush1;   // Must initialize!
	brush1.CreateSolidBrush(RGB(0, 0, 255));   // Blue brush.

	CBrush* pTempBrush = NULL;
	CBrush OrigBrush;

	//CString msg;
	//msg = _T("정정");
	pTempBrush = (CBrush*)pdc->SelectObject(&brush1);
	// Save original brush.
	OrigBrush.FromHandle((HBRUSH)pTempBrush);
	POINT p2;
	float dx = 0.0;
	float dy = 0.0;
	if (direction == 1)
	{
		pdc->MoveTo(p0);
		pdc->LineTo(p1);
		dx = static_cast<float>(p1.x - p0.x);
		dy = static_cast<float>(p1.y - p0.y);
	}
	else
	{
		
		p2.y = p1.y;
		p2.x = p0.x;
		pdc->MoveTo(p0);
		pdc->LineTo(p2);

		//pdc->MoveTo(p2);
		pdc->LineTo(p1);
		//msg = _T("취소");
		dx = static_cast<float>(p1.x - p2.x);
		dy = static_cast<float>(p1.y - p2.y);
	}

	const auto length = std::sqrt(dx*dx + dy*dy);
	if (head_length < 1 || length < head_length) return;

	// ux,uy is a unit vector parallel to the line.
	const auto ux = dx / length;
	const auto uy = dy / length;

	// vx,vy is a unit vector perpendicular to ux,uy
	const auto vx = -uy;
	const auto vy = ux;

	const auto half_width = 0.5f * head_width;

	const POINT arrow[3] =
	{ p1,
		POINT{ Round(p1.x - head_length*ux + half_width*vx),
		Round(p1.y - head_length*uy + half_width*vy) },
		POINT{ Round(p1.x - head_length*ux - half_width*vx),
		Round(p1.y - head_length*uy - half_width*vy) }
	};
	pdc->Polygon(arrow, 3);
	pdc->SetBkMode(TRANSPARENT);
	pdc->SelectObject(&OrigBrush);
}


void SmOrderGrid::CleanOldOrderLine(CCellID& cell)
{
	if (cell.col == _DragStartCol)
		RedrawCell(_OldMMRow, _OldMMCol);
	else if (cell.col > _DragStartCol) {
		for (int i = cell.col; i >= _DragStartCol; --i) {
			RedrawCell(_OldMMRow, i);
		}
	}
	else {
		for (int i = cell.col; i <= _DragStartCol; ++i) {
			RedrawCell(_OldMMRow, i);
		}
	}
}

void SmOrderGrid::CleanOldOrderTrackLine(CCellID& cell)
{
	if (cell.col == _DragStartCol)
		RedrawCell(_OldMMRow, _OldMMCol);
	else if (cell.col > _DragStartCol) {
		for (int i = cell.col; i >= _DragStartCol; --i) {
			RedrawCell(_OldMMRow, i);
		}
	}
	else {
		for (int i = cell.col; i <= _DragStartCol; ++i) {
			RedrawCell(_OldMMRow, i);
		}
	}
	if (cell.row < _DragStartRow) {
		for (int i = cell.row; i <= _DragStartRow; ++i) {
			RedrawCell(i, _DragStartCol);
		}
	}
	else if (cell.row > _DragStartCol) {
		for (int i = _DragStartRow; i <= cell.row; ++i) {
			RedrawCell(i, _DragStartCol);
		}
	}
}

void SmOrderGrid::RedrawOrderTrackCells()
{
	int min_col = std::min(_DragStartCol, _OldMMCol);
	int max_col = std::max(_DragStartCol, _OldMMCol);
	int min_row = std::min(_DragStartRow, _OldMMRow);
	int max_row = std::max(_DragStartRow, _OldMMRow);
	for (int i = min_row; i <= max_row; ++i) {
		for (int j = min_col; j <= max_col; ++j) {
			RedrawCell(i, j);
		}
	}
}

void SmOrderGrid::AddStopOrder(int price, VtPositionType posi)
{
	if (!_Account || !_Symbol)
		return;
	VtOrderManager* orderMgr = VtOrderManagerSelector::GetInstance()->FindAddOrderManager(_Account->AccountNo);
	HdOrderRequest* request = new HdOrderRequest();
	request->Type = 0;
	request->AccountNo = _Account->AccountNo;
	request->Password = _Account->Password;
	request->Price = price;
	request->Position = posi;
	request->Amount = 1;
	request->SymbolCode = _Symbol->ShortCode;
	request->FillCondition = VtFilledCondition::Fas;
	request->PriceType = VtPriceType::Price;
	request->slip = 2;
	request->RequestId = orderMgr->GetOrderRequestID();
	request->SourceId = (long)this;
	request->SubAccountNo = _T("");
	request->FundName = _T("");
	_StopOrderMgr->AddOrderHd(request);

	std::set<std::pair<int, int>> refreshSet;
	ClearOldStopOrders(refreshSet);
	SetStopOrderInfo(refreshSet);
	CalcPosStopOrders(refreshSet);
	RefreshCells(refreshSet);
}

void SmOrderGrid::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_bMouseTracking)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = this->m_hWnd;

		if (::_TrackMouseEvent(&tme)) {
			m_bMouseTracking = TRUE;
		}
	}

	if (_OrderDragStarted) {
		CCellID cell = GetCellFromPt(point);
		if (IsValid(cell) != TRUE) {
			cell.row = _OldMMRow;
			cell.col = _OldMMCol;
		}

		CleanOldOrderLine(cell);
		GetCellRect(cell, _DragEndRect);
		CDC *dc = GetDC();
		CPoint pt1, pt2;
		pt1.x = _DragStartRect.left + (_DragStartRect.right - _DragStartRect.left) / 2;
		pt1.y = _DragStartRect.top + (_DragStartRect.bottom - _DragStartRect.top) / 2;
		pt2.x = _DragEndRect.left + (_DragEndRect.right - _DragEndRect.left) / 2;
		pt2.y = _DragEndRect.top + (_DragEndRect.bottom - _DragEndRect.top) / 2;
		if (cell.col == _DragStartCol)
			DrawArrow(1, &point, dc, pt1, pt2, 6, 6);
		else
			DrawArrow(2, &point, dc, pt1, pt2, 6, 6);
		DrawStopOrders(dc);
		ReleaseDC(dc);
		_OldMMRow = cell.row;
		_OldMMCol = cell.col;
	}

	CGridCtrl::OnMouseMove(nFlags, point);
}

LRESULT SmOrderGrid::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
	m_bMouseTracking = FALSE;

	return 1;
}
