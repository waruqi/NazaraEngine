// Copyright (C) 201 Jérôme Leclercq
// This file is part of the "Nazara Engine - Vulkan Renderer"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_VULKANRENDERER_RENDERTARGET_HPP
#define NAZARA_VULKANRENDERER_RENDERTARGET_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Core/Signal.hpp>
#include <Nazara/VulkanRenderer/Config.hpp>
#include <Nazara/VulkanRenderer/Wrapper/FrameBuffer.hpp>
#include <Nazara/VulkanRenderer/Wrapper/RenderPass.hpp>
#include <Nazara/VulkanRenderer/Wrapper/Semaphore.hpp>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace Nz
{
	class NAZARA_VULKANRENDERER_API VkRenderTarget
	{
		public:
			VkRenderTarget() = default;
			VkRenderTarget(const VkRenderTarget&) = delete;
			VkRenderTarget(VkRenderTarget&&) = delete; ///TOOD?
			virtual ~VkRenderTarget();

			virtual bool Acquire(UInt32* imageIndex, VkSemaphore signalSemaphore = VK_NULL_HANDLE, VkFence signalFence = VK_NULL_HANDLE) const = 0;

			virtual const Vk::Framebuffer& GetFrameBuffer(UInt32 imageIndex) const = 0;
			virtual UInt32 GetFramebufferCount() const = 0;

			inline const Vk::RenderPass& GetRenderPass() const;

			virtual void Present(UInt32 imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE) = 0;

			VkRenderTarget& operator=(const VkRenderTarget&) = delete;
			VkRenderTarget& operator=(VkRenderTarget&&) = delete; ///TOOD?

			// Signals:
			NazaraSignal(OnRenderTargetRelease,	const VkRenderTarget* /*renderTarget*/);
			NazaraSignal(OnRenderTargetSizeChange, const VkRenderTarget* /*renderTarget*/);

		protected:
			void Destroy();

			Vk::RenderPass m_renderPass;
	};
}

#include <Nazara/VulkanRenderer/VkRenderTarget.inl>

#endif // NAZARA_VULKANRENDERER_RENDERTARGET_HPP
