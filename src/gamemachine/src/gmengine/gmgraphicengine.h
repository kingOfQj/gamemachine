﻿#ifndef __GMGRAPHICENGINE_H__
#define __GMGRAPHICENGINE_H__
#include <gmcommon.h>
#include <gmtools.h>
#include <gmmodel.h>
#include <gmcamera.h>
#include <gmrendertechnique.h>
#include <gmthread.h>
BEGIN_NS

#define NO_ANIMATION 0
#define SKELETAL_ANIMATION 1
#define AFFINE_ANIMATION 2

template <typename T>
ITechnique* newTechnique(GMOwnedPtr<ITechnique>& ptr, const IRenderContext* context)
{
	if (!ptr)
		ptr = gm_makeOwnedPtr<T>(context);
	return ptr.get();
}

#define GM_VariablesDesc (GMGraphicEngine::getDefaultShaderVariablesDesc())

template <typename T>
struct GMShaderVariablesTextureDesc
{
	T OffsetX;
	T OffsetY;
	T ScaleX;
	T ScaleY;
	T Enabled;
	T Texture;
};

template <typename T>
struct GMShaderVariablesLightDesc
{
	T Name;
	T Count;
};

template <typename T>
struct GMShaderVariablesMaterialDesc
{
	T Ka;
	T Kd;
	T Ks;
	T Shininess;
	T Refreactivity;
	T F0;
};

constexpr int GMFilterCount = 7;
template <typename T>
struct GMShaderVariablesFilterDesc
{
	T Filter;
	T KernelDeltaX;
	T KernelDeltaY;
	T BlendFactor;
	T Types[GMFilterCount];
};

template <typename T>
struct GMShaderVariablesScreenInfoDesc
{
	T ScreenInfo;
	T ScreenWidth;
	T ScreenHeight;
	T Multisampling;
};

template <typename T>
struct GMShaderVariableShadowInfoDesc
{
	T ShadowInfo;
	T HasShadow;
	T ShadowMatrix;
	T EndClip;
	T CurrentCascadeLevel;
	T Position;
	T ShadowMap;
	T ShadowMapMSAA;
	T ShadowMapWidth;
	T ShadowMapHeight;
	T BiasMin;
	T BiasMax;
	T CascadedShadowLevel;
	T ViewCascade;
	T PCFRows;
};

template <typename T>
struct GMShaderVariableGammaDesc
{
	T GammaCorrection;
	T GammaValue;
	T GammaInvValue;
};

template <typename T>
struct GMShaderVariableHDRDesc
{
	T HDR;
	T ToneMapping;
};

template <typename T>
struct GMShaderVariableDebugDesc
{
	T Normal;
};

template <typename T>
struct GMShaderVariablesDesc_t
{
	// 矩阵
	T ModelMatrix;
	T ViewMatrix;
	T ProjectionMatrix;
	T InverseTransposeModelMatrix;
	T InverseViewMatrix;

	// 骨骼
	T Bones;
	T UseAnimation;

	// 位置
	T ViewPosition;

	// 材质
	GMShaderVariablesTextureDesc<T> TextureAttributes;
	T AmbientTextureName;
	T DiffuseTextureName;
	T SpecularTextureName;
	T NormalMapTextureName;
	T LightMapTextureName;
	T AlbedoTextureName;
	T MetallicRoughnessAOTextureName;
	T CubeMapTextureName;

	// 光照
	T LightCount;

	// 材质
	GMShaderVariablesMaterialDesc<T> MaterialAttributes;
	T MaterialName;

	// 滤镜
	GMShaderVariablesFilterDesc<T> FilterAttributes;

	// 状态
	GMShaderVariablesScreenInfoDesc<T> ScreenInfoAttributes;
	T RasterizerState;
	T BlendState;
	T DepthStencilState;

	// 阴影
	GMShaderVariableShadowInfoDesc<T> ShadowInfo;

	// Gamma校正
	GMShaderVariableGammaDesc<T> GammaCorrection;

	// HDR
	GMShaderVariableHDRDesc<T> HDR;

	// 模型
	T IlluminationModel;
	T ColorVertexOp;

	// 调试
	GMShaderVariableDebugDesc<T> Debug;
};

using GMShaderVariablesDesc = GMShaderVariablesDesc_t<const GMString>;
using GMShaderVariablesIndices = GMShaderVariablesDesc_t<GMint32>;

constexpr GMCascadeLevel GMMaxCascades = 8;
struct GM_EXPORT GMShadowSourceDesc
{
	enum Type
	{
		NoShadow,
		CSMShadow,
	};

	Type type;
	GMCamera camera;
	GMVec4 position;
	float biasMin = 0.0005f;
	float biasMax = 0.005f;
	GMint32 width = 0;
	GMint32 height = 0;
	GMint32 cascades = 1; //!< GameMachine将构造出一副(width*cascadedLevel, height)的纹理来记录shadow map。如果cascadedLevel为1，那么就是普通的阴影贴图。否则，它就是CSM方式的阴影贴图。
	Array<GMfloat, GMMaxCascades> cascadePartitions = { 1 }; //!< 层级的分区，范围为(0, 1]。如果某个层级为0，则按照所在的索引来等分。
	GMint32 pcfRowCount = 3; //!< PCM采样的行数。如为3，那么则采用PCM 3*3进行采样来计算阴影。如果未开启渲染的多重采样，则PCF不生效。
	static GMint64 version;
};

