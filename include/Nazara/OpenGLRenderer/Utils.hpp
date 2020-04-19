// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - OpenGL Renderer"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_UTILS_OPENGL_HPP
#define NAZARA_UTILS_OPENGL_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Core/String.hpp>
#include <Nazara/Renderer/Enums.hpp>
#include <Nazara/Utility/Enums.hpp>
#include <Nazara/OpenGLRenderer/Wrapper/Loader.hpp>
#include <string>

namespace Nz
{
	inline GLenum ToOpenGL(ShaderStageType stageType);

	//NAZARA_OPENGLRENDERER_API std::string TranslateOpenGLError(GLenum code);
}

#include <Nazara/OpenGLRenderer/Utils.inl>

#endif // NAZARA_UTILS_OPENGL_HPP