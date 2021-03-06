#pragma once
#include "Skel/VtGrid.h"
#include <vector>
#include "VtGlobal.h"
struct VtQuote;
class VtOrderConfigManager;
class VtSymbol;
class VtOrderCenterWndHd;
class SmOrderPanel;
class VtSysLogGrid : public VtGrid
{
public:
	VtSysLogGrid();
	~VtSysLogGrid();

	virtual void OnSetup();


	void SetColTitle();
	int _ColCount = 3;
	int RowCount() const { return _RowCount; }
	void RowCount(int val) { _RowCount = val; }
	CFont _defFont;
	CFont _titleFont;
	virtual void OnLClicked(int col, long row, int updn, RECT *rect, POINT *point, int processed);
	virtual void OnRClicked(int col, long row, int updn, RECT *rect, POINT *point, int processed);
	void QuickRedrawCell(int col, long row);
	void OnReceiveQuote(VtSymbol* sym);
	void OnReceiveRealtimeQuote(VtQuote* quote);
	void SetOrderConfigMgr(VtOrderConfigManager* val);
	void ClearText();
	void SetQuote();
	int MaxRow() const { return _MaxRow; }
	void MaxRow(int val); // { _MaxRow = val; }
	void ClearValues();
	SmOrderPanel* CenterWnd() const { return _CenterWnd; }
	void CenterWnd(SmOrderPanel* val) { _CenterWnd = val; }
	int GetGridWidth();
	void UpdateLog();
	void UpdateOrderLog(std::vector<std::pair<std::string, std::string>> log_vec);
	int Mode() const { return _Mode; }
	void Mode(int val) { _Mode = val; }
	void Resize(CRect& rect);
private:
	void InitOrderLog();
	std::vector<std::pair<std::string, std::string>> _OrderLogVec;
	VtOrderConfigManager* _OrderConfigMgr = nullptr;
	int GetMaxRow();
	int GetMaxRow(CRect& rect);
	int _MaxRow = 20;
	int _CellHeight;
	std::vector<int> _ColWidths;
	SmOrderPanel* _CenterWnd = nullptr;
	int _Mode = 0;
	int _RowCount = 80;
};


