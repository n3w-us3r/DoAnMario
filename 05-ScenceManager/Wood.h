#pragma once
#include "GameObject.h"

#define WOOD_BBOX_WIDTH  16
#define WOOD_BBOX_HEIGHT 16

class Wood : public CGameObject
{
public:
	Wood();
	virtual void Render();
	virtual void GetBoundingBox(float& l, float& t, float& r, float& b);
};