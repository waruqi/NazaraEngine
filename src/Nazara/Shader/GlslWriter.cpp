// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Shader generator"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Shader/GlslWriter.hpp>
#include <Nazara/Core/Algorithm.hpp>
#include <Nazara/Core/CallOnExit.hpp>
#include <Nazara/Math/Algorithm.hpp>
#include <Nazara/Shader/ShaderBuilder.hpp>
#include <Nazara/Shader/ShaderAstCloner.hpp>
#include <Nazara/Shader/ShaderAstUtils.hpp>
#include <Nazara/Shader/ShaderAstValidator.hpp>
#include <optional>
#include <stdexcept>
#include <Nazara/Shader/Debug.hpp>

namespace Nz
{
	namespace
	{
		static const char* flipYUniformName = "_NzFlipValue";
		static const char* overridenMain = "_NzMain";

		//FIXME: Have this only once
		std::unordered_map<std::string, ShaderStageType> s_entryPoints = {
			{ "frag", ShaderStageType::Fragment },
			{ "vert", ShaderStageType::Vertex },
		};

		struct PreVisitor : ShaderAst::AstCloner
		{
			using AstCloner::Clone;

			ShaderAst::StatementPtr Clone(ShaderAst::DeclareFunctionStatement& node) override
			{
				auto clone = AstCloner::Clone(node);
				assert(clone->GetType() == ShaderAst::NodeType::DeclareFunctionStatement);

				ShaderAst::DeclareFunctionStatement* func = static_cast<ShaderAst::DeclareFunctionStatement*>(clone.get());

				bool hasEntryPoint = false;

				for (auto& attribute : func->attributes)
				{
					if (attribute.type == ShaderAst::AttributeType::Entry)
					{
						auto it = s_entryPoints.find(std::get<std::string>(attribute.args));
						assert(it != s_entryPoints.end());

						if (it->second == selectedEntryPoint)
						{
							hasEntryPoint = true;
							break;
						}
					}
				}

				if (!hasEntryPoint)
					return ShaderBuilder::NoOp();

				entryPoint = func;

				if (func->name == "main")
					func->name = "_NzMain";

				return clone;
			}

			ShaderStageType selectedEntryPoint;
			ShaderAst::DeclareFunctionStatement* entryPoint = nullptr;
		};

		struct EntryFuncResolver : ShaderAst::AstScopedVisitor
		{
			void Visit(ShaderAst::DeclareFunctionStatement& node) override
			{
				

				if (&node != entryPoint)
					return;

				assert(node.parameters.size() == 1);

				const ShaderAst::ExpressionType& inputType = node.parameters.front().type;
				const ShaderAst::ExpressionType& outputType = node.returnType;

				const Identifier* identifier;

				assert(IsIdentifierType(node.parameters.front().type));
				identifier = FindIdentifier(std::get<ShaderAst::IdentifierType>(inputType).name);
				assert(identifier);

				inputIdentifier = *identifier;

				assert(IsIdentifierType(outputType));
				identifier = FindIdentifier(std::get<ShaderAst::IdentifierType>(outputType).name);
				assert(identifier);

				outputIdentifier = *identifier;
			}

			Identifier inputIdentifier;
			Identifier outputIdentifier;
			ShaderAst::DeclareFunctionStatement* entryPoint;
		};

		struct Builtin
		{
			std::string identifier;
			ShaderStageTypeFlags stageFlags;
		};

		std::unordered_map<std::string, Builtin> builtinMapping = {
			{ "position", { "gl_Position", ShaderStageType::Vertex } }
		};
	}


	struct GlslWriter::State
	{
		const States* states = nullptr;
		ShaderAst::DeclareFunctionStatement* entryFunc = nullptr;
		std::stringstream stream;
		unsigned int indentLevel = 0;
	};


	GlslWriter::GlslWriter() :
	m_currentState(nullptr)
	{
	}

