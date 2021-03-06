#include "stdafx.h"
#include "VtHdClient.h"
#include "VtHdCtrl.h"
#include "resource.h"

int VtHdClient::Init()
{
	return _HdCtrl->Init();
}

VtHdClient::VtHdClient()
{
	_HdCtrl = new VtHdCtrl();
	_HdCtrl->Create(IDD_HDF_CTRL);
	_HdCtrl->ShowWindow(SW_HIDE);
}


VtHdClient::~VtHdClient()
{
	if (_HdCtrl) {
		_HdCtrl->DestroyWindow();
		delete _HdCtrl;
		_HdCtrl = nullptr;
	}
}


void VtHdClient::LogIn()
{
	if (_HdCtrl)
		_HdCtrl->LogIn();
}

int VtHdClient::LogIn(CString id, CString pwd, CString cert)
{
	return _HdCtrl->LogIn(id, pwd, cert);
}

void VtHdClient::LogOut()
{
	if (_HdCtrl)
		_HdCtrl->LogOut();
}

int VtHdClient::LogOut(CString id)
{
	if (_HdCtrl)
		return _HdCtrl->LogOut(id);
	else
		return -1;
}

CString VtHdClient::GetUserId()
{
	CString result = _T("");
	if (_HdCtrl)
		result = _HdCtrl->GetUserId();
	return result;
}

CString VtHdClient::GetUserPassword()
{
	CString result = _T("");
	if (_HdCtrl)
		result = _HdCtrl->GetUserPassword();
	return result;
}

CString VtHdClient::GetAccountPassword()
{
	CString result = _T("");
	if (_HdCtrl)
		result = _HdCtrl->GetAccountPassword();
	return result;
}

void VtHdClient::GetFutureCodeList(std::string catCode)
{
	if (_HdCtrl)
	{
		CString cat = catCode.c_str();
		_HdCtrl->GetFutureCodeList(cat);
	}
}

void VtHdClient::GetOptionCodeList()
{
	if (_HdCtrl)
	{
		CString cat = _T("101O");
		_HdCtrl->GetFutureCodeList(cat);
	}
}

void VtHdClient::GetTradableCodeTable()
{
	if (_HdCtrl)
	{
		_HdCtrl->GetTradableCodeTable();
	}
}

void VtHdClient::GetSymbolMaster(CString symCode)
{
	if (_HdCtrl)
	{
		_HdCtrl->GetSymbolMaster(symCode);
	}
}

void VtHdClient::GetSymbolCode(CString catCode)
{
	if (_HdCtrl)
	{
		_HdCtrl->GetSymbolCode(catCode);
	}
}

void VtHdClient::RegisterProduct(CString symCode)
{
	if (_HdCtrl)
		_HdCtrl->RegisterProduct(symCode);
}

void VtHdClient::UnregisterProduct(CString symCode)
{
	if (_HdCtrl)
		_HdCtrl->UnregisterProduct(symCode);
}

void VtHdClient::RegisterAccount(CString accountNo)
{
	if (_HdCtrl)
		_HdCtrl->RegisterAccount(accountNo);
}

void VtHdClient::UnregisterAccount(CString accountNo)
{
	if (_HdCtrl)
		_HdCtrl->UnregisterAccount(accountNo);
}

void VtHdClient::SendOrder(HdOrderRequest request)
{
	if (_HdCtrl)
		_HdCtrl->SendOrder(request);
}

void VtHdClient::PutOrder(HdOrderRequest&& request)
{
	if (_HdCtrl)
		_HdCtrl->PutOrder(std::move(request));
}

void VtHdClient::ChangeOrder(HdOrderRequest&& request)
{
	if (_HdCtrl)
		_HdCtrl->ChangeOrder(std::move(request));
}

void VtHdClient::CancelOrder(HdOrderRequest&& request)
{
	if (_HdCtrl)
		_HdCtrl->CancelOrder(std::move(request));
}

void VtHdClient::GetDailyProfitLoss(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetDailyProfitLoss(accountNo, pwd);
}

void VtHdClient::GetAccountProfitLoss(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetAccountProfitLoss(accountNo, pwd);
}

void VtHdClient::GetAccountInfo()
{
	if (_HdCtrl)
		_HdCtrl->GetAccountInfo();
}

void VtHdClient::GetFilledHistoryTable(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetFilledHistoryTable(accountNo, pwd);
}

