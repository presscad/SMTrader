// VtHdCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "SMTrader.h"
#include "VtHdCtrl.h"
#include "afxdialogex.h"
#include "HDDefine.h"
#include "HdScheduler.h"
#include "Poco/LocalDateTime.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/DateTimeFormatter.h"
#include <string>
#include "Poco/Delegate.h"
#include "VtSymbolManager.h"
#include "VtSymbol.h"
#include "VtProductCategoryManager.h"
#include "VtProductSubSection.h"
#include "VtOrderDialogManager.h"
#include "VtOrder.h"
#include <iomanip>
#include <sstream>
#include <ios>
#include "VtOrderManagerSelector.h"
#include "VtOrderManager.h"
#include "VtProductOrderManagerSelector.h"
#include "VtProductOrderManager.h"
#include "VtAccountManager.h"
#include "VtAccount.h"
#include "Poco/NumberFormatter.h"
#include "VtPosition.h"
#include "HdFutureGrid.h"
#include "HdOptionGrid.h"
#include <tuple>
#include "ZmConfigManager.h"
#include "VtRealtimeRegisterManager.h"
#include "HdWindowManager.h"
#include "HdAccountPLDlg.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include "VtGlobal.h"
#include "Global/VtDefine.h"
#include "VtAccountAssetDlg.h"
#include "VtStringUtil.h"
#include "VtFundMiniJango.h"
#include "VtChartDataManager.h"
#include "VtChartData.h"
#include "VtOrderQueueManager.h"
#include "chartdir.h"
#include "MainFrm.h"
#include "VtTotalOrderManager.h"
#include "VtChartDataCollector.h"
#include "Log/loguru.hpp"
#include "VtStringUtil.h"
#include "SmCallbackManager.h"
#include "Market/SmSymbolReader.h"
#include "Chart/SmChartData.h"
#include "Chart/SmChartDataManager.h"
#include "SmTaskManager.h"
#include "Market/SmMarketManager.h"
#include "VtLoginManager.h"

using namespace std;

using Poco::Delegate;

using Poco::LocalDateTime;
using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::NumberFormatter;


// VtHdCtrl dialog

IMPLEMENT_DYNAMIC(VtHdCtrl, CDialogEx)

VtHdCtrl::VtHdCtrl(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_HDF_CTRL, pParent)
{
	_FutureGrid = nullptr;
	_OptionGrid = nullptr;

	HdScheduler* scheduler = HdScheduler::GetInstance();
	_TaskCompleteEvent += delegate(scheduler, &HdScheduler::OnTaskCompleted);
}

VtHdCtrl::~VtHdCtrl()
{
	HdScheduler* scheduler = HdScheduler::GetInstance();
	_TaskCompleteEvent -= delegate(scheduler, &HdScheduler::OnTaskCompleted);
	if (m_CommAgent.GetSafeHwnd()) {
		if (m_CommAgent.CommGetConnectState() == 1) {
			if (m_sUserId != "")
				m_CommAgent.CommLogout(m_sUserId);
		}

		m_CommAgent.CommTerminate(TRUE);
		m_CommAgent.DestroyWindow();
	}
}

void VtHdCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


int VtHdCtrl::LogIn(CString id, CString pwd, CString cert)
{
	m_sUserId = id;
	m_sUserPw = pwd;
	m_sAuthPw = cert;
	const char *pLoginSuccess[] = { "로그인 성공"	, "Login Successful" };
	const char *pLoginFail[] = { "로그인 실패"	, "Login Failure" };

	int nRet = m_CommAgent.CommLogin(m_sUserId, m_sUserPw, m_sAuthPw);
	if (nRet > 0) {
		//AfxMessageBox(pLoginSuccess[0]);

		//로긴후 반드시 호출...
		m_CommAgent.CommAccInfo();
		LOG_F(INFO, pLoginSuccess[0]);
	}
	else
	{
		CString strRet;
		strRet.Format("[%d]", nRet);
		//AfxMessageBox(pLoginFail[0] + strRet);
		LOG_F(INFO, _T("%s"), strRet);
	}

	return nRet;
}

int VtHdCtrl::Init()
{
	if (!m_CommAgent.GetSafeHwnd()) {
		m_CommAgent.Create("HDF CommAgent", WS_CHILD, CRect(0, 0, 0, 0), this, 2286);
	}
	else
		return -1000;

	if (m_CommAgent.GetSafeHwnd()) {
		int nRet = m_CommAgent.CommInit(1);
		
		if (nRet < 0) {
			//AfxMessageBox("통신프로그램 실행 오류");
			LOG_F(INFO, _T("통신프로그램 실행 오류"));
		}
		else {
			//AfxMessageBox("통신프로그램 실행 성공");
			LOG_F(INFO, _T("통신프로그램 실행 성공"));
		}

		return nRet;
	}

	return -1000;
}

BEGIN_MESSAGE_MAP(VtHdCtrl, CDialogEx)
END_MESSAGE_MAP()

BEGIN_EVENTSINK_MAP(VtHdCtrl, CDialogEx)
	ON_EVENT(VtHdCtrl, (UINT)-1, 3, OnDataRecv, VTS_BSTR VTS_I4)
	ON_EVENT(VtHdCtrl, (UINT)-1, 4, OnGetBroadData, VTS_BSTR VTS_I4)
	ON_EVENT(VtHdCtrl, (UINT)-1, 5, OnGetMsg, VTS_BSTR VTS_BSTR)
	ON_EVENT(VtHdCtrl, (UINT)-1, 6, OnGetMsgWithRqId, VTS_I4 VTS_BSTR VTS_BSTR)

END_EVENTSINK_MAP()

// VtHdCtrl message handlers
void VtHdCtrl::LogIn()
{
	int nRet = m_CommAgent.CommLogin(m_sUserId, m_sUserPw, m_sAuthPw);
	if (nRet > 0)
	{
		//AfxMessageBox("로그인 성공");

		// 로긴후 반드시 호출...
		m_CommAgent.CommAccInfo();
		//m_CommAgent.CommReqMakeCod("all", 0);

		//GetAccountInfo();
	}
	else
	{
		CString strRet;
		strRet.Format("[%d]", nRet);
		AfxMessageBox("로그인 실패" + strRet);
	}
}


void VtHdCtrl::LogOut()
{
	// 로그아웃한다.
	int nRet = m_CommAgent.CommLogout(m_sUserId);

	CString strRet;
	strRet.Format("[%d]", nRet);
	if (nRet < 0) {
		AfxMessageBox("로그아웃 실패" + strRet);
	}
	else {
		AfxMessageBox("로그아웃 성공" + strRet);
	}
}

int VtHdCtrl::LogOut(CString id)
{
	return m_CommAgent.CommLogout(m_sUserId);
}


void VtHdCtrl::GetSymbolMasterWithCount(std::string symCode, int count)
{
	CString sInput = VtSymbolManager::TrimRight(symCode, ' ').c_str();
	sInput.Append(_T("4"));
	std::string temp;
	temp = PadLeft(count, '0', 4);
	sInput.Append(temp.c_str());
	//단축코드[000], 호가수신시간[075], 현재가[051], 누적거래량[057], 최종결제가[040], 상장일[006], 고가[053]  1건 조회시
	CString sReqFidInput = _T("000001002003004005006007008009010011012013014015016017018019020021022023024025026027028029030031032033034035036037038039040041042043044045046047048049050051052053054055056057058059060061062063064065066067068069070071072073074075076077078079080081082083084085086087088089090091092093094095096097098099100101102103104105106107108109110111112113114115116117118119120121122123124125126127128129130131132133134135136137138139140141142143144145146147148149150151152153154155156157158159160161162163164165166167168169170171172173174175176177178179180181182183184185186187188189190191192193194195196197198199200201202203204205206207208209210211212213214215216217218219220221222223224225226227228229230231232");
	//CString sReqFidInput = _T("000001002003004005049050051052053054075076078079080081082083084085086087089090091092093094095096097098099100101102103104105106107108109115");
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommFIDRqData(DefSymbolMaster, sInput, sReqFidInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdSymbolMaster);
}

void VtHdCtrl::GetMasterFile(std::string fileName)
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string path = configMgr->GetAppPath();
	path.append(_T("\\"));
	path.append(fileName);
	ifstream file(path);

	if (!file.is_open())
		return;

	string content;
	std::vector<std::string> line;

	while (std::getline(file, content))
	{
		line.push_back(content);
	}

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	HdProductInfo* prdtInfo = nullptr;
	for (auto it = line.begin(); it != line.end(); ++it)
	{
		int index = 0;
		std::string info = *it;
		std::string temp;
		temp = info.substr(index, 8);
		temp = VtSymbolManager::TrimRight(temp, ' ');
		prdtInfo = symMgr->AddProductInfo(temp);
		index = index + 8;
		temp = info.substr(index, 2);
		prdtInfo->decimal = std::stoi(temp);
		index = index + 2;
		temp = info.substr(index, 5);
		temp = VtSymbolManager::TrimLeft(temp, ' ');
		prdtInfo->tickSize = temp;
		temp.erase(std::remove(temp.begin(), temp.end(), '.'), temp.end());
		prdtInfo->intTickSize = std::stoi(temp);
		index = index + 5;
		temp = info.substr(index, 5);
		prdtInfo->tickValue = std::stoi(temp);
		index = index + 5;
		temp = info.substr(index, 10);
		temp = VtSymbolManager::TrimLeft(temp, ' ');
		prdtInfo->tradeWin = std::stoi(temp);
		index = index + 10;
		temp = info.substr(index, 30);
		temp = VtSymbolManager::TrimLeft(temp, ' ');
		prdtInfo->name = temp;
	}

	int i = 0;
}

BOOL VtHdCtrl::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}



void VtHdCtrl::GetFutureCodeList()
{
	if (_Blocked)
		return;

	CString sTrCode = DEF_AVAILABLE_CODE_LIST;
	CString sInput = "120180328";
	CString strNextKey = _T("");
	int nResult = m_CommAgent.CommGetConnectState();
	if (nResult == 1)
	{
		int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	}
}

void VtHdCtrl::GetFutureCodeList(CString categoryName)
{
	if (_Blocked)
		return;

	GetSymbolCode(categoryName);
}

int VtHdCtrl::GetSymbolCode(CString categoryName)
{
	if (_Blocked)
		return -1;
	CString sTrCode = DefSymbolCode;
	CString sInput = categoryName;
	CString strNextKey = _T("");
	int nResult = m_CommAgent.CommGetConnectState();
	if (nResult == 1)
	{
		int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
		AddRequest(nRqID, HdTaskType::HdSymbolCode);

		return nRqID;
	}

	return -1;
}

void VtHdCtrl::GetOptionCodeList(CString categoryName)
{
	if (_Blocked)
		return;

	GetSymbolCode(categoryName);
}

void VtHdCtrl::GetTradableCodeTable()
{
	LocalDateTime now;
	std::string curDate(DateTimeFormatter::format(now, "%Y%m%d"));
	CString strDate(curDate.c_str());
	CString sTrCode = DEF_AVAILABLE_CODE_LIST;
	CString sInput = _T("2");
	sInput.Append(strDate);
	CString strNextKey = _T("");
	int nResult = m_CommAgent.CommGetConnectState();
	if (nResult == 1)
	{
		int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
		AddRequest(nRqID, HdTaskType::HdTradableCodeTable);
	}
}

int VtHdCtrl::GetSymbolMaster(CString symCode)
{
	if (_Blocked)
		return -1;

	if (isdigit(symCode.GetAt(2))) {
		CString sInput;
		sInput = symCode;
		sInput.Append(_T("40001"));
		//단축코드[000], 호가수신시간[075], 현재가[051], 누적거래량[057], 최종결제가[040], 상장일[006], 고가[053]  1건 조회시
		CString sReqFidInput = _T("000001002003004005006007008009010011012013014015016017018019020021022023024025026027028029030031032033034035036037038039040041042043044045046047048049050051052053054055056057058059060061062063064065066067068069070071072073074075076077078079080081082083084085086087088089090091092093094095096097098099100101102103104105106107108109110111112113114115116117118119120121122123124125126127128129130131132133134135136137138139140141142143144145146147148149150151152153154155156157158159160161162163164165166167168169170171172173174175176177178179180181182183184185186187188189190191192193194195196197198199200201202203204205206207208209210211212213214215216217218219220221222223224225226227228229230231232");
		//CString sReqFidInput = _T("000001002003004005049050051052053054075076078079080081082083084085086087089090091092093094095096097098099100101102103104105106107108109115");
		CString strNextKey = _T("");
		int nRqID = m_CommAgent.CommFIDRqData(DefSymbolMaster, sInput, sReqFidInput, sInput.GetLength(), strNextKey);
		AddRequest(nRqID, HdTaskType::HdSymbolMaster);
		return nRqID;
	}
	else {
		return GetAbroadQuote((LPCTSTR)symCode);
	}
}

void VtHdCtrl::RegisterProduct(CString symCode)
{
	if (isdigit(symCode.GetAt(2))) {
		int nRealType = 0;
		int nResult = 0;
		CString strKey = symCode;
		TCHAR first = symCode.GetAt(0);
		CString prefix = symCode.Left(3);
		if (first == '1' || first == '4') {
			if (prefix.Compare(_T("167")) == 0 || prefix.Compare(_T("175")) == 0) {
				nRealType = 58;
				nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
				nRealType = 71;
				nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
			}
			else {
				nRealType = 51;
				nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
				nRealType = 65;
				nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
			}
		}
		else if (first == '2' || first == '3') {
			nRealType = 52;
			nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
			nRealType = 66;
			nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
		}
		else {
			nRealType = 82;
			nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
		}
	}
	else {
		std::string code = symCode;
		std::string key = VtStringUtil::PadRight(code, ' ', 32);
		int nRealType = 76; // 시세
		m_CommAgent.CommSetBroad(key.c_str(), nRealType);
		nRealType = 82; // 호가
		m_CommAgent.CommSetBroad(key.c_str(), nRealType);
	}
}

void VtHdCtrl::UnregisterProduct(CString symCode)
{
	int nRealType = 0;
	int nResult = 0;
	CString strKey = symCode;
	TCHAR first = symCode.GetAt(0);
	if (first == '1' || first == '4')
	{
		nRealType = 51;
		nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);
		nRealType = 65;
		nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);
	}
	else if (first == '2' || first == '3')
	{
		nRealType = 52;
		nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);
		nRealType = 66;
		nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);
	}
}

void VtHdCtrl::RegisterAccount(CString accountNo)
{
	CString strUserId = m_sUserId;
	CString strAcctNo = accountNo;
	int nResult = m_CommAgent.CommSetJumunChe(strUserId, strAcctNo);
}

void VtHdCtrl::UnregisterAccount(CString accountNo)
{
	CString strUserId = m_sUserId;
	CString strAcctNo = accountNo;
	int nResult = m_CommAgent.CommRemoveJumunChe(strUserId, strAcctNo);
}

void VtHdCtrl::RequestChartDataFromQ()
{
	if (!_ChartDataReqQueue.empty()) {
		_ChartDataReqQueue.pop();
		GetChartData(_ChartDataReqQueue.front());
	}
}

int VtHdCtrl::GetIntOrderPrice(CString symbol_code, CString strPrice, CString strOrderPrice)
{
	int order_price = _ttoi(strPrice);
	VtSymbol* symbol = VtSymbolManager::GetInstance()->FindHdSymbol((LPCTSTR)symbol_code);
	int fpos = strOrderPrice.Find('.');
	if (fpos == -1) {
		order_price = order_price * std::pow(10, symbol->Decimal);
	}
	else {
		int len = strOrderPrice.GetLength() - (fpos + 1);
		order_price = order_price * std::pow(10, symbol->Decimal - len);
	}

	return order_price;
}

bool VtHdCtrl::CheckPassword(HdOrderRequest& request)
{
	if (!request.Password.empty() && request.Password.length() == 4) {
		return true;
	}
	else {
		AfxMessageBox(_T("계좌에 비밀번호를 설정하지 않았습니다. 주문할수 없습니다."));
		return false;
	}
}

void VtHdCtrl::OnReceiveChartData(CString& sTrCode, LONG& nRqID)
{
	VtChartData* chartData = nullptr;
	VtChartDataManager* chartDataMgr = VtChartDataManager::GetInstance();
	auto it = _ChartDataRequestMap.find(nRqID);
	if (it != _ChartDataRequestMap.end()) { // 차트데이터 요청 맵에서 찾아 본다.
		chartData = chartDataMgr->Find(it->second);
		_ChartDataRequestMap.erase(it);
	}

	if (!chartData)
		return;

	CString strPrevKey = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "이전키");

	int nRepeatCnt = 0;
	if (chartData->Domestic())
		nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
	else
		nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	int savedIndex = ChartDataSize - 1;
	// 먼저 키를 모두 삭제한다.
	chartData->InputDateTimeMap.clear();
	// Received the chart data first.
	auto timeKey = std::make_pair(0, 0);
	for (int i = 0; i < nRepeatCnt; i++) {
		CString strDate;
		CString key(_T(""));
		CString strTime;
		if (chartData->Domestic()) {
			strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "날짜시간");
			if (chartData->ChartType() == VtChartType::MIN)
				strDate.Append(_T("00"));
			else
				strDate.Append(_T("000000"));
			key = _T("OutRec2");
			strTime = strDate.Right(6);
			CString strDate2 = strDate.Left(8);
			timeKey = std::make_pair(_ttoi(strDate2), _ttoi(strTime));
		}
		else {
			strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "국내일자");
			strTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "국내시간");
			timeKey = std::make_pair(_ttoi(strDate), _ttoi(strTime));
			strDate.Append(strTime);
			key = _T("OutRec1");
		}
		if (strDate.GetLength() == 0) {
			break;
		}
		CString strOpen = m_CommAgent.CommGetData(sTrCode, -1, key, i, "시가");
		CString strHigh = m_CommAgent.CommGetData(sTrCode, -1, key, i, "고가");
		CString strLow = m_CommAgent.CommGetData(sTrCode, -1, key, i, "저가");
		CString strClose = m_CommAgent.CommGetData(sTrCode, -1, key, i, "종가");
		CString strVol;
		if (chartData->Domestic())
			strVol = m_CommAgent.CommGetData(sTrCode, -1, key, i, "거래량");
		else
			strVol = m_CommAgent.CommGetData(sTrCode, -1, key, i, "체결량");

		savedIndex = ChartDataSize - 1 - i;
		
		if (strHigh.GetLength() != 0)
			chartData->InputChartData.High[savedIndex] = std::stod((LPCTSTR)strHigh) / std::pow(10, chartData->Decimal());
		if (strLow.GetLength() != 0)
			chartData->InputChartData.Low[savedIndex] = std::stod((LPCTSTR)strLow) / std::pow(10, chartData->Decimal());
		if (strOpen.GetLength() != 0)
			chartData->InputChartData.Open[savedIndex] = std::stod((LPCTSTR)strOpen) / std::pow(10, chartData->Decimal());
		if (strClose.GetLength() != 0)
			chartData->InputChartData.Close[savedIndex] = std::stod((LPCTSTR)strClose) / std::pow(10, chartData->Decimal());
		if (strVol.GetLength() != 0)
			chartData->InputChartData.Volume[savedIndex] = std::stod((LPCTSTR)strVol);

		std::pair<VtDate, VtTime> dateTime = VtChartData::GetDateTime(strDate);
		chartData->InputChartData.Date[savedIndex] = dateTime.first;
		chartData->InputChartData.Time[savedIndex] = dateTime.second;
		chartData->InputChartData.DateTime[savedIndex] = Chart::chartTime(dateTime.first.year, dateTime.first.month, dateTime.first.day, dateTime.second.hour, dateTime.second.min, dateTime.second.sec);

		chartData->InputDateTimeMap[timeKey] = savedIndex;
	}
	if (chartData->GetDataCount() == 0) {
		chartData->SetFirstData();
		chartData->FilledCount(ChartDataSize);
		chartData->Filled(true);
		chartDataMgr->OnReceiveFirstChartData(chartData);
		return;
	}
	chartDataMgr->OnReceiveChartData(chartData);
}

void VtHdCtrl::OnChartData(CString& sTrCode, LONG& nRqID)
{
	VtChartData* chartData = nullptr;
	VtChartDataManager* chartDataMgr = VtChartDataManager::GetInstance();
	auto it = _ChartDataRequestMap.find(nRqID);
	if (it != _ChartDataRequestMap.end()) { // 차트데이터 요청 맵에서 찾아 본다.
		chartData = chartDataMgr->Find(it->second);
		_ChartDataRequestMap.erase(it);
	}

	if (!chartData)
		return;
	double lastTimeInfo = chartData->GetLastTimeInfo();
	double firstTimeInfo = 0.0;

	CString strPrevKey = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "이전키");

	int nRepeatCnt = 0;
	if (chartData->Domestic())
		nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
	else
		nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	int foundIndex = nRepeatCnt;
	int realCount = nRepeatCnt;
	int lastSavedIndex = nRepeatCnt - 1;
	int savedIndex = lastSavedIndex;
	// 먼저 키를 모두 삭제한다.
	chartData->InputDateTimeMap.clear();
	// Received the chart data first.
	auto timeKey = std::make_pair(0, 0);
	for (int i = 0; i < nRepeatCnt; i++) {
		CString strDate;
		CString key(_T(""));
		CString strTime;
		if (chartData->Domestic()) {
			strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "날짜시간");
			if (chartData->ChartType() == VtChartType::MIN)
				strDate.Append(_T("00"));
			else
				strDate.Append(_T("000000"));
			key = _T("OutRec2");
			strTime = strDate.Right(6);
			CString strDate2 = strDate.Left(8);
			timeKey = std::make_pair(_ttoi(strDate2), _ttoi(strTime));
		}
		else {
			strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "국내일자");
			strTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "국내시간");
			strDate.Append(strTime);
			key = _T("OutRec1");
			timeKey = std::make_pair(_ttoi(strDate), _ttoi(strTime));
		}
		if (strDate.GetLength() == 0) {
			if (foundIndex == nRepeatCnt)
				foundIndex = i;
			realCount = i;
			lastSavedIndex = savedIndex;
			break;
		}
		CString strOpen = m_CommAgent.CommGetData(sTrCode, -1, key, i, "시가");
		CString strHigh = m_CommAgent.CommGetData(sTrCode, -1, key, i, "고가");
		CString strLow = m_CommAgent.CommGetData(sTrCode, -1, key, i, "저가");
		CString strClose = m_CommAgent.CommGetData(sTrCode, -1, key, i, "종가");
		CString strVol;
		if (chartData->Domestic())
			strVol = m_CommAgent.CommGetData(sTrCode, -1, key, i, "거래량");
		else
			strVol = m_CommAgent.CommGetData(sTrCode, -1, key, i, "체결량");

		savedIndex = nRepeatCnt - 1 - i;
		double timeInfo = std::stod((LPCTSTR)strDate);

		if (i == 0) {
			firstTimeInfo = timeInfo;
		}

		chartData->InputChartData.TimeInfo[savedIndex] = timeInfo;
		if (lastTimeInfo == chartData->InputChartData.TimeInfo[savedIndex]) {
			foundIndex = savedIndex;
		}
		if (strHigh.GetLength() != 0)
			chartData->InputChartData.High[savedIndex] = std::stod((LPCTSTR)strHigh) / std::pow(10, chartData->Decimal());
		if (strLow.GetLength() != 0)
			chartData->InputChartData.Low[savedIndex] = std::stod((LPCTSTR)strLow) / std::pow(10, chartData->Decimal());
		if (strOpen.GetLength() != 0)
			chartData->InputChartData.Open[savedIndex] = std::stod((LPCTSTR)strOpen) / std::pow(10, chartData->Decimal());
		if (strClose.GetLength() != 0)
			chartData->InputChartData.Close[savedIndex] = std::stod((LPCTSTR)strClose) / std::pow(10, chartData->Decimal());
		if (strVol.GetLength() != 0)
			chartData->InputChartData.Volume[savedIndex] = std::stod((LPCTSTR)strVol);

		std::pair<VtDate, VtTime> dateTime = VtChartData::GetDateTime(strDate);
		chartData->InputChartData.Date[savedIndex] = dateTime.first;
		chartData->InputChartData.Time[savedIndex] = dateTime.second;
		chartData->InputChartData.DateTime[savedIndex] = Chart::chartTime(dateTime.first.year, dateTime.first.month, dateTime.first.day, dateTime.second.hour, dateTime.second.min, dateTime.second.sec);

		chartData->InputDateTimeMap[timeKey] = savedIndex;
		CString strData;
		strData.Format(_T("savedIndex = %d, index = %d, %s,%s,%s,%s,%s,%s,%s \n"), savedIndex, i, strDate, strTime, strOpen, strHigh, strLow, strClose, strVol);
		TRACE(strData);

		//LOG_F(INFO, _T("code %s, datetime =%s, savedIndex = %d"), chartData->SymbolCode().c_str(), strDate, savedIndex);
	}

	if (nRepeatCnt != realCount) {
		for (int p = 0; p < realCount; p++) {
			chartData->InputChartData.TimeInfo[p] = chartData->InputChartData.TimeInfo[lastSavedIndex + p];
			chartData->InputChartData.High[p] = chartData->InputChartData.High[lastSavedIndex + p];
			chartData->InputChartData.Low[p] = chartData->InputChartData.Low[lastSavedIndex + p];
			chartData->InputChartData.Open[p] = chartData->InputChartData.Open[lastSavedIndex + p];
			chartData->InputChartData.Close[p] = chartData->InputChartData.Close[lastSavedIndex + p];
			chartData->InputChartData.Volume[p] = chartData->InputChartData.Volume[lastSavedIndex + p];
			chartData->InputChartData.Date[p] = chartData->InputChartData.Date[lastSavedIndex + p];
			chartData->InputChartData.Time[p] = chartData->InputChartData.Time[lastSavedIndex + p];
			chartData->InputChartData.DateTime[p] = chartData->InputChartData.DateTime[lastSavedIndex + p];
		}
		// Reset the filled count of chart data
		chartData->FilledCount(0);
		chartData->CopyData(realCount, foundIndex);
	}
	else {
		// 마지막으로 받은 시간이 같으면 처리하지 않는다.
		if (lastTimeInfo != firstTimeInfo)
			chartData->CopyData(realCount, foundIndex);
	}

	chartData->Filled(true);
	chartDataMgr->OnReceiveChartData(chartData);
	//RemoveRequest(nRqID);
}