	std::string GlslWriter::Generate(ShaderStageType shaderStage, ShaderAst::StatementPtr& shader, const States& conditions)
	{
		State state;
		m_currentState = &state;
		CallOnExit onExit([this]()
		{
			m_currentState = nullptr;
		});

		std::string error;
		if (!ShaderAst::ValidateAst(shader, &error))
			throw std::runtime_error("Invalid shader AST: " + error);

		PreVisitor previsitor;
		previsitor.selectedEntryPoint = shaderStage;

		ShaderAst::StatementPtr adaptedShader = previsitor.Clone(shader);

		if (!previsitor.entryPoint)
			throw std::runtime_error("missing entry point");

		state.entryFunc = previsitor.entryPoint;

		unsigned int glslVersion;
		if (m_environment.glES)
		{
			if (m_environment.glMajorVersion >= 3 && m_environment.glMinorVersion >= 2)
				glslVersion = 320;
			else if (m_environment.glMajorVersion >= 3 && m_environment.glMinorVersion >= 1)
				glslVersion = 310;
			else if (m_environment.glMajorVersion >= 3)
				glslVersion = 300;
			else if (m_environment.glMajorVersion >= 2)
				glslVersion = 100;
			else
				throw std::runtime_error("This version of OpenGL ES does not support shaders");
		}
		else
		{
			if (m_environment.glMajorVersion >= 3 && m_environment.glMinorVersion >= 3)
				glslVersion = m_environment.glMajorVersion * 100 + m_environment.glMinorVersion * 10;
			else if (m_environment.glMajorVersion >= 3 && m_environment.glMinorVersion >= 2)
				glslVersion = 150;
			else if (m_environment.glMajorVersion >= 3 && m_environment.glMinorVersion >= 1)
				glslVersion = 140;
			else if (m_environment.glMajorVersion >= 3)
				glslVersion = 130;
			else if (m_environment.glMajorVersion >= 2 && m_environment.glMinorVersion >= 1)
				glslVersion = 120;
			else if (m_environment.glMajorVersion >= 2)
				glslVersion = 110;
			else
				throw std::runtime_error("This version of OpenGL does not support shaders");
		}

		// Header
		Append("#version ");
		Append(glslVersion);
		if (m_environment.glES)
			Append(" es");

		AppendLine();
		AppendLine();

		// Extensions

		std::vector<std::string> requiredExtensions;

		if (!m_environment.glES && m_environment.extCallback)
		{
			// GL_ARB_shading_language_420pack (required for layout(binding = X))
			if (glslVersion < 420 && HasExplicitBinding(adaptedShader))
			{
				if (m_environment.extCallback("GL_ARB_shading_language_420pack"))
					requiredExtensions.emplace_back("GL_ARB_shading_language_420pack");
			}

			// GL_ARB_separate_shader_objects (required for layout(location = X))
			if (glslVersion < 410 && HasExplicitLocation(adaptedShader))
			{
				if (m_environment.extCallback("GL_ARB_separate_shader_objects"))
					requiredExtensions.emplace_back("GL_ARB_separate_shader_objects");
			}
		}

		if (!requiredExtensions.empty())
		{
			for (const std::string& ext : requiredExtensions)
				AppendLine("#extension " + ext + " : require");

			AppendLine();
		}

		if (m_environment.glES)
		{
			AppendLine("#if GL_FRAGMENT_PRECISION_HIGH");
			AppendLine("precision highp float;");
			AppendLine("#else");
			AppendLine("precision mediump float;");
			AppendLine("#endif");
			AppendLine();
		}

		PushScope();
		{
			adaptedShader->Visit(*this);

			// Append true GLSL entry point
			AppendEntryPoint(shaderStage, adaptedShader);
		}
		PopScope();

		return state.stream.str();
	}

	void GlslWriter::SetEnv(Environment environment)
	{
		m_environment = std::move(environment);
	}

	const char* GlslWriter::GetFlipYUniformName()
	{
		return flipYUniformName;
	}

	void GlslWriter::Append(const ShaderAst::ExpressionType& type)
	{
		std::visit([&](auto&& arg)
		{
			Append(arg);
		}, type);
	}

	void GlslWriter::Append(ShaderAst::BuiltinEntry builtin)
	{
		switch (builtin)
		{
			case ShaderAst::BuiltinEntry::VertexPosition:
				Append("gl_Position");
				break;
		}
	}

