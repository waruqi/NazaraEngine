// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Shader generator"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Shader/SpirvWriter.hpp>
#include <Nazara/Core/CallOnExit.hpp>
#include <Nazara/Core/StackVector.hpp>
#include <Nazara/Shader/ShaderAstCloner.hpp>
#include <Nazara/Shader/ShaderAstValidator.hpp>
#include <Nazara/Shader/SpirvAstVisitor.hpp>
#include <Nazara/Shader/SpirvBlock.hpp>
#include <Nazara/Shader/SpirvConstantCache.hpp>
#include <Nazara/Shader/SpirvData.hpp>
#include <Nazara/Shader/SpirvSection.hpp>
#include <Nazara/Shader/Ast/TransformVisitor.hpp>
#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>
#include <SpirV/GLSL.std.450.h>
#include <cassert>
#include <map>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <Nazara/Shader/Debug.hpp>

namespace Nz
{
	namespace
	{
		//FIXME: Have this only once
		std::unordered_map<std::string, ShaderStageType> s_entryPoints = {
			{ "frag", ShaderStageType::Fragment },
			{ "vert", ShaderStageType::Vertex },
		};

		struct Builtin
		{
			const char* debugName;
			ShaderStageTypeFlags compatibleStages;
			SpirvBuiltIn decoration;
		};

		std::unordered_map<std::string, Builtin> s_builtinMapping = {
			{ "position", { "VertexPosition", ShaderStageType::Vertex, SpirvBuiltIn::Position } }
		};

		class PreVisitor : public ShaderAst::AstScopedVisitor
		{
			public:
				struct UniformVar
				{
					std::optional<UInt32> bindingIndex;
					UInt32 pointerId;
				};

				using BuiltinDecoration = std::map<UInt32, SpirvBuiltIn>;
				using LocationDecoration = std::map<UInt32, UInt32>;
				using ExtInstList = std::unordered_set<std::string>;
				using ExtVarContainer = std::unordered_map<std::size_t /*varIndex*/, UniformVar>;
				using LocalContainer = std::unordered_set<ShaderAst::ExpressionType>;
				using FunctionContainer = std::vector<std::reference_wrapper<ShaderAst::DeclareFunctionStatement>>;
				using StructContainer = std::vector<ShaderAst::StructDescription>;

				PreVisitor(const SpirvWriter::States& conditions, SpirvConstantCache& constantCache, std::vector<SpirvAstVisitor::FuncData>& funcs) :
				m_conditions(conditions),
				m_constantCache(constantCache),
				m_externalBlockIndex(0),
				m_funcs(funcs)
				{
					m_constantCache.SetStructCallback([this](std::size_t structIndex) -> const ShaderAst::StructDescription&
					{
						assert(structIndex < declaredStructs.size());
						return declaredStructs[structIndex];
					});
				}

				void Visit(ShaderAst::AccessMemberIndexExpression& node) override
				{
					AstRecursiveVisitor::Visit(node);

					for (std::size_t index : node.memberIndices)
						m_constantCache.Register(*m_constantCache.BuildConstant(Int32(index)));

					m_constantCache.Register(*m_constantCache.BuildType(node.cachedExpressionType.value()));
				}

				void Visit(ShaderAst::ConditionalExpression& node) override
				{
					/*std::size_t conditionIndex = m_shader.FindConditionByName(node.conditionName);
					assert(conditionIndex != ShaderAst::InvalidCondition);

					if (TestBit<Nz::UInt64>(m_conditions.enabledConditions, conditionIndex))
						Visit(node.truePath);
					else
						Visit(node.falsePath);*/

					m_constantCache.Register(*m_constantCache.BuildType(node.cachedExpressionType.value()));
				}

				void Visit(ShaderAst::ConditionalStatement& node) override
				{
					/*std::size_t conditionIndex = m_shader.FindConditionByName(node.conditionName);
					assert(conditionIndex != ShaderAst::InvalidCondition);

					if (TestBit<Nz::UInt64>(m_conditions.enabledConditions, conditionIndex))
						Visit(node.statement);*/
				}

				void Visit(ShaderAst::ConstantExpression& node) override
				{
					std::visit([&](auto&& arg)
					{
						m_constantCache.Register(*m_constantCache.BuildConstant(arg));
					}, node.value);

					AstScopedVisitor::Visit(node);
				}

