#pragma once
// Future for us dollars
#include "System/VtSystem.h"
#include "Xml/pugixml.hpp"
class VtChartData;
class VtTrUs3 : public VtSystem
{
public:
	VtTrUs3();
	VtTrUs3(VtSystemType type);
	VtTrUs3(VtSystemType systemType, std::string name);
	virtual ~VtTrUs3();

	virtual void SetDataSrc();
	virtual void InitArgs();
	virtual void CreateSignal(int startIndex, int endIndex);
	virtual VtPositionType UpdateSignal(int index);
	virtual void ReloadSystem(int startIndex, int endIndex);
	virtual VtPositionType UpdateSignal(int start, int end);
	virtual void SetDataMap(VtChartData* chartData);
	virtual void SaveToXml(pugi::xml_node& node);
	virtual void LoadFromXml(pugi::xml_node& node);
	virtual void OnTimer();
	virtual void UpdateSystem(int index);
	virtual void ReadExtraArgs();

	virtual bool CheckEntranceForBuy();
	virtual bool CheckEntranceForSell();
	virtual bool CheckLiqForSell();
	virtual bool CheckLiqForBuy();

	virtual bool CheckEntranceForBuy(size_t index);
	virtual bool CheckEntranceForSell(size_t index);
	virtual bool CheckLiqForSell(size_t index);
	virtual bool CheckLiqForBuy(size_t index);
};