	void GlslWriter::Append(const ShaderAst::IdentifierType& identifierType)
	{
		Append(identifierType.name);
	}

	void GlslWriter::Append(const ShaderAst::MatrixType& matrixType)
	{
		if (matrixType.columnCount == matrixType.rowCount)
		{
			Append("mat");
			Append(matrixType.columnCount);
		}
		else
		{
			Append("mat");
			Append(matrixType.columnCount);
			Append("x");
			Append(matrixType.rowCount);
		}
	}

	void GlslWriter::Append(ShaderAst::PrimitiveType type)
	{
		switch (type)
		{
			case ShaderAst::PrimitiveType::Boolean: return Append("bool");
			case ShaderAst::PrimitiveType::Float32: return Append("float");
			case ShaderAst::PrimitiveType::Int32:   return Append("ivec2");
			case ShaderAst::PrimitiveType::UInt32:  return Append("uint");
		}
	}

	void GlslWriter::Append(const ShaderAst::SamplerType& samplerType)
	{
		switch (samplerType.sampledType)
		{
			case ShaderAst::PrimitiveType::Boolean:
			case ShaderAst::PrimitiveType::Float32:
				break;

			case ShaderAst::PrimitiveType::Int32:   Append("i"); break;
			case ShaderAst::PrimitiveType::UInt32:  Append("u"); break;
		}

		Append("sampler");

		switch (samplerType.dim)
		{
			case ImageType_1D:       Append("1D");      break;
			case ImageType_1D_Array: Append("1DArray"); break;
			case ImageType_2D:       Append("2D");      break;
			case ImageType_2D_Array: Append("2DArray"); break;
			case ImageType_3D:       Append("3D");      break;
			case ImageType_Cubemap:  Append("Cube");    break;
		}
	}

	void GlslWriter::Append(const ShaderAst::UniformType& uniformType)
	{
		/* TODO */
	}

	void GlslWriter::Append(const ShaderAst::VectorType& vecType)
	{
		switch (vecType.type)
		{
			case ShaderAst::PrimitiveType::Boolean: Append("b"); break;
			case ShaderAst::PrimitiveType::Float32: break;
			case ShaderAst::PrimitiveType::Int32:   Append("i"); break;
			case ShaderAst::PrimitiveType::UInt32:  Append("u"); break;
		}

		Append("vec");
		Append(vecType.componentCount);
	}

	void GlslWriter::Append(ShaderAst::MemoryLayout layout)
	{
		switch (layout)
		{
			case ShaderAst::MemoryLayout::Std140:
				Append("std140");
				break;
		}
	}

	void GlslWriter::Append(ShaderAst::NoType)
	{
		return Append("void");
	}

	template<typename T>
	void GlslWriter::Append(const T& param)
	{
		NazaraAssert(m_currentState, "This function should only be called while processing an AST");

		m_currentState->stream << param;
	}
	template<typename T1, typename T2, typename... Args>
	void GlslWriter::Append(const T1& firstParam, const T2& secondParam, Args&&... params)
	{
		Append(firstParam);
		Append(secondParam, std::forward<Args>(params)...);
	}

	void GlslWriter::AppendCommentSection(const std::string& section)
	{
		NazaraAssert(m_currentState, "This function should only be called while processing an AST");

		std::string stars((section.size() < 33) ? (36 - section.size()) / 2 : 3, '*');
		m_currentState->stream << "/*" << stars << ' ' << section << ' ' << stars << "*/";
		AppendLine();
	}