void VtHdCtrl::OnNewOrderHd(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");


	CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
	CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리구분");
	CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");
	CString strPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가격");
	CString strAmount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문수량");
	CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매매구분");
	CString strPriceType = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "가격구분");
	CString strCustom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "사용자정의필드");
	CString strMan = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "조작구분");

 	CString strMsg;
	strMsg.Format("OnNewOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
	//TRACE(strMsg);

	strPrice = strPrice.TrimLeft('0');
	CString strOriOrderPrice;
	strOriOrderPrice = strPrice;
	strPrice.Remove('.'); // 주문가격을 정수로 변환

	strAcctNo.TrimRight(); // 계좌 번호
	strOrdNo.TrimLeft('0'); // 주문 번호
	strSeries.TrimRight(); // 심볼 코드
	strPrice.TrimRight(); // 주문 가격
	strAmount.TrimRight(); // 주문 수량

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderReqId(nRqID);
	// 여기서 주문요청 아이디와 주문요청 객체를 매칭해 준다.
	auto it = _ReqIdToRequestMap.find(nRqID);
	if (it != _ReqIdToRequestMap.end()) {
		HdOrderRequest req = it->second;
		_OrderNoToRequestMap[(LPCTSTR)strOrdNo] = req;
	}

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)strOrdNo);
	// 주문이 없다면 새로 만든다.
	if (!order){
		order = new VtOrder();
		// 주문 요청 아이디 
		order->HtsOrderReqID = nRqID;
		// 일반 주문인지 청산 주문인지 넣어 준다.
		order_req != nullptr ? order->RequestType = order_req->RequestType : order->RequestType = -1;
		if (order_req) {
			// 여기서 시스템 정보를 넣어준다.
			order->StrategyName = order_req->StratageName;
		}
		// 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수로 변환한 주문 가격
		order->intOrderPrice = _ttoi(strPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 주문 가격 - 소수점 표시
		order->orderPrice = _ttof(strOriOrderPrice);

		// 주문 포지션 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0)
		{
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0)
		{
			order->orderPosition = VtPositionType::Sell;
		}

		// 주문 가격 타입 - 지정가 / 시장가
		if (strPriceType.Compare(_T("1")) == 0)
		{
			order->priceType = VtPriceType::Price;
		}
		else if (strPosition.Compare(_T("2")) == 0)
		{
			order->priceType = VtPriceType::Market;
		}

		// 주문 타입 - 신규 주문
		order->orderType = VtOrderType::New;

		// 일반계좌 주문
		order->Type = 0;
	}
	else { // 주문이 있다는 것은 이미 거래소 접수 되었거나 체결된 경우이다. 이 부분은 특별하게 처리해야 한다.
		if (order->state == VtOrderState::Filled) { 
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			if (order_req) {
				// 여기서 시스템 정보를 넣어준다.
				order->StrategyName = order_req->StratageName;
				LOG_F(INFO, _T("OnNewOrderHd 주문역전 :: 이미 체결된 주문입니다! 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, order_req->Type == 1 ? order_req->SubAccountNo.c_str() : order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 여기서 시스템 정보를 넣어준다.
					subAcntOrder->StrategyName = order_req->StratageName;
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 여기서 시스템 정보를 넣어준다.
					subAcntOrder->StrategyName = order_req->StratageName;
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
		else if (order->state == VtOrderState::Accepted) {

			// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
			// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
			if (order_req) {
				// 여기서 시스템 정보를 넣어준다.
				order->StrategyName = order_req->StratageName;
				LOG_F(INFO, _T("OnNewOrderHd 주문역전 :: 이미 거래소 접수된 주문입니다! 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, order_req->Type == 1 ? order_req->SubAccountNo.c_str() : order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
											// 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 여기서 시스템 정보를 넣어준다.
					subAcntOrder->StrategyName = order_req->StratageName;
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
												 // 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 여기서 시스템 정보를 넣어준다.
					subAcntOrder->StrategyName = order_req->StratageName;
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
	}

	// 주문 수신을 처리해 준다.
	orderMgr->OnOrderReceivedHd(order);
	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	if (order_req) {
		// 여기서 시스템 정보를 넣어준다.
		order->StrategyName = order_req->StratageName;
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			// 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 여기서 시스템 정보를 넣어준다.
			subAcntOrder->StrategyName = order_req->StratageName;
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		} 
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			 // 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 여기서 시스템 정보를 넣어준다.
			subAcntOrder->StrategyName = order_req->StratageName;
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 펀드이름을 넣어준다.
			subAcntOrder->FundName = order_req->FundName;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
	}

	// 주문 상태를 바꿔준다.
	order->state = VtOrderState::OrderReceived;
	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	//OnOrderReceived(nRqID, order);

	//OnSubAccountOrder(VtOrderEvent::PutNew, strSubAcntNo, strFundName, order, prevState);

	LOG_F(INFO, _T("신규주문서버확인 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);
}

void VtHdCtrl::OnModifyOrderHd(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

	CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
	CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리구분");
	CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");
	CString strPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가격");
	CString strAmount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문수량");
	CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매매구분");
	CString strPriceType = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "가격구분");
	CString strCustom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "사용자정의필드");
	CString strMan = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "조작구분");
	CString strOriOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "원주문번호");

	CString strMsg;
	strMsg.Format("OnModifyOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
	//WriteLog(strMsg);
	strMsg.Format(_T("%s\n"), strCustom);
	//TRACE(strMsg);

	strPrice = strPrice.TrimLeft('0'); // 주문 가격
	CString strOriOrderPrice;
	strOriOrderPrice = strPrice; // 소수점 주문 가격 저장
	strPrice.Remove('.'); // 주문 가격 정수형으로 변환
	strAcctNo.TrimRight(); // 계좌 번호
	strOrdNo.TrimLeft('0'); // 주문 번호
	strOriOrdNo.TrimLeft('0'); // 원주문 번호
	strSeries.TrimRight(); // 심볼 코드
	strPrice.TrimRight(); // 주문 가격 
	strAmount.TrimRight(); // 주문 수량
	strOriOrdNo.TrimRight(); // 원주문 번호

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderReqId(nRqID);
	if (order_req) {
		// 새로운 주문번호를 넣어 준다.
		order_req->NewOrderNo = (LPCTSTR)strOrdNo;
	}
	// 여기서 주문요청 아이디와 주문요청 객체를 매칭해 준다.
	auto it = _ReqIdToRequestMap.find(nRqID);
	if (it != _ReqIdToRequestMap.end()) {
		HdOrderRequest req = it->second;
		_OrderNoToRequestMap[(LPCTSTR)(strOrdNo)] = req;
		_OrderNoToRequestMap[(LPCTSTR)(strOriOrdNo)] = req;
	}

	// 정정된 원주문이 체결된 경우는 더이상 처리하지 않는다.
	VtOrder* originOrder = orderMgr->FindOrder((LPCTSTR)(strOriOrdNo));
	if (originOrder && originOrder->state == VtOrderState::Filled) {
		// 원주문을 제거해 준다.
		orderMgr->RemoveAcceptedHd(originOrder);
		if (order_req) {
			VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
			VtOrderManager* subOrderMgr = nullptr;
			VtOrder* subAcntOrder = nullptr;

			if (order_req->Type == 1) { // 서브계좌 주문인 경우
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(originOrder);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = originOrder->AccountNo;
				subOrderMgr->OnOrderFilledHd(subAcntOrder);
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
			else if (order_req->Type == 2) { // 펀드 주문인 경우
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(originOrder);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = originOrder->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
				subOrderMgr->OnOrderFilledHd(subAcntOrder);
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
		}
	}

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	if (!order) { // 주문이 없는 경우는 처음으로 이곳으로 주문이 들어와 생성되는 것이다.
		strMsg.Format("new ::::: OnModifyOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
		strMsg.Format(_T("%s\n"), strCustom);
		//TRACE(strMsg);
		order = new VtOrder();
		// 주문 요청 번호
		order->HtsOrderReqID = nRqID;
		// 일반 주문인지 청산 주문인지 넣어 준다.
		order_req != nullptr ? order->RequestType = order_req->RequestType : order->RequestType = -1;
		// 주문 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수로 변환된 주문 가격
		order->intOrderPrice = _ttoi(strPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 원 주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrdNo);
		// 소수로 표시된 주문 가격
		order->orderPrice = _ttof(strOriOrderPrice);

		// 주문 포지션 타입 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0) {
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->orderPosition = VtPositionType::Sell;
		}

		// 주문 가격 타입 - 지정가 / 시장가
		if (strPriceType.Compare(_T("1")) == 0) {
			order->priceType = VtPriceType::Price;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->priceType = VtPriceType::Market;
		}

		// 주문 유형
		order->orderType = VtOrderType::Change;

		// 일반계좌 주문
		order->Type = 0;
	}
	else { // 여기서 역전된 주문을 처리한다.
		if (order->state == VtOrderState::Filled) {
			LOG_F(INFO, _T("OnModifyOrderHd 주문역전 :: 이미 체결된 주문입니다!"));
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
		else if (order->state == VtOrderState::Accepted) {
			LOG_F(INFO, _T("OnModifyOrderHd 주문역전 :: 이미 거래소 접수된 주문입니다!"));

			// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
			// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
											// 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					// 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
	}
	
	// 주문 수신을 처리해 준다.
	orderMgr->OnOrderReceivedHd(order);
	// 주문 상태를 바꿔준다.
	order->state = VtOrderState::OrderReceived;
	SmCallbackManager::GetInstance()->OnOrderEvent(order);
	
	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			// 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			// 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 펀드이름을 넣어준다.
			subAcntOrder->FundName = order_req->FundName;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
	}

	//SendOrderMessage(VtOrderEvent::Modified, order);

	//OnOrderReceived(nRqID, order);

	//OnSubAccountOrder(VtOrderEvent::Modified, strSubAcntNo, strFundName, order, prevState);

	LOG_F(INFO, _T("정정주문서버확인 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, strOriOrdNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);
}

void VtHdCtrl::OnCancelOrderHd(CString& sTrCode, LONG& nRqID)
{
	try
	{
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
		CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리구분");
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");
		CString strPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가격");
		CString strAmount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문수량");
		CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매매구분");
		CString strPriceType = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "가격구분");
		CString strCustom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "사용자정의필드");
		CString strOriOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "원주문번호");

		CString strMsg;
		strMsg.Format("OnCancelOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
		//strMsg.Format(_T("%s\n"), strCustom);
		//TRACE(strMsg);

		// 주문 가격
		strPrice = strPrice.TrimLeft('0');
		CString strOriOrderPrice;
		// 원주문가격 저장
		strOriOrderPrice = strPrice;

		// 주문 가겨을 정수로 변환
		strPrice.Remove('.');
		// 계좌 번호
		strAcctNo.TrimRight();
		// 주문 번호
		strOrdNo.TrimLeft('0');
		// 원주문 번호
		strOriOrdNo.TrimLeft('0');
		// 심볼 코드
		strSeries.TrimRight();
		// 주문 수량
		strAmount.TrimRight();
		// 원주문번호
		strOriOrdNo.TrimRight();

		VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
		HdOrderRequest* order_req = GetOrderRequestByOrderReqId(nRqID);
		if (order_req) {
			// 새로운 주문번호를 넣어 준다.
			order_req->NewOrderNo = (LPCTSTR)(strOrdNo);
		}
		// 여기서 주문요청 아이디와 주문요청 객체를 매칭해 준다.
		auto it = _ReqIdToRequestMap.find(nRqID);
		if (it != _ReqIdToRequestMap.end()) {
			HdOrderRequest req = it->second;
			_OrderNoToRequestMap[(LPCTSTR)(strOrdNo)] = req;
			_OrderNoToRequestMap[(LPCTSTR)(strOriOrdNo)] = req;
		}

		// 취소된 원주문이 체결된 경우는 별도로 처리해 준다.
		VtOrder* originOrder = orderMgr->FindOrder((LPCTSTR)(strOriOrdNo));
		if (originOrder && originOrder->state == VtOrderState::Filled) {
			// 원주문을 제거해 준다.
			orderMgr->RemoveAcceptedHd(originOrder);
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(originOrder);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = originOrder->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(originOrder);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = originOrder->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}

		VtOrder* order = nullptr;
		order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
		if (!order) {
			order = new VtOrder();
			// 주문 요청 번호
			order->HtsOrderReqID = nRqID;
			// 일반 주문인지 청산 주문인지 넣어 준다.
			order_req != nullptr ? order->RequestType = order_req->RequestType : order->RequestType = -1;
			// 주문 번호
			order->AccountNo = (LPCTSTR)strAcctNo;
			// 심볼 코드
			order->shortCode = (LPCTSTR)strSeries;
			// 주문 번호
			order->orderNo = (LPCTSTR)(strOrdNo);
			// 정수로 변환된 주문 가격
			order->intOrderPrice = _ttoi(strPrice);
			// 주문 수량
			order->amount = _ttoi(strAmount);
			// 원 주문 번호
			order->oriOrderNo = (LPCTSTR)(strOriOrdNo);
			// 소수로 표시된 주문 가격
			order->orderPrice = _ttof(strOriOrderPrice);

			// 주문 포지션 타입 - 매수 / 매도
			if (strPosition.Compare(_T("1")) == 0) {
				order->orderPosition = VtPositionType::Buy;
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				order->orderPosition = VtPositionType::Sell;
			}

			// 주문 가격 타입 - 지정가 / 시장가
			if (strPriceType.Compare(_T("1")) == 0) {
				order->priceType = VtPriceType::Price;
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				order->priceType = VtPriceType::Market;
			}

			// 주문 유형
			order->orderType = VtOrderType::Cancel;

			// 일반계좌 주문
			order->Type = 0;
		}
		else {
			if (order->state == VtOrderState::Filled) {
				LOG_F(INFO, _T("OnCancelOrderHd 주문역전 :: 이미 체결된 주문입니다!"));
				// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
				if (order_req) {
					VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
					VtOrderManager* subOrderMgr = nullptr;
					VtOrder* subAcntOrder = nullptr;

					if (order_req->Type == 1) { // 서브계좌 주문인 경우
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						subOrderMgr->OnOrderFilledHd(subAcntOrder);
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
					else if (order_req->Type == 2) { // 펀드 주문인 경우
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						// 펀드이름을 넣어준다.
						subAcntOrder->FundName = order_req->FundName;
						subOrderMgr->OnOrderFilledHd(subAcntOrder);
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
				}
			}
			else if (order->state == VtOrderState::Accepted) {
				LOG_F(INFO, _T("OnCancelOrderHd 주문역전 :: 이미 거래소 접수된 주문입니다!"));

				// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
				// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
				if (order_req) {
					VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
					VtOrderManager* subOrderMgr = nullptr;
					VtOrder* subAcntOrder = nullptr;

					if (order_req->Type == 1) { // 서브계좌 주문인 경우
												// 주문관리자를 생성해 준다.
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
						// 주문상태를 바꿔준다.
						subAcntOrder->state = VtOrderState::Accepted;
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
					else if (order_req->Type == 2) { // 펀드 주문인 경우
													 // 주문관리자를 생성해 준다.
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						// 펀드이름을 넣어준다.
						subAcntOrder->FundName = order_req->FundName;
						subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
						// 주문상태를 바꿔준다.
						subAcntOrder->state = VtOrderState::Accepted;
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
				}
			}
		}

		// 주문 수신을 처리해 준다.
		orderMgr->OnOrderReceivedHd(order);
		// 주문 상태를 바꿔준다.
		order->state = VtOrderState::OrderReceived;
		SmCallbackManager::GetInstance()->OnOrderEvent(order);

		// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
		// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
		if (order_req) {
			VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
			VtOrderManager* subOrderMgr = nullptr;
			VtOrder* subAcntOrder = nullptr;

			if (order_req->Type == 1) { // 서브계좌 주문인 경우
										// 주문관리자를 생성해 준다.
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 서브주문관리자는 서브주문을 처리한다.
				subOrderMgr->OnOrderReceivedHd(subAcntOrder);
				// 주문상태를 바꿔준다.
				subAcntOrder->state = VtOrderState::OrderReceived;
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
			else if (order_req->Type == 2) { // 펀드 주문인 경우
											 // 주문관리자를 생성해 준다.
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
				// 서브주문관리자는 서브주문을 처리한다.
				subOrderMgr->OnOrderReceivedHd(subAcntOrder);
				// 주문상태를 바꿔준다.
				subAcntOrder->state = VtOrderState::OrderReceived;
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
		}

		//SendOrderMessage(VtOrderEvent::Cancelled, order);

		//OnOrderReceived(nRqID, order);

		//OnSubAccountOrder(VtOrderEvent::Cancelled, strSubAcntNo, strFundName, order, prevState);

		LOG_F(INFO, _T("취소주문서버확인 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, strOriOrdNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

	}
	catch (std::exception& e)
	{
		std::string error = e.what();
	}
}

void VtHdCtrl::OnReceiveRealTimeValue(std::string symCode)
{
	auto it = _DataKeyMap.find(symCode);
	if (it != _DataKeyMap.end()) {
		std::set<std::string>& curSet = it->second;
		for (auto its = curSet.begin(); its != curSet.end(); ++its) {
			;
		}
	}
}

void VtHdCtrl::OnReceiveHoga(int time, VtSymbol* sym)
{
	if (!sym)
		return;

	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	std::string symCode = sym->ShortCode;
	std::string code = symCode + _T("SHTQ:5:1");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotSellQty);
	code = symCode + _T("BHTQ:5:1");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotBuyQty);
	code = symCode + _T("SHTC:5:1");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotSellNo);
	code = symCode + _T("BHTC:5:1");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotBuyNo);

	code = symCode + _T("SHTQ:5:5");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotSellQty);
	code = symCode + _T("BHTQ:5:5");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotBuyQty);
	code = symCode + _T("SHTC:5:5");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotSellNo);
	code = symCode + _T("BHTC:5:5");
	dataCollector->OnReceiveData(code, time, sym->Hoga.TotBuyNo);
}

void VtHdCtrl::OnReceiveSise(int time, VtSymbol* sym)
{
	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	std::string symCode = sym->ShortCode;
	std::string code = symCode + _T(":5:1");
	double close = sym->Quote.intClose / std::pow(10, sym->Decimal);
	dataCollector->OnReceiveData(code, time, close);
	code = symCode + _T(":1:1");
	dataCollector->OnReceiveData(code, time, close);
}

void VtHdCtrl::OnOrderReceived(int reqId, VtOrder* order)
{
	if (!order)
		return;
	// 선물사 주문요청번호를 찾아 현재 받은 주문 번호와 매칭시켜 준다.
	auto it = _ReqIdToRequestMap.find(reqId);
	if (it != _ReqIdToRequestMap.end()) {
		HdOrderRequest req = it->second;
		_OrderNoToRequestMap[order->orderNo] = req;
		// 취소나 정정 주문의 경우 이전 주문 번호에 원요청정보를 넣어준다.
		if (order->orderType == VtOrderType::Change ||
			order->orderType == VtOrderType::Cancel) {
			_OrderNoToRequestMap[order->oriOrderNo] = req;
		}

		//_OrderRequestMap.erase(it);

		CString msg;
		msg.Format(_T("주문타입 = %d, subacnt = %s, fundname = %s \n"), (int)req.orderType, req.SubAccountNo.c_str(), req.FundName.c_str());
		//TRACE(msg);
		LOG_F(INFO, _T("내부주문번호 수신 :: 원요청번호 = %d, 선물사요청번호 = %d, 주문번호 = %d, 원주문번호 = %d"), req.RequestId, reqId, order->orderNo, order->oriOrderNo);
	}
	else {
		LOG_F(INFO, _T("내부주문번호 검색 오류 :: 원요청번호 = %d, 선물사요청번호 = %d, 주문번호 = %d, 원주문번호 = %d"), -1, reqId, order->orderNo, order->oriOrderNo);
	}
}

void VtHdCtrl::OnOrderReceivedHd(VtOrder* order)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnOrderReceivedHd(order);
}



void VtHdCtrl::OnOrderReceivedHd(CString& sTrCode, LONG& nRqID)
{

}

void VtHdCtrl::OnOrderAcceptedHd(VtOrder* order)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnOrderAcceptedHd(order);
}



void VtHdCtrl::OnOrderAcceptedHd(CString& strKey, LONG& nRealType)
{
	CString strAcctNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
	CString strAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");
	CString strCustom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "사용자정의필드");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
	CString strPriceType = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "가격구분");
	CString strMan = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "조작구분");
	CString strOriOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "원주문번호");
	CString strFirstOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "최초원주문번호");
	CString strTraderTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "접수시간");

	CString strMsg;
	strMsg.Format("OnOrderAcceptedHd 계좌번호[%s]주문번호[%s][원주문번호[%s]\n", strAcctNo, strOrdNo, strOriOrderNo);
	//WriteLog(strMsg);
	//strMsg.Format(_T("%s\n"), strCustom);
	//TRACE(strMsg);

	strAcctNo.TrimRight(); // 계좌 번호
	strOrdNo.TrimLeft('0'); // 주문 번호
	strOriOrderNo.TrimLeft('0'); // 원주문 번호
	strFirstOrderNo.TrimLeft('0'); // 최초 원주문 번호
	strSeries.TrimRight(); // 심볼 코드
	strPrice = strPrice.TrimLeft('0'); // 주문 가격 트림
	CString strOriOrderPrice; 
	strOriOrderPrice = strPrice; // 원주문가격 저장
	
	// 주문 가격을 정수로 변환
	int count = strPrice.Remove('.');
	// 주문 가격 트림
	strPrice.TrimRight();
	// 주문 수량 트림
	strAmount.TrimRight();

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderNo((LPCTSTR)(strOrdNo));

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	// 주문이 없는 경우는 외부 주문이나 내부주문중 아직 주문 번호가 도착하지 않은 주문이다.
	if (!order) {
		strMsg.Format("new OnOrderAcceptedHd 계좌번호[%s]주문번호[%s]\n", strAcctNo, strOrdNo);
		//WriteLog(strMsg);
		//strMsg.Format(_T("%s\n"), strCustom);
		//TRACE(strMsg);
  		order = new VtOrder();
		// 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수주문가격 설정
		order->intOrderPrice = GetIntOrderPrice(strSeries, strPrice, strOriOrderPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 소수로 표현된 주문 가격
		order->orderPrice = _ttof(strOriOrderPrice);
		// 최초 원주문번호
		order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
		// 원주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrderNo);
		// 주문 유형 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0) {
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->orderPosition = VtPositionType::Sell;
		}

		// 주문 가격 유형 - 지정가 / 시장가
		if (strPriceType.Compare(_T("1")) == 0) {
			order->priceType = VtPriceType::Price;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->priceType = VtPriceType::Market;
		}
		// 거래 시간
		order->tradeTime = (LPCTSTR)strTraderTime;

		// 주문 유형 - 신규, 정정, 취소
		if (strMan.Compare(_T("1")) == 0) {
			order->orderType = VtOrderType::New;
		}
		else if (strMan.Compare(_T("8")) == 0 ||
			strMan.Compare(_T("9")) == 0) {
			order->orderType = VtOrderType::Change;
		}
		else if (strMan.Compare(_T("2")) == 0) {
			order->orderType = VtOrderType::Cancel;
		}
	}
	else { // 이미 주문이 있는 경우
		// 거래소 접수되었지만 그 원주문이 이미 체결된 경우에는 현재 주문도 접수확인 목록에서 제거해 준다.
		// 이미 체결된 상태이기 때문에 추가적인 처리는 하지 않는다.
		VtOrder* origin_order = orderMgr->FindOrder((LPCTSTR)(strOriOrderNo));
		if (origin_order) {
			if (origin_order->state == VtOrderState::Filled || origin_order->state == VtOrderState::Settled) {
				orderMgr->RemoveAcceptedHd(order);
				SmCallbackManager::GetInstance()->OnOrderEvent(order);
				// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
				if (order_req) {
					VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
					VtOrderManager* subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					VtOrder* subAcntOrder = subOrderMgr->FindOrder(order->orderNo);
					if (subAcntOrder) {
						subOrderMgr->RemoveAcceptedHd(subAcntOrder);
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
				}
				return;
			}
		}
		if (order->state == VtOrderState::Filled) {
			LOG_F(INFO, _T("OnAccepted :: // 주문역전 : 주문의 상태가 체결인 경우는 역전된 경우이다."));
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
			return;
		}
	}

	// 주문 처리
	orderMgr->OnOrderAcceptedHd(order);
	// 주문 상태를 바꿔준다.
	order->state = VtOrderState::Accepted;

	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	LOG_F(INFO, _T("본계좌 거래소 접수 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);


	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	// 주문요청 정보가 있다는 것은 서버에서 주문 정보를 이미 받았다는 것이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
			}
			
			subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::Accepted;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("서브계좌 거래소 접수 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 서브계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, order_req->SubAccountNo.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
			}
			
			subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::Accepted;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("펀드주문 거래소 접수 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 펀드이름 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);
		}
	}


	//order->orderEvent = VtOrderEvent::Accepted;

	//SendOrderMessage(VtOrderEvent::Accepted, order);

	//OnSubAccountOrder(VtOrderEvent::Accepted, strSubAcntNo, strFundName, order, prevState);

	

	//LOG_F(INFO, _T("사용자정의 필드 = %s"), strCustom);

}

void VtHdCtrl::OnOrderUnfilledHd(VtOrder* order)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnOrderUnfilledHd(order);
}



void VtHdCtrl::OnOrderUnfilledHd(CString& strKey, LONG& nRealType)
{
	CString strAcctNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
	CString strPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
	CString strAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");
	CString strCustom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "사용자정의필드");
	CString strMan = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "조작구분");
	CString strCancelCnt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "취소수량");
	CString strModyCnt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "정정수량");
	CString strFilledCnt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결수량");
	CString strRemain = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "잔량");

	CString strOriOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "원주문번호");
	CString strFirstOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "최초원주문번호");
	CString strNewBuyRemain = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "신규매수주문잔량");
	CString strNewBuyTotalRemain = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "신규매수주문총잔량");
	CString strNewSellRemain = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "신규매도주문잔량");
	CString strNewSellTotalRemain = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "신규매도주문총잔량");

	// 주문 가격
	strPrice = strPrice.TrimRight();
	CString strOriOrderPrice;
	// 원 주문 가격 저장
	strOriOrderPrice = strPrice;
	// 주문 가격을 정수로 변환
	int count = strPrice.Remove('.');
	// 계좌 번호 트림
	strAcctNo.TrimRight();
	// 주문 번호 트림
	strOrdNo.TrimLeft('0');
	// 원주문 번호 트림
	strOriOrderNo.TrimLeft('0');
	// 첫주문 번호 트림
	strFirstOrderNo.TrimLeft('0');
	// 심볼 코드 트림
	strSeries.TrimRight();
	// 주문 수량 트림
	strAmount.TrimRight();
	// 정정이나 취소시 처리할 수량 트림
	strRemain.TrimRight();
	// 정정이 이루어진 수량
	strModyCnt.TrimRight();
	// 체결된 수량
	strFilledCnt.TrimRight();
	// 취소된 수량
	strCancelCnt.TrimRight();

	CString strMsg;
	strMsg.Format("OnOrderUnfilledHd 계좌번호[%s]주문번호[%s]\n", strAcctNo, strOrdNo);
	//WriteLog(strMsg);
	//strMsg.Format(_T("%s\n"), strCustom);
	//TRACE(strMsg);

	m_strOrdNo = strOrdNo;

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderNo((LPCTSTR)(strOrdNo));


	// 처리할 갯수
	int remainCnt = _ttoi(strRemain);
	// 취소 된 갯수
	int cancelCnt = _ttoi(strCancelCnt);
	// 정정된 갯수
	int modifyCnt = _ttoi(strModyCnt);
	// 주문한 갯수
	int orderCnt = _ttoi(strAmount);

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	if (!order) {
		order = new VtOrder();
		// 주문 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수주문가격 설정
		order->intOrderPrice = GetIntOrderPrice(strSeries, strPrice, strOriOrderPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 소소 주문 가격
		order->orderPrice = _ttof(strOriOrderPrice);
		
		// 주문 유형 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0)
		{
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0)
		{
			order->orderPosition = VtPositionType::Sell;
		}
		// 신규 주문, 정정 주문 , 취소 주문 시 처리할 주문의 수 - 0이면 모두 처리한 것이다.
		order->unacceptedQty = _ttoi(strRemain);
		// 첫 주문 번호
		order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
		// 주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrderNo);
		// 정정한 주문 갯수
		order->modifiedOrderCount = _ttoi(strModyCnt);

		// 주문 상태를 저장함
		VtOrderState prevState = order->state;
		if (remainCnt == orderCnt) {
			// 이미 체결된 주문은 설정하지 않는다.
			if (order->state != VtOrderState::Filled)
				order->state = VtOrderState::Accepted;
		}
		// 정정 주문 완료 상태 확인
		if (remainCnt == 0 && modifyCnt == orderCnt) {
			// 이미 체결된 주문은 설정하지 않는다.
			if (order->state != VtOrderState::Filled)
				order->state = VtOrderState::ConfirmModify;
		}
		// 취소 주문 완료 상태 확인
		if (remainCnt == 0 && cancelCnt == orderCnt) {
			order->unacceptedQty = 0;
			order->amount = 0;
			// 이미 체결된 주문은 설정하지 않는다.
			if (order->state != VtOrderState::Filled)
				order->state = VtOrderState::ConfirmCancel;
		}
	}

	orderMgr->OnOrderUnfilledHd(order);

	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	// 주문요청 정보가 있다는 것은 서버에서 주문 정보를 이미 받았다는 것이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
			}
			subOrderMgr->OnOrderUnfilledHd(subAcntOrder);

			if (remainCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::Accepted;
			}
			// 정정 주문 완료 상태 확인
			if (remainCnt == 0 && modifyCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmModify;
			}
			// 취소 주문 완료 상태 확인
			if (remainCnt == 0 && cancelCnt == orderCnt) {
				subAcntOrder->unacceptedQty = 0;
				subAcntOrder->amount = 0;
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmCancel;
			}

			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
			}
			subOrderMgr->OnOrderUnfilledHd(subAcntOrder);

			if (remainCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::Accepted;
			}
			// 정정 주문 완료 상태 확인
			if (remainCnt == 0 && modifyCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmModify;
			}
			// 취소 주문 완료 상태 확인
			if (remainCnt == 0 && cancelCnt == orderCnt) {
				subAcntOrder->unacceptedQty = 0;
				subAcntOrder->amount = 0;
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmCancel;
			}

			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
	}

// 	order->orderEvent = VtOrderEvent::Unfilled;
// 
// 	SendOrderMessage(VtOrderEvent::Unfilled, order);
// 
// 	OnSubAccountOrder(VtOrderEvent::Unfilled, strSubAcntNo, strFundName, order, prevState);



	//LOG_F(INFO, _T("미체결 수신 : 주문가격 = %s, 원요청번호 %d, 선물사 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, oriReqNo, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, strAcctNo, strSubAcntNo, strFundName, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

}

void VtHdCtrl::OnOrderFilledHd(VtOrder* order)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnOrderFilledHd(order);
}



void VtHdCtrl::OnOrderFilledHd(CString& strKey, LONG& nRealType)
{
	CString strAcctNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");


	CString strFillPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결가격");
	CString strFillAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결수량");
	CString strFillTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결시간");
	CString strCustom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "사용자정의필드");

	CString strMan = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "조작구분");
	// 체결된 가격
	strFillPrice = strFillPrice.TrimLeft();
	CString strOriFill = strFillPrice;
	// 체결된 가격을 정수로 변환
	int count = strFillPrice.Remove('.');
	// 계좌 번호 트림
	strAcctNo.TrimRight();
	// 주문 번호 트림
	strOrdNo.TrimLeft('0');
	// 심볼 코드
	strSeries.TrimRight();
	// 소수로 표시된 체결 가격
	strFillPrice.TrimRight();
	// 체결 수량
	strFillAmount.TrimLeft();
	// 체결된 시각
	strFillTime.TrimRight();

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderNo((LPCTSTR)(strOrdNo));
	// 이미 체결된 주문이 정정된 경우에 대하여 새로운 주문을 없애 준다.
	if (order_req) {
		// 이미 체결된 본 주문 처리
		orderMgr->RemoveAcceptedHd((LPCTSTR)strSeries, order_req->NewOrderNo);

		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subOrderMgr->RemoveAcceptedHd((LPCTSTR)strSeries, order_req->NewOrderNo);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subOrderMgr->RemoveAcceptedHd((LPCTSTR)strSeries, order_req->NewOrderNo);
		}
	}

	VtOrder* order = nullptr;
	// 주문이 있는지 찾아본다. 목록에 없으면 새로 생성해 준다.
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	// 주문이 주문 목록에 없는 경우는 외부 주문이거나 역전되어 오는 주문이다. 
	if (!order) {
		order = new VtOrder();
		// 주문 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);

		// 주문 타입
		if (strPosition.Compare(_T("1")) == 0) {
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->orderPosition = VtPositionType::Sell;
		}
	}

	// 전략 이름을 넣어 준다.
	if (order_req) {
		order->StrategyName = order_req->StratageName;
	}
	// 정수로 체결가격 설정
	order->intFilledPrice = GetIntOrderPrice(strSeries, strFillPrice, strOriFill);

	// 체결된 수량
	order->filledQty = _ttoi(strFillAmount);
	// 체결된 시각
	order->filledTime = (LPCTSTR)strFillTime;
	// 체결된 가격 - 소수로 표시된
	order->filledPrice = _ttof(strOriFill);
	
	orderMgr->OnOrderFilledHd(order);

	SmCallbackManager::GetInstance()->OnOrderEvent(order);
	LOG_F(INFO, _T("메인 주문 체결 확인 : 체결가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 이전주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 체결갯수 = %s, 요청 타입 = %d"), strFillPrice, order->HtsOrderReqID, strSeries, strOrdNo, order_req != nullptr ? order_req->NewOrderNo.c_str() : "0", strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strFillAmount, order->RequestType);

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it) {
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow) {
			((HdAccountPLDlg*)wnd)->OnOrderFilledHd(order);
		}
		else if (type == HdWindowType::FundMiniJangoWindow) {
			((VtFundMiniJango*)wnd)->OnOrderFilledHd(order);
		}
	}

	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	// 주문요청 정보가 있다는 것은 서버에서 주문 정보를 이미 받았다는 것이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 여기서 시스템 정보를 넣어준다.
				subAcntOrder->StrategyName = order_req->StratageName;
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
			}

			// 정수로 체결가격 설정
			subAcntOrder->intFilledPrice = GetIntOrderPrice(strSeries, strFillPrice, strOriFill);

			// 체결된 수량
			subAcntOrder->filledQty = _ttoi(strFillAmount);
			// 체결된 시각
			subAcntOrder->filledTime = (LPCTSTR)strFillTime;
			// 체결된 가격 - 소수로 표시된
			subAcntOrder->filledPrice = _ttof(strOriFill);

			subOrderMgr->OnOrderFilledHd(subAcntOrder);
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("서브계좌 체결 확인 : 체결가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 이전주문번호 = %s, 서브계좌번호 = %s, 주문종류 = %s, 체결갯수 = %s, 요청 타입 = %d"), strFillPrice, order->HtsOrderReqID, strSeries, strOrdNo, order_req->NewOrderNo.c_str(), order_req->SubAccountNo.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strFillAmount, order->RequestType);

		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 여기서 시스템 정보를 넣어준다.
				subAcntOrder->StrategyName = order_req->StratageName;
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
			}

			// 정수로 체결가격 설정
			subAcntOrder->intFilledPrice = GetIntOrderPrice(strSeries, strFillPrice, strOriFill);

			// 체결된 수량
			subAcntOrder->filledQty = _ttoi(strFillAmount);
			// 체결된 시각
			subAcntOrder->filledTime = (LPCTSTR)strFillTime;
			// 체결된 가격 - 소수로 표시된
			subAcntOrder->filledPrice = _ttof(strOriFill);

			subOrderMgr->OnOrderFilledHd(subAcntOrder);
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("펀드주문 체결 확인 : 체결가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 이전주문번호 = %s, 펀드이름 = %s, 주문종류 = %s, 체결갯수 = %s, 요청 타입 = %d"), strFillPrice, order->HtsOrderReqID, strSeries, strOrdNo, order_req->NewOrderNo.c_str(), order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strFillAmount, order->RequestType);

		}
	}
}

HdOrderRequest* VtHdCtrl::GetOrderRequestByOrderNo(std::string order_no)
{
	auto it = _OrderNoToRequestMap.find(order_no);
	if (it != _OrderNoToRequestMap.end())
		return &it->second;
	else
		return nullptr;
}

HdOrderRequest* VtHdCtrl::GetOrderRequestByOrderReqId(int req_id)
{
	auto it = _ReqIdToRequestMap.find(req_id);
	if (it != _ReqIdToRequestMap.end())
		return &it->second;
	else
		return nullptr;
}

void VtHdCtrl::AddRequest(int reqId, HdTaskType taskType)
{
	HdTaskArg arg;
	arg.Type = taskType;
	RequestMap[reqId] = arg;
}

void VtHdCtrl::AddRequest(int reqId, HdTaskType taskType, std::string acntNo, std::string symCode)
{
	HdTaskArg arg;
	arg.Type = taskType;
	arg.AddArg(_T("AccountNo"), acntNo);
	arg.AddArg(_T("SymbolCode"), symCode);
	RequestMap[reqId] = arg;
}

void VtHdCtrl::AddRequest(int reqId, HdTaskType taskType, std::string acntNo)
{
	HdTaskArg arg;
	arg.Type = taskType;
	arg.AddArg(_T("AccountNo"), acntNo);
	RequestMap[reqId] = std::move(arg);
}

void VtHdCtrl::RemoveRequest(int reqId)
{
	auto it = RequestMap.find(reqId);
	if (it != RequestMap.end())
	{
		RequestMap.erase(it);
	}
}

void VtHdCtrl::RemoveOrderRequest(int reqId)
{
	auto it = _ReqIdToRequestMap.find(reqId);
	if (it != _ReqIdToRequestMap.end())
	{
		_ReqIdToRequestMap.erase(it);
	}
}

void VtHdCtrl::RemoveOrderRequest(VtOrder* order)
{
	if (!order)
		return;
	// 수신받은 주문 요청 번호를 현재 주문번호로 찾아 삭제해 준다.
	auto it = _OrderNoToRequestMap.find(order->orderNo);
	if (it != _OrderNoToRequestMap.end()) {
		_OrderNoToRequestMap.erase(it);
	}

	// 수신받은 주문 요청 번호를 현재 원주문번호로 찾아 삭제해 준다.
	it = _OrderNoToRequestMap.find(order->oriOrderNo);
	if (it != _OrderNoToRequestMap.end()) {
		_OrderNoToRequestMap.erase(it);
	}

	// 이 부분은 최초 주문 요청을 확인하기 위해 지우지 않기로 결정함
	// 주문요청번호를 찾아 현재 받은 주문 번호와 매칭시켜 준다.
// 	auto it = _OrderRequestMap.find(order->HtsOrderReqID);
// 	if (it != _OrderRequestMap.end()) {
// 		_OrderRequestMap.erase(it);
// 	}
}

HdTaskArg VtHdCtrl::FindRequest(int reqId)
{
	HdTaskArg arg;
	arg.Type = HdTaskType::HdNoTask;
	auto it = RequestMap.find(reqId);
	if (it != RequestMap.end())
	{
		arg = it->second;
	}

	return arg;
}

void VtHdCtrl::HandleRealData(std::string symCode, int time, int value)
{
	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	dataCollector->OnReceiveData(symCode, time, value);
}

void VtHdCtrl::RefreshAcceptedOrderByError(int reqId)
{
	// 주문요청에 의한 오류일 경우 찾아서 주문을 업데이트 해준다.
	auto it = _ReqIdToRequestMap.find(reqId);
	if (it != _ReqIdToRequestMap.end()) {
		HdOrderRequest& req = it->second;
		VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
		// 본계좌 주문 요청이었을 때
		if (req.Type == 0 || req.Type == -1) {
			VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager(req.AccountNo);
			orderMgr->RefreshAcceptedOrder(req.OrderNo);
		}
		else { // 서브계좌나 펀드 주문이었을 때
			VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager(req.AccountNo);
			orderMgr->RefreshAcceptedOrder(req.OrderNo);
			orderMgr = orderMgrSeledter->FindAddOrderManager(req.SubAccountNo);
			orderMgr->RefreshAcceptedOrder(req.OrderNo);
		}
	}
}

void VtHdCtrl::GetServerTime()
{
	CString sTrCode = _T("o44011");
	CString sInput = _T("     ");
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
}

void VtHdCtrl::RegisterRealtimeDataKey(std::string symCode, std::string dataKey)
{
	auto it = _DataKeyMap.find(symCode);
	if (it != _DataKeyMap.end()) {
		std::set<string>& curSet = it->second;
		curSet.insert(dataKey);
	}
	else {
		std::set<std::string> newSet;
		newSet.insert(dataKey);
		_DataKeyMap[symCode] = newSet;
	}
}

