// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Development Kit"
// For conditions of distribution and use, see copyright notice in Prerequisites.hpp

#pragma once

#ifndef NDK_COMPONENT_HPP
#define NDK_COMPONENT_HPP

#include <NazaraSDK/BaseComponent.hpp>

namespace Ndk
{
	template<typename ComponentType>
	class Component : public BaseComponent, public Nz::HandledObject<ComponentType>
	{
		public:
			Component();
			virtual ~Component();

			std::unique_ptr<BaseComponent> Clone() const override;

			static ComponentIndex RegisterComponent(ComponentId id);

			template<unsigned int N>
			static ComponentIndex RegisterComponent(const char (&name)[N]);
	};
}

#include <NazaraSDK/Component.inl>

#endif // NDK_COMPONENT_HPP
