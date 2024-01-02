// Copyright (C) 2024 Jérôme "SirLynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Graphics module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Graphics/Debug.hpp>

namespace Nz
{
	inline RenderWindow::RenderWindow(Swapchain& swapchain) :
	RenderTarget(DefaultRenderOrder),
	m_swapchain(&swapchain),
	m_windowSwapchain(nullptr)
	{
	}
}

#include <Nazara/Graphics/DebugOff.hpp>
