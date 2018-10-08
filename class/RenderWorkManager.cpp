#include "RenderWorkManager.h"
#include "../vulkan/RenderPass.h"
#include "../vulkan/GlobalDeviceObjects.h"
#include "../vulkan/Framebuffer.h"
#include "../vulkan/SwapChain.h"
#include "../vulkan/DepthStencilBuffer.h"
#include "../vulkan/Texture2D.h"
#include "../vulkan/GlobalVulkanStates.h"
#include "RenderPassDiction.h"
#include "ForwardRenderPass.h"
#include "DeferredMaterial.h"
#include "MotionTileMaxMaterial.h"
#include "MotionNeighborMaxMaterial.h"
#include "ShadowMapMaterial.h"
#include "SSAOMaterial.h"
#include "GaussianBlurMaterial.h"
#include "BloomMaterial.h"
#include "ForwardMaterial.h"
#include "PostProcessingMaterial.h"
#include "MaterialInstance.h"

bool RenderWorkManager::Init()
{
	if (!Singleton<RenderWorkManager>::Init())
		return false;

	m_PBRGbufferMaterial			= GBufferMaterial::CreateDefaultMaterial();
	m_pMotionTileMaxMaterial		= MotionTileMaxMaterial::CreateDefaultMaterial();
	m_pMotionNeighborMaxMaterial	= MotionNeighborMaxMaterial::CreateDefaultMaterial();
	m_pShadingMaterial				= DeferredShadingMaterial::CreateDefaultMaterial();
	m_pShadowMapMaterial			= ShadowMapMaterial::CreateDefaultMaterial();
	m_pSSAOMaterial					= SSAOMaterial::CreateDefaultMaterial();
	m_pBloomMaterial				= BloomMaterial::CreateDefaultMaterial();
	m_pSSAOBlurVMaterial			= GaussianBlurMaterial::CreateDefaultMaterial(FrameBufferDiction::FrameBufferType_SSAO, FrameBufferDiction::FrameBufferType_SSAOBlurV, RenderPassDiction::PipelineRenderPassSSAOBlurV, { true, 1, 1 });
	m_pSSAOBlurHMaterial			= GaussianBlurMaterial::CreateDefaultMaterial(FrameBufferDiction::FrameBufferType_SSAOBlurV, FrameBufferDiction::FrameBufferType_SSAOBlurH, RenderPassDiction::PipelineRenderPassSSAOBlurH, { false, 1, 1 });
	m_pBloomBlurVMaterial			= GaussianBlurMaterial::CreateDefaultMaterial(FrameBufferDiction::FrameBufferType_BloomBlurH, FrameBufferDiction::FrameBufferType_BloomBlurV, RenderPassDiction::PipelineRenderPassBloomBlurV, { true, 1.2f, 1.0f });
	m_pBloomBlurHMaterial			= GaussianBlurMaterial::CreateDefaultMaterial(FrameBufferDiction::FrameBufferType_BloomBlurV, FrameBufferDiction::FrameBufferType_BloomBlurH, RenderPassDiction::PipelineRenderPassBloomBlurH, { false, 1.2f, 1.0f });
	m_postProcessMaterials.push_back(PostProcessingMaterial::CreateDefaultMaterial(0));
	m_postProcessMaterials.push_back(PostProcessingMaterial::CreateDefaultMaterial(1));

	SimpleMaterialCreateInfo info = {};
	info.shaderPaths = { L"../data/shaders/sky_box.vert.spv", L"", L"", L"", L"../data/shaders/sky_box.frag.spv", L"" };
	info.materialUniformVars = {};
	info.vertexFormat = VertexFormatP;
	info.subpassIndex = 1;
	info.pRenderPass = RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassShading);
	info.depthWriteEnable = false;
	info.frameBufferType = FrameBufferDiction::FrameBufferType_Shading;

	m_pSkyBoxMaterial = ForwardMaterial::CreateDefaultMaterial(info);

	info = {};
	info.shaderPaths = { L"../data/shaders/background_motion_gen.vert.spv", L"", L"", L"", L"../data/shaders/background_motion_gen.frag.spv", L"" };
	info.materialUniformVars = {};
	info.vertexFormat = VertexFormatP;
	info.subpassIndex = 1;
	info.pRenderPass = RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassGBuffer);
	info.depthWriteEnable = false;
	info.frameBufferType = FrameBufferDiction::FrameBufferType_GBuffer;

	m_pBackgroundMotionMaterial = ForwardMaterial::CreateDefaultMaterial(info);

	return true;
}

std::shared_ptr<MaterialInstance> RenderWorkManager::AcquirePBRMaterialInstance() const
{
	std::shared_ptr<MaterialInstance> pMaterialInstance = m_PBRGbufferMaterial->CreateMaterialInstance();
	pMaterialInstance->SetRenderMask(1 << Scene);
	return pMaterialInstance;
}

std::shared_ptr<MaterialInstance> RenderWorkManager::AcquireShadowMaterialInstance() const
{
	std::shared_ptr<MaterialInstance> pMaterialInstance = m_pShadowMapMaterial->CreateMaterialInstance();
	pMaterialInstance->SetRenderMask(1 << ShadowMapGen);
	return pMaterialInstance;
}

std::shared_ptr<MaterialInstance> RenderWorkManager::AcquireSkyBoxMaterialInstance() const
{
	std::shared_ptr<MaterialInstance> pMaterialInstance = m_pSkyBoxMaterial->CreateMaterialInstance();
	pMaterialInstance->SetRenderMask(1 << Scene);
	return pMaterialInstance;
}