	void GlslWriter::AppendEntryPoint(ShaderStageType shaderStage, ShaderAst::StatementPtr& shader)
	{
		EntryFuncResolver entryResolver;
		entryResolver.entryPoint = m_currentState->entryFunc;
		entryResolver.ScopedVisit(shader);

		AppendLine();
		AppendLine("// Entry point handling");

		struct InOutField
		{
			std::string name;
			std::string targetName;
		};

		std::vector<InOutField> inputFields;
		const ShaderAst::StructDescription* inputStruct = nullptr;

		auto HandleInOutStructs = [this, shaderStage](const Identifier& identifier, std::vector<InOutField>& fields, const char* keyword, const char* fromPrefix, const char* targetPrefix) -> const ShaderAst::StructDescription*
		{
			assert(std::holds_alternative<ShaderAst::StructDescription>(identifier.value));
			const auto& s = std::get<ShaderAst::StructDescription>(identifier.value);

			for (const auto& member : s.members)
			{
				bool skip = false;
				std::optional<std::string> builtinName;
				std::optional<long long> attributeLocation;
				for (const auto& [attributeType, attributeParam] : member.attributes)
				{
					if (attributeType == ShaderAst::AttributeType::Builtin)
					{
						auto it = builtinMapping.find(std::get<std::string>(attributeParam));
						if (it != builtinMapping.end())
						{
							const Builtin& builtin = it->second;
							if (!builtin.stageFlags.Test(shaderStage))
							{
								skip = true;
								break;
							}

							builtinName = builtin.identifier;
							break;
						}
					}
					else if (attributeType == ShaderAst::AttributeType::Location)
					{
						attributeLocation = std::get<long long>(attributeParam);
						break;
					}
				}

				if (!skip && attributeLocation)
				{
					Append("layout(location = ");
					Append(*attributeLocation);
					Append(") ");
					Append(keyword);
					Append(" ");
					Append(member.type);
					Append(" ");
					Append(targetPrefix);
					Append(member.name);
					AppendLine(";");

					fields.push_back({
						fromPrefix + member.name,
						targetPrefix + member.name
					});
				}
				else if (builtinName)
				{
					fields.push_back({
						fromPrefix + member.name,
						*builtinName
					});
				}
			}
			AppendLine();

			return &s;
		};

		if (!m_currentState->entryFunc->parameters.empty())
			inputStruct = HandleInOutStructs(entryResolver.inputIdentifier, inputFields, "in", "_nzInput.", "_NzIn_");

		std::vector<InOutField> outputFields;
		const ShaderAst::StructDescription* outputStruct = nullptr;
		if (!IsNoType(m_currentState->entryFunc->returnType))
			outputStruct = HandleInOutStructs(entryResolver.outputIdentifier, outputFields, "out", "_nzOutput.", "_NzOut_");

		if (shaderStage == ShaderStageType::Vertex && m_environment.flipYPosition)
			AppendLine("uniform float ", flipYUniformName, ";");

		AppendLine("void main()");
		EnterScope();
		{
			if (inputStruct)
			{
				Append(inputStruct->name);
				AppendLine(" _nzInput;");
				for (const auto& [name, targetName] : inputFields)
				{
					AppendLine(name, " = ", targetName, ";");
				}
				AppendLine();
			}

			if (outputStruct)
				Append(outputStruct->name, " _nzOutput = ");

			Append(m_currentState->entryFunc->name);

			Append("(");
			if (m_currentState->entryFunc)
				Append("_nzInput");
			Append(");");

			if (outputStruct)
			{
				AppendLine();

				for (const auto& [name, targetName] : outputFields)
				{
					bool isOutputPosition = (shaderStage == ShaderStageType::Vertex && m_environment.flipYPosition && targetName == "gl_Position");

					AppendLine();

					Append(targetName, " = ", name);
					if (isOutputPosition)
						Append(" * vec4(1.0, ", flipYUniformName, ", 1.0, 1.0)");

					Append(";");
				}
			}
		}
		LeaveScope();
	}

	void GlslWriter::AppendField(const std::string& structName, const std::string* memberIdentifier, std::size_t remainingMembers)
	{
		Append(".");
		Append(memberIdentifier[0]);

		const Identifier* identifier = FindIdentifier(structName);
		assert(identifier);

		assert(std::holds_alternative<ShaderAst::StructDescription>(identifier->value));
		const auto& s = std::get<ShaderAst::StructDescription>(identifier->value);

		auto memberIt = std::find_if(s.members.begin(), s.members.begin(), [&](const auto& field) { return field.name == memberIdentifier[0]; });
		assert(memberIt != s.members.end());

		const auto& member = *memberIt;

		if (remainingMembers > 1)
			AppendField(std::get<ShaderAst::IdentifierType>(member.type).name, memberIdentifier + 1, remainingMembers - 1);
	}

