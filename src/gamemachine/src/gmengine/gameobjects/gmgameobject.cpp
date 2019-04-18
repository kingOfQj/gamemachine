﻿#include "stdafx.h"
#include "gmgameobject.h"
#include "gmengine/gmgameworld.h"
#include "gmdata/glyph/gmglyphmanager.h"
#include "foundation/gamemachine.h"
#include "gmassets.h"

namespace
{
	void calculateAABB(bool indexMode, GMPart* part, REF GMVec3& min, REF GMVec3& max)
	{
		if (!indexMode)
		{
			for (const auto& v : part->vertices())
			{
				GMVec3 cmp(v.positions[0], v.positions[1], v.positions[2]);
				min = MinComponent(cmp, min);
				max = MaxComponent(cmp, max);
			}
		}
		else
		{
			const GMVertices& v = part->vertices();
			for (const auto& i : part->indices())
			{
				GMVec3 cmp(v[i].positions[0], v[i].positions[1], v[i].positions[2]);
				min = MinComponent(cmp, min);
				max = MaxComponent(cmp, max);
			}
		}
	}

	bool isInsideCameraFrustum(GMCamera* camera, GMVec3 (&points)[8])
	{
		GM_ASSERT(camera);
		const GMFrustum& frustum = camera->getFrustum();
		// 如果min和max出现在了某个平面的两侧，
		GMFrustumPlanes planes;
		frustum.getPlanes(planes);
		return GMCamera::isBoundingBoxInside(planes, points);
	}
}

IComputeShaderProgram* GMGameObject::s_defaultComputeShaderProgram = nullptr;

GMGameObject::GMGameObject(GMAsset asset)
	: GMGameObject()
{
	setAsset(asset);
	updateTransformMatrix();
}


GMGameObject::~GMGameObject()
{
	D(d);
	if (auto shaderProgram = getCullShaderProgram())
	{
		shaderProgram->release(d->cullBufferHandle);
		shaderProgram->release(d->cullBufferGPUResultHandle);
		shaderProgram->release(d->cullBufferCPUResultHandle);
		shaderProgram->release(d->cullConstantBufferHandle);
		shaderProgram->release(d->cullSRVHandle);
		shaderProgram->release(d->cullResultHandle);
	}
}

void GMGameObject::setAsset(GMAsset asset)
{
	D(d);
	if (!asset.isScene())
	{
		if (!asset.isModel())
		{
			GM_ASSERT(false);
			gm_error(gm_dbg_wrap("Asset must be a 'scene' or a 'model' type."));
			return;
		}
		else
		{
			asset = GMScene::createSceneFromSingleModel(asset);
		}
	}
	d->asset = asset;
}

GMScene* GMGameObject::getScene()
{
	D(d);
	return d->asset.getScene();
}

GMModel* GMGameObject::getModel()
{
	GMScene* scene = getScene();
	if (!scene)
		return nullptr;

	if (scene->getModels().size() > 1)
	{
		gm_warning(gm_dbg_wrap("Models are more than one. So it will return nothing."));
		return nullptr;
	}

	return scene->getModels()[0].getModel();
}

void GMGameObject::setWorld(GMGameWorld* world)
{
	D(d);
	GM_ASSERT(!d->world);
	d->world = world;
}

GMGameWorld* GMGameObject::getWorld()
{
	D(d);
	return d->world;
}

void GMGameObject::setPhysicsObject(AUTORELEASE GMPhysicsObject* phyObj)
{
	D(d);
	d->physics.reset(phyObj);
	d->physics->setGameObject(this);
}

void GMGameObject::foreachModel(std::function<void(GMModel*)> cb)
{
	GMScene* scene = getScene();
	if (scene)
	{
		for (auto& model : scene->getModels())
		{
			cb(model.getModel());
		}
	}
}


void GMGameObject::setCullComputeShaderProgram(IComputeShaderProgram* shaderProgram)
{
	D(d);
	d->cullShaderProgram = shaderProgram;
}

