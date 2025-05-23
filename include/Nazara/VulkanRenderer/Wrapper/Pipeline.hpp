// Copyright (C) 2025 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Vulkan renderer"
// For conditions of distribution and use, see copyright notice in Export.hpp

#pragma once

#ifndef NAZARA_VULKANRENDERER_WRAPPER_PIPELINE_HPP
#define NAZARA_VULKANRENDERER_WRAPPER_PIPELINE_HPP

#include <NazaraUtils/Prerequisites.hpp>
#include <Nazara/VulkanRenderer/Wrapper/DeviceObject.hpp>
#include <string_view>

namespace Nz::Vk
{
	class Pipeline
	{
		public:
			inline Pipeline();
			Pipeline(const Pipeline&) = delete;
			Pipeline(Pipeline&&) noexcept;
			inline ~Pipeline();

			inline bool CreateCompute(Device& device, const VkComputePipelineCreateInfo& createInfo, VkPipelineCache cache = VK_NULL_HANDLE, const VkAllocationCallbacks* allocator = nullptr);
			inline bool CreateGraphics(Device& device, const VkGraphicsPipelineCreateInfo& createInfo, VkPipelineCache cache = VK_NULL_HANDLE, const VkAllocationCallbacks* allocator = nullptr);
			inline void Destroy();

			inline Device& GetDevice() const;
			inline VkResult GetLastErrorCode() const;

			inline void SetDebugName(std::string_view name);

			Pipeline& operator=(const Pipeline&) = delete;
			Pipeline& operator=(Pipeline&&) = delete;

			inline operator VkPipeline() const;

		protected:
			inline bool Create(Device& device, VkResult result, const VkAllocationCallbacks* allocator);

			MovablePtr<Device> m_device;
			VkAllocationCallbacks m_allocator;
			VkPipeline m_handle;
			mutable VkResult m_lastErrorCode;
	};
}

#include <Nazara/VulkanRenderer/Wrapper/Pipeline.inl>

#endif // NAZARA_VULKANRENDERER_WRAPPER_PIPELINE_HPP
