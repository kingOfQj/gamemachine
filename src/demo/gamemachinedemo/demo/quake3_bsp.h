﻿#ifndef __DEMO_QUAKE3_BSP_H__
#define __DEMO_QUAKE3_BSP_H__

#include <gamemachine.h>
#include <gmdemogameworld.h>
#include "demonstration_world.h"

namespace gm
{
	class GMBSPGameWorld;
}

GM_PRIVATE_OBJECT_UNALIGNED(Demo_Quake3_BSP)
{
	gm::GMBSPGameWorld* world = nullptr;
	gm::GMSpriteGameObject* sprite = nullptr;
	bool mouseTrace = false;
};

class Demo_Quake3_BSP : public DemoHandler
{
	GM_DECLARE_PRIVATE(Demo_Quake3_BSP)
	GM_DECLARE_BASE(DemoHandler)

public:
	Demo_Quake3_BSP(DemonstrationWorld* parentDemonstrationWorld);

public:
	virtual void init() override;
	virtual void event(gm::GameMachineHandlerEvent evt) override;
	virtual void onActivate() override;
	virtual void onDeactivate() override;

protected:
	virtual void setLookAt() override;
	virtual void setDefaultLights() override;

private:
	void setMouseTrace(bool enabled);

protected:
	const gm::GMString& getDescription() const
	{
		static gm::GMString desc = L"渲染一个雷神之锤3的游戏场景。";
		return desc;
	}
};

#endif