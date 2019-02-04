#include "stdafx.h"
#include "HdScheduler.h"
#include "VtHdClient.h"
#include "VtSymbolManager.h"
#include "VtProductCategoryManager.h"
#include "VtAccountManager.h"
#include "VtAccount.h"
#include "VtGlobal.h"
#include "resource.h"
#include "MainFrm.h"
#include "VtProgressDlg.h"


HdScheduler::HdScheduler()
{
	Begin();
}


HdScheduler::~HdScheduler()
{
	End();
}

void HdScheduler::OnTaskCompleted(HdTaskEventArgs& arg)
{
	if (!_Available)
		_Available = true;

	switch (arg.TaskType) { // �۾� Ÿ�Կ� ���� �б�
	case HdTaskType::HdSymbolCode: { // �� ������ �ɺ��ڵ带 �����´�. - ���� �ɺ� �ڵ常 �����´�.
		int remCnt = RemoveRequest(HdTaskType::HdSymbolCode, arg.RequestId);
		if (_ProgressDlg) {
			SetTaskInfo(_T("GetSymbolCode"), remCnt);
		}
		if (!_ReceivedBatchInfo && remCnt == 0) { // �ɺ� �ڵ� �������Ⱑ �����ٸ� �ɺ� ������ ��û
			GetSymbolMaster();
		}
	}
	break;
	case HdTaskType::HdSymbolMaster: { // ���� ���� ������ �о� �´�.
		int remCnt = RemoveRequest(HdTaskType::HdSymbolMaster, arg.RequestId);
		if (_ProgressDlg) {
			SetTaskInfo(_T("GetSymbolMaster"), remCnt);
		}
		if (!_ReceivedBatchInfo && remCnt == 0){
			GetDeposit();
		}
	}
	break;
	case HdTaskType::HdDeposit: { // ���º� �ڱ� ��Ȳ�� �����´�.
		int remCnt = RemoveRequest(HdTaskType::HdDeposit, arg.RequestId);
		if (_ProgressDlg) {
			SetTaskInfo(_T("GetDeposit"), remCnt);
		}
		if (!_ReceivedBatchInfo && remCnt == 0) {
			GetCustomProfitLoss();
		}
	}
	break;
	case HdTaskType::HdApiCustomerProfitLoss: { // ���º�, ���� ������ �����´�.
		int remCnt = RemoveRequest(HdTaskType::HdApiCustomerProfitLoss, arg.RequestId);
		if (!_ReceivedBatchInfo && remCnt == 0) {
			GetOutstanding();
		}
	}
	break;
	case HdTaskType::HdOutstanding: { // ���� �ܰ��� �����´�.
		int remCnt = RemoveRequest(HdTaskType::HdOutstanding, arg.RequestId);
		if (!_ReceivedBatchInfo && remCnt == 0) {
			GetAcceptedHistory();
		}
	}
	break;
	case HdTaskType::HdAcceptedHistory: { // ���� ����Ȯ�� ����� �����´�.
		int remCnt = RemoveRequest(HdTaskType::HdAcceptedHistory, arg.RequestId);
		if (remCnt == 0) {
			_ReceivedBatchInfo = true;
			FinishGetData();
		}
	}
	break;
	case HdTaskType::HdAccountFeeInfoStep1: {
		if (arg.Acnt) {
			arg.Acnt->GetAccountInfoNFee(2);
		}
	}
	break;
	case HdTaskType::HdAccountFeeInfoStep2: {

	}
	default:
		break;
	}
}

void HdScheduler::AddTask(HdTaskArg&& taskArg)
{
	_TaskQueue.push(taskArg);
}

void HdScheduler::ProcessTask()
{
	while (true) 
	{
		if (!_Available) { continue; }
		_Available = false;
		HdTaskArg taskArg = _TaskQueue.pop();
		if (taskArg.Type == HdTaskType::HdTaskComplete)
		{
			break;
		}
		ExecTask(std::move(taskArg));
	}
}

void HdScheduler::StartTaskThread()
{
	_TaskThread = std::thread(&HdScheduler::ProcessTask, this);
}

void HdScheduler::ClearTasks()
{
	_TaskQueue.Clear();
}

void HdScheduler::Begin()
{
	_RecvComp = nullptr;
	_ReceivedBatchInfo = false;
	_SubSectionCount = 0;
	_SectionCount = 0;
	_SymbolCategoryCount = 0;
	_SymbolCount = 0;
	_Available = true;
	StartTaskThread();
}

void HdScheduler::End()
{
	ClearTasks();
	HdTaskArg arg;
	arg.Type = HdTaskType::HdTaskComplete;
	_TaskQueue.push(std::move(arg));
	_Available = true;
	if (_TaskThread.joinable()) _TaskThread.join();
}

