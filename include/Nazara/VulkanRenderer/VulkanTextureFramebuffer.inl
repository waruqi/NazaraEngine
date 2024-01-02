// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Vulkan renderer"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/VulkanRenderer/Debug.hpp>

namespace Nz
{
	inline Vk::Framebuffer& VulkanTextureFramebuffer::GetFramebuffer()
	{
		return m_framebuffer;
	}

	inline const Vk::Framebuffer& VulkanTextureFramebuffer::GetFramebuffer() const
	{
		return m_framebuffer;
	}
}

#include <Nazara/VulkanRenderer/DebugOff.hpp>