void GMGameObject::onAppendingObjectToWorld()
{
	D(d);
	if (d->cullOption == GMGameObjectCullOption::AABB)
		makeAABB();
}

void GMGameObject::draw()
{
	foreachModel([this](GMModel* m) {
		drawModel(getContext(), m);
	});
	endDraw();
}

void GMGameObject::update(GMDuration dt)
{
	D(d);
	if (d->cullOption == GMGameObjectCullOption::AABB)
	{
		struct CullResult
		{
			GMint32 visible = false;
		};
		constexpr GMuint32 sz = 512;
		CullResult result[sz]; //TODO

		typedef GMVec4 FrustumPlanes[6];

		IComputeShaderProgram* cullShaderProgram = getCullShaderProgram();
		if (cullShaderProgram && d->cullGPUAccelerationValid)
		{
			if (!d->cullBufferHandle)
			{
				typedef std::remove_reference_t<decltype(d->cullAABB[0])> AABB;
				if (cullShaderProgram->createBuffer(sizeof(AABB), gm_sizet_to_uint(d->cullAABB.size()), d->cullAABB.data(), GMComputeBufferType::Structured, &d->cullBufferHandle) &&
					cullShaderProgram->createBuffer(sizeof(CullResult), gm_sizet_to_uint(sz), result, GMComputeBufferType::Structured, &d->cullBufferGPUResultHandle) &&
					cullShaderProgram->createBuffer(sizeof(FrustumPlanes), 1u, NULL, GMComputeBufferType::Constant, &d->cullConstantBufferHandle) &&
					cullShaderProgram->createBufferShaderResourceView(d->cullBufferHandle, &d->cullSRVHandle) &&
					cullShaderProgram->createBufferUnorderedAccessView(d->cullBufferGPUResultHandle, &d->cullResultHandle))
				{
					// create succeed
				}
				else
				{
					gm_warning(gm_dbg_wrap("GMGameObject create buffer or resource view failed. Cull Compute shader has been shut down for current game object."));
					d->cullGPUAccelerationValid = false;
					return;
				}
			}

			GMCamera* camera = d->cullCamera ? d->cullCamera : &getContext()->getEngine()->getCamera();
			GMFrustumPlanes planes;
			camera->getFrustum().getPlanes(planes);
			FrustumPlanes p;
			p[0] = planes.nearPlane.getPlane();
			p[1] = planes.farPlane.getPlane();
			p[2] = planes.topPlane.getPlane();
			p[3] = planes.bottomPlane.getPlane();
			p[4] = planes.leftPlane.getPlane();
			p[5] = planes.rightPlane.getPlane();

			cullShaderProgram->setBuffer(d->cullConstantBufferHandle, &p, sizeof(FrustumPlanes));
			GMComputeSRVHandle srvs[] = { d->cullSRVHandle };
			cullShaderProgram->setShaderResourceView(1, srvs);
			GMComputeUAVHandle uavs[] = { d->cullResultHandle };
			cullShaderProgram->setUnorderedAccessView(1, uavs);
			cullShaderProgram->dispatch(gm_sizet_to_uint(d->cullAABB.size()), 1, 1);
			
			// TODO 可能会存在越界
			if (!d->cullBufferCPUResultHandle)
			{
				if (!cullShaderProgram->createBufferFrom(d->cullBufferGPUResultHandle, &d->cullBufferCPUResultHandle))
				{
					gm_warning(gm_dbg_wrap("GMGameObject create buffer from gpu result buffer failed. Cull Compute shader has been shut down for current game object."));
					d->cullGPUAccelerationValid = false;
				}
			}

			cullShaderProgram->copyBuffer(d->cullBufferCPUResultHandle, d->cullBufferGPUResultHandle);
			CullResult* resultPtr = static_cast<CullResult*>(cullShaderProgram->mapBuffer(d->cullBufferCPUResultHandle));
			Vector<GMAsset>& models = getScene()->getModels();
			for (GMsize_t i = 0; i < models.size(); ++i)
			{
				auto& shader = models[i].getModel()->getShader();
				shader.setDiscard(resultPtr[i].visible == 0);
			}
			cullShaderProgram->unmapBuffer(d->cullBufferGPUResultHandle);
		}
		else
		{
			Vector<GMAsset>& models = getScene()->getModels();
			// 计算每个Model的AABB是否与相机Frustum有交集，如果没有，则不进行绘制
			GM_ASSERT(d->cullAABB.size() == models.size());

			GMAsync::blockedAsync(
				GMAsync::Async,
				GM.getRunningStates().systemInfo.numberOfProcessors,
				d->cullAABB.begin(),
				d->cullAABB.end(),
				[d, &models, this](auto begin, auto end) {
				// 计算一下数据偏移
				for (auto iter = begin; iter != end; ++iter)
				{
					GMVec3 vertices[8];
					GMsize_t offset = iter - d->cullAABB.begin();
					auto& shader = models[offset].getModel()->getShader();

					for (auto i = 0; i < 8; ++i)
					{
						vertices[i] = d->cullAABB[offset].points[i] * d->transforms.transformMatrix;
					}

					if (isInsideCameraFrustum(d->cullCamera ? d->cullCamera : &getContext()->getEngine()->getCamera(), vertices))
						shader.setDiscard(false);
					else
						shader.setDiscard(true);
				}
			}
			);
		}
	}
}