				void Visit(ShaderAst::DeclareExternalStatement& node) override
				{
					assert(node.varIndex);
					std::size_t varIndex = *node.varIndex;
					for (auto& extVar : node.externalVars)
					{
						SpirvConstantCache::Variable variable;
						variable.debugName = extVar.name;
						variable.storageClass = (ShaderAst::IsSamplerType(extVar.type)) ? SpirvStorageClass::UniformConstant : SpirvStorageClass::Uniform;
						variable.type = m_constantCache.BuildPointerType(extVar.type, variable.storageClass);

						UniformVar& uniformVar = extVars[varIndex++];
						uniformVar.pointerId = m_constantCache.Register(variable);

						for (const auto& [attributeType, attributeParam] : extVar.attributes)
						{
							if (attributeType == ShaderAst::AttributeType::Binding)
							{
								uniformVar.bindingIndex = std::get<long long>(attributeParam);
								break;
							}
						}
					}
				}

				void Visit(ShaderAst::DeclareFunctionStatement& node) override
				{
					std::optional<ShaderStageType> entryPointType;
					for (auto& attribute : node.attributes)
					{
						if (attribute.type == ShaderAst::AttributeType::Entry)
						{
							auto it = s_entryPoints.find(std::get<std::string>(attribute.args));
							assert(it != s_entryPoints.end());

							entryPointType = it->second;
							break;
						}
					}

					assert(node.funcIndex);
					std::size_t funcIndex = *node.funcIndex;

					if (funcIndex >= m_funcs.size())
						m_funcs.resize(funcIndex + 1);

					auto& funcData = m_funcs[funcIndex];
					funcData.name = node.name;

					if (!entryPointType)
					{
						std::vector<ShaderAst::ExpressionType> parameterTypes;
						for (auto& parameter : node.parameters)
							parameterTypes.push_back(parameter.type);

						funcData.returnTypeId = m_constantCache.Register(*m_constantCache.BuildType(node.returnType));
						funcData.funcTypeId = m_constantCache.Register(*m_constantCache.BuildFunctionType(node.returnType, parameterTypes));

						for (auto& parameter : node.parameters)
						{
							auto& funcParam = funcData.parameters.emplace_back();
							funcParam.pointerTypeId = m_constantCache.Register(*m_constantCache.BuildPointerType(parameter.type, SpirvStorageClass::Function));
							funcParam.typeId = m_constantCache.Register(*m_constantCache.BuildType(parameter.type));
						}
					}
					else
					{
						using EntryPoint = SpirvAstVisitor::EntryPoint;

						funcData.returnTypeId = m_constantCache.Register(*m_constantCache.BuildType(ShaderAst::NoType{}));
						funcData.funcTypeId = m_constantCache.Register(*m_constantCache.BuildFunctionType(ShaderAst::NoType{}, {}));

						std::optional<EntryPoint::InputStruct> inputStruct;
						std::vector<EntryPoint::Input> inputs;
						if (!node.parameters.empty())
						{
							assert(node.parameters.size() == 1);
							auto& parameter = node.parameters.front();
							assert(std::holds_alternative<ShaderAst::StructType>(parameter.type));

							std::size_t structIndex = std::get<ShaderAst::StructType>(parameter.type).structIndex;
							const ShaderAst::StructDescription& structDesc = declaredStructs[structIndex];

							std::size_t memberIndex = 0;
							for (const auto& member : structDesc.members)
							{
								if (UInt32 varId = HandleEntryInOutType(*entryPointType, funcIndex, member, SpirvStorageClass::Input); varId != 0)
								{
									inputs.push_back({
										m_constantCache.Register(*m_constantCache.BuildConstant(Int32(memberIndex))),
										m_constantCache.Register(*m_constantCache.BuildPointerType(member.type, SpirvStorageClass::Function)),
										varId
									});
								}

								memberIndex++;
							}

							inputStruct = EntryPoint::InputStruct{
								m_constantCache.Register(*m_constantCache.BuildPointerType(parameter.type, SpirvStorageClass::Function)),
								m_constantCache.Register(*m_constantCache.BuildType(parameter.type))
							};
						}

						std::optional<UInt32> outputStructId;
						std::vector<EntryPoint::Output> outputs;
						if (!IsNoType(node.returnType))
						{
							assert(std::holds_alternative<ShaderAst::StructType>(node.returnType));

							std::size_t structIndex = std::get<ShaderAst::StructType>(node.returnType).structIndex;
							const ShaderAst::StructDescription& structDesc = declaredStructs[structIndex];

							std::size_t memberIndex = 0;
							for (const auto& member : structDesc.members)
							{
								if (UInt32 varId = HandleEntryInOutType(*entryPointType, funcIndex, member, SpirvStorageClass::Output); varId != 0)
								{
									outputs.push_back({
										Int32(memberIndex),
										m_constantCache.Register(*m_constantCache.BuildType(member.type)),
										varId
									});
								}

								memberIndex++;
							}

							outputStructId = m_constantCache.Register(*m_constantCache.BuildType(node.returnType));
						}

						funcData.entryPointData = EntryPoint{
							*entryPointType,
							inputStruct,
							outputStructId,
							funcIndex,
							std::move(inputs),
							std::move(outputs)
						};
					}

					m_funcIndex = funcIndex;
					AstScopedVisitor::Visit(node);
					m_funcIndex.reset();
				}

