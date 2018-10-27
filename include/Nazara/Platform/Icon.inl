// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Engine - Platform module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Core/ErrorFlags.hpp>
#include <Nazara/Platform/Debug.hpp>

namespace Nz
{
	Icon::Icon() :
	m_impl(nullptr)
	{
	}

	inline Icon::Icon(const Image& icon)
	{
		ErrorFlags flags(ErrorFlag_ThrowException, true);
		Create(icon);
	}

	Icon::~Icon()
	{
		Destroy();
	}

	bool Icon::IsValid() const
	{
		return m_impl != nullptr;
	}

	template<typename... Args>
	IconRef Icon::New(Args&&... args)
	{
		return std::make_shared<Icon>(std::forward<Args>(args)...);
	}
}

#include <Nazara/Platform/DebugOff.hpp>