bool GMGameObject::canDeferredRendering()
{
	D(d);
	GMScene* scene = getScene();
	if (scene)
	{
		for (decltype(auto) model : scene->getModels())
		{
			if (model.getModel()->getShader().getBlend() == true)
				return false;

			if (model.getModel()->getShader().getVertexColorOp() == GMS_VertexColorOp::Replace)
				return false;

			if (model.getModel()->getType() == GMModelType::Custom)
				return false;
		}
	}
	return true;
}

const IRenderContext* GMGameObject::getContext()
{
	D(d);
	return d->context;
}


bool GMGameObject::isSkeletalObject() const
{
	return false;
}

void GMGameObject::updateTransformMatrix()
{
	D(d);
	d->transforms.transformMatrix = d->transforms.scaling * QuatToMatrix(d->transforms.rotation) * d->transforms.translation;
}

void GMGameObject::setScaling(const GMMat4& scaling)
{
	D(d);
	d->transforms.scaling = scaling;
	if (d->autoUpdateTransformMatrix)
		updateTransformMatrix();
}

void GMGameObject::setTranslation(const GMMat4& translation)
{
	D(d);
	d->transforms.translation = translation;
	if (d->autoUpdateTransformMatrix)
		updateTransformMatrix();
}

void GMGameObject::setRotation(const GMQuat& rotation)
{
	D(d);
	d->transforms.rotation = rotation;
	if (d->autoUpdateTransformMatrix)
		updateTransformMatrix();
}

void GMGameObject::beginUpdateTransform()
{
	setAutoUpdateTransformMatrix(false);
}

void GMGameObject::endUpdateTransform()
{
	setAutoUpdateTransformMatrix(true);
	updateTransformMatrix();
}


void GMGameObject::setCullOption(GMGameObjectCullOption option, GMCamera* camera /*= nullptr*/)
{
	D(d);
	d->cullOption = option;
	// setCullOption一定要在将顶点数据传输到显存之前设置，否则顶点信息将不会在内处中保留，从而无法计算AABB

	d->cullCamera = camera;

	// 如果不需要事先裁剪，重置Shader状态
	if (option == GMGameObjectCullOption::None)
	{
		Vector<GMAsset>& models = getScene()->getModels();
		for (GMsize_t i = 0; i < d->cullAABB.size(); ++i)
		{
			auto& shader = models[i].getModel()->getShader();
			shader.setDiscard(false);
		}
	}
}


void GMGameObject::setDefaultCullShaderProgram(IComputeShaderProgram* shaderProgram)
{
	s_defaultComputeShaderProgram = shaderProgram;
}

