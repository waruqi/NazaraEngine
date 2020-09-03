// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - OpenGL Renderer"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_OPENGLRENDERER_EGLCONTEXTWIN32_HPP
#define NAZARA_OPENGLRENDERER_EGLCONTEXTWIN32_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/OpenGLRenderer/Wrapper/EGL/EGLContextBase.hpp>

namespace Nz::GL
{
	class EGLContextX11 final : public EGLContextBase
	{
		public:
			using EGLContextBase::EGLContextBase;
			EGLContextX11(const EGLContextX11&) = default;
			EGLContextX11(EGLContextX11&&) = default;
			~EGLContextX11() = default;

			bool Create(const ContextParams& params, const EGLContextBase* shareContext = nullptr) override;
			bool Create(const ContextParams& params, WindowHandle window, const EGLContextBase* shareContext = nullptr) override;
			void Destroy() override;

			EGLContextX11& operator=(const EGLContextX11&) = default;
			EGLContextX11& operator=(EGLContextX11&&) = default;

		private:
			::Display* m_xdisplay = nullptr;
	};
}

#include <Nazara/OpenGLRenderer/Wrapper/Linux/EGLContextX11.inl>

#endif