struct GMGlobalBlendStateDesc
{
	bool enabled = false;
	GMS_BlendFunc sourceRGB;
	GMS_BlendFunc destRGB;
	GMS_BlendOp opRGB;
	GMS_BlendFunc sourceAlpha;
	GMS_BlendFunc destAlpha;
	GMS_BlendOp opAlpha;
	GMint32 blendRefCount = 0;
};

GM_PRIVATE_CLASS(GMFramebuffersStack);
class GM_EXPORT GMFramebuffersStack : public GMObject
{
	GM_DECLARE_PRIVATE(GMFramebuffersStack);

public:
	GMFramebuffersStack();
	~GMFramebuffersStack();

public:
	void push(IFramebuffers* framebuffers);
	IFramebuffers* pop();
	IFramebuffers* peek();
};

GM_PRIVATE_CLASS(GMGraphicEngine);
class GM_EXPORT GMGraphicEngine : public GMObject, public IGraphicEngine
{
	GM_DECLARE_PRIVATE(GMGraphicEngine)

public:
	GMGraphicEngine(const IRenderContext* context);
	~GMGraphicEngine();

public:
	virtual void init() override;
	virtual IGBuffer* getGBuffer() override;
	virtual IFramebuffers* getFilterFramebuffers() override;
	virtual void begin() override;
	virtual void draw(const GMGameObjectContainer& forwardRenderingObjects, const GMGameObjectContainer& deferredRenderingObjects) override;
	virtual void end() override;
	virtual GMLightIndex addLight(AUTORELEASE ILight* light) override;
	virtual ILight* getLight(GMLightIndex index) override;
	virtual bool removeLight(GMLightIndex index) override;
	virtual bool removeLight(ILight* light) override;
	virtual void removeLights() override;
	virtual void setStencilOptions(const GMStencilOptions& options) override;
	virtual const GMStencilOptions& getStencilOptions() override;
	virtual void setShaderLoadCallback(IShaderLoadCallback* cb) override;
	virtual void setShadowSource(const GMShadowSourceDesc& desc) override;
	virtual GMCamera& getCamera() override;
	virtual void setCamera(const GMCamera&) override;
	virtual void beginBlend(
		GMS_BlendFunc sfactorRGB,
		GMS_BlendFunc dfactorRGB,
		GMS_BlendOp opRGB,
		GMS_BlendFunc sfactorAlpha,
		GMS_BlendFunc dfactorAlpha,
		GMS_BlendOp opAlpha
	) override;
	virtual void endBlend() override;
	virtual GMRenderTechniqueManager* getRenderTechniqueManager() override;
	virtual GMPrimitiveManager* getPrimitiveManager() override;
	virtual bool msgProc(const GMMessage& e) override;
	virtual GMConfigs& getConfigs() override;
	virtual void createModelDataProxy(const IRenderContext* context, GMModel* model, bool transfer = true) override;
	virtual bool isCurrentMainThread() override;
	virtual ICSMFramebuffers* getCSMFramebuffers();

protected:
	virtual void createShadowFramebuffers(OUT IFramebuffers** framebuffers);
	virtual void resetCSM();
	virtual void createFilterFramebuffer();
	virtual void generateShadowBuffer(const GMGameObjectContainer& forwardRenderingObjects, const GMGameObjectContainer& deferredRenderingObjects);
	virtual bool needUseFilterFramebuffer();

protected:
	void bindFilterFramebuffer();
	void clearFilterFramebuffer();
	void unbindFilterFramebuffer();
	void drawFilterFramebuffer();
	GMGameObject* getFilterQuad();
	IShaderLoadCallback* getShaderLoadCallback();
	const Vector<ILight*>& getLights();

public:
	const GMFilterMode::Mode getCurrentFilterMode();
	const GMVec3 getCurrentFilterBlendFactor();
	void draw(const GMGameObjectContainer& objects);
	IFramebuffers* getShadowMapFramebuffers();
	bool needGammaCorrection();
	GMfloat getGammaValue();
	bool needHDR();
	GMToneMapping::Mode getToneMapping();
	bool isWireFrameMode(GMModel* model);
	bool isNeedDiscardTexture(GMModel* model, GMTextureType type);
	const GMMat4& getCascadeCameraVPMatrix(GMCascadeLevel level);
	const GMShadowSourceDesc& getShadowSourceDesc();
	bool isDrawingShadow();
	const GMGlobalBlendStateDesc& getGlobalBlendState();
	GMFramebuffersStack& getFramebuffersStack();

public:
	static constexpr const GMsize_t getMaxLightCount()
	{
		return 50;
	}
	static const GMShaderVariablesDesc& getDefaultShaderVariablesDesc();

	static constexpr const GMCascadeLevel getMaxCascades()
	{
		return GMMaxCascades;
	}

};

END_NS
#endif