#pragma once
#include "VtBaseIndex.h"
#include "Xml/pugixml.hpp"
class VtBollingerBandIndex :
	public VtBaseIndex
{
public:
	VtBollingerBandIndex();
	VtBollingerBandIndex(VtIndexType type);
	virtual  ~VtBollingerBandIndex();

	int Period;
	int Width;
	int LineColor;
	int FillColor;

	virtual void SaveToXml(pugi::xml_node& node);
	virtual void LoadFromXml(pugi::xml_node& node);
};