void VtHdCtrl::DownloadMasterFiles(std::string param)
{
	m_CommAgent.CommReqMakeCod(param.c_str(), 0);
}

int VtHdCtrl::DownloadDomesticMasterFile(std::string file_name)
{
	CString sTrCode = "v90001";
	CString sInput = file_name.c_str();
	CString strNextKey = "";
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	_SymbolFileReqMap[nRqID] = file_name;
	return nRqID;
}

void VtHdCtrl::ExecuteRequest(std::shared_ptr<HdTaskArg> taskArg)
{
	if (!taskArg)
		return;

	switch (taskArg->Type)
	{
	case HdTaskType::HdSymbolFileDownload:
	{
		std::string file_name = taskArg->GetArg(_T("file_name"));
		int nRqID = DownloadDomesticMasterFile(file_name);
		_TaskReqMap[nRqID] = taskArg;
	}
	break;
	case HdTaskType::HdAcceptedHistory:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetAcceptedHistory(accountNo, password);
	}
	break;
	case HdTaskType::HdFilledHistory:
		break;
	case HdTaskType::HdOutstandingHistory:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetOutstandingHistory(accountNo, password);
	}
	break;
	case HdTaskType::HdOutstanding:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetOutstanding(accountNo, password);
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
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetAsset(accountNo, password);
	}
	break;
	case HdTaskType::HdDeposit:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetDeposit(accountNo, password);
	}
	break;
	case HdTaskType::HdDailyProfitLoss:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetDailyProfitLoss(accountNo, password);
	}
	break;
	case HdTaskType::HdFilledHistoryTable:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetFilledHistoryTable(accountNo, password);
	}
	break;
	case HdTaskType::HdAccountProfitLoss:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetAccountProfitLoss(accountNo, password);
	}
	break;
	case HdTaskType::HdSymbolCode:
	{
		std::string symCode = taskArg->GetArg(_T("Category"));
		//TRACE(symCode.c_str());
		//TRACE(_T("\n"));
		GetSymbolCode(CString(symCode.c_str()));
	}
	break;
	case HdTaskType::HdTradableCodeTable:
		break;
	case HdTaskType::HdApiCustomerProfitLoss:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetApiCustomerProfitLoss(accountNo, password);
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
		std::string symCode = taskArg->GetArg(_T("symbol_code"));
		//TRACE(symCode.c_str());
		//TRACE(_T("\n"));
		int nRqID = GetSymbolMaster(CString(symCode.c_str()));
		_TaskReqMap[nRqID] = taskArg;
	}
	break;
	case HdTaskType::HdStockFutureSymbolMaster:
		break;
	case HdTaskType::HdIndustryMaster:
		break;
	case HdTaskType::HdAccountFeeInfoStep1:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetApiCustomerProfitLoss(HdTaskType::HdAccountFeeInfoStep1, accountNo, password);
	}
	break;
	case HdTaskType::HdAccountFeeInfoStep2:
	{
		std::string accountNo = taskArg->GetArg(_T("AccountNo"));
		std::string password = taskArg->GetArg(_T("Password"));
		GetFilledHistoryTable(HdTaskType::HdAccountFeeInfoStep2, accountNo, password);
	}
	break;
	default:
		break;
	}
}

int VtHdCtrl::GetAbroadQuote(std::string symbol_code)
{
	CString sInput;
	std::string input = symbol_code;
	std::string temp = PadRight(input, ' ', 32);
	sInput = temp.c_str();
	CString sReqFidInput = "000001002003004005006007008009010011012013014015016017018019020021022023024025026027028029030031032033034035036037";
	CString strNextKey = m_CommAgent.CommGetNextKey(_RqID, "");
	_RqID = m_CommAgent.CommFIDRqData(DefAbQuote, sInput, sReqFidInput, sInput.GetLength(), strNextKey);

	return _RqID;
}

int VtHdCtrl::GetAbroadHoga(std::string symbol_code)
{
	CString sInput;
	std::string input = symbol_code;
	std::string temp = PadRight(input, ' ', 32);
	sInput = temp.c_str();
	CString sReqFidInput = _T("000001002003004005006007008009010011012013014015016017018019020021022023024025026027028029030031032033034035036037038039040041042043044045046047048049050051052053054055056057058059060061");

	CString strNextKey = m_CommAgent.CommGetNextKey(_RqID, "");
	_RqID = m_CommAgent.CommFIDRqData(DefAbHoga, sInput, sReqFidInput, sInput.GetLength(), strNextKey);

	return _RqID;
}


void VtHdCtrl::OnAbQuote(CString& sTrCode, LONG& nRqID)
{
	CString	strData000 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strData000.Trim());
	if (!sym)
		return;


	CString	strData002 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "한글종목명");
	CString strCom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일대비");
	CString strComGubun = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일대비구분");
	CString strUpRate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일대비등락율");

	LOG_F(INFO, _T("종목코드 = %s"), strData000);
	CString msg;
	msg.Format("종목코드 = %s, 한글종목명=%s\n", strData000, strData002);
	TRACE(msg);

	strCom.TrimRight();
	strUpRate.TrimRight();


	sym->FullCode = (LPCTSTR)strData000.TrimRight();
	sym->Name = (LPCTSTR)strData002.TrimRight();

	CString	strData050 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "체결시간");
	CString	strData051 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "현재가");
	CString	strData052 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "시가");
	CString	strData053 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "고가");
	CString	strData054 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "저가");
	CString	strData055 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "누적거래량");


	sym->AccAmount = _ttoi(strData055.TrimRight());
	sym->ComToPrev = _ttoi(strCom);
	sym->UpdownRate = _ttoi(strUpRate);
	sym->Quote.intClose = _ttoi(strData051);
	sym->Quote.intOpen = _ttoi(strData052);
	sym->Quote.intHigh = _ttoi(strData053);
	sym->Quote.intLow = _ttoi(strData054);

	SmCallbackManager::GetInstance()->OnQuoteEvent(sym);

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdSymbolMaster;
	FireTaskCompleted(std::move(eventArg));
	RemoveRequest(nRqID);
}


void VtHdCtrl::OnAbHoga(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	CString	strData000 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strData000.Trim());
	if (!sym)
		return;

	CString	strData002 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "호가시간");


	LOG_F(INFO, _T("종목코드 = %s"), strData000);

	CString	strData075 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "호가수신시간");
	CString	strData076 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가1");
	CString	strData077 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가1");
	CString	strData078 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가잔량1");
	CString	strData079 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가잔량1");
	CString	strData080 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수1");
	CString	strData081 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수1");

	sym->Hoga.Ary[0].IntSellPrice = _ttoi(strData076);
	sym->Hoga.Ary[0].IntBuyPrice = _ttoi(strData077);
	sym->Hoga.Ary[0].SellQty = _ttoi(strData078);
	sym->Hoga.Ary[0].BuyQty = _ttoi(strData079);
	sym->Hoga.Ary[0].SellNo = _ttoi(strData080);
	sym->Hoga.Ary[0].BuyNo = _ttoi(strData081);


	CString	strData082 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가2");
	CString	strData083 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가2");
	CString	strData084 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가잔량2");
	CString	strData085 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가잔량2");
	CString	strData086 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수2");
	CString	strData087 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수2");

	sym->Hoga.Ary[1].IntSellPrice = _ttoi(strData082);
	sym->Hoga.Ary[1].IntBuyPrice = _ttoi(strData083);
	sym->Hoga.Ary[1].SellQty = _ttoi(strData084);
	sym->Hoga.Ary[1].BuyQty = _ttoi(strData085);
	sym->Hoga.Ary[1].SellNo = _ttoi(strData086);
	sym->Hoga.Ary[1].BuyNo = _ttoi(strData087);
	CString	strData088 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가3");
	CString	strData089 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가3");
	CString	strData090 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가잔량3");
	CString	strData091 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가잔량3");
	CString	strData092 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수3");
	CString	strData093 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수3");

	sym->Hoga.Ary[2].IntSellPrice = _ttoi(strData088);
	sym->Hoga.Ary[2].IntBuyPrice = _ttoi(strData089);
	sym->Hoga.Ary[2].SellQty = _ttoi(strData090);
	sym->Hoga.Ary[2].BuyQty = _ttoi(strData091);
	sym->Hoga.Ary[2].SellNo = _ttoi(strData092);
	sym->Hoga.Ary[2].BuyNo = _ttoi(strData093);
	CString	strData094 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가4");
	CString	strData095 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가4");
	CString	strData096 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가잔량4");
	CString	strData097 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가잔량4");
	CString	strData098 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수4");
	CString	strData099 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수4");

	sym->Hoga.Ary[3].IntSellPrice = _ttoi(strData094);
	sym->Hoga.Ary[3].IntBuyPrice = _ttoi(strData095);
	sym->Hoga.Ary[3].SellQty = _ttoi(strData096);
	sym->Hoga.Ary[3].BuyQty = _ttoi(strData097);
	sym->Hoga.Ary[3].SellNo = _ttoi(strData098);
	sym->Hoga.Ary[3].BuyNo = _ttoi(strData099);
	CString	strData100 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가5");
	CString	strData101 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가5");
	CString	strData102 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가잔량5");
	CString	strData103 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가잔량5");
	CString	strData104 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수5");
	CString	strData105 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수5");
	sym->Hoga.Ary[4].IntSellPrice = _ttoi(strData100);
	sym->Hoga.Ary[4].IntBuyPrice = _ttoi(strData101);
	sym->Hoga.Ary[4].SellQty = _ttoi(strData102);
	sym->Hoga.Ary[4].BuyQty = _ttoi(strData103);
	sym->Hoga.Ary[4].SellNo = _ttoi(strData104);
	sym->Hoga.Ary[4].BuyNo = _ttoi(strData105);

	CString	strData106 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가총잔량");
	CString	strData107 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가총잔량");
	CString	strData108 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가총건수");
	CString	strData109 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가총건수");

	sym->Hoga.TotSellQty = _ttoi(strData106);
	sym->Hoga.TotBuyQty = _ttoi(strData107);
	sym->Hoga.TotSellNo = _ttoi(strData108);
	sym->Hoga.TotBuyNo = _ttoi(strData109);

	SmCallbackManager::GetInstance()->OnHogaEvent(sym);
}

// const CString DEF_Ab_Asset = "g11004.AQ0605%"
int VtHdCtrl::OnAbGetAsset(CString& sTrCode, LONG& nRqID)
{
	CString strMsg;
	strMsg.Format("해외 예탁자산 및 증거금 응답[%s]", DEF_Ab_Asset);
	//WriteLog(strMsg);

	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strData1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "통화구분");
		CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁금총액");
		CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁금잔액");
		CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "미결제증거금");
		CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문증거금");
		CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "위탁증거금");
		CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "유지증거금");
		CString strData8 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "청산손익");
		CString strData9 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "선물옵션수수료");
		CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "평가손익");
		CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "평가예탁총액");
		CString strData12 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "추가증거금");
		CString strData13 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가능금액");

	}

	

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdAsset;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);

	return nRqID;
}

int VtHdCtrl::OnAbGetDeposit(CString& sTrCode, LONG& nRqID)
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAccount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
		CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "통화코드");
		CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "예탁금총액");
		CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "예탁금잔액");
		CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문증거금");
		CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "위탁증거금");
		CString strData8 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "유지증거금");
		CString strData9 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래수수료");
		CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");
		CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "청산손익");
		CString strData12 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "추가증거금");
		CString strData13 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문가능금");


		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAccount.TrimRight());
		if (acnt)
		{
			acnt->OpenDeposit = _ttof(strData4.TrimRight());
			acnt->OrdableAmt = _ttof(strData13.TrimRight());
			acnt->CurrencyCode = strData3.Trim();
			acnt->OpenPL = _ttof(strData10.TrimRight());
			acnt->TradePL = _ttof(strData11.TrimRight());
			acnt->Fee = _ttof(strData9.TrimRight());
			acnt->Ord_mgn = _ttof(strData6.TrimRight());
			acnt->Trst_mgn = _ttof(strData7.TrimRight());
			acnt->Mnt_mgn = _ttof(strData8.TrimRight());

		}
	}
	Sleep(VtGlobal::ServerSleepTime);
	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdDeposit;
	FireTaskCompleted(std::move(eventArg));
	RemoveRequest(nRqID);
	return 1;
}

int VtHdCtrl::OnAbGetAccountProfitLoss(CString& sTrCode, LONG& nRqID)
{
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	HdTaskArg arg = FindRequest(nRqID);
	if (arg.Type == HdTaskType::HdApiCustomerProfitLoss)
	{
		double fee = 0.0;
		double tradePL = 0.0;
		double totalPL = 0.0;

		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
		for (int i = 0; i < nRepeatCnt; i++) {
			CString strAccount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
			VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAccount);

			CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
			CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "통화코드");
			CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목");
			CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "청산손익");
			CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "청산순손익");
			CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "청산수수료");
			CString strData8 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");
			CString strData9 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "미결제수수료");
			CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "미결제순손익");
			CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");

			VtSymbol* sym = symMgr->FindHdSymbol((LPCTSTR)strData4.TrimRight());
			VtPosition* posi = acnt->FindAdd(sym->ShortCode);
			posi->TradePL = atof(strData5);

			acnt->TradePL = tradePL;
			acnt->Fee = fee;
			acnt->TotalPL = totalPL;
		}

		Sleep(VtGlobal::ServerSleepTime);
		HdTaskEventArgs eventArg;
		eventArg.TaskType = HdTaskType::HdApiCustomerProfitLoss;
		FireTaskCompleted(std::move(eventArg));
		RemoveRequest(nRqID);
		return 1;
	}

	return 1;
}

int VtHdCtrl::OnAbGetOutStanding(CString& sTrCode, LONG& nRqID)
{
	VtRealtimeRegisterManager* realRegMgr = VtRealtimeRegisterManager::GetInstance();
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strAcntName = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
		CString strSymbol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목");
		CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
		CString strPreOpenQty = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "전일미결제수량");
		CString strOpenQty = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일순 미결제수량");
		CString strAvgPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평균단가");
		CString strUnitPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평균단가(소수점반영)");
		CString strOpenPL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");

		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAcnt.TrimRight());
		if (acnt) {
			VtPosition* posi = acnt->FindAdd((LPCTSTR)strSymbol.TrimRight());
			realRegMgr->RegisterProduct(posi->ShortCode);
			if (strPosition.Compare(_T("1")) == 0) {
				posi->Position = VtPositionType::Buy;
				posi->OpenQty = _ttoi(strOpenQty);
				posi->PrevOpenQty = _ttoi(strPreOpenQty);
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				posi->Position = VtPositionType::Sell;
				posi->OpenQty = -1 * _ttoi(strOpenQty);
				posi->PrevOpenQty = -1 * _ttoi(strPreOpenQty);
			}
			posi->AvgPrice = _ttof(strUnitPrice);
			VtSymbol* sym = symMgr->FindHdSymbol((LPCTSTR)strSymbol.TrimRight());
			if (sym && sym->Quote.intClose > 0) {
				posi->CurPrice = sym->Quote.intClose / std::pow(10, sym->Decimal);
				double curClose = sym->Quote.intClose / std::pow(10, sym->Decimal);
				posi->OpenProfitLoss = posi->OpenQty * (curClose - posi->AvgPrice)*sym->Seungsu;
				acnt->TempOpenPL += posi->OpenProfitLoss;
				if (i == nRepeatCnt - 1) {
					//acnt->OpenPL = acnt->TempOpenPL;
					acnt->TempOpenPL = 0.0;
				}
			}
			else {
				acnt->TempOpenPL += _ttoi(strOpenPL);
				if (i == nRepeatCnt - 1) {
					//acnt->OpenPL = acnt->TempOpenPL;
					acnt->TempOpenPL = 0.0;
				}
			}

			VtTotalOrderManager* totalOrderMgr = VtTotalOrderManager::GetInstance();
			totalOrderMgr->AddPosition(0, acnt->AccountNo, posi->ShortCode, posi);

			VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
			VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager(acnt->AccountNo);
			VtProductOrderManager* prdtOrderMgr = orderMgr->FindAddProductOrderManager((LPCTSTR)strSymbol.TrimRight());
			if (prdtOrderMgr)
				prdtOrderMgr->Init(true);
		}
	}


	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnOutstanding();
	Sleep(VtGlobal::ServerSleepTime);
	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdOutstanding;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);

	return nRqID;
}

int VtHdCtrl::OnAbGetAccepted(CString& sTrCode, LONG& nRqID)
{
	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();

	VtOrder* order = nullptr;

	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
		CString strPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문가격");
		CString strAmount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문수량");
		CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
		CString strPriceType = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "가격조건");
		CString strOriOrderNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "원주문번호");
		CString strFirstOrderNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "최초원주문번호");

		strAcctNo.TrimRight();
		strOrdNo.TrimLeft('0');
		strOriOrderNo.TrimLeft('0');
		strFirstOrderNo.TrimLeft('0');
		strSeries.TrimRight();

		VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);

		order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
		if (!order) {
			order = new VtOrder();

			std::string symCode = (LPCTSTR)strSeries;
			VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
			VtSymbol* sym = symMgr->FindHdSymbol(symCode);

			std::string temp;
			strPrice.TrimLeft('0');
			if (sym)
				temp = NumberFormatter::format(_ttof(strPrice), 20, sym->Decimal);
			else {
				HdTaskEventArgs eventArg;
				eventArg.TaskType = HdTaskType::HdAcceptedHistory;
				FireTaskCompleted(std::move(eventArg));
				return -1;
			}

			strPrice = temp.c_str();
			strPrice.TrimLeft();
			strPrice.Remove('.');

			order->AccountNo = (LPCTSTR)strAcctNo;
			order->shortCode = (LPCTSTR)strSeries;
			order->orderNo = (LPCTSTR)(strOrdNo);
			order->intOrderPrice = _ttoi(strPrice);
			order->amount = _ttoi(strAmount);

			if (strPosition.Compare(_T("1")) == 0) {
				order->orderPosition = VtPositionType::Buy;
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				order->orderPosition = VtPositionType::Sell;
			}

			if (strPriceType.Compare(_T("1")) == 0) {
				order->priceType = VtPriceType::Price;
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				order->priceType = VtPriceType::Market;
			}

			order->orderType = VtOrderType::New;

			order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
			order->oriOrderNo = (LPCTSTR)(strOriOrderNo);

			orderMgr->OnOrderAcceptedHd(order);

			OnOrderAcceptedHd(order);
		}
	}

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdAcceptedHistory;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);

	return nRqID;
}

void VtHdCtrl::OnAcceptedHistory(CString& sTrCode, LONG& nRqID)
{
	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();

	VtOrder* order = nullptr;

	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
		CString strPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문가격");
		CString strAmount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문수량");
		CString strCustom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "사용자정의필드");
		CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
		CString strPriceType = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "가격구분");
		CString strMan = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "조작구분");
		CString strOriOrderNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "원주문번호");
		CString strFirstOrderNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "최초원주문번호");

		strAcctNo.TrimRight();
		strOrdNo.TrimLeft('0');
		strOriOrderNo.TrimLeft('0');
		strFirstOrderNo.TrimLeft('0');
		strSeries.TrimRight();

		VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);

		order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
		if (!order) {
			order = new VtOrder();

			std::string symCode = (LPCTSTR)strSeries;
			VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
			VtSymbol* sym = symMgr->FindHdSymbol(symCode);

			std::string temp;
			strPrice.TrimLeft('0');
			if (sym)
				temp = NumberFormatter::format(_ttof(strPrice), 20, sym->Decimal);
			else
				return;

			strPrice = temp.c_str();
			strPrice.TrimLeft();
			strPrice.Remove('.');

			order->AccountNo = (LPCTSTR)strAcctNo;
			order->shortCode = (LPCTSTR)strSeries;
			order->orderNo = (LPCTSTR)(strOrdNo);
			order->intOrderPrice = _ttoi(strPrice);
			order->amount = _ttoi(strAmount);

			if (strPosition.Compare(_T("1")) == 0) {
				order->orderPosition = VtPositionType::Buy;
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				order->orderPosition = VtPositionType::Sell;
			}

			if (strPriceType.Compare(_T("1")) == 0) {
				order->priceType = VtPriceType::Price;
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				order->priceType = VtPriceType::Market;
			}

			order->orderType = VtOrderType::New;

			order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
			order->oriOrderNo = (LPCTSTR)(strOriOrderNo);

			orderMgr->OnOrderAcceptedHd(order);

			OnOrderAcceptedHd(order);
		}
	}

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdAcceptedHistory;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnFilledHistory(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strData1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문번호");
	}

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnFilledHistoryTable(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	VtAccount* acnt = nullptr;
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strSymbol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
		CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일신규거래수량_매도");
		CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일신규거래수량_매수");
		CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매수평균단가");
		CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매도평균단가");
		CString strTradePL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일차금");
		CString strFee = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "수수료");
		CString strPurePL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "순손익");
		acnt = acntMgr->FindAccount((LPCTSTR)strAcnt.TrimRight());
		if (acnt)
		{
			acnt->TempFee += _ttoi(strFee.TrimRight());
		}
	}

	if (acnt)
	{
		acnt->Fee = acnt->TempFee;
		acnt->TempPurePL = 0;
		acnt->TempTradePL = 0;
		acnt->TempFee = 0.0;
		acnt->SumOpenPL();
	}

	HdTaskArg arg = FindRequest(nRqID);
	if (arg.Type == HdTaskType::HdAccountFeeInfoStep2)
	{
		HdWindowManager* wndMgr = HdWindowManager::GetInstance();
		std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
		for (auto it = wndMap.begin(); it != wndMap.end(); ++it)
		{
			auto item = it->second;
			HdWindowType type = item.first;
			CWnd* wnd = item.second;
			if (type == HdWindowType::MiniJangoWindow)
			{
				((HdAccountPLDlg*)wnd)->OnReceiveAccountInfo();
			}
			else if (type == HdWindowType::AssetWindow)
			{
				((VtAccountAssetDlg*)wnd)->OnReceiveAccountInfo();
			}
		}

		VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
		orderDlgMgr->OnReceiveAccountInfo();
	}

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnAccountProfitLoss(CString& sTrCode, LONG& nRqID)
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
		CString strSymbol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
		CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
		CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "전환매수량");
		CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "현재가");
		CString strTradePL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "실현손익");
		CString strFee = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "수수료");
		CString strOpenPL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");
		CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "순손익");
		CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평균단가");
		CString strData12 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "약정금액");
		CString strData13 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "손익율");

		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAcnt.TrimRight());
		if (acnt)
		{
			VtPosition* posi = acnt->FindAdd((LPCTSTR)strSymbol.TrimRight());

			posi->TradePL = atof(strTradePL.TrimRight());
			posi->Fee = atof(strFee.TrimRight());
			posi->OpenProfitLoss = atof(strOpenPL.TrimRight());
			if (i == nRepeatCnt - 1)
				acnt->SumOpenPL();
		}
	}
	Sleep(VtGlobal::ServerSleepTime);
	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdAccountProfitLoss;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnApiCustomerProfitLoss(CString& sTrCode, LONG& nRqID)
{
	CString strData1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁총액");
	CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁현금");
	CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가능총액");
	CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "위탁증거금_당일");
	CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "위탁증거금_익일");
	CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "평가예탁총액_순자산");
	CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "오버나잇가능금");
	CString strData8 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "당일총손익");
	CString strData9 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "당일매매손익");
	CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "당일평가손익");
	CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "선물위탁수수료");
	CString strData12 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "옵션위탁수수료");
	CString strData13 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "익일예탁총액");
	CString strData14 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "청산후주문가능총액");

	CString strMsg = strData9 + strData10 + strData8 + _T("\n");
	//TRACE(strMsg);

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	HdTaskArg arg = FindRequest(nRqID);
	if (arg.Type == HdTaskType::HdApiCustomerProfitLoss)
	{
		std::string acntNo = arg.GetArg(_T("AccountNo"));
		VtAccount* acnt = acntMgr->FindAccount(acntNo);
		if (acnt) {
			acnt->Deposit = _ttoi(strData1.TrimRight());
			acnt->OpenDeposit = _ttoi(strData6.TrimRight());
			acnt->OrdableAmt = _ttoi(strData3.TrimRight());
			acnt->Trst_mgn = _ttoi(strData4.TrimRight());
			acnt->Mnt_mgn = _ttoi(strData4.TrimRight());
			acnt->Add_mgn = _ttoi(strData4.TrimRight());

			double fee = 0.0;
			double tradePL = 0.0;
			double totalPL = 0.0;
			
			int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
			for (int i = 0; i < nRepeatCnt; i++) {
				CString strCode = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "종목코드");
				CString strPos = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "매매구분");
				CString strRemain = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "잔고수량");
				CString strUnitPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "장부단가");
				CString strCurPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "현재가");
				CString strProfit = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "매매손익");
				tradePL += atof(strProfit);
				CString strFee = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "수수료");
				fee += _ttof(strFee);
				CString strTotalProfit = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "총손익");
				totalPL += _ttof(strTotalProfit);
				CString strMoney = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "장부금액");
				CString strOpenProfit = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "평가금액");
				CString strSettle = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "청산가능수량");
				VtSymbol* sym = symMgr->FindHdSymbol((LPCTSTR)strCode.TrimRight());
				VtPosition* posi = acnt->FindAdd(sym->ShortCode);
				posi->TradePL = atof(strProfit);
			}
			acnt->TradePL = tradePL;
			acnt->Fee = fee;
			acnt->TotalPL = totalPL;
		}
		Sleep(VtGlobal::ServerSleepTime);
		HdTaskEventArgs eventArg;
		eventArg.TaskType = HdTaskType::HdApiCustomerProfitLoss;
		FireTaskCompleted(std::move(eventArg));

		RemoveRequest(nRqID);
	}
	else if (arg.Type == HdTaskType::HdAccountFeeInfoStep1) {
		std::string acntNo = arg.GetArg(_T("AccountNo"));
		VtAccount* acnt = acntMgr->FindAccount(acntNo);
		if (acnt) {
			acnt->Deposit = _ttoi(strData1.TrimRight());
			acnt->OpenDeposit = _ttoi(strData6.TrimRight());
			acnt->OrdableAmt = _ttoi(strData3.TrimRight());
			acnt->Trst_mgn = _ttoi(strData4.TrimRight());
			acnt->Mnt_mgn = _ttoi(strData4.TrimRight());
			acnt->Add_mgn = _ttoi(strData4.TrimRight());

			double fee = 0.0;
			double tradePL = 0.0;
			double totalPL = 0.0;

			int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
			for (int i = 0; i < nRepeatCnt; i++) {
				CString strCode = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "종목코드");
				CString strPos = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "매매구분");
				CString strRemain = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "잔고수량");
				CString strUnitPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "장부단가");
				CString strCurPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "현재가");
				CString strProfit = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "매매손익");
				tradePL += atof(strProfit);
				CString strFee = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "수수료");
				fee += _ttof(strFee);
				CString strTotalProfit = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "총손익");
				totalPL += _ttof(strTotalProfit);
				CString strMoney = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "장부금액");
				CString strOpenProfit = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "평가금액");
				CString strSettle = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "청산가능수량");
				VtSymbol* sym = symMgr->FindHdSymbol((LPCTSTR)strCode.TrimRight());
				VtPosition* posi = acnt->FindAdd(sym->ShortCode);
				posi->TradePL = atof(strProfit);
			}
			acnt->TradePL = tradePL;
			acnt->Fee = fee;
			acnt->TotalPL = totalPL;
			Sleep(VtGlobal::ServerSleepTime);
			HdTaskEventArgs eventArg;
			eventArg.TaskType = HdTaskType::HdAccountFeeInfoStep1;
			eventArg.Acnt = acnt;
			FireTaskCompleted(std::move(eventArg));

			RemoveRequest(nRqID);
		}
	}

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it) {
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow) {
			((HdAccountPLDlg*)wnd)->OnReceiveAccountInfo();
		}
		else if (type == HdWindowType::AssetWindow) {
			((VtAccountAssetDlg*)wnd)->OnReceiveAccountInfo();
		}
	}

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnReceiveAccountInfo();
}

void VtHdCtrl::OnServerTime(CString& sTrCode, LONG& nRqID)
{
	CString strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "서버일자");
	CString strTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "서버시간");
	VtTime curTime = VtGlobal::GetTime(_ttoi(strTime));
	VtDate curDate = VtGlobal::GetDate(_ttoi(strDate));

	SYSTEMTIME st;
	st.wYear = curDate.year;
	st.wMonth = curDate.month;
	st.wDay = curDate.day;
	st.wHour = curTime.hour;
	st.wMinute = curTime.min;
	st.wSecond = curTime.sec;
	st.wMilliseconds = 00;
	::SetSystemTime(&st);
}

void VtHdCtrl::OnOutstandingHistory(CString& sTrCode, LONG& nRqID)
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strAcntName = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
		CString strSymbol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목");
		CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
		CString strPreOpenQty = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "전일미결제수량");
		CString strOpenQty = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일미결제수량");
		CString strAvgPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평균단가");
		CString strUnitPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "장부단가");

		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAcnt.TrimRight());
		if (acnt)
		{
			VtPosition* posi = acnt->FindAdd((LPCTSTR)strSymbol.TrimRight());
			if (strPosition.Compare(_T("1")) == 0)
			{
				posi->Position = VtPositionType::Buy;
				posi->OpenQty = _ttoi(strOpenQty);
				posi->PrevOpenQty = _ttoi(strPreOpenQty);
			}
			else if (strPosition.Compare(_T("2")) == 0)
			{
				posi->Position = VtPositionType::Sell;
				posi->OpenQty = -1 * _ttoi(strOpenQty);
				posi->PrevOpenQty = -1 * _ttoi(strPreOpenQty);
			}
			posi->AvgPrice = _ttof(strUnitPrice);
			VtSymbol* sym = symMgr->FindHdSymbol((LPCTSTR)strSymbol.TrimRight());
			if (sym)
			{
				posi->CurPrice = sym->Quote.intClose / std::pow(10, sym->Decimal);
			}
		}
	}

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdOutstandingHistory;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnOutstanding(CString& sTrCode, LONG& nRqID)
{
	VtRealtimeRegisterManager* realRegMgr = VtRealtimeRegisterManager::GetInstance();
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strAcntName = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
		CString strSymbol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목");
		CString strPosition = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
		CString strPreOpenQty = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "전일미결제수량");
		CString strOpenQty = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일미결제수량");
		CString strAvgPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평균단가");
		CString strUnitPrice = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "장부단가");
		CString strOpenPL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");

		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAcnt.TrimRight());
		if (acnt) {
			VtPosition* posi = acnt->FindAdd((LPCTSTR)strSymbol.TrimRight());
			realRegMgr->RegisterProduct(posi->ShortCode);
			if (strPosition.Compare(_T("1")) == 0) {
				posi->Position = VtPositionType::Buy;
				posi->OpenQty = _ttoi(strOpenQty);
				posi->PrevOpenQty = _ttoi(strPreOpenQty);
			}
			else if (strPosition.Compare(_T("2")) == 0) {
				posi->Position = VtPositionType::Sell;
				posi->OpenQty = -1 * _ttoi(strOpenQty);
				posi->PrevOpenQty = -1 * _ttoi(strPreOpenQty);
			}
			posi->AvgPrice = _ttof(strUnitPrice);
			VtSymbol* sym = symMgr->FindHdSymbol((LPCTSTR)strSymbol.TrimRight());
			if (sym && sym->Quote.intClose > 0) {
				posi->CurPrice = sym->Quote.intClose / std::pow(10, sym->Decimal);
				double curClose = sym->Quote.intClose / std::pow(10, sym->Decimal);
				posi->OpenProfitLoss = posi->OpenQty * (curClose - posi->AvgPrice)*sym->Seungsu;
				acnt->TempOpenPL += posi->OpenProfitLoss;
				if (i == nRepeatCnt - 1) {
					acnt->OpenPL = acnt->TempOpenPL;
					acnt->TempOpenPL = 0.0;
				}
			}
			else {
				acnt->TempOpenPL += _ttoi(strOpenPL);
				if (i == nRepeatCnt - 1) {
					acnt->OpenPL = acnt->TempOpenPL;
					acnt->TempOpenPL = 0.0;
				}
			}

			VtTotalOrderManager* totalOrderMgr = VtTotalOrderManager::GetInstance();
			totalOrderMgr->AddPosition(0, acnt->AccountNo, posi->ShortCode, posi);

			VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
			VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager(acnt->AccountNo);
			VtProductOrderManager* prdtOrderMgr = orderMgr->FindAddProductOrderManager((LPCTSTR)strSymbol.TrimRight());
			if (prdtOrderMgr)
				prdtOrderMgr->Init(true);
		}
	}


	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnOutstanding();
	Sleep(VtGlobal::ServerSleepTime);
	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdOutstanding;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnCmeAcceptedHistory(CString& sTrCode, LONG& nRqID)
{
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnCmeFilledHistory(CString& sTrCode, LONG& nRqID)
{
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnCmeOutstandingHistory(CString& sTrCode, LONG& nRqID)
{
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnCmeOutstanding(CString& sTrCode, LONG& nRqID)
{
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnAsset(CString& sTrCode, LONG& nRqID)
{
	CString strData1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
	CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌명");
	CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁총액");
	CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁현금");
	CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁대용");
	CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "가결제금액");
	CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전환매손익");
	CString strData8 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "위탁증거금");
	CString strData9 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "장중선물옵션수수료");
	CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "보유포지션에대한위탁증거금");
	CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문증거금");
	CString strData12 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가능총액");
	CString strData13 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "선입선출에의한전환매손익");
	CString strData14 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "장중옵션매수도대금");
	CString strData15 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "평가손익");
	CString strData16 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "총손익");
	CString strData17 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "장중선물옵션수수료2");
	CString strData18 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "순손익");
	CString strData19 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "평가예탁총액");
	CString strData20 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "인출가능총액");
	CString strData21 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "인출가능현금");

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdAsset;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnDeposit(CString& sTrCode, LONG& nRqID)
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAccount = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "예탁금액-총액");
		CString strData3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "예탁금액-현금");
		CString strData4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "예탁금액-대용");
		CString strData5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "예탁외화");
		CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일손익");
		CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "위탁수수료");
		CString strData8 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "당일순손익");
		CString strData9 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가예탁총액");
		CString strData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "순자산-총평가액");
		CString strData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문가능금액");

		//double	Open_pos_mgn = 0;	/* 미결제증거금													*/
		//double	Ord_mgn = 0;	/* 주문증거금													*/
		//double	Trst_mgn = 0;	/* 위탁증거금													*/
		//double	Mnt_mgn = 0;	/* 유지증거금													*/
		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAccount.TrimRight());
		if (acnt)
		{
			acnt->OpenDeposit = _ttof(strData9.TrimRight());
			acnt->OrdableAmt = _ttof(strData11.TrimRight());
			
		}
	}
	Sleep(VtGlobal::ServerSleepTime);
	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdDeposit;
	FireTaskCompleted(std::move(eventArg));
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnCmeAsset(CString& sTrCode, LONG& nRqID)
{
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnCmePureAsset(CString& sTrCode, LONG& nRqID)
{
	RemoveRequest(nRqID);
}

void VtHdCtrl::OnDailyProfitLoss(CString& sTrCode, LONG& nRqID)
{
	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	for (int i = 0; i < nRepeatCnt; i++)
	{
		CString strAcnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
		CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌명");
		CString strSymbol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목");
		CString strTotalPL = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "총손익");
		CString strData6 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "수수료");
		CString strData7 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "순손익");
		strAcnt.Remove('-');
		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAcnt.TrimRight());
		if (acnt)
		{
			VtPosition* posi = acnt->FindAdd((LPCTSTR)strSymbol.TrimRight());

			posi->TradePL = atof(strTotalPL.TrimRight());

			acnt->SumOpenPL();
		}
	}

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdDailyProfitLoss;
	FireTaskCompleted(std::move(eventArg));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnSymbolCode(CString& sTrCode, LONG& nRqID)
{
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	if (nRepeatCnt > 0) {
		VtSymbol* sym = nullptr;
		for (int i = 0; i < nRepeatCnt; i++) {
			CString sData = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
			//TRACE(sData);
			//TRACE(_T("\n"));
			//WriteLog(sData);
			
			sym = symMgr->FindAddSymbol((LPCTSTR)sData.Trim());
			sym->ShortCode = sData.Trim();

			auto ym = VtSymbolManager::GetExpireYearMonth(sym->ShortCode);
			sym->ExpireYear = std::get<0>(ym);
			sym->ExpireMonth = std::get<1>(ym);

			std::string symCode = sym->ShortCode;
			std::string subSecCode = symCode.substr(0, 3);
			VtProductCategoryManager* prdtCatMgr = VtProductCategoryManager::GetInstance();
			VtProductSubSection* subSection = prdtCatMgr->FindProductSubSection(subSecCode);
			if (subSection) {
				subSection->AddSymbol(sym);
			}
			
		}
		Sleep(VtGlobal::ServerSleepTime);
		HdTaskEventArgs eventArg;
		eventArg.TaskType = HdTaskType::HdSymbolCode;
		eventArg.RequestId = nRqID;
		FireTaskCompleted(std::move(eventArg));
	}

	RemoveRequest(nRqID);
}



