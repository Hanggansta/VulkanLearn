#pragma once

#include "DeviceObjectBase.h"
#include <gli\gli.hpp>

class SwapChain;
class MemoryKey;
class CommandBuffer;
class StagingBuffer;

class Image : public DeviceObjectBase<Image>
{
public:
	~Image();

public:
	VkImage GetDeviceHandle() const { return m_image; }
	VkDescriptorImageInfo GetDescriptorInfo() const { return m_descriptorImgInfo; }
	const VkImageCreateInfo& GetImageInfo() const { return m_info; }
	virtual uint32_t GetMemoryProperty() const { return m_memProperty; }
	virtual VkMemoryRequirements GetMemoryReqirments() const;
	virtual VkImageViewCreateInfo GetViewInfo() const { return m_viewInfo; }
	virtual VkImageView GetViewDeviceHandle() const { return m_view; }
	virtual void EnsureImageLayout();

protected:
	virtual void BindMemory(VkDeviceMemory memory, uint32_t offset) const;

	bool Init(const std::shared_ptr<Device>& pDevice, const std::shared_ptr<Image>& pSelf, VkImage img);
	bool Init(const std::shared_ptr<Device>& pDevice, const std::shared_ptr<Image>& pSelf, const VkImageCreateInfo& info, uint32_t memoryPropertyFlag);
	bool Init(const std::shared_ptr<Device>& pDevice, const std::shared_ptr<Image>& pSelf, const gli::texture& gliTex, const VkImageCreateInfo& info, uint32_t memoryPropertyFlag);

	virtual void CreateImageView();
	virtual void CreateSampler();

	void UpdateByteStream(const gli::texture& gliTex);
	virtual std::shared_ptr<StagingBuffer> PrepareStagingBuffer(const gli::texture& gliTex, const std::shared_ptr<CommandBuffer>& pCmdBuffer);
	virtual void ChangeImageLayoutBeforeCopy(const gli::texture& gliTex, const std::shared_ptr<CommandBuffer>& pCmdBuffer);
	virtual void ExecuteCopy(const gli::texture& gliTex, const std::shared_ptr<StagingBuffer>& pStagingBuffer, const std::shared_ptr<CommandBuffer>& pCmdBuffer);
	virtual void ChangeImageLayoutAfterCopy(const gli::texture& gliTex, const std::shared_ptr<CommandBuffer>& pCmdBuffer);

protected:
	VkImage						m_image;
	VkImageCreateInfo			m_info;

	bool						m_shouldDestoryRawImage = true;

	// Put image view here, maybe for now, since I don't need one image with multiple view at this moment
	VkImageViewCreateInfo		m_viewInfo;
	VkImageView					m_view;
	VkSamplerCreateInfo			m_samplerInfo;
	VkSampler					m_sampler;

	VkDescriptorImageInfo		m_descriptorImgInfo;

	uint32_t					m_memProperty;

	std::shared_ptr<MemoryKey>	m_pMemKey;

	friend class DeviceMemoryManager;
};