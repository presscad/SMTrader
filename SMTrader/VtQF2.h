#pragma once

// Future for us dollars
#include "System/VtSystem.h"
#include "Xml/pugixml.hpp"
class VtChartData;
/// <summary>
/// ����, ������ ƽ�� ���� ƽ���� %�� ���԰��ݿ� �Է°��� ������ 100
/// Ʈ���ϸ���ž�� �ִ����(���� ����)���� �и��� %�� û����
/// </summary>
class VtQF2 : public VtSystem
{
public:
	VtQF2();
	VtQF2(VtSystemType type);
	VtQF2(VtSystemType systemType, std::string name);
	virtual ~VtQF2();

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

	bool CheckEntranceForBuy();
	bool CheckEntranceForSell();
	bool CheckLiqForSell();
	bool CheckLiqForBuy();
	VtPositionType CheckEntranceForBuy(size_t index);
	VtPositionType CheckEntranceForSell(size_t index);
	VtPositionType CheckLiqForSell(size_t index);
	VtPositionType CheckLiqForBuy(size_t index);
};