void VtHdCtrl::OnSymbolMaster(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
	CString	strData000 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "단축코드");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strData000.Trim());
	if (!sym)
		return;
	// 	if (sym->RecentMonth())
	// 	{
	// 		VtRealtimeRegisterManager* realTimeRegiMgr = VtRealtimeRegisterManager::GetInstance();
	// 		realTimeRegiMgr->RegisterProduct(sym->ShortCode);
	// 	}

	CString	strData001 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");
	CString	strData002 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "한글종목명");
	CString	strData003 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목한글약명");
	CString	strData004 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "영문종목명");
	CString	strData005 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "축약종목명");
	CString strCom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일대비");
	CString strUpRate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "등락률");

	LOG_F(INFO, _T("종목코드 = %s"), strData001);
	//TRACE(strData001);
	//TRACE(_T("\n"));
	std::string code = sym->ShortCode.substr(0, 3);
	HdProductInfo* prdtInfo = symMgr->FindProductInfo(code);
	if (prdtInfo) {
		sym->Decimal = prdtInfo->decimal;
		sym->Seungsu = prdtInfo->tradeWin;
		sym->intTickSize = prdtInfo->intTickSize;
		sym->TickValue = prdtInfo->tickValue;
		sym->TickSize = _ttof(prdtInfo->tickSize.c_str());
	}
	strCom.TrimRight();
	strUpRate.TrimRight();


	sym->FullCode = (LPCTSTR)strData001.TrimRight();
	sym->Name = (LPCTSTR)strData002.TrimRight();
	sym->ShortName = (LPCTSTR)strData003.TrimRight();
	sym->EngName = (LPCTSTR)strData004.TrimRight();
	sym->BriefName = (LPCTSTR)strData005.TrimRight();




	CString	strData049 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "거래단위");

	CString	strData050 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "체결시간");
	CString	strData051 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "현재가");
	CString	strData052 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "시가");
	CString	strData053 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "고가");
	CString	strData054 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "저가");
	CString strPreClose = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일종가");
	CString strPreHigh = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일고가");
	CString strPreLow = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전일저가");


	sym->ComToPrev = _ttoi(strCom);
	sym->UpdownRate = _ttoi(strUpRate);
	sym->Quote.intClose = _ttoi(strData051);
	sym->Quote.intOpen = _ttoi(strData052);
	sym->Quote.intHigh = _ttoi(strData053);
	sym->Quote.intLow = _ttoi(strData054);
	sym->Quote.intPreClose = _ttoi(strPreClose);

	std::string midCode = sym->ShortCode.substr(0, 3);
	if (midCode.compare(_T("101")) == 0) {
		symMgr->Kospi200Current = sym->Quote.intClose;
	}

	CString	strData075 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "호가수신시간");
	CString	strData076 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가1");
	CString	strData077 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가1");
	CString	strData078 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가수량1");
	CString	strData079 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가수량1");
	CString	strData080 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수1");
	CString	strData081 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수1");

	sym->Hoga.Ary[0].IntSellPrice = _ttoi(strData076);
	sym->Hoga.Ary[0].IntBuyPrice = _ttoi(strData077);
	sym->Hoga.Ary[0].SellQty = _ttoi(strData078);
	sym->Hoga.Ary[0].BuyQty = _ttoi(strData079);
	sym->Hoga.Ary[0].SellNo = _ttoi(strData080);
	sym->Hoga.Ary[0].BuyNo = _ttoi(strData081);


	CString	strData082 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가2");
	CString	strData083 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가2");
	CString	strData084 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가수량2");
	CString	strData085 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가수량2");
	CString	strData086 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수2");
	CString	strData087 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수2");

	sym->Hoga.Ary[1].IntSellPrice = _ttoi(strData082);
	sym->Hoga.Ary[1].IntBuyPrice = _ttoi(strData083);
	sym->Hoga.Ary[1].SellQty = _ttoi(strData084);
	sym->Hoga.Ary[1].BuyQty = _ttoi(strData085);
	sym->Hoga.Ary[1].SellNo = _ttoi(strData086);
	sym->Hoga.Ary[1].BuyNo = _ttoi(strData087);
	CString	strData088 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가3");
	CString	strData089 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가3");
	CString	strData090 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가수량3");
	CString	strData091 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가수량3");
	CString	strData092 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수3");
	CString	strData093 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수3");

	sym->Hoga.Ary[2].IntSellPrice = _ttoi(strData088);
	sym->Hoga.Ary[2].IntBuyPrice = _ttoi(strData089);
	sym->Hoga.Ary[2].SellQty = _ttoi(strData090);
	sym->Hoga.Ary[2].BuyQty = _ttoi(strData091);
	sym->Hoga.Ary[2].SellNo = _ttoi(strData092);
	sym->Hoga.Ary[2].BuyNo = _ttoi(strData093);
	CString	strData094 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가4");
	CString	strData095 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가4");
	CString	strData096 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가수량4");
	CString	strData097 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가수량4");
	CString	strData098 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수4");
	CString	strData099 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수4");

	sym->Hoga.Ary[3].IntSellPrice = _ttoi(strData094);
	sym->Hoga.Ary[3].IntBuyPrice = _ttoi(strData095);
	sym->Hoga.Ary[3].SellQty = _ttoi(strData096);
	sym->Hoga.Ary[3].BuyQty = _ttoi(strData097);
	sym->Hoga.Ary[3].SellNo = _ttoi(strData098);
	sym->Hoga.Ary[3].BuyNo = _ttoi(strData099);
	CString	strData100 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가5");
	CString	strData101 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가5");
	CString	strData102 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가수량5");
	CString	strData103 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가수량5");
	CString	strData104 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가건수5");
	CString	strData105 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가건수5");
	sym->Hoga.Ary[4].IntSellPrice = _ttoi(strData100);
	sym->Hoga.Ary[4].IntBuyPrice = _ttoi(strData101);
	sym->Hoga.Ary[4].SellQty = _ttoi(strData102);
	sym->Hoga.Ary[4].BuyQty = _ttoi(strData103);
	sym->Hoga.Ary[4].SellNo = _ttoi(strData104);
	sym->Hoga.Ary[4].BuyNo = _ttoi(strData105);

	CString	strData106 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가총수량");
	CString	strData107 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가총수량");
	CString	strData108 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도호가총건수");
	CString	strData109 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수호가총건수");

	sym->Hoga.TotSellQty = _ttoi(strData106);
	sym->Hoga.TotBuyQty = _ttoi(strData107);
	sym->Hoga.TotSellNo = _ttoi(strData108);
	sym->Hoga.TotBuyNo = _ttoi(strData109);


	CString	strData133 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "만기년월");
	//TRACE(strData133);
	//TRACE(_T("\n"));

	CString	strData159 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "KOSPI200지수");
	//TRACE(strData159);
	//TRACE(_T("\n"));


	SmCallbackManager::GetInstance()->OnMasterEvent(sym);

	// 심볼 마스터 정보를 받자마자 실시간 등록을 해준다.
	VtRealtimeRegisterManager::GetInstance()->RegisterProduct(sym->ShortCode);

	char firstCode = sym->ShortCode.at(0);

	if (_FutureGrid && firstCode == '1') {
		_FutureGrid->OnSymbolMaster(sym);
	}

	if (_OptionGrid && (firstCode == '2' || firstCode == '3')) {
		_OptionGrid->OnSymbolMaster(sym);
	}

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnSymbolMaster(sym);

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it) {
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow) {
			((HdAccountPLDlg*)wnd)->OnSymbolMaster(sym);
		}
	}

	HdTaskEventArgs eventArg;
	eventArg.TaskType = HdTaskType::HdSymbolMaster;
	FireTaskCompleted(std::move(eventArg));
	//TRACE(_T("MasterCompleted!\n"));

	RemoveRequest(nRqID);
}

void VtHdCtrl::OnFutureHoga(CString& strKey, LONG& nRealType)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;

	CString	strData076 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가1");
	CString	strData077 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가1");
	CString	strData078 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량1");
	CString	strData079 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량1");
	CString	strData080 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수1");
	CString	strData081 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수1");

	sym->Hoga.Ary[0].IntSellPrice = _ttoi(strData076);
	sym->Hoga.Ary[0].IntBuyPrice = _ttoi(strData077);
	sym->Hoga.Ary[0].SellQty = _ttoi(strData078);
	sym->Hoga.Ary[0].BuyQty = _ttoi(strData079);
	sym->Hoga.Ary[0].SellNo = _ttoi(strData080);
	sym->Hoga.Ary[0].BuyNo = _ttoi(strData081);


	CString	strData082 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가2");
	CString	strData083 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가2");
	CString	strData084 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량2");
	CString	strData085 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량2");
	CString	strData086 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수2");
	CString	strData087 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수2");

	sym->Hoga.Ary[1].IntSellPrice = _ttoi(strData082);
	sym->Hoga.Ary[1].IntBuyPrice = _ttoi(strData083);
	sym->Hoga.Ary[1].SellQty = _ttoi(strData084);
	sym->Hoga.Ary[1].BuyQty = _ttoi(strData085);
	sym->Hoga.Ary[1].SellNo = _ttoi(strData086);
	sym->Hoga.Ary[1].BuyNo = _ttoi(strData087);
	CString	strData088 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가3");
	CString	strData089 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가3");
	CString	strData090 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량3");
	CString	strData091 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량3");
	CString	strData092 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수3");
	CString	strData093 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수3");

	sym->Hoga.Ary[2].IntSellPrice = _ttoi(strData088);
	sym->Hoga.Ary[2].IntBuyPrice = _ttoi(strData089);
	sym->Hoga.Ary[2].SellQty = _ttoi(strData090);
	sym->Hoga.Ary[2].BuyQty = _ttoi(strData091);
	sym->Hoga.Ary[2].SellNo = _ttoi(strData092);
	sym->Hoga.Ary[2].BuyNo = _ttoi(strData093);
	CString	strData094 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가4");
	CString	strData095 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가4");
	CString	strData096 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량4");
	CString	strData097 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량4");
	CString	strData098 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수4");
	CString	strData099 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수4");

	sym->Hoga.Ary[3].IntSellPrice = _ttoi(strData094);
	sym->Hoga.Ary[3].IntBuyPrice = _ttoi(strData095);
	sym->Hoga.Ary[3].SellQty = _ttoi(strData096);
	sym->Hoga.Ary[3].BuyQty = _ttoi(strData097);
	sym->Hoga.Ary[3].SellNo = _ttoi(strData098);
	sym->Hoga.Ary[3].BuyNo = _ttoi(strData099);
	CString	strData100 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가5");
	CString	strData101 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가5");
	CString	strData102 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량5");
	CString	strData103 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량5");
	CString	strData104 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수5");
	CString	strData105 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수5");
	sym->Hoga.Ary[4].IntSellPrice = _ttoi(strData100);
	sym->Hoga.Ary[4].IntBuyPrice = _ttoi(strData101);
	sym->Hoga.Ary[4].SellQty = _ttoi(strData102);
	sym->Hoga.Ary[4].BuyQty = _ttoi(strData103);
	sym->Hoga.Ary[4].SellNo = _ttoi(strData104);
	sym->Hoga.Ary[4].BuyNo = _ttoi(strData105);

	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "호가시간");

	CString	strData106 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가총수량");
	CString	strData107 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가총수량");
	CString	strData108 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가총건수");
	CString	strData109 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가총건수");

	sym->Hoga.TotSellQty = _ttoi(strData106);
	sym->Hoga.TotBuyQty = _ttoi(strData107);
	sym->Hoga.TotSellNo = _ttoi(strData108);
	sym->Hoga.TotBuyNo = _ttoi(strData109);


	// 	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	// 	std::string symCode = sym->ShortCode;
	// 	std::string code = symCode + _T("SHTQ:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotSellQty);
	// 	code = symCode + _T("BHTQ:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotBuyQty);
	// 	code = symCode + _T("SHTC:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotSellNo);
	// 	code = symCode + _T("BHTC:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotBuyNo);

	SmCallbackManager::GetInstance()->OnHogaEvent(sym);

	OnReceiveHoga(_ttoi(strTime), sym);

	orderDlgMgr->OnReceiveHoga(sym);
}

void VtHdCtrl::OnOptionHoga(CString& strKey, LONG& nRealType)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;

	CString	strData076 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가1");
	CString	strData077 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가1");
	CString	strData078 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량1");
	CString	strData079 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량1");
	CString	strData080 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수1");
	CString	strData081 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수1");

	sym->Hoga.Ary[0].IntSellPrice = _ttoi(strData076);
	sym->Hoga.Ary[0].IntBuyPrice = _ttoi(strData077);
	sym->Hoga.Ary[0].SellQty = _ttoi(strData078);
	sym->Hoga.Ary[0].BuyQty = _ttoi(strData079);
	sym->Hoga.Ary[0].SellNo = _ttoi(strData080);
	sym->Hoga.Ary[0].BuyNo = _ttoi(strData081);


	CString	strData082 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가2");
	CString	strData083 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가2");
	CString	strData084 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량2");
	CString	strData085 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량2");
	CString	strData086 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수2");
	CString	strData087 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수2");

	sym->Hoga.Ary[1].IntSellPrice = _ttoi(strData082);
	sym->Hoga.Ary[1].IntBuyPrice = _ttoi(strData083);
	sym->Hoga.Ary[1].SellQty = _ttoi(strData084);
	sym->Hoga.Ary[1].BuyQty = _ttoi(strData085);
	sym->Hoga.Ary[1].SellNo = _ttoi(strData086);
	sym->Hoga.Ary[1].BuyNo = _ttoi(strData087);
	CString	strData088 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가3");
	CString	strData089 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가3");
	CString	strData090 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량3");
	CString	strData091 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량3");
	CString	strData092 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수3");
	CString	strData093 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수3");

	sym->Hoga.Ary[2].IntSellPrice = _ttoi(strData088);
	sym->Hoga.Ary[2].IntBuyPrice = _ttoi(strData089);
	sym->Hoga.Ary[2].SellQty = _ttoi(strData090);
	sym->Hoga.Ary[2].BuyQty = _ttoi(strData091);
	sym->Hoga.Ary[2].SellNo = _ttoi(strData092);
	sym->Hoga.Ary[2].BuyNo = _ttoi(strData093);
	CString	strData094 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가4");
	CString	strData095 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가4");
	CString	strData096 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량4");
	CString	strData097 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량4");
	CString	strData098 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수4");
	CString	strData099 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수4");

	sym->Hoga.Ary[3].IntSellPrice = _ttoi(strData094);
	sym->Hoga.Ary[3].IntBuyPrice = _ttoi(strData095);
	sym->Hoga.Ary[3].SellQty = _ttoi(strData096);
	sym->Hoga.Ary[3].BuyQty = _ttoi(strData097);
	sym->Hoga.Ary[3].SellNo = _ttoi(strData098);
	sym->Hoga.Ary[3].BuyNo = _ttoi(strData099);
	CString	strData100 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가5");
	CString	strData101 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가5");
	CString	strData102 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량5");
	CString	strData103 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량5");
	CString	strData104 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수5");
	CString	strData105 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수5");
	sym->Hoga.Ary[4].IntSellPrice = _ttoi(strData100);
	sym->Hoga.Ary[4].IntBuyPrice = _ttoi(strData101);
	sym->Hoga.Ary[4].SellQty = _ttoi(strData102);
	sym->Hoga.Ary[4].BuyQty = _ttoi(strData103);
	sym->Hoga.Ary[4].SellNo = _ttoi(strData104);
	sym->Hoga.Ary[4].BuyNo = _ttoi(strData105);

	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "호가시간");
	CString	strData106 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가총수량");
	CString	strData107 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가총수량");
	CString	strData108 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가총건수");
	CString	strData109 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가총건수");

	sym->Hoga.TotSellQty = _ttoi(strData106);
	sym->Hoga.TotBuyQty = _ttoi(strData107);
	sym->Hoga.TotSellNo = _ttoi(strData108);
	sym->Hoga.TotBuyNo = _ttoi(strData109);

	// 	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	// 	std::string symCode = sym->ShortCode;
	// 	std::string code = symCode + _T("SHTQ:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotSellQty);
	// 	code = symCode + _T("BHTQ:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotBuyQty);
	// 	code = symCode + _T("SHTC:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotSellNo);
	// 	code = symCode + _T("BHTC:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotBuyNo);

	OnReceiveHoga(_ttoi(strTime), sym);
	SmCallbackManager::GetInstance()->OnHogaEvent(sym);

	orderDlgMgr->OnReceiveHoga(sym);
}

void VtHdCtrl::OnProductHoga(CString& strKey, LONG& nRealType)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "kfutcode");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;

	CString	strData076 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerho1");
	CString	strData077 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidho1");
	CString	strData078 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerrem1");
	CString	strData079 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidrem1");
	CString	strData080 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offercnt1");
	CString	strData081 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidcnt1");

	sym->Hoga.Ary[0].IntSellPrice = _ttoi(strData076);
	sym->Hoga.Ary[0].IntBuyPrice = _ttoi(strData077);
	sym->Hoga.Ary[0].SellQty = _ttoi(strData078);
	sym->Hoga.Ary[0].BuyQty = _ttoi(strData079);
	sym->Hoga.Ary[0].SellNo = _ttoi(strData080);
	sym->Hoga.Ary[0].BuyNo = _ttoi(strData081);


	CString	strData082 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerho2");
	CString	strData083 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidho2");
	CString	strData084 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerrem2");
	CString	strData085 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidrem2");
	CString	strData086 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offercnt2");
	CString	strData087 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidcnt2");

	sym->Hoga.Ary[1].IntSellPrice = _ttoi(strData082);
	sym->Hoga.Ary[1].IntBuyPrice = _ttoi(strData083);
	sym->Hoga.Ary[1].SellQty = _ttoi(strData084);
	sym->Hoga.Ary[1].BuyQty = _ttoi(strData085);
	sym->Hoga.Ary[1].SellNo = _ttoi(strData086);
	sym->Hoga.Ary[1].BuyNo = _ttoi(strData087);
	CString	strData088 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerho3");
	CString	strData089 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidho3");
	CString	strData090 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerrem3");
	CString	strData091 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidrem3");
	CString	strData092 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offercnt3");
	CString	strData093 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidcnt3");

	sym->Hoga.Ary[2].IntSellPrice = _ttoi(strData088);
	sym->Hoga.Ary[2].IntBuyPrice = _ttoi(strData089);
	sym->Hoga.Ary[2].SellQty = _ttoi(strData090);
	sym->Hoga.Ary[2].BuyQty = _ttoi(strData091);
	sym->Hoga.Ary[2].SellNo = _ttoi(strData092);
	sym->Hoga.Ary[2].BuyNo = _ttoi(strData093);
	CString	strData094 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerho4");
	CString	strData095 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidho4");
	CString	strData096 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerrem4");
	CString	strData097 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidrem4");
	CString	strData098 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offercnt4");
	CString	strData099 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidcnt4");

	sym->Hoga.Ary[3].IntSellPrice = _ttoi(strData094);
	sym->Hoga.Ary[3].IntBuyPrice = _ttoi(strData095);
	sym->Hoga.Ary[3].SellQty = _ttoi(strData096);
	sym->Hoga.Ary[3].BuyQty = _ttoi(strData097);
	sym->Hoga.Ary[3].SellNo = _ttoi(strData098);
	sym->Hoga.Ary[3].BuyNo = _ttoi(strData099);
	CString	strData100 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerho5");
	CString	strData101 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidho5");
	CString	strData102 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offerrem5");
	CString	strData103 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidrem5");
	CString	strData104 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "offercnt5");
	CString	strData105 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "bidcnt5");
	sym->Hoga.Ary[4].IntSellPrice = _ttoi(strData100);
	sym->Hoga.Ary[4].IntBuyPrice = _ttoi(strData101);
	sym->Hoga.Ary[4].SellQty = _ttoi(strData102);
	sym->Hoga.Ary[4].BuyQty = _ttoi(strData103);
	sym->Hoga.Ary[4].SellNo = _ttoi(strData104);
	sym->Hoga.Ary[4].BuyNo = _ttoi(strData105);

	CString	strData106 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "totofferrem");
	CString	strData107 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "totbidrem");
	CString	strData108 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "totoffercnt");
	CString	strData109 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "totbidcnt");

	sym->Hoga.TotSellQty = _ttoi(strData106);
	sym->Hoga.TotBuyQty = _ttoi(strData107);
	sym->Hoga.TotSellNo = _ttoi(strData108);
	sym->Hoga.TotBuyNo = _ttoi(strData109);

	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "호가시간");

	// 	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	// 	std::string symCode = sym->ShortCode;
	// 	std::string code = symCode + _T("SHTQ:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotSellQty);
	// 	code = symCode + _T("BHTQ:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotBuyQty);
	// 	code = symCode + _T("SHTC:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotSellNo);
	// 	code = symCode + _T("BHTC:5:1");
	// 	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Hoga.TotBuyNo);

	OnReceiveHoga(_ttoi(strTime), sym);
	SmCallbackManager::GetInstance()->OnHogaEvent(sym);
	orderDlgMgr->OnReceiveHoga(sym);
}

// 잔고는 매도한번 매수한번 두번이 온다.
void VtHdCtrl::OnRemain(CString& strKey, LONG& nRealType)
{
	CString strMsg;
	strMsg.Format("리얼번호[%d] 수신!", nRealType);
	//WriteLog(strMsg);

	CString strAccount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strSymbol = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목");
	CString strRemainCount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "당일미결제수량");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");

	CString strData5 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "미체결수량");
	CString strAvg = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "평균단가");
	CString strPreRemainCount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "전일미결제수량");
	CString strRemainBalance = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "당일미결제약정금액");
	CString strUnitPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "장부단가");
	int count = _ttoi(strRemainCount.TrimRight());
	// 매수와 매도가 번갈아 오므로 잔고가 0인 포지션은 처리하지 않는다.
	if (count > 0) {
		VtAccountManager* acntMgr = VtAccountManager::GetInstance();
		VtAccount* acnt = acntMgr->FindAccount((LPCTSTR)strAccount.TrimRight());
		if (acnt) {
			VtPosition* posi = acnt->FindAdd((LPCTSTR)strSymbol.TrimRight());
			VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
			orderDlgMgr->OnRemain(posi);


			LOG_F(INFO, _T("잔고 표시 >> 계좌번호 : %s, 종목코드 = %s, 포지션 = %s, 갯수 = %s"), strAccount, strSymbol, _ttoi(strPosition) == 1 ? _T("매수") : _T("매도"), strRemainCount);
		}
	}
}

void VtHdCtrl::OnRealPosition(CString& strKey, LONG& nRealType)
{
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "업종코드");
	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "투자자코드");
	CString strVolume = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수수량");
	CString strUpdown = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도수량");

	CString strData1 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "업종코드");
	CString strData2 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "투자자코드");
}

void VtHdCtrl::OnRealFutureQuote(CString& strKey, LONG& nRealType)
{
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결시간");
	CString strVolume = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결량");
	CString strUpdown = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결구분");
	CString msg;
	

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;

	CString	strData051 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "현재가");
	CString	strData052 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "시가");
	CString	strData053 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "고가");
	CString	strData054 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "저가");
	CString strCom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "전일대비");
	CString strUpRate = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "등락율");
	sym->ComToPrev = _ttoi(strCom);
	sym->UpdownRate = _ttoi(strUpRate);
	sym->Quote.intClose = _ttoi(strData051);
	sym->Quote.intOpen = _ttoi(strData052);
	sym->Quote.intHigh = _ttoi(strData053);
	sym->Quote.intLow = _ttoi(strData054);

	sym->Quote.close = sym->Quote.intClose / std::pow(10, sym->Decimal);
	sym->Quote.open = sym->Quote.intOpen / std::pow(10, sym->Decimal);
	sym->Quote.high = sym->Quote.intHigh / std::pow(10, sym->Decimal);
	sym->Quote.low = sym->Quote.intLow / std::pow(10, sym->Decimal);

	// 차트 종가를 업데이트 한다.
	sym->UpdateChartValue();

	OnReceiveSise(_ttoi(strTime), sym);

	VtQuoteItem quoteItem;

	if (strUpdown.Compare(_T("+")) == 0)
		quoteItem.MatchKind = 1;
	else
		quoteItem.MatchKind = 2;

	quoteItem.ClosePrice = _ttoi(strData051);

	strTime.Insert(2, ':');
	strTime.Insert(5, ':');

	quoteItem.Time = strTime;


	quoteItem.ContQty = _ttoi(strVolume);


	quoteItem.Decimal = sym->Decimal;

	sym->Quote.QuoteItemQ.push_front(quoteItem);

	SmCallbackManager::GetInstance()->OnQuoteEvent(sym);

	VtOrderManagerSelector* orderMgrSelector = VtOrderManagerSelector::GetInstance();
	for (auto it = orderMgrSelector->_OrderManagerMap.begin(); it != orderMgrSelector->_OrderManagerMap.end(); ++it)
	{
		VtOrderManager* orderMgr = it->second;
		orderMgr->OnReceiveQuoteHd(sym);
	}

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it)
	{
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow)
		{
			((HdAccountPLDlg*)wnd)->OnReceiveQuote(sym);
		}
		else if (type == HdWindowType::FundMiniJangoWindow)
		{
			((VtFundMiniJango*)wnd)->OnReceiveQuote(sym);
		}
		else if (type == HdWindowType::AssetWindow)
		{
			((VtAccountAssetDlg*)wnd)->OnReceiveAccountInfo();
		}
	}

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnReceiveQuote(sym);

	VtChartDataManager* chartDataMgr = VtChartDataManager::GetInstance();
	chartDataMgr->OnReceiveQuoteHd(sym);

	CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
	mainFrm->OnReceiveQuoteHd(sym);

	msg.Format(_T("code = %s, current = %s\n"), strSeries, strData051);
	//TRACE(msg);
}

void VtHdCtrl::OnRealOptionQuote(CString& strKey, LONG& nRealType)
{
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결시간");
	CString strVolume = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결량");
	CString strUpdown = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결구분");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;

	CString	strData051 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "현재가");
	CString	strData052 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "시가");
	CString	strData053 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "고가");
	CString	strData054 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "저가");

	sym->Quote.intClose = _ttoi(strData051);
	sym->Quote.intOpen = _ttoi(strData052);
	sym->Quote.intHigh = _ttoi(strData053);
	sym->Quote.intLow = _ttoi(strData054);

	sym->Quote.close = sym->Quote.intClose / std::pow(10, sym->Decimal);
	sym->Quote.open = sym->Quote.intOpen / std::pow(10, sym->Decimal);
	sym->Quote.high = sym->Quote.intHigh / std::pow(10, sym->Decimal);
	sym->Quote.low = sym->Quote.intLow / std::pow(10, sym->Decimal);

	// 차트 종가를 업데이트 한다.
	sym->UpdateChartValue();

	OnReceiveSise(_ttoi(strTime), sym);

	VtQuoteItem quoteItem;

	if (strUpdown.Compare(_T("+")) == 0)
		quoteItem.MatchKind = 1;
	else
		quoteItem.MatchKind = 2;

	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	std::string code = sym->ShortCode + _T(":5:1");
	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Quote.intClose);

	quoteItem.ClosePrice = _ttoi(strData051);

	strTime.Insert(2, ':');
	strTime.Insert(5, ':');
	quoteItem.Time = strTime;


	quoteItem.ContQty = _ttoi(strVolume);


	quoteItem.Decimal = sym->Decimal;

	sym->Quote.QuoteItemQ.push_front(quoteItem);
	SmCallbackManager::GetInstance()->OnQuoteEvent(sym);

	VtOrderManagerSelector* orderMgrSelector = VtOrderManagerSelector::GetInstance();
	for (auto it = orderMgrSelector->_OrderManagerMap.begin(); it != orderMgrSelector->_OrderManagerMap.end(); ++it)
	{
		VtOrderManager* orderMgr = it->second;
		orderMgr->OnReceiveQuoteHd(sym);
	}

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	auto wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it)
	{
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow)
		{
			((HdAccountPLDlg*)wnd)->OnReceiveQuote(sym);
		}
		else if (type == HdWindowType::FundMiniJangoWindow)
		{
			((VtFundMiniJango*)wnd)->OnReceiveQuote(sym);
		}
		else if (type == HdWindowType::AssetWindow)
		{
			((VtAccountAssetDlg*)wnd)->OnReceiveAccountInfo();
		}
	}

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnReceiveQuote(sym);

	VtChartDataManager* chartDataMgr = VtChartDataManager::GetInstance();
	chartDataMgr->OnReceiveQuoteHd(sym);
}

void VtHdCtrl::OnRealProductQuote(CString& strKey, LONG& nRealType)
{
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결시간");
	CString strVolume = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "cvolume");
	CString strUpdown = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "cgubun");

	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;

	CString	strData051 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "현재가");
	CString	strData052 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "open");
	CString	strData053 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "high");
	CString	strData054 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "low");

	sym->Quote.intClose = _ttoi(strData051);
	sym->Quote.intOpen = _ttoi(strData052);
	sym->Quote.intHigh = _ttoi(strData053);
	sym->Quote.intLow = _ttoi(strData054);

	sym->Quote.close = sym->Quote.intClose / std::pow(10, sym->Decimal);
	sym->Quote.open = sym->Quote.intOpen / std::pow(10, sym->Decimal);
	sym->Quote.high = sym->Quote.intHigh / std::pow(10, sym->Decimal);
	sym->Quote.low = sym->Quote.intLow / std::pow(10, sym->Decimal);

	// 차트 종가를 업데이트 한다.
	sym->UpdateChartValue();

	OnReceiveSise(_ttoi(strTime), sym);

	VtQuoteItem quoteItem;

	if (strUpdown.Compare(_T("+")) == 0)
		quoteItem.MatchKind = 1;
	else
		quoteItem.MatchKind = 2;

	VtChartDataCollector* dataCollector = VtChartDataCollector::GetInstance();
	std::string code = sym->ShortCode + _T(":5:1");
	dataCollector->OnReceiveData(code, _ttoi(strTime), sym->Quote.intClose);

	quoteItem.ClosePrice = _ttoi(strData051);

	strTime.Insert(2, ':');
	strTime.Insert(5, ':');
	quoteItem.Time = strTime;


	quoteItem.ContQty = _ttoi(strVolume);


	quoteItem.Decimal = sym->Decimal;

	sym->Quote.QuoteItemQ.push_front(quoteItem);
	SmCallbackManager::GetInstance()->OnQuoteEvent(sym);

	VtOrderManagerSelector* orderMgrSelector = VtOrderManagerSelector::GetInstance();
	for (auto it = orderMgrSelector->_OrderManagerMap.begin(); it != orderMgrSelector->_OrderManagerMap.end(); ++it)
	{
		VtOrderManager* orderMgr = it->second;
		orderMgr->OnReceiveQuoteHd(sym);
	}

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it)
	{
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow)
		{
			((HdAccountPLDlg*)wnd)->OnReceiveQuote(sym);
		}
		else if (type == HdWindowType::FundMiniJangoWindow)
		{
			((VtFundMiniJango*)wnd)->OnReceiveQuote(sym);
		}
		else if (type == HdWindowType::AssetWindow)
		{
			((VtAccountAssetDlg*)wnd)->OnReceiveAccountInfo();
		}
	}

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->OnReceiveQuote(sym);

	VtChartDataManager* chartDataMgr = VtChartDataManager::GetInstance();
	chartDataMgr->OnReceiveQuoteHd(sym);
}