void RenderWorkManager::SyncMaterialData()
{
	m_PBRGbufferMaterial->SyncBufferData();
	m_pShadowMapMaterial->SyncBufferData();
}

void RenderWorkManager::Draw(const std::shared_ptr<CommandBuffer>& pDrawCmdBuffer, uint32_t pingpong)
{
	m_PBRGbufferMaterial->BeforeRenderPass(pDrawCmdBuffer);
	m_pBackgroundMotionMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassGBuffer)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_GBuffer));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y });
	m_PBRGbufferMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_GBuffer));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassGBuffer)->NextSubpass(pDrawCmdBuffer);
	m_pBackgroundMotionMaterial->DrawScreenQuad(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_GBuffer));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassGBuffer)->EndRenderPass(pDrawCmdBuffer);
	m_pBackgroundMotionMaterial->AfterRenderPass(pDrawCmdBuffer);
	m_PBRGbufferMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pMotionTileMaxMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassMotionTileMax)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_MotionTileMax));
	m_pMotionTileMaxMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_MotionTileMax));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassMotionTileMax)->EndRenderPass(pDrawCmdBuffer);
	m_pMotionTileMaxMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pMotionNeighborMaxMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassMotionNeighborMax)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_MotionNeighborMax));
	m_pMotionNeighborMaxMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_MotionNeighborMax));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassMotionNeighborMax)->EndRenderPass(pDrawCmdBuffer);
	m_pMotionNeighborMaxMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pShadowMapMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassShadowMap)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_ShadowMap));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetShadowGenWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetShadowGenWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetShadowGenWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetShadowGenWindowSize().y });
	m_pShadowMapMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_ShadowMap));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassShadowMap)->EndRenderPass(pDrawCmdBuffer);
	m_pShadowMapMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pSSAOMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassSSAO)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_SSAO));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().y });
	m_pSSAOMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_SSAO));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassSSAO)->EndRenderPass(pDrawCmdBuffer);
	m_pSSAOMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pSSAOBlurVMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassSSAOBlurV)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_SSAOBlurV));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().y });
	m_pSSAOBlurVMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_SSAOBlurV));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassSSAOBlurV)->EndRenderPass(pDrawCmdBuffer);
	m_pSSAOBlurVMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pSSAOBlurHMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassSSAOBlurH)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_SSAOBlurH));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetSSAOWindowSize().y });
	m_pSSAOBlurHMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_SSAOBlurH));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassSSAOBlurH)->EndRenderPass(pDrawCmdBuffer);
	m_pSSAOBlurHMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pShadingMaterial->BeforeRenderPass(pDrawCmdBuffer);
	m_pSkyBoxMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassShading)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_Shading));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y });
	m_pShadingMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_Shading));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassShading)->NextSubpass(pDrawCmdBuffer);
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y });
	m_pSkyBoxMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_Shading));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassShading)->EndRenderPass(pDrawCmdBuffer);
	m_pSkyBoxMaterial->AfterRenderPass(pDrawCmdBuffer);
	m_pShadingMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pBloomMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassBloom)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_BloomBlurH));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().y });
	m_pBloomMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_BloomBlurH));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassBloom)->EndRenderPass(pDrawCmdBuffer);
	m_pBloomMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pBloomBlurVMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassBloomBlurV)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_BloomBlurV));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().y });
	m_pBloomBlurVMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_BloomBlurV));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassBloomBlurV)->EndRenderPass(pDrawCmdBuffer);
	m_pBloomBlurVMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_pBloomBlurHMaterial->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassBloomBlurH)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_BloomBlurH));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetBloomWindowSize().y });
	m_pBloomBlurHMaterial->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_BloomBlurH));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassBloomBlurH)->EndRenderPass(pDrawCmdBuffer);
	m_pBloomBlurHMaterial->AfterRenderPass(pDrawCmdBuffer);


	m_postProcessMaterials[pingpong]->BeforeRenderPass(pDrawCmdBuffer);
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassPostProcessing)->BeginRenderPass(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_PostProcessing, pingpong));
	GetGlobalVulkanStates()->SetViewport({ 0, 0, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y, 0, 1 });
	GetGlobalVulkanStates()->SetScissorRect({ 0, 0, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().x, (uint32_t)UniformData::GetInstance()->GetGlobalUniforms()->GetGameWindowSize().y });
	m_postProcessMaterials[pingpong]->Draw(pDrawCmdBuffer, FrameBufferDiction::GetInstance()->GetFrameBuffer(FrameBufferDiction::FrameBufferType_PostProcessing, pingpong));
	RenderPassDiction::GetInstance()->GetPipelineRenderPass(RenderPassDiction::PipelineRenderPassPostProcessing)->EndRenderPass(pDrawCmdBuffer);
	m_postProcessMaterials[pingpong]->AfterRenderPass(pDrawCmdBuffer);
}

void RenderWorkManager::OnFrameBegin()
{
	m_PBRGbufferMaterial->OnFrameBegin();
	m_pShadowMapMaterial->OnFrameBegin();
	m_pSkyBoxMaterial->OnFrameBegin();
}

void RenderWorkManager::OnFrameEnd()
{
	m_PBRGbufferMaterial->OnFrameEnd();
	m_pShadowMapMaterial->OnFrameEnd();
	m_pSkyBoxMaterial->OnFrameEnd();
}