				void Visit(ShaderAst::DeclareStructStatement& node) override
				{
					AstScopedVisitor::Visit(node);

					assert(node.structIndex);
					std::size_t structIndex = *node.structIndex;
					if (structIndex >= declaredStructs.size())
						declaredStructs.resize(structIndex + 1);

					declaredStructs[structIndex] = node.description;

					m_constantCache.Register(*m_constantCache.BuildType(node.description));
				}

				void Visit(ShaderAst::DeclareVariableStatement& node) override
				{
					AstScopedVisitor::Visit(node);

					assert(m_funcIndex);
					auto& func = m_funcs[*m_funcIndex];

					assert(node.varIndex);
					func.varIndexToVarId[*node.varIndex] = func.variables.size();

					auto& var = func.variables.emplace_back();
					var.typeId = m_constantCache.Register(*m_constantCache.BuildPointerType(node.varType, SpirvStorageClass::Function));
				}

				void Visit(ShaderAst::IdentifierExpression& node) override
				{
					m_constantCache.Register(*m_constantCache.BuildType(node.cachedExpressionType.value()));

					AstScopedVisitor::Visit(node);
				}

				void Visit(ShaderAst::IntrinsicExpression& node) override
				{
					AstScopedVisitor::Visit(node);

					switch (node.intrinsic)
					{
						// Require GLSL.std.450
						case ShaderAst::IntrinsicType::CrossProduct:
							extInsts.emplace("GLSL.std.450");
							break;

						// Part of SPIR-V core
						case ShaderAst::IntrinsicType::DotProduct:
							break;
					}

					m_constantCache.Register(*m_constantCache.BuildType(node.cachedExpressionType.value()));
				}

				void Visit(ShaderAst::SwizzleExpression& node) override
				{
					AstScopedVisitor::Visit(node);

					m_constantCache.Register(*m_constantCache.BuildType(node.cachedExpressionType.value()));
				}

				UInt32 HandleEntryInOutType(ShaderStageType entryPointType, std::size_t funcIndex, const ShaderAst::StructDescription::StructMember& member, SpirvStorageClass storageClass)
				{
					std::optional<std::reference_wrapper<Builtin>> builtinOpt;
					std::optional<long long> attributeLocation;
					for (const auto& [attributeType, attributeParam] : member.attributes)
					{
						if (attributeType == ShaderAst::AttributeType::Builtin)
						{
							auto it = s_builtinMapping.find(std::get<std::string>(attributeParam));
							if (it != s_builtinMapping.end())
							{
								builtinOpt = it->second;
								break;
							}
						}
						else if (attributeType == ShaderAst::AttributeType::Location)
						{
							attributeLocation = std::get<long long>(attributeParam);
							break;
						}
					}

					if (builtinOpt)
					{
						Builtin& builtin = *builtinOpt;
						if ((builtin.compatibleStages & entryPointType) == 0)
							return 0;

						SpirvBuiltIn builtinDecoration = builtin.decoration;

						SpirvConstantCache::Variable variable;
						variable.debugName = builtin.debugName;
						variable.funcId = funcIndex;
						variable.storageClass = storageClass;
						variable.type = m_constantCache.BuildPointerType(member.type, storageClass);

						UInt32 varId = m_constantCache.Register(variable);
						builtinDecorations[varId] = builtinDecoration;

						return varId;
					}
					else if (attributeLocation)
					{
						SpirvConstantCache::Variable variable;
						variable.debugName = member.name;
						variable.funcId = funcIndex;
						variable.storageClass = storageClass;
						variable.type = m_constantCache.BuildPointerType(member.type, storageClass);

						UInt32 varId = m_constantCache.Register(variable);
						locationDecorations[varId] = *attributeLocation;

						return varId;
					}

					return 0;
				}

				BuiltinDecoration builtinDecorations;
				ExtInstList extInsts;
				ExtVarContainer extVars;
				LocationDecoration locationDecorations;
				StructContainer declaredStructs;

