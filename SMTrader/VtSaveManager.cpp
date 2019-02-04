#include "stdafx.h"
#include "VtSaveManager.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <filesystem>
#include "ZmConfigManager.h"
#include "VtFundManager.h"
#include <msgpack.hpp>
#include "VtAccountManager.h"
#include "VtOrderDialogManager.h"
#include "SimpleBinStream.h"
#include "VtSymbol.h"
#include "VtOrderManagerSelector.h"
#include "HdWindowManager.h"
#include "VtStringUtil.h"
#include "File/path.h"
#include "File/resolver.h"
#include <exception>
#include "VtSystemGroupManager.h"


using namespace std;
using same_endian_type = std::is_same<simple::LittleEndian, simple::LittleEndian>;

VtSaveManager::VtSaveManager()
{
}


VtSaveManager::~VtSaveManager()
{
}

void VtSaveManager::SaveOrders()
{
	SaveOrders(_T("orderlist.dat"));
}

void VtSaveManager::SaveOrders(std::string fileName)
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(fileName);

	if (!fexists(appPath)) {
		std::ofstream newFile(appPath);
		newFile.close();
	}

	simple::file_ostream<same_endian_type> outfile(appPath.c_str());
}

void VtSaveManager::SaveFundList(std::string fileName)
{
	
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(fileName);	

	simple::file_ostream<same_endian_type> outfile(appPath.c_str());
	
	VtFundManager* fundMgr = VtFundManager::GetInstance();
	fundMgr->Save(outfile);
	outfile.flush();
	outfile.close();
}

void VtSaveManager::SaveFundList()
{
	SaveFundList(_T("fundlist.dat"));
}

void VtSaveManager::LoadFundList(std::string fileName)
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(fileName);

	try {
		filesystem::path path1(appPath);
		if (!path1.exists()) {
			std::ofstream outfile(appPath);
			outfile.close();
		}

		simple::file_istream<same_endian_type> in(appPath.c_str());
		if (in.file_length() == 0)
			return;
		VtFundManager* fundMgr = VtFundManager::GetInstance();

		fundMgr->Load(in);
	}
	catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in LoadFundList"), error);
	}
}

void VtSaveManager::SaveAccountList(std::string fileName)
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(fileName);
	simple::file_ostream<same_endian_type> outfile(appPath.c_str());

	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	acntMgr->Save(outfile);

	outfile.flush();
	outfile.close();
}

void VtSaveManager::SaveAccountList()
{
	SaveAccountList(_T("accountlist.dat"));
}

void VtSaveManager::LoadAccountList(std::string fileName)
{
	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);

		filesystem::path path1(appPath);
		if (!path1.exists()) {
			std::ofstream outfile(appPath);
			outfile.close();
		}

		simple::file_istream<same_endian_type> in(appPath.c_str());
		if (in.file_length() == 0)
			return;

		VtAccountManager* acntMgr = VtAccountManager::GetInstance();
		acntMgr->Load(in);
	} catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in LoadAccountList"), error);
	}
}

void VtSaveManager::SaveOrderWndList(std::string fileName, CMainFrame* mainFrm)
{
	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);
		simple::file_ostream<same_endian_type> outfile(appPath.c_str());

		VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
		orderDlgMgr->MainFrm(mainFrm);
		orderDlgMgr->Save(outfile);

		HdWindowManager* dlgMgr = HdWindowManager::GetInstance();
		dlgMgr->MainFrm(mainFrm);
		dlgMgr->Save(outfile);

		outfile.flush();
		outfile.close();
	} catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in SaveOrderWndList"), error);
	}
}

void VtSaveManager::LoadOrderWndList(std::string fileName, CMainFrame* mainFrm)
{
	if (!mainFrm)
		return;

	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(fileName);

	filesystem::path path1(appPath);
	if (!path1.exists()) {
		std::ofstream outfile(appPath);
		outfile.close();
	}

	simple::file_istream<same_endian_type> in(appPath.c_str());
	if (in.file_length() == 0)
		return;
	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->MainFrm(mainFrm);
	orderDlgMgr->Load(in);

	HdWindowManager* dlgMgr = HdWindowManager::GetInstance();
	dlgMgr->MainFrm(mainFrm);
	dlgMgr->Load(in);

	GetSymbolMasters();
}

void VtSaveManager::SaveDialogList(std::string fileName, CMainFrame* mainFrm)
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(fileName);
	simple::file_ostream<same_endian_type> outfile(appPath.c_str());

	HdWindowManager* dlgMgr = HdWindowManager::GetInstance();
	dlgMgr->MainFrm(mainFrm);
	dlgMgr->Save(outfile);

	outfile.flush();
	outfile.close();
}

void VtSaveManager::LoadDialogList(std::string fileName, CMainFrame* mainFrm)
{
	if (!mainFrm)
		return;

	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);

		simple::file_istream<same_endian_type> in(appPath.c_str());
		if (in.file_length() == 0)
			return;

		HdWindowManager* dlgMgr = HdWindowManager::GetInstance();
		dlgMgr->MainFrm(mainFrm);
		dlgMgr->Load(in);
	} catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in LoadDialogList"), error);
	}
}

