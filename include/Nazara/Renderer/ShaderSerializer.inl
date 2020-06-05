// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Renderer module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Renderer/ShaderSerializer.hpp>
#include <Nazara/Renderer/Debug.hpp>

namespace Nz::ShaderAst
{
	template<typename T>
	void ShaderSerializerBase::Container(T& container)
	{
		bool isWriting = IsWriting();

		UInt32 size;
		if (isWriting)
			size = UInt32(container.size());

		Value(size);
		if (!isWriting)
			container.resize(size);
	}


	template<typename T>
	void ShaderSerializerBase::Enum(T& enumVal)
	{
		bool isWriting = IsWriting();

		UInt32 value;
		if (isWriting)
			value = static_cast<UInt32>(enumVal);

		Value(value);
		if (!isWriting)
			enumVal = static_cast<T>(value);
	}

	template<typename T>
	inline void ShaderSerializerBase::Node(std::shared_ptr<T>& node)
	{
		bool isWriting = IsWriting();

		NodePtr value;
		if (isWriting)
			value = node;

		Node(value);
		if (!isWriting)
			node = std::static_pointer_cast<T>(value);
	}

	inline void ShaderSerializerBase::Value(std::size_t& val)
	{
		bool isWriting = IsWriting();

		UInt32 value;
		if (isWriting)
			value = static_cast<UInt32>(val);

		Value(value);
		if (!isWriting)
			val = static_cast<std::size_t>(value);
	}

	inline ShaderSerializer::ShaderSerializer(ByteArray& byteArray) :
	m_byteArray(byteArray),
	m_stream(&m_byteArray, OpenModeFlags(OpenMode_WriteOnly))
	{
	}

	inline ShaderUnserializer::ShaderUnserializer(const ByteArray& byteArray) :
	m_byteArray(byteArray),
	m_stream(const_cast<ByteArray*>(&m_byteArray), OpenModeFlags(OpenMode_ReadOnly))
	{
	}
}

#include <Nazara/Renderer/DebugOff.hpp>