			private:
				const SpirvWriter::States& m_conditions;
				SpirvConstantCache& m_constantCache;
				std::optional<std::size_t> m_funcIndex;
				std::size_t m_externalBlockIndex;
				std::vector<SpirvAstVisitor::FuncData>& m_funcs;
		};
	}

	struct SpirvWriter::State
	{
		State() :
		constantTypeCache(nextVarIndex)
		{
		}

		struct Func
		{
			const ShaderAst::DeclareFunctionStatement* statement = nullptr;
			UInt32 typeId;
			UInt32 id;
		};

		std::unordered_map<std::string, UInt32> extensionInstructions;
		std::unordered_map<std::string, UInt32> varToResult;
		std::vector<SpirvAstVisitor::FuncData> funcs;
		std::vector<UInt32> resultIds;
		UInt32 nextVarIndex = 1;
		SpirvConstantCache constantTypeCache; //< init after nextVarIndex
		PreVisitor* preVisitor;

		// Output
		SpirvSection header;
		SpirvSection constants;
		SpirvSection debugInfo;
		SpirvSection annotations;
		SpirvSection instructions;
	};

	SpirvWriter::SpirvWriter() :
	m_currentState(nullptr)
	{
	}

	std::vector<UInt32> SpirvWriter::Generate(ShaderAst::StatementPtr& shader, const States& conditions)
	{
		std::string error;
		if (!ShaderAst::ValidateAst(shader, &error))
			throw std::runtime_error("Invalid shader AST: " + error);

		ShaderAst::TransformVisitor transformVisitor;
		ShaderAst::StatementPtr transformedShader = transformVisitor.Transform(shader);

		m_context.states = &conditions;

		State state;
		m_currentState = &state;
		CallOnExit onExit([this]()
		{
			m_currentState = nullptr;
		});

		// Register all extended instruction sets
		PreVisitor preVisitor(conditions, state.constantTypeCache, state.funcs);
		transformedShader->Visit(preVisitor);

		m_currentState->preVisitor = &preVisitor;

		for (const std::string& extInst : preVisitor.extInsts)
			state.extensionInstructions[extInst] = AllocateResultId();

		SpirvAstVisitor visitor(*this, state.instructions, state.funcs);
		transformedShader->Visit(visitor);

		AppendHeader();

		for (auto&& [varIndex, extVar] : preVisitor.extVars)
		{
			if (extVar.bindingIndex)
			{
				state.annotations.Append(SpirvOp::OpDecorate, extVar.pointerId, SpirvDecoration::Binding, *extVar.bindingIndex);
				state.annotations.Append(SpirvOp::OpDecorate, extVar.pointerId, SpirvDecoration::DescriptorSet, 0);
			}
		}

		for (auto&& [varId, builtin] : preVisitor.builtinDecorations)
			state.annotations.Append(SpirvOp::OpDecorate, varId, SpirvDecoration::BuiltIn, builtin);

		for (auto&& [varId, location] : preVisitor.locationDecorations)
			state.annotations.Append(SpirvOp::OpDecorate, varId, SpirvDecoration::Location, location);

		m_currentState->constantTypeCache.Write(m_currentState->annotations, m_currentState->constants, m_currentState->debugInfo);

		std::vector<UInt32> ret;
		MergeSections(ret, state.header);
		MergeSections(ret, state.debugInfo);
		MergeSections(ret, state.annotations);
		MergeSections(ret, state.constants);
		MergeSections(ret, state.instructions);

		return ret;
	}

	void SpirvWriter::SetEnv(Environment environment)
	{
		m_environment = std::move(environment);
	}

	UInt32 SpirvWriter::AllocateResultId()
	{
		return m_currentState->nextVarIndex++;
	}