	void GlslWriter::AppendLine(const std::string& txt)
	{
		NazaraAssert(m_currentState, "This function should only be called while processing an AST");

		m_currentState->stream << txt << '\n' << std::string(m_currentState->indentLevel, '\t');
	}

	template<typename... Args>
	void GlslWriter::AppendLine(Args&&... params)
	{
		(Append(std::forward<Args>(params)), ...);
		AppendLine();
	}

	void GlslWriter::EnterScope()
	{
		NazaraAssert(m_currentState, "This function should only be called while processing an AST");

		m_currentState->indentLevel++;
		AppendLine("{");
	}

	void GlslWriter::LeaveScope(bool skipLine)
	{
		NazaraAssert(m_currentState, "This function should only be called while processing an AST");

		m_currentState->indentLevel--;
		AppendLine();

		if (skipLine)
			AppendLine("}");
		else
			Append("}");
	}

	void GlslWriter::Visit(ShaderAst::ExpressionPtr& expr, bool encloseIfRequired)
	{
		bool enclose = encloseIfRequired && (GetExpressionCategory(*expr) != ShaderAst::ExpressionCategory::LValue);

		if (enclose)
			Append("(");

		expr->Visit(*this);

		if (enclose)
			Append(")");
	}

	void GlslWriter::Visit(ShaderAst::AccessMemberExpression& node)
	{
		Visit(node.structExpr, true);

		const ShaderAst::ExpressionType& exprType = node.structExpr->cachedExpressionType.value();
		assert(IsIdentifierType(exprType));

		AppendField(std::get<ShaderAst::IdentifierType>(exprType).name, node.memberIdentifiers.data(), node.memberIdentifiers.size());
	}

	void GlslWriter::Visit(ShaderAst::AssignExpression& node)
	{
		node.left->Visit(*this);

		switch (node.op)
		{
			case ShaderAst::AssignType::Simple:
				Append(" = ");
				break;
		}

		node.right->Visit(*this);
	}

	void GlslWriter::Visit(ShaderAst::BranchStatement& node)
	{
		bool first = true;
		for (const auto& statement : node.condStatements)
		{
			if (!first)
				Append("else ");

			Append("if (");
			statement.condition->Visit(*this);
			AppendLine(")");

			EnterScope();
			PushScope();
			statement.statement->Visit(*this);
			PopScope();
			LeaveScope();

			first = false;
		}

		if (node.elseStatement)
		{
			AppendLine("else");

			EnterScope();
			PushScope();
			node.elseStatement->Visit(*this);
			PopScope();
			LeaveScope();
		}
	}

	void GlslWriter::Visit(ShaderAst::BinaryExpression& node)
	{
		Visit(node.left, true);

		switch (node.op)
		{
			case ShaderAst::BinaryType::Add:       Append(" + "); break;
			case ShaderAst::BinaryType::Subtract:  Append(" - "); break;
			case ShaderAst::BinaryType::Multiply:  Append(" * "); break;
			case ShaderAst::BinaryType::Divide:    Append(" / "); break;

			case ShaderAst::BinaryType::CompEq:    Append(" == "); break;
			case ShaderAst::BinaryType::CompGe:    Append(" >= "); break;
			case ShaderAst::BinaryType::CompGt:    Append(" > ");  break;
			case ShaderAst::BinaryType::CompLe:    Append(" <= "); break;
			case ShaderAst::BinaryType::CompLt:    Append(" < ");  break;
			case ShaderAst::BinaryType::CompNe:    Append(" != "); break;
		}

		Visit(node.right, true);
	}

	void GlslWriter::Visit(ShaderAst::CastExpression& node)
	{
		Append(node.targetType);
		Append("(");

		bool first = true;
		for (const auto& exprPtr : node.expressions)
		{
			if (!exprPtr)
				break;

			if (!first)
				m_currentState->stream << ", ";

			exprPtr->Visit(*this);
			first = false;
		}

		Append(")");
	}