void VtHdClient::GetFilledHistoryTable(HdTaskType taskType, std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetFilledHistoryTable(taskType, accountNo, pwd);
}

void VtHdClient::AbGetChartData(VtChartDataRequest&& chartReqeust)
{
	if (_HdCtrl)
		_HdCtrl->AbGetChartData(std::move(chartReqeust));
}

void VtHdClient::SetBlock(bool flag)
{
	if (_HdCtrl)
		_HdCtrl->Blocked(flag);
}

void VtHdClient::RegisterExpected(CString symCode)
{
	if (_HdCtrl)
		_HdCtrl->RegisterExpected(symCode);
}

void VtHdClient::UnregisterExpected(CString symCode)
{
	if (_HdCtrl)
		_HdCtrl->UnregisterExpected(symCode);
}

void VtHdClient::GetServerTime()
{
	if (_HdCtrl)
		_HdCtrl->GetServerTime();
}

void VtHdClient::RegisterRealtimeDataKey(std::string symCode, std::string dataKey)
{
	if (_HdCtrl)
		_HdCtrl->RegisterRealtimeDataKey(symCode, dataKey);
}

void VtHdClient::DownloadMasterFiles(std::string param)
{
	_HdCtrl->DownloadMasterFiles(param);
}

void VtHdClient::DownloadDomesticMasterFile(std::string file_name)
{
	_HdCtrl->DownloadDomesticMasterFile(file_name);
}

void VtHdClient::GetOutstandingHistory(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetOutstandingHistory(accountNo, pwd);
}

void VtHdClient::GetOutstanding(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetOutstanding(accountNo, pwd);
}

void VtHdClient::GetAcceptedHistory(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetAcceptedHistory(accountNo, pwd);
}

void VtHdClient::GetApiCustomerProfitLoss(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetApiCustomerProfitLoss(accountNo, pwd);
}

void VtHdClient::GetApiCustomerProfitLoss(HdTaskType taskType, std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetApiCustomerProfitLoss(taskType, accountNo, pwd);
}

void VtHdClient::GetAsset(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetAsset(accountNo, pwd);
}

void VtHdClient::GetDeposit(std::string accountNo, std::string pwd)
{
	if (_HdCtrl)
		_HdCtrl->GetDeposit(accountNo, pwd);
}

void VtHdClient::SetFutureGrid(HdFutureGrid* grid)
{
	if (_HdCtrl)
		_HdCtrl->FutureGrid(grid);
}

void VtHdClient::SetOptionGrid(HdOptionGrid* grid)
{
	if (_HdCtrl)
		_HdCtrl->OptionGrid(grid);
}

void VtHdClient::GetChartData(VtChartDataRequest&& chartReqeust)
{
	if (_HdCtrl)
		_HdCtrl->GetChartData(std::move(chartReqeust));
}

void VtHdClient::GetChartData(SmChartDataRequest req)
{
	_HdCtrl->GetChartData(req);
}

void VtHdClient::ExecuteRequest(std::shared_ptr<HdTaskArg> arg)
{
	_HdCtrl->ExecuteRequest(arg);
}

void VtHdClient::GetAbroadQuote(std::string symbol_code)
{
	_HdCtrl->GetAbroadQuote(symbol_code);
}

void VtHdClient::GetAbroadHoga(std::string symbol_code)
{
	_HdCtrl->GetAbroadHoga(symbol_code);
}

void VtHdClient::AbPutOrder(HdOrderRequest&& request)
{
	_HdCtrl->AbPutOrder(std::move(request));
}

void VtHdClient::AbChangeOrder(HdOrderRequest&& request)
{
	_HdCtrl->AbChangeOrder(std::move(request));
}

void VtHdClient::AbCancelOrder(HdOrderRequest&& request)
{
	_HdCtrl->AbCancelOrder(std::move(request));
}

void VtHdClient::GetMasterFile(std::string fileName)
{
	if (_HdCtrl)
		_HdCtrl->GetMasterFile(fileName);
}

void VtHdClient::GetSymbolMasterWithCount(std::string symCode, int count)
{
	if (_HdCtrl)
		_HdCtrl->GetSymbolMasterWithCount(symCode, count);
}

void VtHdClient::RegisterCurrent()
{
	if (_HdCtrl)
		_HdCtrl->RegisterCurrent();
}

void VtHdClient::UnregisterCurrent()
{
	if (_HdCtrl)
		_HdCtrl->UnregisterCurrent();
}