	void SpirvWriter::AppendHeader()
	{
		m_currentState->header.AppendRaw(SpirvMagicNumber); //< Spir-V magic number

		UInt32 version = (m_environment.spvMajorVersion << 16) | m_environment.spvMinorVersion << 8;
		m_currentState->header.AppendRaw(version); //< Spir-V version number (1.0 for compatibility)
		m_currentState->header.AppendRaw(0); //< Generator identifier (TODO: Register generator to Khronos)

		m_currentState->header.AppendRaw(m_currentState->nextVarIndex); //< Bound (ID count)
		m_currentState->header.AppendRaw(0); //< Instruction schema (required to be 0 for now)

		m_currentState->header.Append(SpirvOp::OpCapability, SpirvCapability::Shader);

		for (const auto& [extInst, resultId] : m_currentState->extensionInstructions)
			m_currentState->header.Append(SpirvOp::OpExtInstImport, resultId, extInst);

		m_currentState->header.Append(SpirvOp::OpMemoryModel, SpirvAddressingModel::Logical, SpirvMemoryModel::GLSL450);

		std::optional<UInt32> fragmentFuncId;
		for (auto& func : m_currentState->funcs)
		{
			m_currentState->debugInfo.Append(SpirvOp::OpName, func.funcId, func.name);

			if (func.entryPointData)
			{
				auto& entryPointData = func.entryPointData.value();

				SpirvExecutionModel execModel;

				switch (entryPointData.stageType)
				{
					case ShaderStageType::Fragment:
						execModel = SpirvExecutionModel::Fragment;
						break;

					case ShaderStageType::Vertex:
						execModel = SpirvExecutionModel::Vertex;
						break;

					default:
						throw std::runtime_error("not yet implemented");
				}

				m_currentState->header.AppendVariadic(SpirvOp::OpEntryPoint, [&](const auto& appender)
				{
					appender(execModel);
					appender(func.funcId);
					appender(func.name);

					for (const auto& input : entryPointData.inputs)
						appender(input.varId);

					for (const auto& output : entryPointData.outputs)
						appender(output.varId);
				});

				if (entryPointData.stageType == ShaderStageType::Fragment)
					fragmentFuncId = func.funcId;
			}
		}

		if (fragmentFuncId)
			m_currentState->header.Append(SpirvOp::OpExecutionMode, *fragmentFuncId, SpirvExecutionMode::OriginUpperLeft);

	}

	SpirvConstantCache::TypePtr SpirvWriter::BuildFunctionType(const ShaderAst::DeclareFunctionStatement& functionNode)
	{
		std::vector<ShaderAst::ExpressionType> parameterTypes;
		parameterTypes.reserve(functionNode.parameters.size());

		for (const auto& parameter : functionNode.parameters)
			parameterTypes.push_back(parameter.type);

		return m_currentState->constantTypeCache.BuildFunctionType(functionNode.returnType, parameterTypes);
	}

	UInt32 SpirvWriter::GetConstantId(const ShaderAst::ConstantValue& value) const
	{
		return m_currentState->constantTypeCache.GetId(*m_currentState->constantTypeCache.BuildConstant(value));
	}

	UInt32 SpirvWriter::GetExtVarPointerId(std::size_t extVarIndex) const
	{
		auto it = m_currentState->preVisitor->extVars.find(extVarIndex);
		assert(it != m_currentState->preVisitor->extVars.end());

		return it->second.pointerId;
	}

	UInt32 SpirvWriter::GetFunctionTypeId(const ShaderAst::DeclareFunctionStatement& functionNode)
	{
		return m_currentState->constantTypeCache.GetId({ *BuildFunctionType(functionNode) });
	}

	UInt32 SpirvWriter::GetPointerTypeId(const ShaderAst::ExpressionType& type, SpirvStorageClass storageClass) const
	{
		return m_currentState->constantTypeCache.GetId(*m_currentState->constantTypeCache.BuildPointerType(type, storageClass));
	}

	UInt32 SpirvWriter::GetTypeId(const ShaderAst::ExpressionType& type) const
	{
		return m_currentState->constantTypeCache.GetId(*m_currentState->constantTypeCache.BuildType(type));
	}

	UInt32 SpirvWriter::RegisterConstant(const ShaderAst::ConstantValue& value)
	{
		return m_currentState->constantTypeCache.Register(*m_currentState->constantTypeCache.BuildConstant(value));
	}

	UInt32 SpirvWriter::RegisterFunctionType(const ShaderAst::DeclareFunctionStatement& functionNode)
	{
		return m_currentState->constantTypeCache.Register({ *BuildFunctionType(functionNode) });
	}

	UInt32 SpirvWriter::RegisterPointerType(ShaderAst::ExpressionType type, SpirvStorageClass storageClass)
	{
		return m_currentState->constantTypeCache.Register(*m_currentState->constantTypeCache.BuildPointerType(type, storageClass));
	}

	UInt32 SpirvWriter::RegisterType(ShaderAst::ExpressionType type)
	{
		assert(m_currentState);
		return m_currentState->constantTypeCache.Register(*m_currentState->constantTypeCache.BuildType(type));
	}

	void SpirvWriter::MergeSections(std::vector<UInt32>& output, const SpirvSection& from)
	{
		const std::vector<UInt32>& bytecode = from.GetBytecode();

		std::size_t prevSize = output.size();
		output.resize(prevSize + bytecode.size());
		std::copy(bytecode.begin(), bytecode.end(), output.begin() + prevSize);
	}
}