	void GlslWriter::Visit(ShaderAst::ConditionalExpression& node)
	{
		/*std::size_t conditionIndex = m_context.shader->FindConditionByName(node.conditionName);
		assert(conditionIndex != ShaderAst::InvalidCondition);

		if (TestBit<Nz::UInt64>(m_context.states->enabledConditions, conditionIndex))
			Visit(node.truePath);
		else
			Visit(node.falsePath);*/
	}

	void GlslWriter::Visit(ShaderAst::ConditionalStatement& node)
	{
		/*std::size_t conditionIndex = m_context.shader->FindConditionByName(node.conditionName);
		assert(conditionIndex != ShaderAst::InvalidCondition);

		if (TestBit<Nz::UInt64>(m_context.states->enabledConditions, conditionIndex))
			Visit(node.statement);*/
	}

	void GlslWriter::Visit(ShaderAst::ConstantExpression& node)
	{
		std::visit([&](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<T, Vector2i32> || std::is_same_v<T, Vector3i32> || std::is_same_v<T, Vector4i32>)
				Append("i"); //< for ivec

			if constexpr (std::is_same_v<T, bool>)
				Append((arg) ? "true" : "false");
			else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, Int32> || std::is_same_v<T, UInt32>)
				Append(std::to_string(arg));
			else if constexpr (std::is_same_v<T, Vector2f> || std::is_same_v<T, Vector2i32>)
				Append("vec2(" + std::to_string(arg.x) + ", " + std::to_string(arg.y) + ")");
			else if constexpr (std::is_same_v<T, Vector3f> || std::is_same_v<T, Vector3i32>)
				Append("vec3(" + std::to_string(arg.x) + ", " + std::to_string(arg.y) + ", " + std::to_string(arg.z) + ")");
			else if constexpr (std::is_same_v<T, Vector4f> || std::is_same_v<T, Vector4i32>)
				Append("vec4(" + std::to_string(arg.x) + ", " + std::to_string(arg.y) + ", " + std::to_string(arg.z) + ", " + std::to_string(arg.w) + ")");
			else
				static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
		}, node.value);
	}

	void GlslWriter::Visit(ShaderAst::DeclareExternalStatement& node)
	{
		

		for (const auto& externalVar : node.externalVars)
		{
			std::optional<long long> bindingIndex;
			bool isStd140 = false;
			for (const auto& [attributeType, attributeParam] : externalVar.attributes)
			{
				if (attributeType == ShaderAst::AttributeType::Binding)
					bindingIndex = std::get<long long>(attributeParam);
				else if (attributeType == ShaderAst::AttributeType::Layout)
				{
					if (std::get<std::string>(attributeParam) == "std140")
						isStd140 = true;
				}
			}

			if (bindingIndex)
			{
				Append("layout(binding = ");
				Append(*bindingIndex);
				if (isStd140)
					Append(", std140");

				Append(") uniform ");

				if (IsUniformType(externalVar.type))
				{
					Append("_NzBinding_");
					AppendLine(externalVar.name);

					EnterScope();
					{
						const Identifier* identifier = FindIdentifier(std::get<ShaderAst::UniformType>(externalVar.type).containedType.name);
						assert(identifier);

						assert(std::holds_alternative<ShaderAst::StructDescription>(identifier->value));
						const auto& s = std::get<ShaderAst::StructDescription>(identifier->value);

						bool first = true;
						for (const auto& [name, attribute, type] : s.members)
						{
							if (!first)
								AppendLine();

							first = false;

							Append(type);
							Append(" ");
							Append(name);
							Append(";");
						}
					}
					LeaveScope(false);
				}
				else
					Append(externalVar.type);

				Append(" ");
				Append(externalVar.name);
				AppendLine(";");
			}
		}
	}

	void GlslWriter::Visit(ShaderAst::DeclareFunctionStatement& node)
	{
		NazaraAssert(m_currentState, "This function should only be called while processing an AST");

		Append(node.returnType);
		Append(" ");
		Append(node.name);
		Append("(");
		for (std::size_t i = 0; i < node.parameters.size(); ++i)
		{
			if (i != 0)
				Append(", ");
			Append(node.parameters[i].type);
			Append(" ");
			Append(node.parameters[i].name);
		}
		Append(")\n");

		EnterScope();
		PushScope();
		{
			for (auto& statement : node.statements)
				statement->Visit(*this);
		}
		PopScope();
		LeaveScope();
	}

	void GlslWriter::Visit(ShaderAst::DeclareStructStatement& node)
	{
		RegisterStruct(node.description);

		Append("struct ");
		AppendLine(node.description.name);
		EnterScope();
		{
			bool first = true;
			for (const auto& [name, attribute, type] : node.description.members)
			{
				if (!first)
					AppendLine();

				first = false;

				Append(type);
				Append(" ");
				Append(name);
				Append(";");
			}
		}
		LeaveScope(false);
		AppendLine(";");
	}

	void GlslWriter::Visit(ShaderAst::DeclareVariableStatement& node)
	{
		RegisterVariable(node.varName, node.varType);

		Append(node.varType);
		Append(" ");
		Append(node.varName);
		if (node.initialExpression)
		{
			Append(" = ");
			node.initialExpression->Visit(*this);
		}

		AppendLine(";");
	}

	void GlslWriter::Visit(ShaderAst::DiscardStatement& /*node*/)
	{
		Append("discard;");
	}

	void GlslWriter::Visit(ShaderAst::ExpressionStatement& node)
	{
		node.expression->Visit(*this);
		AppendLine(";");
	}

	void GlslWriter::Visit(ShaderAst::IdentifierExpression& node)
	{
		Append(node.identifier);
	}

	void GlslWriter::Visit(ShaderAst::IntrinsicExpression& node)
	{
		switch (node.intrinsic)
		{
			case ShaderAst::IntrinsicType::CrossProduct:
				Append("cross");
				break;

			case ShaderAst::IntrinsicType::DotProduct:
				Append("dot");
				break;

			case ShaderAst::IntrinsicType::SampleTexture:
				Append("texture");
				break;
		}

		Append("(");
		for (std::size_t i = 0; i < node.parameters.size(); ++i)
		{
			if (i != 0)
				Append(", ");

			node.parameters[i]->Visit(*this);
		}
		Append(")");
	}

	void GlslWriter::Visit(ShaderAst::MultiStatement& node)
	{
		PushScope();

		bool first = true;
		for (const ShaderAst::StatementPtr& statement : node.statements)
		{
			if (!first && statement->GetType() != ShaderAst::NodeType::NoOpStatement)
				AppendLine();

			statement->Visit(*this);

			first = false;
		}

		PopScope();
	}

	void GlslWriter::Visit(ShaderAst::NoOpStatement& /*node*/)
	{
		/* nothing to do */
	}

	void GlslWriter::Visit(ShaderAst::ReturnStatement& node)
	{
		if (node.returnExpr)
		{
			Append("return ");
			node.returnExpr->Visit(*this);
			Append(";");
		}
		else
			Append("return;");
	}

	void GlslWriter::Visit(ShaderAst::SwizzleExpression& node)
	{
		Visit(node.expression, true);
		Append(".");

		for (std::size_t i = 0; i < node.componentCount; ++i)
		{
			switch (node.components[i])
			{
				case ShaderAst::SwizzleComponent::First:
					Append("x");
					break;

				case ShaderAst::SwizzleComponent::Second:
					Append("y");
					break;

				case ShaderAst::SwizzleComponent::Third:
					Append("z");
					break;

				case ShaderAst::SwizzleComponent::Fourth:
					Append("w");
					break;
			}
		}
	}

	bool GlslWriter::HasExplicitBinding(ShaderAst::StatementPtr& shader)
	{
		/*for (const auto& uniform : shader.GetUniforms())
		{
			if (uniform.bindingIndex.has_value())
				return true;
		}*/

		return false;
	}

	bool GlslWriter::HasExplicitLocation(ShaderAst::StatementPtr& shader)
	{
		/*for (const auto& input : shader.GetInputs())
		{
			if (input.locationIndex.has_value())
				return true;
		}

		for (const auto& output : shader.GetOutputs())
		{
			if (output.locationIndex.has_value())
				return true;
		}*/

		return false;
	}

}