void VtHdCtrl::OnSubAccountOrder(VtOrderEvent event, CString& strSubAcntNo, CString& strFundName, VtOrder* parentOrder, VtOrderState prevState)
{
	if (!parentOrder)
		return;

	// 여기서 SubAccount 대한 처리를 한다.
	// SubAccount 는 본계좌에 정보가 기록되고 또 따로 다른 주문관리자에서 관리가 된다.
	if (strSubAcntNo.GetLength() > 0) {
		
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = orderMgrSelecter->FindAddOrderManager((LPCTSTR)strSubAcntNo);
		// 부모의 주문 번호로 주문을 찾아 본다. 
		// 없을 경우 복사해 준다. 있을 경우는 정보를 복사해 준다.
		VtOrder* subAcntOrder = subOrderMgr->FindOrder(parentOrder->orderNo);
		if (!subAcntOrder)
			subAcntOrder = subOrderMgr->CloneOrder(parentOrder);
		else
			subOrderMgr->CopyOrder(parentOrder, subAcntOrder);
		// 처리전 상태를 입력해 준다.
		subAcntOrder->state = prevState;
		// 서브계좌 번호를 넣어 준다.
		subAcntOrder->AccountNo = strSubAcntNo;
		// 부모계좌 번호를 넣어준다.
		subAcntOrder->ParentAccountNo = parentOrder->AccountNo;
		// 펀드 이름이 있으면 펀드 이름을 넣어 준다.
		if (strFundName.GetLength() > 0) {
			subAcntOrder->Type = 2;
			subAcntOrder->FundName = strFundName;
		}
		else {
			subAcntOrder->Type = 1;
			subAcntOrder->FundName = _T("");
		}

		switch (event) {
		case VtOrderEvent::PutNew:
		case VtOrderEvent::Modified:
		case VtOrderEvent::Cancelled: {
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 이미 주문이 접수된 경우에는 접수 과정을 수행한다.
			if (subAcntOrder->state == VtOrderState::Accepted) {
				LOG_F(INFO, _T("서브계좌 처리 : 서버 주문 수신 역전 처리 >> 주문상태 = %d, 주문번호 = %d, 원주문번호 = %d, 서브계좌 = %s, 펀드이름 = %s"), (int)prevState, parentOrder->orderNo, parentOrder->oriOrderNo, strSubAcntNo, strFundName);
				subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
			}
			else {
				LOG_F(INFO, _T("서브계좌 처리 : 주문도착 >> 주문상태 = %d, 주문번호 = %d, 원주문번호 = %d, 서브계좌 = %s, 펀드이름 = %s"), (int)prevState, parentOrder->orderNo, parentOrder->oriOrderNo, strSubAcntNo, strFundName);
				// 주문 상태를 넣어 준다.
				subAcntOrder->state = VtOrderState::OrderReceived;
			}
		}
		break;
		case VtOrderEvent::Accepted: {
			LOG_F(INFO, _T("서브계좌 처리 : 주문접수확인 >> 주문상태 = %d, 주문번호 = %d, 원주문번호 = %d, 서브계좌 = %s, 펀드이름 = %s"), (int)prevState, parentOrder->orderNo, parentOrder->oriOrderNo, strSubAcntNo, strFundName);
			subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
			// 주문 상태를 넣어 준다.
			subAcntOrder->state = VtOrderState::Accepted;
		}
		break;
		case VtOrderEvent::Unfilled: {
			LOG_F(INFO, _T("서브계좌 처리 : 주문미체결 >> 주문상태 = %d, 주문번호 = %d, 원주문번호 = %d, 서브계좌 = %s, 펀드이름 = %s"), (int)prevState, parentOrder->orderNo, parentOrder->oriOrderNo, strSubAcntNo, strFundName);
			subOrderMgr->OnOrderUnfilledHd(subAcntOrder);
			// 주문 상태를 넣어 준다.
			subAcntOrder->state = parentOrder->state;
		}
		break;
		case VtOrderEvent::Filled: {
			LOG_F(INFO, _T("서브계좌 처리 : 주문체결 >> 주문상태 = %d, 주문번호 = %d, 원주문번호 = %d, 서브계좌 = %s, 펀드이름 = %s"), (int)prevState, parentOrder->orderNo, parentOrder->oriOrderNo, strSubAcntNo, strFundName);
			subOrderMgr->OnOrderFilledHd(subAcntOrder);
			// 주문 상태를 넣어 준다.
			if (subAcntOrder->state != VtOrderState::Settled)
				subAcntOrder->state = VtOrderState::Filled;
		}
		break;
		default:
			break;
		}

		SendOrderMessage(event, subAcntOrder);
	}
}

void VtHdCtrl::SendOrderMessage(VtOrderEvent orderEvent, VtOrder* order)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->SendOrderMessage(orderEvent, order);
}

void VtHdCtrl::OnExpected(CString& strKey, LONG& nRealType)
{
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
	VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
	if (!sym)
		return;
	CString strExpected = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "예상체결가격");
	sym->Quote.intExpected = _ttoi(strExpected);
	orderDlgMgr->OnExpected(sym);
}

void VtHdCtrl::RegisterCurrent()
{
	CString strKey = "001";
	int     nRealType = 101;
	int nResult = m_CommAgent.CommSetBroad(strKey, nRealType);

	strKey = "101";
	nResult = m_CommAgent.CommSetBroad(strKey, nRealType);

	strKey = "106";
	nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
}

void VtHdCtrl::UnregisterCurrent()
{
	CString strKey = "001";
	int     nRealType = 101;
	int nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);

	strKey = "101";
	nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);
}

int VtHdCtrl::GetAcceptedHistory(std::string accountNo, std::string pwd)
{
	// 해외 계좌일 경우 해외 함수를 호출 한다.
	if (accountNo.length() == 6) {
		return AbGetAccepted(accountNo, pwd);
	}
	Sleep(VtGlobal::ServerSleepTime);

	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0104&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAcceptedHistory);

	return nRqID;
}

void VtHdCtrl::GetFilledHistory(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0107&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdFilledHistory);
}

int VtHdCtrl::GetOutstandingHistory(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0110&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdOutstandingHistory);

	return nRqID;
}

int VtHdCtrl::GetOutstanding(std::string accountNo, std::string pwd)
{
	if (accountNo.length() == 6) {
		return AbGetOutStanding(accountNo, pwd);
	}

	Sleep(VtGlobal::ServerSleepTime);
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ1305&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdOutstanding);

	return nRqID;
}

void VtHdCtrl::GetCmeAcceptedHistory(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0116&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdCmeAcceptedHistory);
}

void VtHdCtrl::GetCmeFilledHistory(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0119&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdCmeFilledHistory);
}

void VtHdCtrl::GetCmeOutstandingHistory(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0122&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdCmeOutstandingHistory);
}

void VtHdCtrl::GetCmeOutstanding(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ1306&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdCmeOutstanding);
}

int VtHdCtrl::GetAsset(std::string accountNo, std::string pwd)
{
	if (accountNo.length() == 6) {
		return AbGetAsset(accountNo, pwd);
	}

	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0217&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAsset);

	return nRqID;
}

int VtHdCtrl::GetDeposit(std::string accountNo, std::string pwd)
{
	if (accountNo.length() == 6) {
		return AbGetDeposit(accountNo, pwd);
	}

	Sleep(VtGlobal::ServerSleepTime);
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0242&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdDeposit);

	return nRqID;
}

void VtHdCtrl::GetCmeAsset(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0215&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdCmeAsset);
}

void VtHdCtrl::GetCmePureAsset(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ1303&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdCmePureAsset);
}

int VtHdCtrl::GetDailyProfitLoss(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0502&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdDailyProfitLoss);

	return nRqID;
}

int VtHdCtrl::GetFilledHistoryTable(std::string accountNo, std::string pwd)
{
	std::string reqString;

	LocalDateTime now;
	std::string curDate(DateTimeFormatter::format(now, "%Y%m%d"));

	reqString.append(curDate);

	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0509&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdFilledHistoryTable);

	return nRqID;
}

int VtHdCtrl::GetFilledHistoryTable(HdTaskType taskType, std::string accountNo, std::string pwd)
{
	std::string reqString;

	LocalDateTime now;
	std::string curDate(DateTimeFormatter::format(now, "%Y%m%d"));

	reqString.append(curDate);

	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0509&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, taskType);

	return nRqID;
}

void VtHdCtrl::AbGetChartData(VtChartDataRequest&& chartReqeust)
{
	std::string temp;
	std::string reqString;

	temp = PadRight(chartReqeust.productCode, ' ', 32);
	reqString.append(temp);

	LocalDateTime now;
	std::string curDate(DateTimeFormatter::format(now, "%Y%m%d"));
	//reqString.append(curDate);
	//reqString.append(curDate);
	reqString.append(_T("99999999"));
	reqString.append(_T("99999999"));
	reqString.append(_T("9999999999"));

	if (chartReqeust.next == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	if (chartReqeust.chartType == VtChartType::TICK)
		reqString.append("1");
	else if (chartReqeust.chartType == VtChartType::MIN)
		reqString.append("2");
	else if (chartReqeust.chartType == VtChartType::DAY)
		reqString.append("3");
	else
		reqString.append("2");

	temp = PadLeft(chartReqeust.cycle, '0', 2);
	reqString.append(temp);

	temp = PadLeft(chartReqeust.count, '0', 5);
	reqString.append(temp);


	CString sTrCode = "o51200";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	//int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), "");

	CString sReqFidInput = "000001002003004005006007008009010011012013014015";
	//CString strNextKey = m_CommAgent.CommGetNextKey(nRqID, "");
	int nRqID = m_CommAgent.CommFIDRqData(sTrCode, sInput, sReqFidInput, sInput.GetLength(), strNextKey);

	AddRequest(nRqID, HdTaskType::HdChartData);
	_ChartDataRequestMap[nRqID] = chartReqeust.dataKey;
}

void VtHdCtrl::RegisterExpected(CString symCode)
{
	int nRealType = 310;
	int nResult = 0;
	CString strKey = symCode;
	nResult = m_CommAgent.CommSetBroad(strKey, nRealType);
}

void VtHdCtrl::UnregisterExpected(CString symCode)
{
	int nRealType = 310;
	int nResult = 0;
	CString strKey = symCode;
	nResult = m_CommAgent.CommRemoveBroad(strKey, nRealType);
}

int VtHdCtrl::GetAccountProfitLoss(std::string accountNo, std::string pwd)
{
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	reqString.append(_T("001"));
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ0521&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAccountProfitLoss);

	return nRqID;
}

int VtHdCtrl::GetApiCustomerProfitLoss(std::string accountNo, std::string pwd)
{
	if (accountNo.length() == 6) {
		return AbGetAccountProfitLoss(accountNo, pwd);
	}

	Sleep(VtGlobal::ServerSleepTime);
	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ1302&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdApiCustomerProfitLoss, accountNo);
	_AccountPLReqMap[nRqID] = accountNo;

	CString msg;
	msg.Format(_T("acnt = %s, rqId = %d \n"), accountNo.c_str(), nRqID);
	//TRACE(msg);

	return nRqID;
}

int VtHdCtrl::GetApiCustomerProfitLoss(HdTaskType taskType, std::string accountNo, std::string pwd)
{
	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	temp = PadRight(accountNo, ' ', 11);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);


	CString sTrCode = "g11002.DQ1302&";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, taskType, accountNo);

	CString msg;
	msg.Format(_T("acnt = %s, rqId = %d \n"), accountNo.c_str(), nRqID);
	//TRACE(msg);

	return nRqID;
}

void VtHdCtrl::OnRcvdDomesticChartData(CString& sTrCode, LONG& nRqID)
{
	try {
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
	
		CString msg;

		auto it = _ChartDataReqMap.find(nRqID);
		if (it == _ChartDataReqMap.end())
			return;
		SmChartDataRequest req = it->second;
		SmChartDataManager * chartDataMgr = SmChartDataManager::GetInstance();
		SmChartData * chart_data = chartDataMgr->AddChartData(req);
		int total_count = nRepeatCnt;
		int current_count = 1;
				
		for (int i = nRepeatCnt - 1; i >= 0; --i) {
			CString strDate = "";
			CString strTime = "";

			CString tempDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "날짜시간");

			if (chart_data->ChartType() == SmChartType::MIN)
				tempDate.Append(_T("00"));
			else
				tempDate.Append(_T("000000"));


			strTime = tempDate.Right(6);
			strDate = tempDate.Left(8);

			CString strOpen = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "시가");
			CString strHigh = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "고가");
			CString strLow = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "저가");
			CString strClose = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "종가");
			CString strVol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "거래량");

			if (strDate.GetLength() == 0)
				continue;

			//msg.Format(_T("OnRcvdAbroadChartData ::code = %s, index = %d, date = %s, t = %s, o = %s, h = %s, l = %s, c = %s, v = %s\n"), req.symbolCode.c_str(), i, strDate, strTime, strOpen, strHigh, strLow, strClose, strVol);
			//TRACE(msg);

			SmChartDataItem data;
			data.symbolCode = req.symbolCode;
			data.chartType = req.chartType;
			data.cycle = req.cycle;
			data.date = strDate.Trim();
			data.time = strTime.Trim();
			data.date_time = data.date + data.time;
			data.h = _ttoi(strHigh);
			data.l = _ttoi(strLow);
			data.o = _ttoi(strOpen);
			data.c = _ttoi(strClose);
			data.v = _ttoi(strVol);

			// 차트데이터 아이템을 차트데이터에 더한다.
			chart_data->AddData(data);
		}

		LOG_F(INFO, "OnRcvdDomesticChartData %s", req.GetDataKey().c_str());

		// 아직 처리되지 못한 데이터는 큐를 통해서 처리한다.
		RequestChartDataFromQ();

		// 차트 데이터 수신 요청 목록에서 제거한다.
		_ChartDataReqMap.erase(it);

		if (nRepeatCnt == chart_data->CycleDataSize()) {
			// 사이클 차트 데이터 수신 완료를 알릴다.
			SmCallbackManager::GetInstance()->OnChartEvent(chart_data);
		}
		else {
			// 최초 차트 데이터가 완료되었음을 알린다.
			SmCallbackManager::GetInstance()->OnChartEvent(chart_data);
			chart_data->Received(true);
			// 주기 데이터를 등록해 준다.
			SmChartDataManager* chartDataMgr = SmChartDataManager::GetInstance();
			chartDataMgr->RegisterTimer(chart_data);
		}
	}
	catch (std::exception e)
	{
		std::string error = e.what();
		LOG_F(INFO, "%s", error);
	}
}

void VtHdCtrl::OnRcvdAbroadChartData(CString& sTrCode, LONG& nRqID)
{
	try {
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
		//influxdb_cpp::server_info si("127.0.0.1", 8086, "abroad_future", "angelpie", "orion1");
		//influxdb_cpp::server_info si("127.0.0.1", 8086, "test_x", "test", "test");
		CString msg;

		auto it = _ChartDataReqMap.find(nRqID);
		if (it == _ChartDataReqMap.end())
			return;
		SmChartDataRequest req = it->second;
		SmChartDataManager* chartDataMgr = SmChartDataManager::GetInstance();
		SmChartData* chart_data = chartDataMgr->AddChartData(req);
		int total_count = nRepeatCnt;
		int current_count = 1;
		// Received the chart data first.
		for (int i = nRepeatCnt - 1; i >= 0; --i) {
			CString strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "국내일자");
			CString strTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "국내시간");
			CString strOpen = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "시가");
			CString strHigh = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "고가");
			CString strLow = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "저가");
			CString strClose = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종가");
			CString strVol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "체결량");

			if (strDate.GetLength() == 0)
				continue;

			//msg.Format(_T("OnRcvdAbroadChartData ::code = %s, index = %d, date = %s, t = %s, o = %s, h = %s, l = %s, c = %s, v = %s\n"), req.symbolCode.c_str(), i, strDate, strTime, strOpen, strHigh, strLow, strClose, strVol);
			//TRACE(msg);

			SmChartDataItem data;
			data.symbolCode = req.symbolCode;
			data.chartType = req.chartType;
			data.cycle = req.cycle;
			data.date = strDate.Trim();
			data.time = strTime.Trim();
			data.h = _ttoi(strHigh);
			data.l = _ttoi(strLow);
			data.o = _ttoi(strOpen);
			data.c = _ttoi(strClose);
			data.v = _ttoi(strVol);

			chart_data->AddData(data);
		}


		LOG_F(INFO, "OnRcvdAbroadChartData %s", req.GetDataKey().c_str());

		// 아직 처리되지 못한 데이터는 큐를 통해서 처리한다.
		RequestChartDataFromQ();

		// 차트 데이터 수신 요청 목록에서 제거한다.
		_ChartDataReqMap.erase(it);

		if (nRepeatCnt == chart_data->CycleDataSize()) {
			// 사이클 차트 데이터 수신 완료를 알릴다.
			SmCallbackManager::GetInstance()->OnChartEvent(chart_data);
		}
		else {
			// 최초 차트 데이터가 완료되었음을 알린다.
			SmCallbackManager::GetInstance()->OnChartEvent(chart_data);
			chart_data->Received(true);
			// 주기 데이터를 등록해 준다.
			SmChartDataManager* chartDataMgr = SmChartDataManager::GetInstance();
			chartDataMgr->RegisterTimer(chart_data);
		}
	}
	catch (std::exception e)
	{
		std::string error = e.what();
		LOG_F(INFO, "%s", error);
	}
}

void VtHdCtrl::OnRcvdAbroadChartData2(CString& sTrCode, LONG& nRqID)
{
	try {
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");
		//influxdb_cpp::server_info si("127.0.0.1", 8086, "abroad_future", "angelpie", "orion1");
		//influxdb_cpp::server_info si("127.0.0.1", 8086, "test_x", "test", "test");
		CString msg;

		auto it = _ChartDataReqMap.find(nRqID);
		if (it == _ChartDataReqMap.end())
			return;
		SmChartDataRequest req = it->second;
		SmChartDataManager* chartDataMgr = SmChartDataManager::GetInstance();
		SmChartData* chart_data = chartDataMgr->AddChartData(req);
		int total_count = nRepeatCnt;
		int current_count = 1;
		// Received the chart data first.
		for (int i = 0; i < nRepeatCnt; ++i) {
			CString strDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "국내일자");
			CString strTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "국내시간");
			CString strOpen = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "시가");
			CString strHigh = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "고가");
			CString strLow = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "저가");
			CString strClose = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "종가");
			CString strVol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "체결량");
			CString strTotalVol = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "누적거래량");


			if (strDate.GetLength() == 0)
				continue;

			msg.Format(_T("OnRcvdAbroadChartData :: index = %d, date = %s, t = %s, o = %s, h = %s, l = %s, c = %s, v = %s\n"), i, strDate, strTime, strOpen, strHigh, strLow, strClose, strVol);
			TRACE(msg);


			SmChartDataItem data;
			data.symbolCode = req.symbolCode;
			data.chartType = req.chartType;
			data.cycle = req.cycle;
			data.date = strDate.Trim();
			data.time = strTime.Trim();
			data.h = _ttoi(strHigh);
			data.l = _ttoi(strLow);
			data.o = _ttoi(strOpen);
			data.c = _ttoi(strClose);
			data.v = _ttoi(strVol);

			chart_data->AddData(data);

		}

		LOG_F(INFO, "OnRcvdAbroadChartData2 %s", req.GetDataKey().c_str());

		RequestChartDataFromQ();
		// 차트 데이터 수신 요청 목록에서 제거한다.
		_ChartDataReqMap.erase(it);

		if (nRepeatCnt == chart_data->CycleDataSize()) {
			// 사이클 차트 데이터 수신 완료를 알릴다.
			SmCallbackManager::GetInstance()->OnChartEvent(chart_data);
		}
		else {
			// 최초 차트 데이터가 완료되었음을 알린다.
			SmCallbackManager::GetInstance()->OnChartEvent(chart_data);
			chart_data->Received(true);
			// 주기 데이터를 등록해 준다.
			SmChartDataManager* chartDataMgr = SmChartDataManager::GetInstance();
			chartDataMgr->RegisterTimer(chart_data);
		}
	}
	catch (std::exception e)
	{
		std::string error = e.what();
		LOG_F(INFO, "%s", error);
	}
}

void VtHdCtrl::GetChartData()
{
	//AddRequest(nRqID, HdTaskType::HdChartData);
}

void VtHdCtrl::GetChartData(VtChartDataRequest&& chartReqeust)
{
	std::string temp;
	std::string reqString;

	temp = PadRight(chartReqeust.productCode, ' ', 15);
	reqString.append(temp);

	LocalDateTime now;
	std::string curDate(DateTimeFormatter::format(now, "%Y%m%d"));
	reqString.append(curDate);

	reqString.append(_T("999999"));

	temp = PadLeft(chartReqeust.count, '0', 4);
	reqString.append(temp);

	temp = PadLeft(chartReqeust.cycle, '0', 3);
	reqString.append(temp);

	if (chartReqeust.chartType == VtChartType::TICK)
		reqString.append("0");
	else if (chartReqeust.chartType == VtChartType::MIN)
		reqString.append("1");
	else if (chartReqeust.chartType == VtChartType::DAY)
		reqString.append("2");
	else if (chartReqeust.chartType == VtChartType::WEEK)
		reqString.append("3");
	else if (chartReqeust.chartType == VtChartType::MONTH)
		reqString.append("4");
	else
		reqString.append("1");

	if (chartReqeust.next == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	temp = PadRight(chartReqeust.reqKey, ' ', 21);
	reqString.append(temp);

	reqString.append(_T("0"));
	reqString.append(_T("0"));
	reqString.append(_T("00"));
	reqString.append(_T("000000"));
	reqString.append(_T(" "));

	if (chartReqeust.seq == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	CString sTrCode = "v90003";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), "");
	AddRequest(nRqID, HdTaskType::HdChartData);
	_ChartDataRequestMap[nRqID] = chartReqeust.dataKey;
}

void VtHdCtrl::GetChartData(SmChartDataRequest req)
{
	LOG_F(INFO, "GetChartData symbol Code: %s", req.symbolCode.c_str());
	LOG_F(INFO, "GetChartData : %s", req.GetDataKey().c_str());
	if (req.GetDataKey().length() < 8)
		return;
	if (isdigit(req.symbolCode.at(2))) {
		if (req.symbolCode.length() < 8)
			return;
		std::string prefix = req.symbolCode.substr(0, 3);
		GetChartDataForDomestic(req);
	}
	else {

		if (req.symbolCode.length() < 4)
			return;
		
		if (req.chartType == SmChartType::TICK)
			GetChartDataLongCycle(req);
		else if (req.chartType == SmChartType::MIN)
			GetChartDataShortCycle(req);
		else if (req.chartType == SmChartType::DAY)
			GetChartDataLongCycle(req);
		else if (req.chartType == SmChartType::WEEK)
			GetChartDataLongCycle(req);
		else if (req.chartType == SmChartType::MONTH)
			GetChartDataLongCycle(req);
		else
			GetChartDataShortCycle(req);
	}
}

void VtHdCtrl::GetChartDataForDomestic(SmChartDataRequest req)
{
	std::string temp;
	std::string reqString;

	temp = VtStringUtil::PadRight(req.symbolCode, ' ', 15);
	reqString.append(temp);

	std::string str = VtStringUtil::getCurentDate();
	reqString.append(str);

	reqString.append(_T("999999"));

	temp = VtStringUtil::PadLeft(req.count, '0', 4);
	reqString.append(temp);

	temp = VtStringUtil::PadLeft(req.cycle, '0', 3);
	reqString.append(temp);

	if (req.chartType == SmChartType::TICK)
		reqString.append("0");
	else if (req.chartType == SmChartType::MIN)
		reqString.append("1");
	else if (req.chartType == SmChartType::DAY)
		reqString.append("2");
	else if (req.chartType == SmChartType::WEEK)
		reqString.append("3");
	else if (req.chartType == SmChartType::MONTH)
		reqString.append("4");
	else
		reqString.append("1");

	if (req.next == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	temp = VtStringUtil::PadRight(req.reqKey, ' ', 21);
	reqString.append(temp);

	reqString.append(_T("0"));
	reqString.append(_T("0"));
	reqString.append(_T("00"));
	reqString.append(_T("000000"));
	reqString.append(_T(" "));

	if (req.seq == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	CString sTrCode = "v90003";
	CString sInput = reqString.c_str();
	LOG_F(INFO, "GetChartDataDomestic %s", reqString.c_str());
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), "");
	_ChartDataReqMap[nRqID] = req;
}