void GMGameObject::drawModel(const IRenderContext* context, GMModel* model)
{
	D(d);
	IGraphicEngine* engine = context->getEngine();
	if (model->getShader().getDiscard())
		return;

	ITechnique* technique = engine->getTechnique(model->getType());
	if (technique != d->drawContext.currentTechnique)
	{
		if (d->drawContext.currentTechnique)
			d->drawContext.currentTechnique->endScene();

		technique->beginScene(getScene());
		d->drawContext.currentTechnique = technique;
	}

	technique->beginModel(model, this);
	technique->draw(model);
	technique->endModel();
}

void GMGameObject::endDraw()
{
	D(d);
	if (d->drawContext.currentTechnique)
		d->drawContext.currentTechnique->endScene();

	d->drawContext.currentTechnique = nullptr;
}


void GMGameObject::makeAABB()
{
	D(d);
	typedef std::remove_reference_t<decltype(d->cullAABB[0])> AABB;
	const Vector<GMAsset>& models = getScene()->getModels();
	for (auto modelAsset : models)
	{
		GMModel* model = modelAsset.getModel();
		GM_ASSERT(model->isNeedTransfer());
		const GMParts& parts = model->getParts();
		GMVec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
		GMVec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (auto part : parts)
		{
			calculateAABB(model->getDrawMode() == GMModelDrawMode::Index, part, min, max);
		}

		// 将AABB和gameobject添加到成员数据中
		AABB aabb = {
			GMVec4(min.getX(), min.getY(), min.getZ(), 1),
			GMVec4(min.getX(), min.getY(), max.getZ(), 1),
			GMVec4(min.getX(), max.getY(), max.getZ(), 1),
			GMVec4(max.getX(), max.getY(), max.getZ(), 1),
			GMVec4(min.getX(), max.getY(), min.getZ(), 1),
			GMVec4(max.getX(), min.getY(), max.getZ(), 1),
			GMVec4(max.getX(), max.getY(), min.getZ(), 1),
			GMVec4(max.getX(), min.getY(), min.getZ(), 1),
		};
		d->cullAABB.push_back(aabb);
	}
}


IComputeShaderProgram* GMGameObject::getCullShaderProgram()
{
	D(d);
	return s_defaultComputeShaderProgram ? s_defaultComputeShaderProgram : d->cullShaderProgram;
}

GMCubeMapGameObject::GMCubeMapGameObject(GMTextureAsset texture)
{
	createCubeMap(texture);
}

void GMCubeMapGameObject::deactivate()
{
	D(d);
	GMGameWorld* world = getWorld();
	if (world)
		world->getContext()->getEngine()->update(GMUpdateDataType::TurnOffCubeMap);
}

void GMCubeMapGameObject::createCubeMap(GMTextureAsset texture)
{
	GMfloat vertices[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,

		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,

		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,

		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,

		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};

	GMfloat v[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
	};

	GMModel* model = new GMModel();
	model->setType(GMModelType::CubeMap);
	model->getShader().getTextureList().getTextureSampler(GMTextureType::CubeMap).addFrame(texture);
	GMPart* part = new GMPart(model);
	for (GMuint32 i = 0; i < 12; i++)
	{
		GMVertex V0 = { { vertices[i * 9 + 0], vertices[i * 9 + 1], vertices[i * 9 + 2] }, { vertices[i * 9 + 0], vertices[i * 9 + 1], vertices[i * 9 + 2] } };
		GMVertex V1 = { { vertices[i * 9 + 3], vertices[i * 9 + 4], vertices[i * 9 + 5] },{ vertices[i * 9 + 3], vertices[i * 9 + 4], vertices[i * 9 + 5] } };
		GMVertex V2 = { { vertices[i * 9 + 6], vertices[i * 9 + 7], vertices[i * 9 + 8] },{ vertices[i * 9 + 6], vertices[i * 9 + 7], vertices[i * 9 + 8] } };
		part->vertex(V0);
		part->vertex(V1);
		part->vertex(V2);
	}

	setAsset(GMScene::createSceneFromSingleModel(GMAsset(GMAssetType::Model, model)));
}

bool GMCubeMapGameObject::canDeferredRendering()
{
	return false;
}
