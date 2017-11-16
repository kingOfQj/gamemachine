﻿#include "stdafx.h"
#include "gmcontrolgameobject.h"
#include "foundation/utilities/gmprimitivecreator.h"

//////////////////////////////////////////////////////////////////////////
GMControlEvent::GMControlEvent(GMControlEventType type, GMEventName eventName)
	: m_type(type)
	, m_eventName(eventName)
{
}

bool GM2DMouseDownEvent::buttonDown(Button button)
{
	if (button == GM2DMouseDownEvent::Left)
		return !!(m_state.trigger_button & GMMouseButton_Left);
	if (button == GM2DMouseDownEvent::Right)
		return !!(m_state.trigger_button & GMMouseButton_Right);
	if (button == GM2DMouseDownEvent::Middle)
		return !!(m_state.trigger_button & GMMouseButton_Middle);
	return false;
}
//////////////////////////////////////////////////////////////////////////
GMControlGameObject::GMControlGameObject(GMControlGameObject* parent)
{
	D(d);
	if (parent)
		parent->addChild(this);

	GMRect client = GM.getMainWindow()->getClientRect();
	d->clientSize = client;
}

GMControlGameObject::~GMControlGameObject()
{
	D(d);
	for (auto& child : d->children)
	{
		delete child;
	}

	if (d->stencil)
	{
		GMModel* model = d->stencil->getModel();
		GM_delete(model);
		GM_delete(d->stencil);
	}
}

void GMControlGameObject::onAppendingObjectToWorld()
{
	D(d);
	// 创建一个模板，绘制子对象的时候，子对象不能溢出此模板
	class __Cb : public IPrimitiveCreatorShaderCallback
	{
	public:
		virtual void onCreateShader(Shader& shader) override
		{
			shader.setBlend(true);
			shader.setBlendFactorDest(GMS_BlendFunc::ONE);
			shader.setBlendFactorSource(GMS_BlendFunc::ONE);
		}
	} _cb;

	GMModel* model = nullptr;
	createQuadModel(&_cb, &model);
	d->stencil = new GMGameObject(GMAssets::createIsolatedAsset(GMAssetType::Model, model));
	d->stencil->setWorld(getWorld());
	GM.initObjectPainter(d->stencil->getModel());

	Base::onAppendingObjectToWorld();
}

void GMControlGameObject::setScaling(const linear_math::Matrix4x4& scaling)
{
	D(d);
	Base::setScaling(scaling);
	scalingGeometry(scaling);

	if (d->stencil)
		d->stencil->setScaling(scaling);
	for (auto& child : d->children)
	{
		child->setScaling(scaling);
	}
}

void GMControlGameObject::notifyControl()
{
	D(d);
	updateUI();

	IInput* input = GM.getMainWindow()->getInputMananger();
	IMouseState& mouseState = input->getMouseState();
	GMMouseState ms = mouseState.mouseState();

	if (insideGeometry(ms.posX, ms.posY))
	{
		if (ms.trigger_button != GMMouseButton_None)
		{
			GM2DMouseDownEvent e(ms);
			event(&e);
		}
		else
		{
			if (!d->mouseHovered)
			{
				d->mouseHovered = true;
				GM2DMouseHoverEvent e(ms);
				event(&e);
			}
		}
	}
	else
	{
		if (d->mouseHovered)
		{
			d->mouseHovered = false;
			GM2DMouseLeaveEvent e(ms);
			event(&e);
		}
	}
}

void GMControlGameObject::event(GMControlEvent* e)
{
	D(d);
	emitEvent(e->getEventName());
}

bool GMControlGameObject::insideGeometry(GMint x, GMint y)
{
	D(d);
	GMint tx = d->geometry.x + d->geometry.width / 2, ty = d->geometry.y + d->geometry.height / 2;
	GMRect scaledRect = {
		(GMint) ((d->geometry.x - tx) * d->geometryScaling[0] + tx),
		(GMint) ((d->geometry.y - ty) * d->geometryScaling[1] + ty),
		(GMint) (d->geometry.width * d->geometryScaling[0]),
		(GMint) (d->geometry.height * d->geometryScaling[1]),
	};

	return d->parent ?
		d->parent->insideGeometry(x, y) && GM_in_rect(scaledRect, x, y) :
		GM_in_rect(scaledRect, x, y);
}

void GMControlGameObject::updateUI()
{
	D(d);
	switch (GM.peekMessage().msgType)
	{
	case GameMachineMessageType::WindowSizeChanged:
		GMRect nowClient = GM.getMainWindow()->getClientRect();
		GMfloat scaleX = (GMfloat)nowClient.width / d->clientSize.width,
			scaleY = (GMfloat)nowClient.height / d->clientSize.height;

		GM_ASSERT(scaleX != 0 && scaleY != 0);
		d->geometryScaling[0] = scaleX;
		d->geometryScaling[1] = scaleY;
		d->clientSize = nowClient;
		break;
	}
}

GMRectF GMControlGameObject::toViewportCoord(const GMRect& in)
{
	GMRect client = GM.getMainWindow()->getClientRect();
	GMRectF out = {
		in.x * 2.f / client.width - 1.f,
		1.f - in.y * 2.f / client.height,
		(GMfloat)in.width / client.width,
		(GMfloat)in.height / client.height
	};
	return out;
}

void GMControlGameObject::updateMatrices()
{
	D(d);
	GMRectF coord = toViewportCoord(d->geometry);
	// coord表示左上角的绘制坐标，平移的时候需要换算到中心处
	GMfloat x = coord.x + coord.width / 2.f, y = coord.y - coord.height / 2;

	// TODO
	// setTranslation(linear_math::translate(linear_math::Vector3(x, y, 0)));
}

void GMControlGameObject::addChild(GMControlGameObject* child)
{
	D(d);
	child->setParent(this);
	d->children.push_back(child);
}

void GMControlGameObject::scalingGeometry(const linear_math::Matrix4x4& scaling)
{
	D(d);
	GM_ASSERT(scaling[0][0] > 0);
	GM_ASSERT(scaling[1][1] > 0);
	d->geometryScaling[0] = scaling[0][0];
	d->geometryScaling[1] = scaling[1][1];
}

void GMControlGameObject::createQuadModel(IPrimitiveCreatorShaderCallback* callback, OUT GMModel** model)
{
	D(d);
	GM_ASSERT(model);

	GMRectF coord = toViewportCoord(d->geometry);
	GMfloat extents[3] = {
		coord.width,
		coord.height,
		1.f,
	};

	//GMPrimitiveCreator::createQuad(extents, GMPrimitiveCreator::origin(), model, callback, GMMeshType::Model2D, GMPrimitiveCreator::Center);
	// TODO:
	GMfloat pos[3] = { coord.x, coord.y, 0 };
	GMPrimitiveCreator::createQuad(extents, pos, model, callback, GMMeshType::Model2D, GMPrimitiveCreator::TopLeft);
}