void VtHdCtrl::GetChartDataShortCycle(SmChartDataRequest req)
{
	std::string temp;
	std::string reqString;
	// 종목 코드 32 자리
	temp = VtStringUtil::PadRight(req.symbolCode, ' ', 32);
	reqString.append(temp);

	std::string str = VtStringUtil::getCurentDate();
	CString msg;
	//msg.Format("%s \n", str.c_str());
	//TRACE(msg);
	reqString.append(str);
	reqString.append(str);
	reqString.append(_T("9999999999"));

	if (req.next == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	if (req.chartType == SmChartType::TICK)
		reqString.append("1");
	else if (req.chartType == SmChartType::MIN)
		reqString.append("2");
	else if (req.chartType == SmChartType::DAY)
		reqString.append("3");
	else if (req.chartType == SmChartType::WEEK)
		reqString.append("4");
	else if (req.chartType == SmChartType::MONTH)
		reqString.append("5");
	else
		reqString.append("2");

	temp = VtStringUtil::PadLeft(req.cycle, '0', 2);
	reqString.append(temp);

	temp = VtStringUtil::PadLeft(req.count, '0', 5);
	reqString.append(temp);

	CString sTrCode = "o51200";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	//int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), "");

	CString sReqFidInput = "000001002003004005006007008009010011012013014015";
	int nRqID = m_CommAgent.CommFIDRqData(sTrCode, sInput, sReqFidInput, sInput.GetLength(), strNextKey);

	//TRACE(sInput);
	_ChartDataReqMap[nRqID] = req;
}

void VtHdCtrl::GetChartDataLongCycle(SmChartDataRequest req)
{
	std::string temp;
	std::string reqString;
	// 최초 요청시 18자리 공백
	reqString.append("                  ");

	temp = VtStringUtil::PadRight(req.symbolCode, ' ', 32);
	reqString.append(temp);

	time_t rawtime;
	struct tm* timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%Y%m%d-%H:%M:%S", timeinfo);
	std::string str(buffer);
	CString msg;
	msg.Format("%s \n", str.c_str());
	//TRACE(msg);
	//reqString.append(curDate);
	//reqString.append(curDate);
	//reqString.append(_T("99999999"));
	reqString.append(_T("99999999"));
	reqString.append(_T("9999999999"));

	if (req.next == 0)
		reqString.append(_T("0"));
	else
		reqString.append(_T("1"));

	if (req.chartType == SmChartType::TICK)
		reqString.append("6");
	else if (req.chartType == SmChartType::MIN)
		reqString.append("2");
	else if (req.chartType == SmChartType::DAY)
		reqString.append("3");
	else if (req.chartType == SmChartType::WEEK)
		reqString.append("4");
	else if (req.chartType == SmChartType::MONTH)
		reqString.append("5");
	else
		reqString.append("2");

	temp = VtStringUtil::PadLeft(req.cycle, '0', 3);
	reqString.append(temp);

	temp = VtStringUtil::PadLeft(req.count, '0', 5);
	reqString.append(temp);

	reqString.append(_T("1"));
	reqString.append(_T("1"));


	CString sTrCode = "o44005";
	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	//int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), "");

	CString sReqFidInput = "000001002003004005006007008009010011012013014015";
	//CString strNextKey = m_CommAgent.CommGetNextKey(nRqID, "");
	//int nRqID = m_CommAgent.CommFIDRqData(sTrCode, sInput, sReqFidInput, sInput.GetLength(), strNextKey);
	int nRqID = m_CommAgent.CommRqData(sTrCode, sInput, sInput.GetLength(), strNextKey);
	//TRACE(sInput);
	_ChartDataReqMap[nRqID] = req;
}

void VtHdCtrl::GetAccountInfo()
{
	if (_Blocked)
		return;

	typedef struct
	{
		char szAcctNo[11]; // 계좌번호
		char szAcctNm[30]; // 계좌명
		char szAcctGb[01]; // 계좌구분 '1': 해외, '2': FX, '9':국내
	}HDF_ACCOUNT_UNIT;

	typedef struct
	{
		char szCount[5];
		HDF_ACCOUNT_UNIT *pHdfAccUnit;
	}HDF_ACCOUNT_INFO;

	HDF_ACCOUNT_INFO *pHdfAccInfo = NULL;
	HDF_ACCOUNT_UNIT *pHdfAccUnit = NULL;
	CString strRcvBuff = m_CommAgent.CommGetAccInfo();


	VtAccountManager* acntMgr = VtAccountManager::GetInstance();

	pHdfAccInfo = (HDF_ACCOUNT_INFO *)strRcvBuff.GetBuffer();
	CString strCount(pHdfAccInfo->szCount, sizeof(pHdfAccInfo->szCount));
	for (int i = 0; i < atoi(strCount); i++)
	{
		pHdfAccUnit = (HDF_ACCOUNT_UNIT *)(pHdfAccInfo->szCount + sizeof(pHdfAccInfo->szCount) + (sizeof(HDF_ACCOUNT_UNIT) * i));
		CString strAcctNo(pHdfAccUnit->szAcctNo, sizeof(pHdfAccUnit->szAcctNo));
		CString strAcctNm(pHdfAccUnit->szAcctNm, sizeof(pHdfAccUnit->szAcctNm));
		CString strAcctGb(pHdfAccUnit->szAcctGb, sizeof(pHdfAccUnit->szAcctGb));// 계좌 구분 추가. - 20140331 sivas

		CString strMsg;
		strMsg.Format("[%s][%s][%s]", strAcctNo, strAcctNm, strAcctGb);

		if (!acntMgr->FindAccount((LPCTSTR)strAcctNo.TrimRight()))
		{
			VtAccount* acnt = new VtAccount();
			acnt->AccountNo = (LPCTSTR)strAcctNo.TrimRight();
			acnt->AccountName = (LPCTSTR)strAcctNm.TrimRight();
			acnt->Type = _ttoi(strAcctGb);

			acntMgr->AddAccount(acnt);
		}
	}
}

void VtHdCtrl::SendOrder(HdOrderRequest request)
{
	if (request.orderType == VtOrderType::New)
		PutOrder(std::move(request));
	else if (request.orderType == VtOrderType::Change)
		ChangeOrder(std::move(request));
	else if (request.orderType == VtOrderType::Cancel)
		CancelOrder(std::move(request));
}

void VtHdCtrl::PutOrder(HdOrderRequest&& request)
{
	std::lock_guard<std::mutex> lock(m_);
	if (!CheckPassword(request))
		return;
	if (request.Market == 1) {
		AbPutOrder(request);
		return;
	}

	std::string orderString;
	std::string temp;
	temp = PadRight(request.AccountNo, ' ', 11);
	orderString.append(temp);
	temp = PadRight(request.Password, ' ', 8);
	orderString.append(temp);
	temp = PadRight(request.SymbolCode, ' ', 32);
	orderString.append(temp);

	if (request.Position == VtPositionType::Buy)
		orderString.append(_T("1"));
	else if (request.Position == VtPositionType::Sell)
		orderString.append(_T("2"));

	if (request.PriceType == VtPriceType::Price)
		orderString.append(_T("1"));
	else if (request.PriceType == VtPriceType::Market)
		orderString.append(_T("2"));

	if (request.FillCondition == VtFilledCondition::Fas)
		orderString.append(_T("1"));
	else if (request.FillCondition == VtFilledCondition::Fok)
		orderString.append(_T("2"));
	else if (request.FillCondition == VtFilledCondition::Fak)
		orderString.append(_T("3"));

	if (request.PriceType == VtPriceType::Price)
		temp = PadRight(request.Price, ' ', 13);
	else if (request.PriceType == VtPriceType::Market)
		temp = PadRight(0, ' ', 13);
	orderString.append(temp);

	temp = PadRight(request.Amount, ' ', 5);
	orderString.append(temp);

	std::string userDefined;
	if (request.Type == 0)
		userDefined.append(_T("00000"));
	else if (request.Type == 1)
		userDefined.append(_T("11111"));
	else if (request.Type == 2)
		userDefined.append(_T("22222"));
	else
		return;
	userDefined.append(",");
	userDefined.append("fund_name");
	userDefined.append(",");
	userDefined.append(request.FundName);
	userDefined.append(",");
	userDefined.append("sub_account_no");
	userDefined.append(",");
	userDefined.append(request.SubAccountNo);
	userDefined.append(",");
	if (request.Type == 0)
		temp = PadRight(userDefined, '0', 60);
	else if (request.Type == 1)
		temp = PadRight(userDefined, '1', 60);
	else if (request.Type == 2)
		temp = PadRight(userDefined, '2', 60);

	orderString.append(temp);

	CString sTrCode = "g12001.DO1601&";
	CString sInput = orderString.c_str();
	int nRqID = m_CommAgent.CommJumunSvr(sTrCode, sInput);
	AddRequest(nRqID, HdTaskType::HdOrderNew);
	request.HtsOrderReqID = nRqID;
	// 주문요청 번호와 함께 주문요청 맵에 넣어준다.
	_ReqIdToRequestMap[nRqID] = request;

	//LOG_F(INFO,_T("PutOrder :: Req id = %d, order string = %s"), nRqID, orderString.c_str());
	LOG_F(INFO, _T("신규주문요청 : 요청번호 = %d, 종목이름 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %d, 요청 타입 = %d"),
		nRqID, request.SymbolCode.c_str(), request.OrderNo.c_str(), request.AccountNo.c_str(), request.SubAccountNo.c_str(),
		request.FundName.c_str(), request.Position == VtPositionType::Buy ? _T("매수") : _T("매도"), request.Amount, request.RequestType);

}

void VtHdCtrl::ChangeOrder(HdOrderRequest&& request)
{
	if (!CheckPassword(request))
		return;

	if (request.Market == 1) {
		AbChangeOrder(request);
		return;
	}

	std::string orderString;
	std::string temp;
	temp = PadRight(request.AccountNo, ' ', 11);
	orderString.append(temp);
	temp = PadRight(request.Password, ' ', 8);
	orderString.append(temp);
	temp = PadRight(request.SymbolCode, ' ', 32);
	orderString.append(temp);

	if (request.Position == VtPositionType::Buy)
		orderString.append(_T("1"));
	else if (request.Position == VtPositionType::Sell)
		orderString.append(_T("2"));

	if (request.PriceType == VtPriceType::Price)
		orderString.append(_T("1"));
	else if (request.PriceType == VtPriceType::Market)
		orderString.append(_T("2"));

	if (request.FillCondition == VtFilledCondition::Fas)
		orderString.append(_T("1"));
	else if (request.FillCondition == VtFilledCondition::Fok)
		orderString.append(_T("2"));
	else if (request.FillCondition == VtFilledCondition::Fak)
		orderString.append(_T("3"));

	if (request.PriceType == VtPriceType::Price)
		temp = PadRight(request.Price, ' ', 13);
	else if (request.PriceType == VtPriceType::Market)
		temp = PadRight(0, ' ', 13);
	orderString.append(temp);

	temp = PadRight(request.Amount, ' ', 5);
	orderString.append(temp);

	temp = PadLeft(request.OrderNo, '0', 7);
	orderString.append(temp);

	std::string userDefined;
	if (request.Type == 0)
		userDefined.append(_T("00000"));
	else if (request.Type == 1)
		userDefined.append(_T("11111"));
	else if (request.Type == 2)
		userDefined.append(_T("22222"));
	else
		return;
	userDefined.append(",");
	userDefined.append("fund_name");
	userDefined.append(",");
	userDefined.append(request.FundName);
	userDefined.append(",");
	userDefined.append("sub_account_no");
	userDefined.append(",");
	userDefined.append(request.SubAccountNo);
	userDefined.append(",");
	if (request.Type == 0)
		temp = PadRight(userDefined, '0', 60);
	else if (request.Type == 1)
		temp = PadRight(userDefined, '1', 60);
	else if (request.Type == 2)
		temp = PadRight(userDefined, '2', 60);
	orderString.append(temp);

	CString sTrCode = "g12001.DO1901&";
	CString sInput = orderString.c_str();
	int nRqID = m_CommAgent.CommJumunSvr(sTrCode, sInput);
	AddRequest(nRqID, HdTaskType::HdOrderChange);
	request.HtsOrderReqID = nRqID;
	// 주문요청 번호와 함께 주문요청 맵에 넣어준다.
	_ReqIdToRequestMap[nRqID] = request;
	LOG_F(INFO, _T("정정주문요청 : 요청번호 = %d, 종목이름 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %d, 요청 타입 = %d "),
		nRqID, request.SymbolCode.c_str(), request.OrderNo.c_str(), request.AccountNo.c_str(), request.SubAccountNo.c_str(),
		request.FundName.c_str(), request.Position == VtPositionType::Buy ? _T("매수") : _T("매도"), request.Amount, request.RequestType);
}

void VtHdCtrl::CancelOrder(HdOrderRequest&& request)
{
	if (!CheckPassword(request))
		return;

	if (request.Market == 1) {
		AbCancelOrder(request);
		return;
	}

	std::string orderString;
	std::string temp;
	temp = PadRight(request.AccountNo, ' ', 11);
	orderString.append(temp);
	temp = PadRight(request.Password, ' ', 8);
	orderString.append(temp);
	temp = PadRight(request.SymbolCode, ' ', 32);
	orderString.append(temp);

	if (request.Position == VtPositionType::Buy)
		orderString.append(_T("1"));
	else if (request.Position == VtPositionType::Sell)
		orderString.append(_T("2"));

	if (request.PriceType == VtPriceType::Price)
		orderString.append(_T("1"));
	else if (request.PriceType == VtPriceType::Market)
		orderString.append(_T("2"));

	if (request.FillCondition == VtFilledCondition::Fas)
		orderString.append(_T("1"));
	else if (request.FillCondition == VtFilledCondition::Fok)
		orderString.append(_T("2"));
	else if (request.FillCondition == VtFilledCondition::Fak)
		orderString.append(_T("3"));

	if (request.PriceType == VtPriceType::Price)
		temp = PadRight(request.Price, ' ', 13);
	else if (request.PriceType == VtPriceType::Market)
		temp = PadRight(0, ' ', 13);
	orderString.append(temp);

	temp = PadRight(request.Amount, ' ', 5);
	orderString.append(temp);

	temp = PadLeft(request.OrderNo, '0', 7);
	orderString.append(temp);

	std::string userDefined;
	if (request.Type == 0)
		userDefined.append(_T("00000"));
	else if (request.Type == 1)
		userDefined.append(_T("11111"));
	else if (request.Type == 2)
		userDefined.append(_T("22222"));
	else
		return;
	userDefined.append(",");
	userDefined.append("fund_name");
	userDefined.append(",");
	userDefined.append(request.FundName);
	userDefined.append(",");
	userDefined.append("sub_account_no");
	userDefined.append(",");
	userDefined.append(request.SubAccountNo);
	userDefined.append(",");
	if (request.Type == 0)
		temp = PadRight(userDefined, '0', 60);
	else if (request.Type == 1)
		temp = PadRight(userDefined, '1', 60);
	else if (request.Type == 2)
		temp = PadRight(userDefined, '2', 60);
	orderString.append(temp);

	CString sTrCode = "g12001.DO1701&";
	CString sInput = orderString.c_str();
	int nRqID = m_CommAgent.CommJumunSvr(sTrCode, sInput);
	AddRequest(nRqID, HdTaskType::HdOrderCancel);
	request.HtsOrderReqID = nRqID;
	// 주문요청 번호와 함께 주문요청 맵에 넣어준다.
	_ReqIdToRequestMap[nRqID] = request;
	//LOG_F(INFO, _T("Cancel Order :: Req id = %d, order string = %s"), nRqID, orderString.c_str());
	LOG_F(INFO, _T("취소주문요청 : 요청번호 = %d, 종목이름 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %d, 요청 타입 = %d"),
		nRqID, request.SymbolCode.c_str(), request.OrderNo.c_str(), request.AccountNo.c_str(), request.SubAccountNo.c_str(),
		request.FundName.c_str(), request.Position == VtPositionType::Buy ? _T("매수") : _T("매도"), request.Amount, request.RequestType);
}

void VtHdCtrl::AbPutOrder(HdOrderRequest& request)
{
	if (!CheckPassword(request))
		return;

	std::string orderString;
	std::string temp;
	// 계좌 번호
	temp = PadRight(request.AccountNo, ' ', 6);
	orderString.append(temp);
	// 비밀번호
	temp = PadRight(request.Password, ' ', 8);
	orderString.append(temp);
	// 종목 코드
	temp = PadRight(request.SymbolCode, ' ', 32);
	orderString.append(temp);

	// 매매구분
	if (request.Position == VtPositionType::Buy)
		orderString.append(_T("1"));
	else if (request.Position == VtPositionType::Sell)
		orderString.append(_T("2"));

	// 가격 조건
	if (request.PriceType == VtPriceType::Price)
		orderString.append(_T("1"));
	else if (request.PriceType == VtPriceType::Market)
		orderString.append(_T("2"));
	// 체결 조건
	if (request.FillCondition == VtFilledCondition::Fas)
		orderString.append(_T("1"));
	else if (request.FillCondition == VtFilledCondition::Fok)
		orderString.append(_T("2"));
	else if (request.FillCondition == VtFilledCondition::Fak)
		orderString.append(_T("3"));
	else if (request.FillCondition == VtFilledCondition::Day)
		orderString.append(_T("0"));

	// 주문 가격
	if (request.PriceType == VtPriceType::Price)
		temp = PadRight(request.Price, ' ', 15);
	else if (request.PriceType == VtPriceType::Market)
		temp = PadRight(0, ' ', 15);
	orderString.append(temp);

	// 주문 수량
	temp = PadRight(request.Amount, ' ', 10);
	orderString.append(temp);
	// 기타 설정
	temp = PadRight(1, ' ', 35);
	orderString.append(temp);

	std::string userDefined;
	if (request.Type == 0)
		userDefined.append(_T("00000"));
	else if (request.Type == 1)
		userDefined.append(_T("11111"));
	else if (request.Type == 2)
		userDefined.append(_T("22222"));
	else
		return;
	userDefined.append(",");
	userDefined.append("fund_name");
	userDefined.append(",");
	userDefined.append(request.FundName);
	userDefined.append(",");
	userDefined.append("sub_account_no");
	userDefined.append(",");
	userDefined.append(request.SubAccountNo);
	userDefined.append(",");
	if (request.Type == 0)
		temp = PadRight(userDefined, '0', 60);
	else if (request.Type == 1)
		temp = PadRight(userDefined, '1', 60);
	else if (request.Type == 2)
		temp = PadRight(userDefined, '2', 60);

	orderString.append(temp);

	CString sTrCode = "g12003.AO0401%";
	CString sInput = orderString.c_str();
	int nRqID = m_CommAgent.CommJumunSvr(sTrCode, sInput);
	AddRequest(nRqID, HdTaskType::HdOrderNew);
	request.HtsOrderReqID = nRqID;
	// 주문요청 번호와 함께 주문요청 맵에 넣어준다.
	_ReqIdToRequestMap[nRqID] = request;

	//LOG_F(INFO,_T("PutOrder :: Req id = %d, order string = %s"), nRqID, orderString.c_str());
	LOG_F(INFO, _T("신규주문요청 : 요청번호 = %d, 종목이름 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %d, 요청 타입 = %d"),
		nRqID, request.SymbolCode.c_str(), request.OrderNo.c_str(), request.AccountNo.c_str(), request.SubAccountNo.c_str(),
		request.FundName.c_str(), request.Position == VtPositionType::Buy ? _T("매수") : _T("매도"), request.Amount, request.RequestType);
}

void VtHdCtrl::AbChangeOrder(HdOrderRequest& request)
{
	std::lock_guard<std::mutex> lock(m_);
	if (!CheckPassword(request))
		return;

	std::string orderString;
	std::string temp;
	// 계좌 번호
	temp = PadRight(request.AccountNo, ' ', 6);
	orderString.append(temp);
	// 비밀번호
	temp = PadRight(request.Password, ' ', 8);
	orderString.append(temp);
	// 종목 코드
	temp = PadRight(request.SymbolCode, ' ', 32);
	orderString.append(temp);

	// 가격 조건
	if (request.PriceType == VtPriceType::Price)
		orderString.append(_T("1"));
	else if (request.PriceType == VtPriceType::Market)
		orderString.append(_T("2"));
	// 체결 조건
	if (request.FillCondition == VtFilledCondition::Fas)
		orderString.append(_T("1"));
	else if (request.FillCondition == VtFilledCondition::Fok)
		orderString.append(_T("2"));
	else if (request.FillCondition == VtFilledCondition::Fak)
		orderString.append(_T("3"));
	else if (request.FillCondition == VtFilledCondition::Day)
		orderString.append(_T("0"));

	// 주문 가격
	if (request.PriceType == VtPriceType::Price)
		temp = PadRight(request.Price, ' ', 15);
	else if (request.PriceType == VtPriceType::Market)
		temp = PadRight(0, ' ', 15);
	orderString.append(temp);

	// 정정 수량
	temp = PadRight(request.Amount, ' ', 10);
	orderString.append(temp);
	// 정정이나 취소시 원주문 번호
	temp = PadRight(request.OrderNo, ' ', 10);
	orderString.append(temp);
	// 기타설정
	temp = PadRight(1, ' ', 26);
	orderString.append(temp);

	std::string userDefined;
	if (request.Type == 0)
		userDefined.append(_T("00000"));
	else if (request.Type == 1)
		userDefined.append(_T("11111"));
	else if (request.Type == 2)
		userDefined.append(_T("22222"));
	else
		return;
	userDefined.append(",");
	userDefined.append("fund_name");
	userDefined.append(",");
	userDefined.append(request.FundName);
	userDefined.append(",");
	userDefined.append("sub_account_no");
	userDefined.append(",");
	userDefined.append(request.SubAccountNo);
	userDefined.append(",");
	if (request.Type == 0)
		temp = PadRight(userDefined, '0', 60);
	else if (request.Type == 1)
		temp = PadRight(userDefined, '1', 60);
	else if (request.Type == 2)
		temp = PadRight(userDefined, '2', 60);

	orderString.append(temp);

	CString sTrCode = "g12003.AO0402%";
	CString sInput = orderString.c_str();
	int nRqID = m_CommAgent.CommJumunSvr(sTrCode, sInput);
	AddRequest(nRqID, HdTaskType::HdOrderNew);
	request.HtsOrderReqID = nRqID;
	// 주문요청 번호와 함께 주문요청 맵에 넣어준다.
	_ReqIdToRequestMap[nRqID] = request;

	CString sInputEx = "1234561234    6AM16                           107155           1         16Ke0005641                         ";

	//LOG_F(INFO,_T("PutOrder :: Req id = %d, order string = %s"), nRqID, orderString.c_str());
	LOG_F(INFO, _T("신규주문요청 : 요청번호 = %d, 종목이름 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %d, 요청 타입 = %d"),
		nRqID, request.SymbolCode.c_str(), request.OrderNo.c_str(), request.AccountNo.c_str(), request.SubAccountNo.c_str(),
		request.FundName.c_str(), request.Position == VtPositionType::Buy ? _T("매수") : _T("매도"), request.Amount, request.RequestType);

}

void VtHdCtrl::AbCancelOrder(HdOrderRequest& request)
{
	std::lock_guard<std::mutex> lock(m_);
	if (!CheckPassword(request))
		return;

	std::string orderString;
	std::string temp;
	// 계좌 번호
	temp = PadRight(request.AccountNo, ' ', 6);
	orderString.append(temp);
	// 비밀번호
	temp = PadRight(request.Password, ' ', 8);
	orderString.append(temp);
	// 종목 코드
	temp = PadRight(request.SymbolCode, ' ', 32);
	orderString.append(temp);

	// 가격 조건
	if (request.PriceType == VtPriceType::Price)
		orderString.append(_T("1"));
	else if (request.PriceType == VtPriceType::Market)
		orderString.append(_T("2"));
	// 체결 조건
	if (request.FillCondition == VtFilledCondition::Fas)
		orderString.append(_T("1"));
	else if (request.FillCondition == VtFilledCondition::Fok)
		orderString.append(_T("2"));
	else if (request.FillCondition == VtFilledCondition::Fak)
		orderString.append(_T("3"));
	else if (request.FillCondition == VtFilledCondition::Day)
		orderString.append(_T("0"));

	// 주문 가격 15
	temp = "               ";
	orderString.append(temp);

	// 정정 수량 10
	temp = "          ";
	orderString.append(temp);
	// 정정이나 취소시 원주문 번호
	temp = PadRight(request.OrderNo, ' ', 10);
	orderString.append(temp);
	// 기타설정 26
	temp = "                          ";
	orderString.append(temp);

	std::string userDefined;
	if (request.Type == 0)
		userDefined.append(_T("00000"));
	else if (request.Type == 1)
		userDefined.append(_T("11111"));
	else if (request.Type == 2)
		userDefined.append(_T("22222"));
	else
		return;
	userDefined.append(",");
	userDefined.append("fund_name");
	userDefined.append(",");
	userDefined.append(request.FundName);
	userDefined.append(",");
	userDefined.append("sub_account_no");
	userDefined.append(",");
	userDefined.append(request.SubAccountNo);
	userDefined.append(",");
	if (request.Type == 0)
		temp = PadRight(userDefined, '0', 60);
	else if (request.Type == 1)
		temp = PadRight(userDefined, '1', 60);
	else if (request.Type == 2)
		temp = PadRight(userDefined, '2', 60);

	orderString.append(temp);

	CString sTrCode = "g12003.AO0403%";
	CString sInput = orderString.c_str();
	int nRqID = m_CommAgent.CommJumunSvr(sTrCode, sInput);
	AddRequest(nRqID, HdTaskType::HdOrderNew);
	request.HtsOrderReqID = nRqID;
	// 주문요청 번호와 함께 주문요청 맵에 넣어준다.
	_ReqIdToRequestMap[nRqID] = request;

	//LOG_F(INFO,_T("PutOrder :: Req id = %d, order string = %s"), nRqID, orderString.c_str());
	LOG_F(INFO, _T("신규주문요청 : 요청번호 = %d, 종목이름 = %s, 원주문 번호 = %d, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %d, 요청 타입 = %d"),
		nRqID, request.SymbolCode.c_str(), request.OrderNo.c_str(), request.AccountNo.c_str(), request.SubAccountNo.c_str(),
		request.FundName.c_str(), request.Position == VtPositionType::Buy ? _T("매수") : _T("매도"), request.Amount, request.RequestType);


}

void VtHdCtrl::AbOnNewOrderHd(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");


	CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
	CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
	CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");
	CString strProcMsg = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderReqId(nRqID);
	// 여기서 주문요청 아이디와 주문요청 객체를 매칭해 준다.
	auto it = _ReqIdToRequestMap.find(nRqID);
	if (it != _ReqIdToRequestMap.end()) {
		HdOrderRequest req = it->second;
		_OrderNoToRequestMap[(LPCTSTR)(strOrdNo)] = req;
	}

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	// 주문이 없다면 새로 만든다.
	if (!order) {
		order = new VtOrder();
		// 주문 요청 아이디 
		order->HtsOrderReqID = nRqID;
		// 일반 주문인지 청산 주문인지 넣어 준다.
		order_req != nullptr ? order->RequestType = order_req->RequestType : order->RequestType = -1;
		// 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 주문 타입 - 신규 주문
		order->orderType = VtOrderType::New;

		// 일반계좌 주문
		order->Type = 0;
	}
	else { // 주문이 있다는 것은 이미 거래소 접수 되었거나 체결된 경우이다. 이 부분은 특별하게 처리해야 한다.
		if (order->state == VtOrderState::Filled) {
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			if (order_req) {
				//LOG_F(INFO, _T("OnNewOrderHd 주문역전 :: 이미 체결된 주문입니다! 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, order_req->Type == 1 ? order_req->SubAccountNo.c_str() : order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
		else if (order->state == VtOrderState::Accepted) {

			// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
			// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
			if (order_req) {
				//LOG_F(INFO, _T("OnNewOrderHd 주문역전 :: 이미 거래소 접수된 주문입니다! 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, order_req->Type == 1 ? order_req->SubAccountNo.c_str() : order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
											// 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
												 // 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
	}

	// 주문 수신을 처리해 준다.
	orderMgr->OnOrderReceivedHd(order);
	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
									// 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
										 // 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 펀드이름을 넣어준다.
			subAcntOrder->FundName = order_req->FundName;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
	}

	// 주문 상태를 바꿔준다.
	order->state = VtOrderState::OrderReceived;
	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	//OnOrderReceived(nRqID, order);

	//OnSubAccountOrder(VtOrderEvent::PutNew, strSubAcntNo, strFundName, order, prevState);

	//LOG_F(INFO, _T("신규주문서버확인 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

}

void VtHdCtrl::AbOnModifyOrderHd(CString& sTrCode, LONG& nRqID)
{
	int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

	CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
	CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
	CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");
	CString strProcMsg = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
	CString strCustom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "사용자정의필드");
	CString strOriOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "원주문번호");
	CString strFirstOriOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "최초원주문번호");

	CString strMsg;
	strMsg.Format("OnModifyOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
	//WriteLog(strMsg);
	strMsg.Format(_T("%s\n"), strCustom);
	//TRACE(strMsg);

	strAcctNo.TrimRight(); // 계좌 번호
	strOrdNo.TrimLeft('0'); // 주문 번호
	strOriOrdNo.TrimRight(); // 원주문 번호

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderReqId(nRqID);
	if (order_req) {
		// 새로운 주문번호를 넣어 준다.
		order_req->NewOrderNo = (LPCTSTR)(strOrdNo);
	}
	// 여기서 주문요청 아이디와 주문요청 객체를 매칭해 준다.
	auto it = _ReqIdToRequestMap.find(nRqID);
	if (it != _ReqIdToRequestMap.end()) {
		HdOrderRequest req = it->second;
		_OrderNoToRequestMap[(LPCTSTR)(strOrdNo)] = req;
		_OrderNoToRequestMap[(LPCTSTR)(strOriOrdNo)] = req;
	}

	// 정정된 원주문이 체결된 경우는 더이상 처리하지 않는다.
	VtOrder* originOrder = orderMgr->FindOrder((LPCTSTR)(strOriOrdNo));
	if (originOrder && originOrder->state == VtOrderState::Filled) {
		// 원주문을 제거해 준다.
		orderMgr->RemoveAcceptedHd(originOrder);
		if (order_req) {
			VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
			VtOrderManager* subOrderMgr = nullptr;
			VtOrder* subAcntOrder = nullptr;

			if (order_req->Type == 1) { // 서브계좌 주문인 경우
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(originOrder);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = originOrder->AccountNo;
				subOrderMgr->OnOrderFilledHd(subAcntOrder);
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
			else if (order_req->Type == 2) { // 펀드 주문인 경우
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(originOrder);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = originOrder->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
				subOrderMgr->OnOrderFilledHd(subAcntOrder);
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
		}
	}

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	if (!order) { // 주문이 없는 경우는 처음으로 이곳으로 주문이 들어와 생성되는 것이다.
		strMsg.Format("new ::::: OnModifyOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
		strMsg.Format(_T("%s\n"), strCustom);
		//TRACE(strMsg);
		order = new VtOrder();
		// 주문 요청 번호
		order->HtsOrderReqID = nRqID;
		// 일반 주문인지 청산 주문인지 넣어 준다.
		order_req != nullptr ? order->RequestType = order_req->RequestType : order->RequestType = -1;
		// 주문 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 원 주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrdNo);
	
		// 주문 유형
		order->orderType = VtOrderType::Change;

		// 일반계좌 주문
		order->Type = 0;
	}
	else { // 여기서 역전된 주문을 처리한다.
		if (order->state == VtOrderState::Filled) {
			LOG_F(INFO, _T("OnModifyOrderHd 주문역전 :: 이미 체결된 주문입니다!"));
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
		else if (order->state == VtOrderState::Accepted) {
			LOG_F(INFO, _T("OnModifyOrderHd 주문역전 :: 이미 거래소 접수된 주문입니다!"));

			// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
			// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
											// 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
												 // 주문관리자를 생성해 준다.
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;

					subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
					// 주문상태를 바꿔준다.
					subAcntOrder->state = VtOrderState::Accepted;
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}
	}

	// 주문 수신을 처리해 준다.
	orderMgr->OnOrderReceivedHd(order);
	// 주문 상태를 바꿔준다.
	order->state = VtOrderState::OrderReceived;
	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
									// 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
										 // 주문관리자를 생성해 준다.
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			// 본주문을 복사한다.
			subAcntOrder = subOrderMgr->CloneOrder(order);
			// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
			subAcntOrder->Type = order_req->Type;
			// 서브계좌로  계좌번호를 바꿔준다.
			subAcntOrder->AccountNo = order_req->SubAccountNo;
			// 서브계좌 번호를 저장해 준다.
			subAcntOrder->SubAccountNo = order_req->SubAccountNo;
			// 부모계좌 번호를 넣어준다.
			subAcntOrder->ParentAccountNo = order->AccountNo;
			// 펀드이름을 넣어준다.
			subAcntOrder->FundName = order_req->FundName;
			// 서브주문관리자는 서브주문을 처리한다.
			subOrderMgr->OnOrderReceivedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::OrderReceived;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
	}

	//SendOrderMessage(VtOrderEvent::Modified, order);

	//OnOrderReceived(nRqID, order);

	//OnSubAccountOrder(VtOrderEvent::Modified, strSubAcntNo, strFundName, order, prevState);

	//LOG_F(INFO, _T("정정주문서버확인 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, strOriOrdNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

}

void VtHdCtrl::AbOnCancelOrderHd(CString& sTrCode, LONG& nRqID)
{
	try
	{
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
		CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");
		CString strProcMsg = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
		CString strCustom = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "사용자정의필드");
		CString strOriOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "원주문번호");
		CString strFirstOriOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "최초원주문번호");

		CString strMsg;
		strMsg.Format("OnCancelOrderHd 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]\n", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
		//strMsg.Format(_T("%s\n"), strCustom);
		//TRACE(strMsg);

		// 계좌 번호
		strAcctNo.TrimRight();
		// 주문 번호
		strOrdNo.TrimLeft('0');
		strOriOrdNo.TrimLeft('0'); // 원주문 번호
		strFirstOriOrdNo.TrimLeft('0'); // 최초 원주문 번호
		// 원주문번호
		strOriOrdNo.TrimRight();

		VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
		HdOrderRequest* order_req = GetOrderRequestByOrderReqId(nRqID);
		if (order_req) {
			// 새로운 주문번호를 넣어 준다.
			order_req->NewOrderNo = (LPCTSTR)(strOrdNo);
		}
		// 여기서 주문요청 아이디와 주문요청 객체를 매칭해 준다.
		auto it = _ReqIdToRequestMap.find(nRqID);
		if (it != _ReqIdToRequestMap.end()) {
			HdOrderRequest req = it->second;
			_OrderNoToRequestMap[(LPCTSTR)(strOrdNo)] = req;
			_OrderNoToRequestMap[(LPCTSTR)(strOriOrdNo)] = req;
		}

		// 취소된 원주문이 체결된 경우는 별도로 처리해 준다.
		VtOrder* originOrder = orderMgr->FindOrder((LPCTSTR)(strOriOrdNo));
		if (originOrder && originOrder->state == VtOrderState::Filled) {
			// 원주문을 제거해 준다.
			orderMgr->RemoveAcceptedHd(originOrder);
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(originOrder);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = originOrder->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(originOrder);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = originOrder->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
		}

		VtOrder* order = nullptr;
		order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
		if (!order) {
			order = new VtOrder();
			// 주문 요청 번호
			order->HtsOrderReqID = nRqID;
			// 일반 주문인지 청산 주문인지 넣어 준다.
			order_req != nullptr ? order->RequestType = order_req->RequestType : order->RequestType = -1;
			// 주문 번호
			order->AccountNo = (LPCTSTR)strAcctNo;
			// 주문 번호
			order->orderNo = (LPCTSTR)(strOrdNo);
			// 원 주문 번호
			order->oriOrderNo = (LPCTSTR)(strOriOrdNo);
			// 주문 유형
			order->orderType = VtOrderType::Cancel;

			// 일반계좌 주문
			order->Type = 0;
		}
		else {
			if (order->state == VtOrderState::Filled) {
				LOG_F(INFO, _T("OnCancelOrderHd 주문역전 :: 이미 체결된 주문입니다!"));
				// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
				if (order_req) {
					VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
					VtOrderManager* subOrderMgr = nullptr;
					VtOrder* subAcntOrder = nullptr;

					if (order_req->Type == 1) { // 서브계좌 주문인 경우
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						subOrderMgr->OnOrderFilledHd(subAcntOrder);
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
					else if (order_req->Type == 2) { // 펀드 주문인 경우
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						// 펀드이름을 넣어준다.
						subAcntOrder->FundName = order_req->FundName;
						subOrderMgr->OnOrderFilledHd(subAcntOrder);
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
				}
			}
			else if (order->state == VtOrderState::Accepted) {
				LOG_F(INFO, _T("OnCancelOrderHd 주문역전 :: 이미 거래소 접수된 주문입니다!"));

				// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
				// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
				if (order_req) {
					VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
					VtOrderManager* subOrderMgr = nullptr;
					VtOrder* subAcntOrder = nullptr;

					if (order_req->Type == 1) { // 서브계좌 주문인 경우
												// 주문관리자를 생성해 준다.
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
						// 주문상태를 바꿔준다.
						subAcntOrder->state = VtOrderState::Accepted;
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
					else if (order_req->Type == 2) { // 펀드 주문인 경우
													 // 주문관리자를 생성해 준다.
						subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
						// 본주문을 복사한다.
						subAcntOrder = subOrderMgr->CloneOrder(order);
						// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
						subAcntOrder->Type = order_req->Type;
						// 서브계좌로  계좌번호를 바꿔준다.
						subAcntOrder->AccountNo = order_req->SubAccountNo;
						// 서브계좌 번호를 저장해 준다.
						subAcntOrder->SubAccountNo = order_req->SubAccountNo;
						// 부모계좌 번호를 넣어준다.
						subAcntOrder->ParentAccountNo = order->AccountNo;
						// 펀드이름을 넣어준다.
						subAcntOrder->FundName = order_req->FundName;
						subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
						// 주문상태를 바꿔준다.
						subAcntOrder->state = VtOrderState::Accepted;
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
				}
			}
		}

		// 주문 수신을 처리해 준다.
		orderMgr->OnOrderReceivedHd(order);
		// 주문 상태를 바꿔준다.
		order->state = VtOrderState::OrderReceived;
		SmCallbackManager::GetInstance()->OnOrderEvent(order);

		// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
		// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
		if (order_req) {
			VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
			VtOrderManager* subOrderMgr = nullptr;
			VtOrder* subAcntOrder = nullptr;

			if (order_req->Type == 1) { // 서브계좌 주문인 경우
										// 주문관리자를 생성해 준다.
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 서브주문관리자는 서브주문을 처리한다.
				subOrderMgr->OnOrderReceivedHd(subAcntOrder);
				// 주문상태를 바꿔준다.
				subAcntOrder->state = VtOrderState::OrderReceived;
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
			else if (order_req->Type == 2) { // 펀드 주문인 경우
											 // 주문관리자를 생성해 준다.
				subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
				// 서브주문관리자는 서브주문을 처리한다.
				subOrderMgr->OnOrderReceivedHd(subAcntOrder);
				// 주문상태를 바꿔준다.
				subAcntOrder->state = VtOrderState::OrderReceived;
				SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
			}
		}

		//SendOrderMessage(VtOrderEvent::Cancelled, order);

		//OnOrderReceived(nRqID, order);

		//OnSubAccountOrder(VtOrderEvent::Cancelled, strSubAcntNo, strFundName, order, prevState);

		//LOG_F(INFO, _T("취소주문서버확인 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, nRqID, strSeries, strOrdNo, strOriOrdNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

	}
	catch (std::exception& e)
	{
		std::string error = e.what();
	}
}

void VtHdCtrl::AbOnOrderReceivedHd(CString& sTrCode, LONG& nRqID)
{

}

void VtHdCtrl::AbOnOrderAcceptedHd(CString& strKey, LONG& nRealType)
{
	CString strAcctNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
	CString strPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
	CString strAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");
	CString strCustom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "사용자정의필드");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
	CString strMan = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문구분"); // 	1:신규 2:정정 3:취소
	CString strOriOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "원주문번호");
	CString strFirstOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "최초원주문번호");
	CString strTraderTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "접수시간");

	CString strMsg;
	strMsg.Format("OnOrderAcceptedHd 계좌번호[%s]주문번호[%s][원주문번호[%s]\n", strAcctNo, strOrdNo, strOriOrderNo);
	//WriteLog(strMsg);
	//strMsg.Format(_T("%s\n"), strCustom);
	//TRACE(strMsg);

	strAcctNo.TrimRight(); // 계좌 번호
	strOrdNo.TrimLeft('0'); // 주문 번호
	strOriOrderNo.TrimLeft('0'); // 원주문 번호
	strFirstOrderNo.TrimLeft('0'); // 최초 원주문 번호
	strSeries.TrimRight(); // 심볼 코드
	strPrice.TrimRight();
	strPrice = strPrice.TrimLeft('0'); // 주문 가격 트림
	CString strOriOrderPrice;
	strOriOrderPrice = strPrice; // 원주문가격 저장

								 // 주문 가격을 정수로 변환
	int count = strPrice.Remove('.');
	// 주문 가격 트림
	strPrice.TrimRight();
	// 주문 수량 트림
	strAmount.TrimRight();

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderNo((LPCTSTR)(strOrdNo));

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	// 주문이 없는 경우는 외부 주문이나 내부주문중 아직 주문 번호가 도착하지 않은 주문이다.
	if (!order) {
		strMsg.Format("new OnOrderAcceptedHd 계좌번호[%s]주문번호[%s]\n", strAcctNo, strOrdNo);
		//WriteLog(strMsg);
		//strMsg.Format(_T("%s\n"), strCustom);
		//TRACE(strMsg);
		order = new VtOrder();
		// 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수주문가격 설정
		order->intOrderPrice = GetIntOrderPrice(strSeries, strPrice, strOriOrderPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 소수로 표현된 주문 가격
		order->orderPrice = _ttof(strOriOrderPrice);
		// 최초 원주문번호
		order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
		// 원주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrderNo);
		// 주문 유형 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0) {
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->orderPosition = VtPositionType::Sell;
		}

		// 거래 시간
		order->tradeTime = (LPCTSTR)strTraderTime;

		// 주문 유형 - 신규, 정정, 취소
		if (strMan.Compare(_T("1")) == 0) {
			order->orderType = VtOrderType::New;
		}
		else if (strMan.Compare(_T("2")) == 0){
			order->orderType = VtOrderType::Change;
		}
		else if (strMan.Compare(_T("3")) == 0) {
			order->orderType = VtOrderType::Cancel;
		}
	}
	else { // 이미 주문이 있는 경우
		   // 거래소 접수되었지만 그 원주문이 이미 체결된 경우에는 현재 주문도 접수확인 목록에서 제거해 준다.
		   // 이미 체결된 상태이기 때문에 추가적인 처리는 하지 않는다.
		   // 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수주문가격 설정
		order->intOrderPrice = GetIntOrderPrice(strSeries, strPrice, strOriOrderPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 소수로 표현된 주문 가격
		order->orderPrice = _ttof(strOriOrderPrice);
		// 최초 원주문번호
		order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
		// 원주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrderNo);
		// 주문 유형 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0) {
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->orderPosition = VtPositionType::Sell;
		}

		// 거래 시간
		order->tradeTime = (LPCTSTR)strTraderTime;

		// 주문 유형 - 신규, 정정, 취소
		if (strMan.Compare(_T("1")) == 0) {
			order->orderType = VtOrderType::New;
		}
		else if (strMan.Compare(_T("2")) == 0) {
			order->orderType = VtOrderType::Change;
		}
		else if (strMan.Compare(_T("3")) == 0) {
			order->orderType = VtOrderType::Cancel;
		}

		VtOrder* origin_order = orderMgr->FindOrder((LPCTSTR)(strOriOrderNo));
		if (origin_order) {
			if (origin_order->state == VtOrderState::Filled || origin_order->state == VtOrderState::Settled) {
				orderMgr->RemoveAcceptedHd(order);
				SmCallbackManager::GetInstance()->OnOrderEvent(order);
				// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
				if (order_req) {
					VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
					VtOrderManager* subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					VtOrder* subAcntOrder = subOrderMgr->FindOrder(order->orderNo);
					if (subAcntOrder) {
						subOrderMgr->RemoveAcceptedHd(subAcntOrder);
						SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
					}
				}
				return;
			}
		}
		if (order->state == VtOrderState::Filled) {
			LOG_F(INFO, _T("OnAccepted :: // 주문역전 : 주문의 상태가 체결인 경우는 역전된 경우이다."));
			// 여기서 이미 체결된 주문에 대하여 혹시 접수확인 목록에 들어가 있다면 없애준다.
			if (order_req) {
				VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
				VtOrderManager* subOrderMgr = nullptr;
				VtOrder* subAcntOrder = nullptr;

				if (order_req->Type == 1) { // 서브계좌 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);

					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
				else if (order_req->Type == 2) { // 펀드 주문인 경우
					subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
					// 본주문을 복사한다.
					subAcntOrder = subOrderMgr->CloneOrder(order);
					// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
					subAcntOrder->Type = order_req->Type;
					// 서브계좌로  계좌번호를 바꿔준다.
					subAcntOrder->AccountNo = order_req->SubAccountNo;
					// 서브계좌 번호를 저장해 준다.
					subAcntOrder->SubAccountNo = order_req->SubAccountNo;
					// 부모계좌 번호를 넣어준다.
					subAcntOrder->ParentAccountNo = order->AccountNo;
					// 펀드이름을 넣어준다.
					subAcntOrder->FundName = order_req->FundName;
					subOrderMgr->OnOrderFilledHd(subAcntOrder);
					SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
				}
			}
			return;
		}
	}

	// 주문 처리
	orderMgr->OnOrderAcceptedHd(order);
	// 주문 상태를 바꿔준다.
	order->state = VtOrderState::Accepted;

	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	LOG_F(INFO, _T("본계좌 거래소 접수 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);


	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	// 주문요청 정보가 있다는 것은 서버에서 주문 정보를 이미 받았다는 것이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
			}

			subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::Accepted;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("서브계좌 거래소 접수 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 서브계좌번호 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, order_req->SubAccountNo.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				// 본주문을 복사한다.
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
			}

			subOrderMgr->OnOrderAcceptedHd(subAcntOrder);
			// 주문상태를 바꿔준다.
			subAcntOrder->state = VtOrderState::Accepted;
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("펀드주문 거래소 접수 : 주문가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문번호 = %s, 펀드이름 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);
		}
	}


	//order->orderEvent = VtOrderEvent::Accepted;

	//SendOrderMessage(VtOrderEvent::Accepted, order);

	//OnSubAccountOrder(VtOrderEvent::Accepted, strSubAcntNo, strFundName, order, prevState);



	//LOG_F(INFO, _T("사용자정의 필드 = %s"), strCustom);

}

void VtHdCtrl::AbOnOrderUnfilledHd(CString& strKey, LONG& nRealType)
{
	CString strAcctNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
	CString strPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
	CString strAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");
	CString strCustom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "사용자정의필드");
	CString strMan = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문순서"); // 1:원주문, 2:정정/취소
	CString strCancelCnt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "취소수량");
	CString strModyCnt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "정정수량");
	CString strFilledCnt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결수량");
	CString strRemain = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "잔량");

	CString strOriOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "원주문번호");
	CString strFirstOrderNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "최초원주문번호");

	// 주문 가격
	strPrice = strPrice.TrimRight();
	CString strOriOrderPrice;
	// 원 주문 가격 저장
	strOriOrderPrice = strPrice;
	// 주문 가격을 정수로 변환
	int count = strPrice.Remove('.');
	// 계좌 번호 트림
	strAcctNo.TrimRight();
	// 주문 번호 트림
	strOrdNo.TrimLeft('0');
	// 원주문 번호 트림
	strOriOrderNo.TrimLeft('0');
	// 첫주문 번호 트림
	strFirstOrderNo.TrimLeft('0');
	// 심볼 코드 트림
	strSeries.TrimRight();
	// 주문 수량 트림
	strAmount.TrimRight();
	// 정정이나 취소시 처리할 수량 트림
	strRemain.TrimRight();
	// 정정이 이루어진 수량
	strModyCnt.TrimRight();
	// 체결된 수량
	strFilledCnt.TrimRight();
	// 취소된 수량
	strCancelCnt.TrimRight();

	CString strMsg;
	strMsg.Format("OnOrderUnfilledHd 계좌번호[%s]주문번호[%s]\n", strAcctNo, strOrdNo);
	//WriteLog(strMsg);
	//strMsg.Format(_T("%s\n"), strCustom);
	//TRACE(strMsg);

	m_strOrdNo = strOrdNo;

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderNo((LPCTSTR)(strOrdNo));


	// 처리할 갯수
	int remainCnt = _ttoi(strRemain);
	// 취소 된 갯수
	int cancelCnt = _ttoi(strCancelCnt);
	// 정정된 갯수
	int modifyCnt = _ttoi(strModyCnt);
	// 주문한 갯수
	int orderCnt = _ttoi(strAmount);

	VtOrder* order = nullptr;
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	if (!order) {
		order = new VtOrder();
		// 주문 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);
		// 정수주문가격 설정
		order->intOrderPrice = GetIntOrderPrice(strSeries, strPrice, strOriOrderPrice);
		// 주문 수량
		order->amount = _ttoi(strAmount);
		// 소소 주문 가격
		order->orderPrice = _ttof(strOriOrderPrice);

		// 주문 유형 - 매수 / 매도
		if (strPosition.Compare(_T("1")) == 0)
		{
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0)
		{
			order->orderPosition = VtPositionType::Sell;
		}
		// 신규 주문, 정정 주문 , 취소 주문 시 처리할 주문의 수 - 0이면 모두 처리한 것이다.
		order->unacceptedQty = _ttoi(strRemain);
		// 첫 주문 번호
		order->firstOrderNo = (LPCTSTR)(strFirstOrderNo);
		// 주문 번호
		order->oriOrderNo = (LPCTSTR)(strOriOrderNo);
		// 정정한 주문 갯수
		order->modifiedOrderCount = _ttoi(strModyCnt);

		// 주문 상태를 저장함
		VtOrderState prevState = order->state;
		if (remainCnt == orderCnt) {
			// 이미 체결된 주문은 설정하지 않는다.
			if (order->state != VtOrderState::Filled)
				order->state = VtOrderState::Accepted;
		}
		// 정정 주문 완료 상태 확인
		if (remainCnt == 0 && modifyCnt == orderCnt) {
			// 이미 체결된 주문은 설정하지 않는다.
			if (order->state != VtOrderState::Filled)
				order->state = VtOrderState::ConfirmModify;
		}
		// 취소 주문 완료 상태 확인
		if (remainCnt == 0 && cancelCnt == orderCnt) {
			order->unacceptedQty = 0;
			order->amount = 0;
			// 이미 체결된 주문은 설정하지 않는다.
			if (order->state != VtOrderState::Filled)
				order->state = VtOrderState::ConfirmCancel;
		}
	}

	orderMgr->OnOrderUnfilledHd(order);

	SmCallbackManager::GetInstance()->OnOrderEvent(order);

	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	// 주문요청 정보가 있다는 것은 서버에서 주문 정보를 이미 받았다는 것이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
			}
			subOrderMgr->OnOrderUnfilledHd(subAcntOrder);

			if (remainCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::Accepted;
			}
			// 정정 주문 완료 상태 확인
			if (remainCnt == 0 && modifyCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmModify;
			}
			// 취소 주문 완료 상태 확인
			if (remainCnt == 0 && cancelCnt == orderCnt) {
				subAcntOrder->unacceptedQty = 0;
				subAcntOrder->amount = 0;
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmCancel;
			}

			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
			}
			subOrderMgr->OnOrderUnfilledHd(subAcntOrder);

			if (remainCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::Accepted;
			}
			// 정정 주문 완료 상태 확인
			if (remainCnt == 0 && modifyCnt == orderCnt) {
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmModify;
			}
			// 취소 주문 완료 상태 확인
			if (remainCnt == 0 && cancelCnt == orderCnt) {
				subAcntOrder->unacceptedQty = 0;
				subAcntOrder->amount = 0;
				// 이미 체결된 주문은 설정하지 않는다.
				if (subAcntOrder->state != VtOrderState::Filled)
					subAcntOrder->state = VtOrderState::ConfirmCancel;
			}

			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);
		}
	}

	// 	order->orderEvent = VtOrderEvent::Unfilled;
	// 
	// 	SendOrderMessage(VtOrderEvent::Unfilled, order);
	// 
	// 	OnSubAccountOrder(VtOrderEvent::Unfilled, strSubAcntNo, strFundName, order, prevState);



	//LOG_F(INFO, _T("미체결 수신 : 주문가격 = %s, 원요청번호 %d, 선물사 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 원주문 번호 = %s, 계좌번호 = %s, 서브계좌번호 = %s, 펀드 이름 = %s, 주문종류 = %s, 주문갯수 = %s, 요청 타입 = %d"), strPrice, oriReqNo, order->HtsOrderReqID, strSeries, strOrdNo, strOriOrderNo, strAcctNo, strSubAcntNo, strFundName, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strAmount, order->RequestType);

}