void VtSaveManager::SaveTotal(std::string fileName, CMainFrame* mainFrm)
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));

	std::string dirName = appPath;

	// ���ó�¥ ���丮 �̸��� �����.
	dirName.append(VtStringUtil::getTimeStr());
	filesystem::path path1(dirName);
	if (!path1.exists()) { // ���丮�� �������� ���� ���
		// ���� ��¥�� ���丮 ����
		filesystem::create_directory(path1);
		// �� ���丮 �ؿ� ������ �����.
		dirName.append(_T("\\"));
		dirName.append(fileName);
		std::ofstream outfile(dirName);
		outfile.close();
	}
	appPath.append(VtStringUtil::getTimeStr());
	appPath.append(_T("\\"));
	appPath.append(fileName);
	// ������ ���� ������ �����Ѵ�.
	simple::file_ostream<same_endian_type> outfile(appPath.c_str());

	VtFundManager* fundMgr = VtFundManager::GetInstance();
	fundMgr->Save(outfile);

	VtAccountManager* acntMgr = VtAccountManager::GetInstance();
	acntMgr->Save(outfile);

	VtOrderDialogManager* orderDlgMgr = VtOrderDialogManager::GetInstance();
	orderDlgMgr->MainFrm(mainFrm);
	orderDlgMgr->Save(outfile);

	HdWindowManager* dlgMgr = HdWindowManager::GetInstance();
	dlgMgr->MainFrm(mainFrm);
	dlgMgr->Save(outfile);

	// ������ ������ ������ �ݴ´�.
	outfile.flush();
	outfile.close();
}

void VtSaveManager::SaveLoginInfo(std::string fileName, std::string id, std::string pwd, std::string cert, bool save)
{
	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);
		simple::file_ostream<same_endian_type> outfile(appPath.c_str());

		outfile << id;
		outfile << pwd;
		outfile << cert;
		outfile << save;

		outfile.flush();
		outfile.close();
	}
	catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in SaveOrderWndList"), error);
	}
}

void VtSaveManager::LoadLoginInfo(std::string fileName, std::string& id, std::string& pwd, std::string& cert, bool& save)
{
	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);

		filesystem::path path1(appPath);
		if (!path1.exists()) { // ���丮�� �������� ���� ���
			std::ofstream outfile(appPath);
			outfile.close();
		}

		simple::file_istream<same_endian_type> in(appPath.c_str());
		if (in.file_length() == 0)
			return;

		
		in >> id;
		in >> pwd;
		in >> cert;
		in >> save;
	}
	catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in LoginInfo"), error);
	}
}

void VtSaveManager::SaveSystems(std::string fileName)
{
	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);

		filesystem::path path1(appPath);
		if (!path1.exists()) { // ���丮�� �������� ���� ���
			std::ofstream outfile(appPath);
			outfile.close();
		}
		simple::file_ostream<same_endian_type> outfile(appPath.c_str());

		VtSystemGroupManager* sysGrpMgr = VtSystemGroupManager::GetInstance();
		sysGrpMgr->Save(outfile);

		// ������ ������ ������ �ݴ´�.
		outfile.flush();
		outfile.close();
	}
	catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in LoginInfo"), error);
	}
}

void VtSaveManager::LoadSystems(std::string fileName)
{
	try {
		ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
		std::string appPath;
		appPath = configMgr->GetAppPath();
		appPath.append(_T("\\"));
		appPath.append(fileName);

		filesystem::path path1(appPath);
		if (!path1.exists()) { // ���丮�� �������� ���� ���
			std::ofstream outfile(appPath);
			outfile.close();
		}

		VtSystemGroupManager* sysGrpMgr = VtSystemGroupManager::GetInstance();
		simple::file_istream<same_endian_type> in(appPath.c_str());
		if (in.file_length() == 0) {
			sysGrpMgr->InitSystemGroup();
			return;
		}

		sysGrpMgr->Load(in);
	}
	catch (std::exception& e) {
		std::string error = e.what();
		LOG_F(INFO, _T("Error : %s in LoginInfo"), error);
	}
}

bool VtSaveManager::IsAccountFileExist()
{
	ZmConfigManager* configMgr = ZmConfigManager::GetInstance();
	std::string appPath;
	appPath = configMgr->GetAppPath();
	appPath.append(_T("\\"));
	appPath.append(_T("accountlist.dat"));

	filesystem::path path1(appPath);
	if (path1.exists()) {
		simple::file_istream<same_endian_type> in(appPath.c_str());
		if (in.file_length() == 0)
			return false;
		else
			return true;
	}
	else
		return false;
}

void VtSaveManager::GetSymbolMasters()
{
	for (auto it = _SymbolVector.begin(); it != _SymbolVector.end(); ++it)
	{
		VtSymbol* sym = *it;
		sym->GetSymbolMaster();
	}
}