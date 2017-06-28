#pragma once

#include "DeviceObjectBase.h"

class DescriptorPool;
class DescriptorSetLayout;

class DescriptorSet : public DeviceObjectBase
{
public:
	~DescriptorSet();

	bool Init(const std::shared_ptr<Device>& pDevice, 
		const std::shared_ptr<DescriptorPool>& pDescriptorPool,
		const std::shared_ptr<DescriptorSetLayout>& pDescriptorSetLayout);

public:
	const std::shared_ptr<DescriptorPool> GetDescriptorPool() const { return m_pDescriptorPool; }
	const std::shared_ptr<DescriptorSetLayout> GetDescriptorSetLayout() const { return m_pDescriptorSetLayout; }
	VkDescriptorSet GetDeviceHandle() const { return m_descriptorSet; }

public:
	static std::shared_ptr<DescriptorSet> Create(const std::shared_ptr<Device>& pDevice,
		const std::shared_ptr<DescriptorPool>& pDescriptorPool,
		const std::shared_ptr<DescriptorSetLayout>& pDescriptorSetLayout);

protected:
	VkDescriptorSet									m_descriptorSet;
	std::shared_ptr<DescriptorPool>					m_pDescriptorPool;
	std::shared_ptr<DescriptorSetLayout>			m_pDescriptorSetLayout;
};