void VtHdCtrl::AbOnOrderFilledHd(CString& strKey, LONG& nRealType)
{
	CString strAcctNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
	CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
	CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목");
	CString strPosition = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");


	CString strFillPrice = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결가격");
	CString strFillAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결수량");
	CString strFillTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결시간");
	CString strCustom = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "사용자정의필드");

	// 체결된 가격
	strFillPrice = strFillPrice.TrimLeft();
	CString strOriFill = strFillPrice;
	// 체결된 가격을 정수로 변환
	int count = strFillPrice.Remove('.');
	// 계좌 번호 트림
	strAcctNo.TrimRight();
	// 주문 번호 트림
	strOrdNo.TrimLeft('0');
	// 심볼 코드
	strSeries.TrimRight();
	// 소수로 표시된 체결 가격
	strFillPrice.TrimRight();
	// 체결 수량
	strFillAmount.TrimLeft();
	// 체결된 시각
	strFillTime.TrimRight();

	VtOrderManagerSelector* orderMgrSeledter = VtOrderManagerSelector::GetInstance();
	VtOrderManager* orderMgr = orderMgrSeledter->FindAddOrderManager((LPCTSTR)strAcctNo);
	HdOrderRequest* order_req = GetOrderRequestByOrderNo((LPCTSTR)(strOrdNo));
	// 이미 체결된 주문이 정정된 경우에 대하여 새로운 주문을 없애 준다.
	if (order_req) {
		// 이미 체결된 본 주문 처리
		orderMgr->RemoveAcceptedHd((LPCTSTR)strSeries, order_req->NewOrderNo);

		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subOrderMgr->RemoveAcceptedHd((LPCTSTR)strSeries, order_req->NewOrderNo);
		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subOrderMgr->RemoveAcceptedHd((LPCTSTR)strSeries, order_req->NewOrderNo);
		}
	}

	VtOrder* order = nullptr;
	// 주문이 있는지 찾아본다. 목록에 없으면 새로 생성해 준다.
	order = orderMgr->FindOrder((LPCTSTR)(strOrdNo));
	// 주문이 주문 목록에 없는 경우는 외부 주문이거나 역전되어 오는 주문이다. 
	if (!order) {
		order = new VtOrder();
		// 주문 계좌 번호
		order->AccountNo = (LPCTSTR)strAcctNo;
		// 심볼 코드
		order->shortCode = (LPCTSTR)strSeries;
		// 주문 번호
		order->orderNo = (LPCTSTR)(strOrdNo);

		// 주문 타입
		if (strPosition.Compare(_T("1")) == 0) {
			order->orderPosition = VtPositionType::Buy;
		}
		else if (strPosition.Compare(_T("2")) == 0) {
			order->orderPosition = VtPositionType::Sell;
		}
	}

	// 정수로 체결가격 설정
	order->intFilledPrice = GetIntOrderPrice(strSeries, strFillPrice, strOriFill);

	// 체결된 수량
	order->filledQty = _ttoi(strFillAmount);
	// 체결된 시각
	order->filledTime = (LPCTSTR)strFillTime;
	// 체결된 가격 - 소수로 표시된
	order->filledPrice = _ttof(strOriFill);

	orderMgr->OnOrderFilledHd(order);

	SmCallbackManager::GetInstance()->OnOrderEvent(order);
	LOG_F(INFO, _T("메인 주문 체결 확인 : 체결가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 이전주문번호 = %s, 계좌번호 = %s, 주문종류 = %s, 체결갯수 = %s, 요청 타입 = %d"), strFillPrice, order->HtsOrderReqID, strSeries, strOrdNo, order_req != nullptr ? order_req->NewOrderNo.c_str() : "0", strAcctNo, strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strFillAmount, order->RequestType);

	HdWindowManager* wndMgr = HdWindowManager::GetInstance();
	std::map<CWnd*, std::pair<HdWindowType, CWnd*>>& wndMap = wndMgr->GetWindowMap();
	for (auto it = wndMap.begin(); it != wndMap.end(); ++it) {
		auto item = it->second;
		HdWindowType type = item.first;
		CWnd* wnd = item.second;
		if (type == HdWindowType::MiniJangoWindow) {
			((HdAccountPLDlg*)wnd)->OnOrderFilledHd(order);
		}
		else if (type == HdWindowType::FundMiniJangoWindow) {
			((VtFundMiniJango*)wnd)->OnOrderFilledHd(order);
		}
	}

	// 메인주문과 서브계좌 주문은 계좌만 다를 뿐 완전히 동일하다.
	// 주문요청이 있을 경우 - 주문 요청이 없는 경우는 외부 주문이다.
	// 주문요청 정보가 있다는 것은 서버에서 주문 정보를 이미 받았다는 것이다.
	if (order_req) {
		VtOrderManagerSelector* orderMgrSelecter = VtOrderManagerSelector::GetInstance();
		VtOrderManager* subOrderMgr = nullptr;
		VtOrder* subAcntOrder = nullptr;

		if (order_req->Type == 1) { // 서브계좌 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
			}

			// 정수로 체결가격 설정
			subAcntOrder->intFilledPrice = GetIntOrderPrice(strSeries, strFillPrice, strOriFill);

			// 체결된 수량
			subAcntOrder->filledQty = _ttoi(strFillAmount);
			// 체결된 시각
			subAcntOrder->filledTime = (LPCTSTR)strFillTime;
			// 체결된 가격 - 소수로 표시된
			subAcntOrder->filledPrice = _ttof(strOriFill);

			subOrderMgr->OnOrderFilledHd(subAcntOrder);
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("서브계좌 체결 확인 : 체결가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 이전주문번호 = %d, 서브계좌번호 = %s, 주문종류 = %s, 체결갯수 = %s, 요청 타입 = %d"), strFillPrice, order->HtsOrderReqID, strSeries, strOrdNo, order_req->NewOrderNo, order_req->SubAccountNo.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strFillAmount, order->RequestType);

		}
		else if (order_req->Type == 2) { // 펀드 주문인 경우
			subOrderMgr = orderMgrSelecter->FindAddOrderManager(order_req->SubAccountNo);
			subAcntOrder = subOrderMgr->FindOrder((LPCTSTR)(strOrdNo));
			if (!subAcntOrder) {
				subAcntOrder = subOrderMgr->CloneOrder(order);
				// 주문 종류를 복사해 준다. 0 : 계좌 주문, 1 : 서브계좌 주문, 2 : 펀드 주문
				subAcntOrder->Type = order_req->Type;
				// 서브계좌로  계좌번호를 바꿔준다.
				subAcntOrder->AccountNo = order_req->SubAccountNo;
				// 서브계좌 번호를 저장해 준다.
				subAcntOrder->SubAccountNo = order_req->SubAccountNo;
				// 부모계좌 번호를 넣어준다.
				subAcntOrder->ParentAccountNo = order->AccountNo;
				// 펀드이름을 넣어준다.
				subAcntOrder->FundName = order_req->FundName;
			}

			// 정수로 체결가격 설정
			subAcntOrder->intFilledPrice = GetIntOrderPrice(strSeries, strFillPrice, strOriFill);

			// 체결된 수량
			subAcntOrder->filledQty = _ttoi(strFillAmount);
			// 체결된 시각
			subAcntOrder->filledTime = (LPCTSTR)strFillTime;
			// 체결된 가격 - 소수로 표시된
			subAcntOrder->filledPrice = _ttof(strOriFill);

			subOrderMgr->OnOrderFilledHd(subAcntOrder);
			SmCallbackManager::GetInstance()->OnOrderEvent(subAcntOrder);

			LOG_F(INFO, _T("펀드주문 체결 확인 : 체결가격 = %s, 요청번호 = %d, 종목이름 = %s, 주문 번호 = %s, 이전주문번호 = %d, 펀드이름 = %s, 주문종류 = %s, 체결갯수 = %s, 요청 타입 = %d"), strFillPrice, order->HtsOrderReqID, strSeries, strOrdNo, order_req->NewOrderNo, order_req->FundName.c_str(), strPosition.Compare(_T("1")) == 0 ? _T("매수") : _T("매도"), strFillAmount, order->RequestType);

		}
	}
}

int VtHdCtrl::AbGetAsset(std::string accountNo, std::string pwd)
{
	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	// 계좌 번호
	temp = PadRight(accountNo, ' ', 6);
	reqString.append(temp);
	// 비밀번호
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);

	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(DEF_Ab_Asset, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAsset);

	return nRqID;
}

int VtHdCtrl::AbGetDeposit(std::string accountNo, std::string pwd)
{
	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	reqString.append("1");
	// 아이디 
	reqString.append(VtLoginManager::GetInstance()->ID);

	temp = PadRight(accountNo, ' ', 6);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);
	// 그룹명 - 공백
	reqString.append("                    ");
	// 통화코드
	reqString.append("USD");


	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(DEF_Ab_AccountProfitLoss, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAsset);

	return nRqID;
}

int VtHdCtrl::AbGetAccountProfitLoss(std::string accountNo, std::string pwd)
{
	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	reqString.append("1");
	// 아이디 
	reqString.append(VtLoginManager::GetInstance()->ID);

	temp = PadRight(accountNo, ' ', 6);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);
	// 그룹명 - 공백
	reqString.append("                    ");
	// 통화코드
	reqString.append("USD");


	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(DEF_Ab_SymbolProfitLoss, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAsset);

	return nRqID;
}

int VtHdCtrl::AbGetOutStanding(std::string accountNo, std::string pwd)
{
	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	reqString.append("1");
	// 아이디 
	reqString.append(VtLoginManager::GetInstance()->ID);

	temp = PadRight(accountNo, ' ', 6);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);
	// 그룹명 - 공백
	reqString.append("                    ");

	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(DEF_Ab_Outstanding, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAsset);

	return nRqID;
}

int VtHdCtrl::AbGetAccepted(std::string accountNo, std::string pwd)
{
	if (_Blocked)
		return -1;

	std::string reqString;
	std::string temp;
	reqString.append("1");
	// 아이디 
	reqString.append(VtLoginManager::GetInstance()->ID);

	temp = PadRight(accountNo, ' ', 6);
	reqString.append(temp);
	temp = PadRight(pwd, ' ', 8);
	reqString.append(temp);
	// 그룹명 - 공백
	reqString.append("                    ");

	CString sInput = reqString.c_str();
	CString strNextKey = _T("");
	int nRqID = m_CommAgent.CommRqData(DEF_Ab_Accepted, sInput, sInput.GetLength(), strNextKey);
	AddRequest(nRqID, HdTaskType::HdAsset);

	return nRqID;
}

std::string VtHdCtrl::PadLeft(int input, char padding, int len)
{
	std::ostringstream out;
	out << std::internal << std::right << std::setfill(padding) << std::setw(len) << input;
	return out.str();
}

std::string VtHdCtrl::PadLeft(std::string input, char padding, int len)
{
	std::ostringstream out;
	out << std::right << std::setfill(padding) << std::setw(len) << input;
	return out.str();
}

std::string VtHdCtrl::PadLeft(double input, char padding, int len, int decimal)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(decimal) << std::right << std::setfill(padding) << std::setw(len) << input;
	return out.str();
}

std::string VtHdCtrl::PadRight(int input, char padding, int len)
{
	std::ostringstream out;
	out << std::internal << std::left << std::setfill(padding) << std::setw(len) << input;
	return out.str();
}

std::string VtHdCtrl::PadRight(std::string input, char padding, int len)
{
	std::ostringstream out;
	out << std::left << std::setfill(padding) << std::setw(len) << input;
	return out.str();
}

std::string VtHdCtrl::PadRight(double input, char padding, int len, int decimal)
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(decimal) << std::left << std::setfill(padding) << std::setw(len) << input;
	return out.str();
}

