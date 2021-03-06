#pragma once
#include "VtStrategyGrid.h"
#include "HdWindowEvent.h"
#include "Poco/BasicEvent.h"
#include "SimpleBinStream.h"
#include "Xml/pugixml.hpp"
using same_endian_type = std::is_same<simple::LittleEndian, simple::LittleEndian>;
using Poco::BasicEvent;
// VtStrategyToolWnd dialog
const int PosiTimer = 0x00000011;
class VtSystem;
class VtStrategyToolWnd : public CDialogEx
{
	DECLARE_DYNAMIC(VtStrategyToolWnd)

public:
	VtStrategyToolWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~VtStrategyToolWnd();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STRATEGE_TOOLS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	VtStrategyGrid _GridCtrl;
	BasicEvent<HdWindowEventArgs> _WindowEvent;
	void FireWindowEvent(HdWindowEventArgs&& arg)
	{
		_WindowEvent(this, arg);
	}
public:
	void SaveToXml(pugi::xml_node& node);
	void LoadFromXml(pugi::xml_node& node);
	void Save(simple::file_ostream<same_endian_type>& ss);
	void Load(simple::file_istream<same_endian_type>& ss);
	void UpdateSystem(VtSystem* sys, bool enable);
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
