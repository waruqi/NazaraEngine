// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Shader generator"
// For conditions of distribution and use, see copyright notice in Config.hpp

#pragma once

#ifndef NAZARA_SPIRVEXPRESSIONLOAD_HPP
#define NAZARA_SPIRVEXPRESSIONLOAD_HPP

#include <Nazara/Prerequisites.hpp>
#include <Nazara/Shader/Config.hpp>
#include <Nazara/Shader/ShaderAstVisitorExcept.hpp>
#include <Nazara/Shader/ShaderVarVisitorExcept.hpp>
#include <Nazara/Shader/SpirvData.hpp>
#include <vector>

namespace Nz
{
	class SpirvBlock;
	class SpirvWriter;

	class NAZARA_SHADER_API SpirvExpressionLoad : public ShaderAstVisitorExcept, public ShaderVarVisitorExcept
	{
		public:
			inline SpirvExpressionLoad(SpirvWriter& writer, SpirvBlock& block);
			SpirvExpressionLoad(const SpirvExpressionLoad&) = delete;
			SpirvExpressionLoad(SpirvExpressionLoad&&) = delete;
			~SpirvExpressionLoad() = default;

			UInt32 Evaluate(ShaderNodes::Expression& node);

			using ShaderAstVisitor::Visit;
			void Visit(ShaderNodes::AccessMember& node) override;
			void Visit(ShaderNodes::Identifier& node) override;

			using ShaderVarVisitor::Visit;
			void Visit(ShaderNodes::InputVariable& var) override;
			void Visit(ShaderNodes::LocalVariable& var) override;
			void Visit(ShaderNodes::ParameterVariable& var) override;
			void Visit(ShaderNodes::UniformVariable& var) override;

			SpirvExpressionLoad& operator=(const SpirvExpressionLoad&) = delete;
			SpirvExpressionLoad& operator=(SpirvExpressionLoad&&) = delete;

		private:
			struct Pointer
			{
				SpirvStorageClass storage;
				UInt32 resultId;
				UInt32 pointedTypeId;
			};

			struct Value
			{
				UInt32 resultId;
			};

			SpirvBlock& m_block;
			SpirvWriter& m_writer;
			std::variant<std::monostate, Pointer, Value> m_value;
	};
}

#include <Nazara/Shader/SpirvExpressionLoad.inl>

#endif