void HdScheduler::ExecTask(HdTaskArg&& taskArg)
{
	VtHdClient* client = VtHdClient::GetInstance();
	switch (taskArg.Type)
	{
	case HdTaskType::HdAcceptedHistory:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetAcceptedHistory(accountNo, password);
	}
		break;
	case HdTaskType::HdFilledHistory:
		break;
	case HdTaskType::HdOutstandingHistory:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetOutstandingHistory(accountNo, password);
	}
		break;
	case HdTaskType::HdOutstanding:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetOutstanding(accountNo, password);
	}
		break;
	case HdTaskType::HdCmeAcceptedHistory:
		break;
	case HdTaskType::HdCmeFilledHistory:
		break;
	case HdTaskType::HdCmeOutstandingHistory:
		break;
	case HdTaskType::HdCmeOutstanding:
		break;
	case HdTaskType::HdCmeAsset:
		break;
	case HdTaskType::HdCmePureAsset:
		break;
	case HdTaskType::HdAsset:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetAsset(accountNo, password);
	}
		break;
	case HdTaskType::HdDeposit:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetDeposit(accountNo, password);
	}
		break;
	case HdTaskType::HdDailyProfitLoss:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetDailyProfitLoss(accountNo, password);
	}
		break;
	case HdTaskType::HdFilledHistoryTable:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetFilledHistoryTable(accountNo, password);
	}
		break;
	case HdTaskType::HdAccountProfitLoss:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetAccountProfitLoss(accountNo, password);
	}
		break;
	case HdTaskType::HdSymbolCode:
	{
		std::string symCode = taskArg.GetArg(_T("Category"));
		//TRACE(symCode.c_str());
		//TRACE(_T("\n"));
		client->GetSymbolCode(CString(symCode.c_str()));
	}
		break;
	case HdTaskType::HdTradableCodeTable:
		break;
	case HdTaskType::HdApiCustomerProfitLoss:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetApiCustomerProfitLoss(accountNo, password);
	}
		break;
	case HdTaskType::HdChartData:
		break;
	case HdTaskType::HdCurrentQuote:
		break;
	case HdTaskType::HdDailyQuote:
		break;
	case HdTaskType::HdTickQuote:
		break;
	case HdTaskType::HdSecondQutoe:
		break;
	case HdTaskType::HdSymbolMaster:
	{
		std::string symCode = taskArg.GetArg(_T("SymbolCode"));
		//TRACE(symCode.c_str());
		//TRACE(_T("\n"));
		client->GetSymbolMaster(CString(symCode.c_str()));
	}
		break;
	case HdTaskType::HdStockFutureSymbolMaster:
		break;
	case HdTaskType::HdIndustryMaster:
		break;
	case HdTaskType::HdAccountFeeInfoStep1:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetApiCustomerProfitLoss(HdTaskType::HdAccountFeeInfoStep1, accountNo, password);
	}
		break;
	case HdTaskType::HdAccountFeeInfoStep2:
	{
		std::string accountNo = taskArg.GetArg(_T("AccountNo"));
		std::string password = taskArg.GetArg(_T("Password"));
		client->GetFilledHistoryTable(HdTaskType::HdAccountFeeInfoStep2, accountNo, password);
	}
		break;
	default:
		break;
	}
}

int HdScheduler::AddRequest(HdTaskType reqType, int reqId)
{
	auto it = RequestMap.find(reqType);
	if (it != RequestMap.end())
	{
		RequestQueue& rq = RequestMap[reqType];
		rq.push(reqId);
		return rq.size();
	}
	else
	{
		RequestQueue rq;
		rq.push(reqId);
		RequestMap[reqType] = rq;
		return rq.size();
	}
}

int HdScheduler::RemoveRequest(HdTaskType reqType, int reqId)
{
	auto it = RequestMap.find(reqType);
	if (it != RequestMap.end())
	{
		RequestQueue& rq = RequestMap[reqType];
		if (rq.size() == 0)
			return 0;
		rq.pop();
		return rq.size();
	}

	return -1;
}

void HdScheduler::GetSymbolCode()
{
	VtProductCategoryManager* prdtCatMgr = VtProductCategoryManager::GetInstance();
	HdTaskInfo taskInfo;
	prdtCatMgr->GetSymbolCode(taskInfo.argVec, _T("�Ϲ�"));
	taskInfo.TaskName = _T("���������� �������� ���Դϴ�.");
	taskInfo.TotalTaskCount = taskInfo.argVec.size();
	taskInfo.RemainTaskCount = taskInfo.TotalTaskCount;
	_TaskInfoMap[_T("GetSymbolCode")] = taskInfo;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
	AddTaskList(taskInfo.argVec);
}