void VtHdCtrl::OnDataRecv(CString sTrCode, LONG nRqID)
{
	if (_Blocked)
		return;

	CString strMsg;

	if (sTrCode == DEF_AVAILABLE_CODE_LIST)
	{
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
		for (int i = 0; i < nRepeatCnt; i++)
		{
			CString strData1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래가능품목코드");
			CString strData2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래가능품목명");
			TRACE(strData1 + strData2);
			//TRACE(_T("\n"));
			LOG_F(INFO, "거래가능 목록: %s", strData1 + strData2);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// 국내
	///////////////////////////////////////////////////////////////////////////////////////////
	if (sTrCode == DefAbChartData) {
		OnRcvdAbroadChartData(sTrCode, nRqID);
	}
	else if (sTrCode == DefAbsChartData2) {
		OnRcvdAbroadChartData2(sTrCode, nRqID);
	}
	else if (sTrCode == DefChartData) {
		OnRcvdDomesticChartData(sTrCode, nRqID);
	}
	else if (sTrCode == DefSymbolCode)
	{
		OnSymbolCode(sTrCode, nRqID);
	}
	else if (sTrCode == DefSymbolMaster)
	{
		OnSymbolMaster(sTrCode, nRqID);
	}
	else if (sTrCode == DefPutOrder)
	{
		OnNewOrderHd(sTrCode, nRqID);
	}
	else if (sTrCode == DefModifyOrder)
	{
		OnModifyOrderHd(sTrCode, nRqID);
	}
	else if (sTrCode == DefCancelOrder)
	{
		OnCancelOrderHd(sTrCode, nRqID);
	}
	else if (sTrCode == DefFilledHistoryTable)
	{
		OnFilledHistoryTable(sTrCode, nRqID);
	}
	else if (sTrCode == DefOutstandingHistory)
	{
		OnOutstandingHistory(sTrCode, nRqID);
	}
	else if (sTrCode == DefOutstanding)
	{
		OnOutstanding(sTrCode, nRqID);
	}
	else if (sTrCode == DefAcceptedHistory)
	{
		OnAcceptedHistory(sTrCode, nRqID);
	}
	else if (sTrCode == DefAsset)
	{
		OnAsset(sTrCode, nRqID);
	}
	else if (sTrCode == DefDeposit)
	{
		OnDeposit(sTrCode, nRqID);
	}
	else if (sTrCode == DefChartData || sTrCode == DefAbChartData)
	{
		//OnChartData(sTrCode, nRqID);
		OnReceiveChartData(sTrCode, nRqID);
	}
	else if (sTrCode == DefDailyProfitLoss)
	{
		OnDailyProfitLoss(sTrCode, nRqID);
	}
	else if (sTrCode == DefAccountProfitLoss)
	{
		OnAccountProfitLoss(sTrCode, nRqID);
	}
	else if (sTrCode == DefApiCustomerProfitLoss)
	{
		OnApiCustomerProfitLoss(sTrCode, nRqID);
	}
	else if (sTrCode == DefServerTime) {
		OnServerTime(sTrCode, nRqID);
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// 해외
	///////////////////////////////////////////////////////////////////////////////////////////
	else if (sTrCode == DEF_Ab_Asset)
	{
		OnAbGetAsset(sTrCode, nRqID);
	}
	else if (sTrCode == DefAbQuote)
	{
		OnAbQuote(sTrCode, nRqID);
	}
	else if (sTrCode == DefAbHoga)
	{
		OnAbHoga(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_HW_ORD_CODE_NEW)
	{
		AbOnNewOrderHd(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_HW_ORD_CODE_MOD)
	{
		AbOnModifyOrderHd(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_HW_ORD_CODE_CNL)
	{
		AbOnCancelOrderHd(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_Ab_AccountProfitLoss) {
		OnAbGetDeposit(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_Ab_SymbolProfitLoss) {
		OnAbGetAccountProfitLoss(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_Ab_Outstanding) {
		OnAbGetOutStanding(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_Ab_Accepted) {
		OnAbGetAccepted(sTrCode, nRqID);
	}
	else if (sTrCode == DEF_FX_ORD_CODE_MOD) {
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
		CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");

		CString strMsg;
		strMsg.Format("주문응답 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
	}
	else if (sTrCode == DEF_FX_ORD_CODE_CNL) {
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
		CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");

		CString strMsg;
		strMsg.Format("주문응답 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
	}
	else if (sTrCode == DEF_HW_MSTINFO)
	{
		// OCX에서 /mst/JMCODE.cod 파일을 생성시킨다.
		//WriteLog("해외 종목 서비스 요청 완료!!!");
	}
	///////////////////////////////////////////////////////////////////////////////////////////
	// FX마진		//@lhe 2012.06.22
	///////////////////////////////////////////////////////////////////////////////////////////
	else if (sTrCode == DEF_FX_JANGO)
	{
		CString strMsg;
		strMsg.Format("FX 계좌정보 : 자산내역조회[%s]", DEF_FX_JANGO);
		//WriteLog(strMsg);

		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString strAcctNm = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌명");
			CString strEntrTot = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁총액");
			CString strTrstMgn = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "위탁증거금");
			CString strMntMgn = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "유지증거금");

			strAcctNm.TrimRight();
			strMsg.Format("계좌명[%s] 예탁총액[%s] 위탁증거금[%s] 유지증거금[%s]", strAcctNm, strEntrTot, strTrstMgn, strMntMgn);
			//WriteLog(strMsg);
		}
	}
	else if (sTrCode == DEF_FX_FID_CODE)
	{
		CString strMsg;
		strMsg.Format("FX 마스터데이타[%s]", DEF_FX_FID_CODE);
		//WriteLog(strMsg);

		CString strSeries = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "종목코드");
		CString strOffer = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매도가격");
		CString strBid = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "매수가격");
		CString strTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "호가시간");

		//m_edSeriesO.SetWindowText(strSeries);
		//m_edTimeO.SetWindowText(strTime);
		//m_edClosePO.SetWindowText(strOffer);
		//m_edVolumeO.SetWindowText(strBid);

		strMsg.Format("FID 종목[%s]시간[%s]매도가격[%s]매수가격[%s]", strSeries, strTime, strOffer, strBid);
		//WriteLog(strMsg);
	}
	else if (sTrCode == DEF_FX_ORD_CODE_NEW)
	{
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		CString strExchTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "접수구분");
		CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
		CString strOrdNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문번호");

		CString strMsg;
		strMsg.Format("주문응답 번호[%d][%s]처리[%s]계좌번호[%s]주문번호[%s]", nRqID, strExchTp, strProcTp, strAcctNo, strOrdNo);
		//WriteLog(strMsg);
	}
	else if (sTrCode == "o44005")
	{
		// 해외 차트 조회
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");

		CString strMsg;
		strMsg.Format("응답개수[%d]", nRepeatCnt);
		//WriteLog(strMsg);

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "일자");
			CString sTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "시간");
			CString sCloseP = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "종가");
			CString sVolume = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "체결량");

			strMsg.Format("[%s][%s][%s][%s]", sDate, sTime, sCloseP, sVolume);
			//WriteLog(strMsg);
		}
	}
	// 20140328 계좌정보 수신 부분 추가. - 20140328 sivas
	else if (sTrCode == DEF_ACCT_INFO)
	{
		CString strMsg = "계좌정보 수신완료!!!";
		//WriteLog(strMsg);

		// 계좌 구분 추가. - 20140331 sivas
		typedef	struct
		{
			char 	szAcctNo[11];		// 계좌번호
			char	szAcctNm[30];		// 계좌명
			char	szAcctGb[01];		// 계좌구분  '1': 해외, '2': FX, '9':국내
		}HDF_ACCOUNT_UNIT;

		typedef struct
		{
			char szCount[5];
			HDF_ACCOUNT_UNIT *pHdfAccUnit;
		}HDF_ACCOUNT_INFO;

		HDF_ACCOUNT_INFO *pHdfAccInfo = NULL;
		HDF_ACCOUNT_UNIT *pHdfAccUnit = NULL;
		CString strRcvBuff = m_CommAgent.CommGetAccInfo();

		VtAccountManager* acntMgr = VtAccountManager::GetInstance();
		pHdfAccInfo = (HDF_ACCOUNT_INFO *)strRcvBuff.GetBuffer();
		CString strCount(pHdfAccInfo->szCount, sizeof(pHdfAccInfo->szCount));
		for (int i = 0; i<atoi(strCount); i++) {
			pHdfAccUnit = (HDF_ACCOUNT_UNIT *)(pHdfAccInfo->szCount + sizeof(pHdfAccInfo->szCount) + (sizeof(HDF_ACCOUNT_UNIT) * i));
			CString strAcctNo(pHdfAccUnit->szAcctNo, sizeof(pHdfAccUnit->szAcctNo));
			CString strAcctNm(pHdfAccUnit->szAcctNm, sizeof(pHdfAccUnit->szAcctNm));
			CString strAcctGb(pHdfAccUnit->szAcctGb, sizeof(pHdfAccUnit->szAcctGb));// 계좌 구분 추가. - 20140331 sivas

			strMsg.Format("[%s][%s][%s]\n", strAcctNo, strAcctNm, strAcctGb);
			//WriteLog(strMsg);
			//TRACE(strMsg);
			if (strAcctGb.Compare(_T("9")) == 0 || strAcctGb.Compare(_T("1")) == 0) {
				VtAccountInfo acnt_info;
				acnt_info.account_no = (LPCTSTR)strAcctNo.TrimRight();
				acnt_info.account_name = (LPCTSTR)strAcctNm.TrimRight();
				acnt_info.account_type = _ttoi(strAcctGb);
				acntMgr->ServerAccountMap[acnt_info.account_no] = acnt_info;
				VtRealtimeRegisterManager* regMgr = VtRealtimeRegisterManager::GetInstance();
				regMgr->RegisterAccount(acnt_info.account_no);
			}

		}
	}
	else if (sTrCode == "l41601")
	{
		// input = "1012014051620020"
		// req = "000001002003004005006007008009"
		int nRepeatCnt = 20;	// 조회시 요청한 개수
		if (nRepeatCnt > 0)
		{
			typedef struct
			{
				char		baseid[8];		/*  [기초자산ID]기초자산ID     */
				char		date[8];		/*  [일자]일자     */
				char		price[6];		/*  [현재가]현재가     */
				char		sign[1];		/*  [대비구분]대비구분     */
				char		change[6];		/*  [대비]대비     */
				char		open[6];		/*  [시가]시가     */
				char		high[6];		/*  [고가]고가     */
				char		low[6];			/*  [저가]저가     */
				char		volume[15];		/*  [누적거래량]누적거래량     */
				char		jnilclose[6];	/*  [전일종가]     */

			}HDF_I41601;
			int nBuffSize = sizeof(HDF_I41601);
			CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 0, nRepeatCnt * nBuffSize, 0, "A");

			// Struct만큼 잘라서 사용하세요...

			//WriteLog(strBuff);
		}
	}
	else if (sTrCode == "g11004.AQ0207%")
	{
		CString strProcTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
		CString strProMsg = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
		CString strAcctNo = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
		CString strAcctNm = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌명");
		CString strTp = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "구분");
		CString strYN = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "일치여부");

		CString strBuff;
		strBuff.Format("[%s][%s][%s][%s][%s][%s]", strProcTp, strProMsg, strAcctNo, strAcctNm, strTp, strYN);

		//WriteLog(strBuff);
	}
	else if (sTrCode == "o51100")
	{
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
			CString sTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목명");
			CString sCloseP = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "현재가");
			CString sVolume = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "전일대비구분");

			CString strMsg;
			strMsg.Format("[%s][%s][%s][%s]", sDate, sTime, sCloseP, sVolume);
			//WriteLog(strMsg);
		}
	}
	else if (sTrCode == "o51200")
	{
		CString strMsg;
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "일자");
			CString sTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "시간");
			CString sCloseP = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종가");
			CString sVolume = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "체결량");

			strMsg.Format("[%s][%s][%s][%s]", sDate, sTime, sCloseP, sVolume);
			//WriteLog(strMsg);
		}

		int nBuffSize = 129 + 16;	// Fid조회는 필드 마지막에 구분자 1자리가 있으므로 각 필드 만큼 더해준다.
		CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 0, nRepeatCnt * nBuffSize, 0, "A");
		strMsg.Format("[%s]", strBuff);
		//WriteLog(strMsg);

		CString strNextKey = m_CommAgent.CommGetNextKey(nRqID, "");
		if (strNextKey != "")
		{
			int nRet = MessageBox("조회를 계속 할까요?", "현대선물", MB_YESNO);
			if (nRet == IDYES)
			{
				//m_strNextKey = strNextKey;
				//OnBnClickedRqtest();
			}
		}
	}
	else if (sTrCode == "g11004.AQ0401%")
	{
		CString strMsg;
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문번호");
			CString sTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
			CString sCloseP = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
			CString sVolume = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문가격");

			strMsg.Format("[%s][%s][%s][%s]", sDate, sTime, sCloseP, sVolume);
			//WriteLog(strMsg);
		}

		int nBuffSize = 226;
		CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 0, nRepeatCnt * nBuffSize, 0, "A");
		strMsg.Format("[%s]", strBuff);
		//WriteLog(strMsg);
	}
	else if (sTrCode == "g11004.AQ0408%")
	{
		CString strMsg;
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sData01 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "처리일자");
			CString sData02 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문번호");
			CString sData03 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문/체결구분");
			CString sData04 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목코드");
			CString sData05 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "매매구분");
			CString sData06 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문수량");
			CString sData07 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "정정수량");
			CString sData08 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "취소수량");
			CString sData09 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "체결수량");
			CString sData10 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문잔량");
			CString sData11 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문/체결가격");
			CString sData12 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "가격조건");
			CString sData13 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "통신주문구분");
			CString sData14 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "주문전략구분");
			CString sData15 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래소접수/거부구분");
			CString sData16 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래소접수/거부처리시간");
			CString sData17 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래소접수번호/거부코드");
			CString sData18 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "거래소주문상태");
			CString sData19 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "작업일자");
			CString sData20 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "작업시간");
			CString sData21 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "작업사원");
			CString sData22 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "사설IP");
			CString sData23 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "공인IP");
			CString sData24 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "처리일자");
			CString sData25 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "최초원주문번호");
			CString sData26 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "stop가격");


			strMsg.Format("[%s][%s][%s][%s]", sData01, sData02, sData03, sData04);
			//WriteLog(strMsg);
		}

		int nBuffSize = 226;
		CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 0, nRepeatCnt * nBuffSize, 0, "A");
		strMsg.Format("[%s]", strBuff);
		//WriteLog(strMsg);


		CString strNextKey = m_CommAgent.CommGetNextKey(nRqID, "");
		if (strNextKey != "")
		{
			int nRet = MessageBox("조회를 계속 할까요?", "현대선물", MB_YESNO);
			if (nRet == IDYES)
			{
				//m_strNextKey = strNextKey;
				//OnBnClickedRqtest();
			}
		}
	}
	else if (sTrCode == "o44011")	// 서버 현재시간
	{
		CString sDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "서버일자");
		CString sTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "서버시간");

		strMsg.Format("서버날짜[%s]서버시간[%s]", sDate, sTime);
		//WriteLog(strMsg);
	}
	else if (sTrCode == "n51003")
	{
		// 공지사항 조회
		CString sTemp1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "제목");

		strMsg.Format("[%s]", sTemp1);
		//WriteLog(strMsg);

		CString strNextKey = m_CommAgent.CommGetNextKey(nRqID, "");
		if (strNextKey != "")
		{
			int nRet = MessageBox("n51003 조회를 계속 할까요?", "현대선물", MB_YESNO);
			if (nRet == IDYES)
			{
				//m_strNextKey = strNextKey;
				//OnBnClickedRqtest();
			}
		}
	}
	else if (sTrCode == "g11002.DQ0321&")
	{
		CString sTemp1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리코드");
		CString sTemp2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "처리메시지");
		CString sTemp3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
		CString sTemp4 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "일치여부");
		CString sTemp5 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌구분");

		strMsg.Format("처리코드[%s]처리메시지[%s]계좌번호[%s]일치여부[%s]계좌구분[%s]", sTemp1, sTemp2, sTemp3, sTemp4, sTemp5);
		//WriteLog(strMsg);
	}
	else if (sTrCode == "n51006")
	{
		CString strMsg;
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		CString sLen = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "전체사이즈");

		int nBuffSize = 10 + 1 + 255 + atoi(sLen);
		CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 0, nBuffSize, 0, "A");
		strMsg.Format("[%s]", strBuff);
		//WriteLog(strMsg);
		AfxMessageBox(strMsg);
	}
	else if (sTrCode == "l41600")
	{
		/***
		if ( nRepeatCnt > 0 )
		{
		typedef struct
		{
		char		baseid[8];		//  [기초자산ID]기초자산ID
		char		date[8];		//  [일자]일자
		char		price[6];		//  [현재가]현재가
		char		sign[1];		//  [대비구분]대비구분
		char		change[6];		//  [대비]대비
		char		open[6];		//  [시가]시가
		char		high[6];		//  [고가]고가
		char		low[6];			//  [저가]저가
		char		volume[15];		//  [누적거래량]누적거래량
		char		jnilclose[6];	//  [전일종가]

		}HDF_I41601;
		int nBuffSize = sizeof(HDF_I41601);
		CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 0, nRepeatCnt * nBuffSize, 0, "A");

		// Struct만큼 잘라서 사용하세요...
		WriteLog(strBuff);
		}
		***/

		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
		CString sLen1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "품목명");
		CString sLen2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "체결시간");
		CString sLen3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "현재가");

		//WriteLog(sLen1 + sLen2 + sLen3);
	}
	else if (sTrCode == "g11002.CQ0101&")
	{
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");
		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString strAcctNm = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "사용자ID");
			CString strEntrTot = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
			CString strTrstMgn = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "대출구분");
			CString strMntMgn = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "TMP1");

			strAcctNm.TrimRight();
			strMsg.Format("사용자ID[%s] 계좌번호[%s] 대출구분[%s] TMP1[%s]", strAcctNm, strEntrTot, strTrstMgn, strMntMgn);
			//WriteLog(strMsg);
		}
	}
	else if (sTrCode == "o44010")
	{
		// 해외 차트 조회
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec2");

		CString sRcvCnt = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "수신봉개수");

		nRepeatCnt = atoi(sRcvCnt);

		CString strMsg;
		strMsg.Format("응답개수[%d]", nRepeatCnt);
		//WriteLog(strMsg);

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sDate = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "시작일자");
			CString sTime = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "시작시간");
			CString sCloseP = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "종가");
			CString sVolume = m_CommAgent.CommGetData(sTrCode, -1, "OutRec2", i, "체결량");

			strMsg.Format("[%s][%s][%s][%s]", sDate, sTime, sCloseP, sVolume);
			//WriteLog(strMsg);
		}
	}
	else if (sTrCode == "v90001")
	{
		// 종목정보 요청 기능.
		long nFileSize = atol(m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "파일크기"));
		CString strMsg;
		strMsg.Format("파일크기[%d]", nFileSize);
		//WriteLog(strMsg);


		CString strFileNm = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "파일명");
		CString strProcCd = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "응답코드");
		LOG_F(INFO, "symbol file download :: file_name = %s, code = %s", strFileNm, strProcCd);
		if (strProcCd == "REOK")
		{
			ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
			std::string appPath = configMgr->GetAppPath();
			std::string configPath = appPath;
			configPath.append(_T("\\mst\\"));
			configPath.append(strFileNm);
			CString strCommonFileName = configPath.c_str();

			CFile commonfile;
			// open file
			if (!commonfile.Open(strCommonFileName, CFile::modeWrite /*| CFile::typeBinary*/))
			{
				if (commonfile.Open(strCommonFileName, CFile::modeCreate | CFile::modeWrite /*| CFile::typeBinary*/) == FALSE)
				{
					CString strMsg;
					strMsg.Format("%s화일 생성에 실패하였습니다. ", strCommonFileName);
					LOG_F(INFO, "symbol file download error :: file_name = %s", strMsg);
					return;
				}
			}

			CString strBuff = m_CommAgent.CommGetDataDirect(sTrCode, -1, 128 + 4 + 8, nFileSize, 0, "A");
			commonfile.Write(strBuff, nFileSize);
			commonfile.Close();
			auto it = SmSymbolReader::GetInstance()->DomesticSymbolMasterFileSet.find((LPCTSTR)strFileNm);
			if (it != SmSymbolReader::GetInstance()->DomesticSymbolMasterFileSet.end()) {
				SmSymbolReader::GetInstance()->DomesticSymbolMasterFileSet.erase(it);
			}

			

			CString strNextKey = m_CommAgent.CommGetNextKey(nRqID, "");

			RemoveRequest(nRqID);

		}

		Sleep(VtGlobal::ServerSleepTime);
		HdTaskEventArgs eventArg;
		eventArg.TaskType = HdTaskType::HdSymbolFileDownload;
		FireTaskCompleted(std::move(eventArg));
		//WriteLog("처리완료");
	}
	else if (sTrCode == "g11002.DQ0242&")
	{
		CString sTemp1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
		CString sTemp2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "예탁금액-총액");
		CString sTemp3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "주문가능금액");

		strMsg.Format("계좌번호[%s]예탁금액-당일손익[%s]주문가능금액[%s]", sTemp1, sTemp2, sTemp3);
		//WriteLog(strMsg);
	}
	else if (sTrCode == "g11002.DQ1305&" || sTrCode == "g11002.DQ1306&")
	{
		CString strMsg;
		int nRepeatCnt = m_CommAgent.CommGetRepeatCnt(sTrCode, -1, "OutRec1");

		for (int i = 0; i<nRepeatCnt; i++)
		{
			CString sTemp1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "계좌번호");
			CString sTemp2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "종목");
			CString sTemp3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", i, "평가손익");

			strMsg.Format("계좌번호[%s]종목[%s]평가손익[%s]", sTemp1, sTemp2, sTemp3);
			//WriteLog(strMsg);
		}
	}
	else if (sTrCode == "g11002.DQ1303&")
	{
		CString sTemp1 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "계좌번호");
		CString sTemp2 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "가결제금액");
		CString sTemp3 = m_CommAgent.CommGetData(sTrCode, -1, "OutRec1", 0, "평가예탁총액");

		strMsg.Format("계좌번호[%s]예탁금액현금[%s]평가예탁총액[%s]", sTemp1, sTemp2, sTemp3);
		//WriteLog(strMsg);
	}

	auto it = _TaskReqMap.find(nRqID);
	if (it != _TaskReqMap.end()) {
		std::shared_ptr<HdTaskArg> arg = it->second;
		SmTaskManager::GetInstance()->OnRequestComplete(arg);

		CString msg;
		msg.Format(_T("OnDataRcvd :: nRqId = %d, strCode = %s\n"), nRqID, sTrCode);
		TRACE(msg);
	}
}

void VtHdCtrl::OnGetBroadData(CString strKey, LONG nRealType)
{
	if (_Blocked)
		return;

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	switch (nRealType)
	{
		/////////////////////////////////////////////////////////////////////////////////////////////////
		// 국내
		/////////////////////////////////////////////////////////////////////////////////////////////////
	case 51:	// 선물호가
	{
		OnFutureHoga(strKey, nRealType);
	}
	break;
	case 52:	// 옵션호가
	{
		OnOptionHoga(strKey, nRealType);
	}
	break;
	case 58: //상품선물호가실시간
	{
		OnProductHoga(strKey, nRealType);
	}
	break;
	case 65:	// 선물체결
	{
		OnRealFutureQuote(strKey, nRealType);
	}
	break;
	case 71:	// 상품선물체결
	{
		OnRealProductQuote(strKey, nRealType);
	}
	break;
	case 66:	// 옵션체결
	{
		OnRealOptionQuote(strKey, nRealType);
	}
	break;

	case 121:
	{
		OnRealPosition(strKey, nRealType);
	}
	break;
	case 310: {
		OnExpected(strKey, nRealType);
		break;
	}

	case 183:	// 미결제
	{
		OnRemain(strKey, nRealType);
	}
	break;
	case 261:	// 주문접수
	{
		OnOrderAcceptedHd(strKey, nRealType);
	}
	break;

	case 262:	// 주문미체결
	{
		OnOrderUnfilledHd(strKey, nRealType);
	}
	break;
	case 265:	// 주문체결
	{
		OnOrderFilledHd(strKey, nRealType);
	}
	break;

	/////////////////////////////////////////////////////////////////////////////////////////
	// 해외				        /////////////////////////////////////////////////////////////////////////////////////////
	case 76:	//해외호가
	{
		CString	strData000 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");

		VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
		VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strData000.Trim());
		if (!sym)
			return;

		CString	strData002 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "호가시간");


		LOG_F(INFO, _T("종목코드 = %s"), strData000);

		CString	strData075 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "호가수신시간");
		CString	strData076 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가1");
		CString	strData077 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가1");
		CString	strData078 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량1");
		CString	strData079 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량1");
		CString	strData080 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수1");
		CString	strData081 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수1");

		sym->Hoga.Ary[0].IntSellPrice = _ttoi(strData076);
		sym->Hoga.Ary[0].IntBuyPrice = _ttoi(strData077);
		sym->Hoga.Ary[0].SellQty = _ttoi(strData078);
		sym->Hoga.Ary[0].BuyQty = _ttoi(strData079);
		sym->Hoga.Ary[0].SellNo = _ttoi(strData080);
		sym->Hoga.Ary[0].BuyNo = _ttoi(strData081);


		CString	strData082 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가2");
		CString	strData083 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가2");
		CString	strData084 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량2");
		CString	strData085 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량2");
		CString	strData086 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수2");
		CString	strData087 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수2");

		sym->Hoga.Ary[1].IntSellPrice = _ttoi(strData082);
		sym->Hoga.Ary[1].IntBuyPrice = _ttoi(strData083);
		sym->Hoga.Ary[1].SellQty = _ttoi(strData084);
		sym->Hoga.Ary[1].BuyQty = _ttoi(strData085);
		sym->Hoga.Ary[1].SellNo = _ttoi(strData086);
		sym->Hoga.Ary[1].BuyNo = _ttoi(strData087);
		CString	strData088 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가3");
		CString	strData089 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가3");
		CString	strData090 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량3");
		CString	strData091 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량3");
		CString	strData092 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수3");
		CString	strData093 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수3");

		sym->Hoga.Ary[2].IntSellPrice = _ttoi(strData088);
		sym->Hoga.Ary[2].IntBuyPrice = _ttoi(strData089);
		sym->Hoga.Ary[2].SellQty = _ttoi(strData090);
		sym->Hoga.Ary[2].BuyQty = _ttoi(strData091);
		sym->Hoga.Ary[2].SellNo = _ttoi(strData092);
		sym->Hoga.Ary[2].BuyNo = _ttoi(strData093);
		CString	strData094 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가4");
		CString	strData095 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가4");
		CString	strData096 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량4");
		CString	strData097 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량4");
		CString	strData098 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수4");
		CString	strData099 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수4");

		sym->Hoga.Ary[3].IntSellPrice = _ttoi(strData094);
		sym->Hoga.Ary[3].IntBuyPrice = _ttoi(strData095);
		sym->Hoga.Ary[3].SellQty = _ttoi(strData096);
		sym->Hoga.Ary[3].BuyQty = _ttoi(strData097);
		sym->Hoga.Ary[3].SellNo = _ttoi(strData098);
		sym->Hoga.Ary[3].BuyNo = _ttoi(strData099);
		CString	strData100 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가5");
		CString	strData101 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가5");
		CString	strData102 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가수량5");
		CString	strData103 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가수량5");
		CString	strData104 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가건수5");
		CString	strData105 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가건수5");
		sym->Hoga.Ary[4].IntSellPrice = _ttoi(strData100);
		sym->Hoga.Ary[4].IntBuyPrice = _ttoi(strData101);
		sym->Hoga.Ary[4].SellQty = _ttoi(strData102);
		sym->Hoga.Ary[4].BuyQty = _ttoi(strData103);
		sym->Hoga.Ary[4].SellNo = _ttoi(strData104);
		sym->Hoga.Ary[4].BuyNo = _ttoi(strData105);

		CString	strData106 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가총수량");
		CString	strData107 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가총수량");
		CString	strData108 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가총건수");
		CString	strData109 = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가총건수");

		sym->Hoga.TotSellQty = _ttoi(strData106);
		sym->Hoga.TotBuyQty = _ttoi(strData107);
		sym->Hoga.TotSellNo = _ttoi(strData108);
		sym->Hoga.TotBuyNo = _ttoi(strData109);

		SmCallbackManager::GetInstance()->OnHogaEvent(sym);
	}
	break;
	case 82:	//해외체결
	{
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "기준체결시간");
		CString strCloseP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결가");
		CString strOpen = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "시가");
		CString strHigh = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "고가");
		CString strLow = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "저가");
		CString strVolume = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결량");
		CString strUpdown = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "체결구분");
		CString strAccAmount = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "누적거래량");
		CString strPreDayCmp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "전일대비");

		//m_edSeriesO.SetWindowText(strSeries);
		//m_edTimeO.SetWindowText(strTime);
		CString strFormatPrice = strCloseP;
		strFormatPrice = m_CommAgent.CommGetHWOrdPrice(strSeries, strCloseP, 0);
		//m_edClosePO.SetWindowText(strFormatPrice);
		//m_edVolumeO.SetWindowText(strVolume);

		CString strType1, strType2, strType3, strType4;
		strCloseP = strCloseP;//"1241300";
		strType1 = m_CommAgent.CommGetHWOrdPrice(strSeries, strCloseP, 0);
		strType2 = m_CommAgent.CommGetHWOrdPrice(strSeries, strType1, 1);
		strType3 = m_CommAgent.CommGetHWOrdPrice(strSeries, strType2, 2);
		strType4 = m_CommAgent.CommGetHWOrdPrice(strSeries, strType3, 0);

		CString strMsg;
		strMsg.Format("Real 82 Recv[%s]->[%s]->[%s]->[%s]->[%s]", strCloseP, strType1, strType2, strType3, strType4);

		//WriteLog(strMsg);

		CString strBuff = m_CommAgent.CommGetDataDirect(strKey, nRealType, 0, 230, 0, "A");
		CString strSeries1(strBuff, 10);
		strMsg = strSeries1;
		//WriteLog(strMsg);

		VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
		VtSymbol* sym = symMgr->FindSymbol((LPCTSTR)strSeries.Trim());
		if (!sym)
			return;

		sym->Quote.intClose = _ttoi(strCloseP);
		sym->Quote.intOpen = _ttoi(strOpen);
		sym->Quote.intHigh = _ttoi(strHigh);
		sym->Quote.intLow = _ttoi(strLow);
		sym->AccAmount = _ttoi(strAccAmount);
		sym->ComToPrev = _ttoi(strPreDayCmp);

		VtQuoteItem quoteItem;

		if (strUpdown.Compare(_T("+")) == 0)
			quoteItem.MatchKind = 1;
		else
			quoteItem.MatchKind = 2;


		quoteItem.ClosePrice = _ttoi(strCloseP);

		strTime.Insert(2, ':');
		strTime.Insert(5, ':');

		quoteItem.Time = strTime;


		quoteItem.ContQty = _ttoi(strVolume);


		quoteItem.Decimal = sym->Decimal;

		sym->Quote.QuoteItemQ.push_front(quoteItem);

		// 여기서 종목의 포지션을 업데이트 해 준다. 
		VtOrderManagerSelector* orderMgrSelector = VtOrderManagerSelector::GetInstance();
		for (auto it = orderMgrSelector->_OrderManagerMap.begin(); it != orderMgrSelector->_OrderManagerMap.end(); ++it)
		{
			VtOrderManager* orderMgr = it->second;
			orderMgr->OnReceiveQuoteHd(sym);
		}

		// 차트 데이터를 업데이트 해준다.
		VtChartDataManager* chartDataMgr = VtChartDataManager::GetInstance();
		chartDataMgr->OnReceiveQuoteHd(sym);

		SmCallbackManager::GetInstance()->OnQuoteEvent(sym);
	}
	break;
	case 196: //해외주문접수
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");

		strOrdNo.Trim();
		strSeries.Trim();
		strOrdP.Trim();
		strOrdQ.Trim();

		CString strErrCd = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "처리코드");
		CString strErrMsg = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "처리메세지");
		strErrCd.Trim();
		strErrMsg.Trim();

		CString strMsg;

		strMsg.Format("[%d] 주문접수코드[%s][%s]", nRealType, strErrCd, strErrMsg);
		//WriteLog(strMsg);

		strMsg.Format("[%d] 주문접수 번호[%s]종목[%s]매매[%s]가격[%s]수량[%s]", nRealType, strOrdNo, strSeries, strBySlTp, strOrdP, strOrdQ);
		//WriteLog(strMsg);

		m_strOrdNo = strOrdNo;
	}
	break;
	case 186: //해외미체결내역
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");

		strOrdNo.TrimRight();
		strSeries.TrimRight();
		strOrdP.TrimRight();
		strOrdQ.TrimRight();

		if (m_strOrdNo == strOrdNo)
		{
			CString strMsg;
			strMsg.Format("미체결 주문번호[%s]종목[%s]매매[%s]가격[%s]수량[%s]", strOrdNo, strSeries, strBySlTp, strOrdP, strOrdQ);
			//WriteLog(strMsg);
		}
	}
	break;
	case 187: //해외미결제내역
	{

	}
	break;
	case 189: //해외체결내역
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");

		strOrdNo.TrimRight();
		strSeries.TrimRight();
		strOrdP.TrimRight();
		strOrdQ.TrimRight();

		if (m_strOrdNo == strOrdNo)
		{
			CString strMsg;
			strMsg.Format("체결 주문번호[%s]종목[%s]매매[%s]가격[%s]수량[%s]", strOrdNo, strSeries, strBySlTp, strOrdP, strOrdQ);
			//WriteLog(strMsg);
		}
	}
	break;
	case 296: // 해외 주문 접수
		AbOnOrderAcceptedHd(strKey, nRealType);
		break;
	case 286: // 해외 주문 미체결
		AbOnOrderUnfilledHd(strKey, nRealType);
		break;
	case 289: // 해외 주문 체결
		AbOnOrderFilledHd(strKey, nRealType);
		break;
	// FX마진		//@lhe 2012.05.16
	case 171: //FX 시세
	{
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "단축코드");
		CString strTime = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "호가시간");
		CString strOffer = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매도호가");
		CString strBid = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매수호가");

		//m_edSeriesO.SetWindowText(strSeries);
		//m_edTimeO.SetWindowText(strTime);
		//m_edClosePO.SetWindowText(strOffer);
		//m_edVolumeO.SetWindowText(strBid);

		CString strMsg;
		strMsg.Format("FX시세  종목[%s] 호가시간[%s] 매도호가[%s]매수호가[%s]", strSeries, strTime, strOffer, strBid);

		//WriteLog(strMsg);
	}
	break;
	case 191: //FX미체결내역
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");

		strOrdNo.TrimRight();
		strSeries.TrimRight();
		strOrdP.TrimRight();
		strOrdQ.TrimRight();

		if (m_strOrdNo == strOrdNo)
		{
			CString strMsg;
			strMsg.Format("미체결 주문번호[%s]종목[%s]매매[%s]가격[%s]수량[%s]", strOrdNo, strSeries, strBySlTp, strOrdP, strOrdQ);
			//WriteLog(strMsg);
		}
	}
	break;
	case 192: //FX미청산내역
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "진입가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "진입수량");

		strOrdNo.TrimRight();
		strSeries.TrimRight();
		strOrdP.TrimRight();
		strOrdQ.TrimRight();

		if (m_strOrdNo == strOrdNo)
		{
			CString strMsg;
			strMsg.Format("미청산 주문번호[%s]종목[%s]매매[%s]가격[%s]수량[%s]", strOrdNo, strSeries, strBySlTp, strOrdP, strOrdQ);
			//WriteLog(strMsg);
		}
	}
	break;
	case 193: //FX청산내역
	{
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "청산가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "청산수량");

		strSeries.TrimRight();
		strOrdP.TrimRight();
		strOrdQ.TrimRight();

		CString strMsg;
		strMsg.Format("청산 종목[%s]매매[%s]가격[%s]수량[%s]", strSeries, strBySlTp, strOrdP, strOrdQ);
		//WriteLog(strMsg);
	}
	break;
	case 197: //FX주문접수
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strErrcd = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "처리코드");
		CString strErrMsg = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "처리메세지");

		strOrdNo.TrimRight();
		strErrcd.TrimRight();
		strErrMsg.TrimRight();

		CString strMsg;
		strMsg.Format("주문접수 번호[%s]처리코드[%s]처리메시지[%s]", strOrdNo, strErrcd, strErrMsg);
		//WriteLog(strMsg);

		//m_strOrdNo = strOrdNo;
	}
	break;
	case 101:
	{
		CString strCode = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "업종코드");
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "지수");
		CString strErrcd = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "전일비");
		CString strErrMsg = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "등락율");
		strCode.TrimRight();
		strOrdNo.TrimRight();
		strErrcd.TrimRight();
		strErrMsg.TrimRight();

		CString strMsg;
		strMsg.Format("code=%s,지수[%s]전일비[%s]등락율[%s]\n", strCode,strOrdNo, strErrcd, strErrMsg);
		//WriteLog(strMsg);
		TRACE(strMsg);
		VtSymbolManager* symMgr = VtSymbolManager::GetInstance();
		if (strCode.CompareNoCase(_T("001")) == 0)
			symMgr->KospiCurrent = _ttoi(strOrdNo);
		if (strCode.CompareNoCase(_T("101")) == 0)
			symMgr->Kospi200Current = _ttoi(strOrdNo);
		if (strCode.CompareNoCase("106") == 0)
			symMgr->Kosdaq150Current = _ttoi(strOrdNo);

	}
	break;
	case 199:
	{
		CString strOrdNo = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문번호");
		CString strSeries = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "종목코드");
		CString strBySlTp = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "매매구분");
		CString strOrdP = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가격");
		CString strOrdQ = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문수량");

		strOrdNo.TrimRight();
		strSeries.TrimRight();
		strOrdP.TrimRight();
		strOrdQ.TrimRight();

		CString strMsg;
		strMsg.Format("Yes미체결 주문번호[%s]종목[%s]매매[%s]가격[%s]수량[%s]", strOrdNo, strSeries, strBySlTp, strOrdP, strOrdQ);
		//WriteLog(strMsg);
	}
	break;
	case 208:
	{
		CString strBuff = m_CommAgent.CommGetDataDirect("0", nRealType, 0, 15, 0, "A");
		//WriteLog(strBuff);
	}
	break;
	case 184:	// 국내실시간잔고
	case 188:	// 해외실시간잔고
	{
		CString strAccno = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "계좌번호");
		CString strAmt = m_CommAgent.CommGetData(strKey, nRealType, "OutRec1", 0, "주문가능금액");
		CString strMsg;
		strMsg.Format("리얼번호[%d]계좌번호[%s]주문가능금액[%s]", nRealType, strAccno, strAmt);
		//WriteLog(strMsg);
	}
	break;
	default:
	{
		CString strMsg;
		strMsg.Format("[%d] 알수없는 실시간 요청값", nRealType);
		//WriteLog(strMsg);
	}
	break;
	}
}

void VtHdCtrl::OnGetMsg(CString strCode, CString strMsg)
{
	if (!_ServerConnected || _Blocked)
		return;

	CString strLog;
	strLog.Format("OnGetMsg[코드번호 = %s][메시지 = %s]\n", strCode, strMsg);
	std::string msg = (LPCTSTR)strMsg;
	if (strCode.Compare(_T("99997")) == 0) {
		LOG_F(INFO, _T("OnGetMsg[코드번호 = %s][메시지 = 입력값 오류!!]"), strCode);
	}
	else {
		LOG_F(INFO, _T("OnGetMsg[코드번호 = %s][메시지 = %s]"), strCode, strMsg);
		VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
		orderDlgMgr->OnReceiveMsg(strLog);
	}

	//TRACE(strLog);
	if (strCode.Compare(_T("-9998")) == 0) {
		_ServerConnected = false;
		AfxMessageBox(_T("서버와의 연결이 끊어졌습니다. 프로그램을 종료합니다."));
		CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
		mainFrm->SendMessage(WM_CLOSE, 0, 0);
		return;
	}
}

void VtHdCtrl::OnGetMsgWithRqId(int nRqId, CString strCode, CString strMsg)
{
	if (_Blocked)
		return;

	CString msg;
	msg.Format(_T("nRqId = %d, strCode = %s, strMsg = %s\n"), nRqId, strCode, strMsg);
	//TRACE(msg);

	CString strLog;
	strLog.Format("[요청번호 = %d, 코드번호 = %s][메시지 = %s]\n", nRqId, strCode, strMsg);
	TRACE(strLog);

	auto it = _SymbolFileReqMap.find(nRqId);
	if (it != _SymbolFileReqMap.end()) {
		Sleep(VtGlobal::ServerSleepTime);
		HdTaskEventArgs eventArg;
		eventArg.TaskType = HdTaskType::HdSymbolFileDownload;
		FireTaskCompleted(std::move(eventArg));
	}

	it = _AccountPLReqMap.find(nRqId);
	if (it != _AccountPLReqMap.end()) {
		Sleep(VtGlobal::ServerSleepTime);
		HdTaskEventArgs eventArg;
		eventArg.TaskType = HdTaskType::HdApiCustomerProfitLoss;
		FireTaskCompleted(std::move(eventArg));
	}
	
	if (strCode.Compare("0332") == 0) {
		CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
		mainFrm->StartPreProcess();
	}
	// 49007 49003
	else if (strCode.Compare(_T("99997")) == 0) {
		LOG_F(INFO, _T("[요청번호 = %d, 코드번호 = %s][메시지 = 입력갑오류!!]\n"), nRqId, strCode);
	}
	else {
		LOG_F(INFO, _T("[요청번호 = %d, 코드번호 = %s][메시지 = %s]\n"), nRqId, strCode, strMsg);
		auto it = _ReqIdToRequestMap.find(nRqId);
		if (it != _ReqIdToRequestMap.end()) {
			HdOrderRequest& req = it->second;
			LOG_F(INFO, _T("[주문오류 :: 요청번호 = %d, 주문요청타입 = %d, 주문번호 = %s, 계좌번호 = %s, 종목코드 = %s\n"), nRqId, req.Type, req.OrderNo.c_str(), req.AccountNo.c_str(), req.SymbolCode.c_str());
		}
		// 주문요청 오류에 따른 접수 목록 업데이트
 		RefreshAcceptedOrderByError(nRqId);
		VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
		// 주문창을 업데이트 한다.
		orderDlgMgr->OnReceiveMsgWithReqId(nRqId, strLog);
	}

	if (_ttoi(strCode) != 0) {
		if (strCode.Compare(_T("91012")) == 0 ||
			strCode.Compare(_T("99992")) == 0) {
			AfxMessageBox(strMsg);
		}

		if (strCode.Compare("0330") == 0 || 
			strCode.Compare("0331") == 0 ||
			strCode.Compare("0332") == 0 ) {
			return;
		}

		HdTaskEventArgs eventArg;
		HdTaskArg arg = FindRequest(nRqId);
		eventArg.TaskType = arg.Type;
		eventArg.RequestId = nRqId;
		FireTaskCompleted(std::move(eventArg));

		VtSysLog syslog;
		syslog.LogTime = VtGlobal::GetDateTime();
		syslog.ErrorCode = _ttoi(strCode);
		syslog.LogText = (LPCTSTR)strMsg;
		VtGlobal::PushLog(std::move(syslog));
		CMainFrame* mainFrm = (CMainFrame*)AfxGetMainWnd();
		mainFrm->UpdateSysLog();
	}
}