void HdScheduler::GetSymbolMaster()
{
	VtProductCategoryManager* prdtCatMgr = VtProductCategoryManager::GetInstance();
	HdTaskInfo taskInfo;
	prdtCatMgr->GetRecentMonthSymbolMasterByCategory(taskInfo.argVec, _T("�Ϲ�"));
	taskInfo.TaskName = _T("���簡�� �������� ���Դϴ�.");
	taskInfo.TotalTaskCount = taskInfo.argVec.size();
	taskInfo.RemainTaskCount = taskInfo.TotalTaskCount;
	_TaskInfoMap[_T("GetSymbolMaster")] = taskInfo;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
	AddTaskList(taskInfo.argVec);
}

void HdScheduler::GetDeposit()
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int i = 0;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		AddRequest(HdTaskType::HdDeposit, i++);
	}

	HdTaskInfo taskInfo;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		VtAccount* acnt = it->second;
		acnt->GetDeposit(taskInfo.argVec);
	}

	taskInfo.TaskName = _T("��Ź�� ������ �������� ���Դϴ�.");
	taskInfo.TotalTaskCount = taskInfo.argVec.size();
	taskInfo.RemainTaskCount = taskInfo.TotalTaskCount;
	_TaskInfoMap[_T("GetDeposit")] = taskInfo;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
	AddTaskList(taskInfo.argVec);
}

void HdScheduler::GetCustomProfitLoss()
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	HdTaskInfo taskInfo;
	int i = 0;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		AddRequest(HdTaskType::HdApiCustomerProfitLoss, i++);
	}
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		VtAccount* acnt = it->second;
		acnt->GetApiCustomerProfitLoss(taskInfo.argVec);
	}

	taskInfo.TaskName = _T("���������� �������� ���Դϴ�.");
	taskInfo.TotalTaskCount = taskInfo.argVec.size();
	taskInfo.RemainTaskCount = taskInfo.TotalTaskCount;
	_TaskInfoMap[_T("GetCustomProfitLoss")] = taskInfo;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
	AddTaskList(taskInfo.argVec);
}

void HdScheduler::GetOutstanding()
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int i = 0;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		AddRequest(HdTaskType::HdOutstanding, i++);
	}

	HdTaskInfo taskInfo;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		VtAccount* acnt = it->second;
		acnt->GetOutstanding(taskInfo.argVec);
	}

	taskInfo.TaskName = _T("�ܰ������� �������� ���Դϴ�.");
	taskInfo.TotalTaskCount = taskInfo.argVec.size();
	taskInfo.RemainTaskCount = taskInfo.TotalTaskCount;
	_TaskInfoMap[_T("GetOutstanding")] = taskInfo;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
	AddTaskList(taskInfo.argVec);
}

void HdScheduler::GetAcceptedHistory()
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int i = 0;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		AddRequest(HdTaskType::HdAcceptedHistory, i++);
	}

	HdTaskInfo taskInfo;
	for (auto it = acntMgr->AccountMap.begin(); it != acntMgr->AccountMap.end(); ++it) {
		VtAccount* acnt = it->second;
		acnt->GetAcceptedHistory(taskInfo.argVec);
	}

	taskInfo.TaskName = _T("��ü�� ������ �������� ���Դϴ�.");
	taskInfo.TotalTaskCount = taskInfo.argVec.size();
	taskInfo.RemainTaskCount = taskInfo.TotalTaskCount;
	_TaskInfoMap[_T("GetAcceptedHistory")] = taskInfo;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
	AddTaskList(taskInfo.argVec);
}

void HdScheduler::AddTaskList(std::vector<std::pair<std::string, HdTaskArg>>& argVec)
{
	for (auto it = argVec.begin(); it != argVec.end(); ++it) {
		_TaskQueue.push(std::get<1>(*it));
	}
}

void HdScheduler::SetTaskInfo(std::string code, int remCnt)
{
	HdTaskInfo& taskInfo = _TaskInfoMap[code];
	taskInfo.RemainTaskCount = remCnt;
	if (_ProgressDlg) {
		_ProgressDlg->SetTaskInfo(taskInfo);
	}
}

void HdScheduler::FinishGetData()
{
	//CMainFrame* pMainWnd = (CMainFrame*)AfxGetMainWnd();
	//pMainWnd->OnReceiveComplete();

	if (_ProgressDlg) {
		_ProgressDlg->CloseDialog();
		_ProgressDlg = nullptr;
	}
}