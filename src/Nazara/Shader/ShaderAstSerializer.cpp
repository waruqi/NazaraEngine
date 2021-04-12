// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Shader generator"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Shader/ShaderAstSerializer.hpp>
#include <Nazara/Shader/ShaderAstExpressionVisitor.hpp>
#include <Nazara/Shader/ShaderAstStatementVisitor.hpp>
#include <Nazara/Shader/Debug.hpp>

namespace Nz::ShaderAst
{
	namespace
	{
		constexpr UInt32 s_magicNumber = 0x4E534852;
		constexpr UInt32 s_currentVersion = 1;

		class ShaderSerializerVisitor : public AstExpressionVisitor, public AstStatementVisitor
		{
			public:
				ShaderSerializerVisitor(AstSerializerBase& serializer) :
				m_serializer(serializer)
				{
				}

#define NAZARA_SHADERAST_NODE(Node) void Visit(Node& node) override \
				{ \
					m_serializer.Serialize(node); \
				}
#include <Nazara/Shader/ShaderAstNodes.hpp>

			private:
				AstSerializerBase& m_serializer;
		};
	}

	void AstSerializerBase::Serialize(AccessMemberIdentifierExpression& node)
	{
		Node(node.structExpr);

		Container(node.memberIdentifiers);
		for (std::string& identifier : node.memberIdentifiers)
			Value(identifier);
	}

	void AstSerializerBase::Serialize(AccessMemberIndexExpression& node)
	{
		Node(node.structExpr);

		Container(node.memberIndices);
		for (std::size_t& identifier : node.memberIndices)
			SizeT(identifier);
	}

	void AstSerializerBase::Serialize(AssignExpression& node)
	{
		Enum(node.op);
		Node(node.left);
		Node(node.right);
	}

	void AstSerializerBase::Serialize(BinaryExpression& node)
	{
		Enum(node.op);
		Node(node.left);
		Node(node.right);
	}

	void AstSerializerBase::Serialize(CastExpression& node)
	{
		Type(node.targetType);
		for (auto& expr : node.expressions)
			Node(expr);
	}

	void AstSerializerBase::Serialize(ConditionalExpression& node)
	{
		Value(node.conditionName);
		Node(node.truePath);
		Node(node.falsePath);
	}
	
	void AstSerializerBase::Serialize(ConstantExpression& node)
	{
		UInt32 typeIndex;
		if (IsWriting())
			typeIndex = UInt32(node.value.index());

		Value(typeIndex);

		// Waiting for template lambda in C++20
		auto SerializeValue = [&](auto dummyType)
		{
			using T = std::decay_t<decltype(dummyType)>;

			auto& value = (IsWriting()) ? std::get<T>(node.value) : node.value.emplace<T>();
			Value(value);
		};

		static_assert(std::variant_size_v<decltype(node.value)> == 10);
		switch (typeIndex)
		{
			case 0: SerializeValue(bool()); break;
			case 1: SerializeValue(float()); break;
			case 2: SerializeValue(Int32()); break;
			case 3: SerializeValue(UInt32()); break;
			case 4: SerializeValue(Vector2f()); break;
			case 5: SerializeValue(Vector3f()); break;
			case 6: SerializeValue(Vector4f()); break;
			case 7: SerializeValue(Vector2i32()); break;
			case 8: SerializeValue(Vector3i32()); break;
			case 9: SerializeValue(Vector4i32()); break;
			default: throw std::runtime_error("unexpected data type");
		}
	}

	void AstSerializerBase::Serialize(DeclareVariableStatement& node)
	{
		Value(node.varName);
		Type(node.varType);
		Node(node.initialExpression);
	}

	void AstSerializerBase::Serialize(IdentifierExpression& node)
	{
		Value(node.identifier);
	}

	void AstSerializerBase::Serialize(IntrinsicExpression& node)
	{
		Enum(node.intrinsic);
		Container(node.parameters);
		for (auto& param : node.parameters)
			Node(param);
	}

	void AstSerializerBase::Serialize(SwizzleExpression& node)
	{
		SizeT(node.componentCount);
		Node(node.expression);

		for (std::size_t i = 0; i < node.componentCount; ++i)
			Enum(node.components[i]);
	}

	void AstSerializerBase::Serialize(VariableExpression& node)
	{
		SizeT(node.variableId);
	}


	void AstSerializerBase::Serialize(BranchStatement& node)
	{
		Container(node.condStatements);
		for (auto& condStatement : node.condStatements)
		{
			Node(condStatement.condition);
			Node(condStatement.statement);
		}

		Node(node.elseStatement);
	}

	void AstSerializerBase::Serialize(ConditionalStatement& node)
	{
		Value(node.conditionName);
		Node(node.statement);
	}

	void AstSerializerBase::Serialize(DeclareExternalStatement& node)
	{
		Attributes(node.attributes);

		Container(node.externalVars);
		for (auto& extVar : node.externalVars)
		{
			Attributes(extVar.attributes);
			Value(extVar.name);
			Type(extVar.type);
		}
	}

	void AstSerializerBase::Serialize(DeclareFunctionStatement& node)
	{
		Value(node.name);
		Type(node.returnType);

		Attributes(node.attributes);

		Container(node.parameters);
		for (auto& parameter : node.parameters)
		{
			Value(parameter.name);
			Type(parameter.type);
		}

		Container(node.statements);
		for (auto& statement : node.statements)
			Node(statement);
	}

	void AstSerializerBase::Serialize(DeclareStructStatement& node)
	{
		Value(node.description.name);

		Container(node.description.members);
		for (auto& member : node.description.members)
		{
			Value(member.name);
			Type(member.type);
		}
	}

	void AstSerializerBase::Serialize(DiscardStatement& /*node*/)
	{
		/* Nothing to do */
	}

	void AstSerializerBase::Serialize(ExpressionStatement& node)
	{
		Node(node.expression);
	}

	void AstSerializerBase::Serialize(MultiStatement& node)
	{
		Container(node.statements);
		for (auto& statement : node.statements)
			Node(statement);
	}

	void AstSerializerBase::Serialize(NoOpStatement& /*node*/)
	{
		/* Nothing to do */
	}

	void AstSerializerBase::Serialize(ReturnStatement& node)
	{
		Node(node.returnExpr);
	}

	void ShaderAstSerializer::Serialize(StatementPtr& shader)
	{
		m_stream << s_magicNumber << s_currentVersion;

		Node(shader);

		m_stream.FlushBits();
	}
	
	void AstSerializerBase::Attributes(std::vector<Attribute>& attributes)
	{
		Container(attributes);
		for (auto& attribute : attributes)
		{
			Enum(attribute.type);

			if (IsWriting())
			{
				std::visit([&](auto&& arg)
				{
					using T = std::decay_t<decltype(arg)>;

					if constexpr (std::is_same_v<T, std::monostate>)
					{
						UInt8 typeId = 0;
						Value(typeId);
					}
					else if constexpr (std::is_same_v<T, long long>)
					{
						UInt8 typeId = 1;
						UInt64 v = UInt64(arg);
						Value(typeId);
						Value(v);
					}
					else if constexpr (std::is_same_v<T, std::string>)
					{
						UInt8 typeId = 2;
						Value(typeId);
						Value(arg);
					}
					else
						static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");

				}, attribute.args);
			}
			else
			{
				UInt8 typeId;
				Value(typeId);

				switch (typeId)
				{
					case 0:
						attribute.args.emplace<std::monostate>();
						break;

					case 1:
					{
						UInt64 arg;
						Value(arg);

						attribute.args = static_cast<long long>(arg);
						break;
					}

					case 2:
					{
						std::string arg;
						Value(arg);

						attribute.args = std::move(arg);
						break;
					}

					default:
						throw std::runtime_error("invalid attribute type id");
				}
			}
		}
	}

	bool ShaderAstSerializer::IsWriting() const
	{
		return true;
	}

	void ShaderAstSerializer::Node(ExpressionPtr& node)
	{
		NodeType nodeType = (node) ? node->GetType() : NodeType::None;
		m_stream << static_cast<Int32>(nodeType);

		if (node)
		{
			ShaderSerializerVisitor visitor(*this);
			node->Visit(visitor);
		}
	}

	void ShaderAstSerializer::Node(StatementPtr& node)
	{
		NodeType nodeType = (node) ? node->GetType() : NodeType::None;
		m_stream << static_cast<Int32>(nodeType);

		if (node)
		{
			ShaderSerializerVisitor visitor(*this);
			node->Visit(visitor);
		}
	}

	void ShaderAstSerializer::Type(ExpressionType& type)
	{
		std::visit([&](auto&& arg)
		{
			using T = std::decay_t<decltype(arg)>;

			if constexpr (std::is_same_v<T, NoType>)
				m_stream << UInt8(0);
			else if constexpr (std::is_same_v<T, PrimitiveType>)
			{
				m_stream << UInt8(1);
				m_stream << UInt32(arg);
			}
			else if constexpr (std::is_same_v<T, IdentifierType>)
			{
				m_stream << UInt8(2);
				m_stream << arg.name;
			}
			else if constexpr (std::is_same_v<T, MatrixType>)
			{
				m_stream << UInt8(3);
				m_stream << UInt32(arg.columnCount);
				m_stream << UInt32(arg.rowCount);
				m_stream << UInt32(arg.type);
			}
			else if constexpr (std::is_same_v<T, SamplerType>)
			{
				m_stream << UInt8(4);
				m_stream << UInt32(arg.dim);
				m_stream << UInt32(arg.sampledType);
			}
			else if constexpr (std::is_same_v<T, StructType>)
			{
				m_stream << UInt8(5);
				m_stream << UInt32(arg.structIndex);
			}
			else if constexpr (std::is_same_v<T, UniformType>)
			{
				m_stream << UInt8(6);
				m_stream << std::get<IdentifierType>(arg.containedType).name;
			}
			else if constexpr (std::is_same_v<T, VectorType>)
			{
				m_stream << UInt8(7);
				m_stream << UInt32(arg.componentCount);
				m_stream << UInt32(arg.type);
			}
			else
				static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
		}, type);
	}

	void ShaderAstSerializer::Value(bool& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(float& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(std::string& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Int32& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Vector2f& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Vector3f& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Vector4f& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Vector2i32& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Vector3i32& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(Vector4i32& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(UInt8& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(UInt16& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(UInt32& val)
	{
		m_stream << val;
	}

	void ShaderAstSerializer::Value(UInt64& val)
	{
		m_stream << val;
	}

	StatementPtr ShaderAstUnserializer::Unserialize()
	{
		UInt32 magicNumber;
		UInt32 version;
		m_stream >> magicNumber;
		if (magicNumber != s_magicNumber)
			throw std::runtime_error("invalid shader file");

		m_stream >> version;
		if (version > s_currentVersion)
			throw std::runtime_error("unsupported version");

		StatementPtr node;

		Node(node);
		if (!node)
			throw std::runtime_error("functions can only have statements");

		return node;
	}

	bool ShaderAstUnserializer::IsWriting() const
	{
		return false;
	}

	void ShaderAstUnserializer::Node(ExpressionPtr& node)
	{
		Int32 nodeTypeInt;
		m_stream >> nodeTypeInt;

		if (nodeTypeInt < static_cast<Int32>(NodeType::None) || nodeTypeInt > static_cast<Int32>(NodeType::Max))
			throw std::runtime_error("invalid node type");

		NodeType nodeType = static_cast<NodeType>(nodeTypeInt);
		switch (nodeType)
		{
			case NodeType::None: break;

#define NAZARA_SHADERAST_EXPRESSION(Node) case NodeType:: Node : node = std::make_unique<Node>(); break;
#include <Nazara/Shader/ShaderAstNodes.hpp>

			default: throw std::runtime_error("unexpected node type");
		}

		if (node)
		{
			ShaderSerializerVisitor visitor(*this);
			node->Visit(visitor);
		}
	}

	void ShaderAstUnserializer::Node(StatementPtr& node)
	{
		Int32 nodeTypeInt;
		m_stream >> nodeTypeInt;

		if (nodeTypeInt < static_cast<Int32>(NodeType::None) || nodeTypeInt > static_cast<Int32>(NodeType::Max))
			throw std::runtime_error("invalid node type");

		NodeType nodeType = static_cast<NodeType>(nodeTypeInt);
		switch (nodeType)
		{
			case NodeType::None: break;

#define NAZARA_SHADERAST_STATEMENT(Node) case NodeType:: Node : node = std::make_unique<Node>(); break;
#include <Nazara/Shader/ShaderAstNodes.hpp>

			default: throw std::runtime_error("unexpected node type");
		}

		if (node)
		{
			ShaderSerializerVisitor visitor(*this);
			node->Visit(visitor);
		}
	}

	void ShaderAstUnserializer::Type(ExpressionType& type)
	{
		UInt8 typeIndex;
		Value(typeIndex);

		switch (typeIndex)
		{
			/*
			if constexpr (std::is_same_v<T, NoType>)
				m_stream << UInt8(0);
			else if constexpr (std::is_same_v<T, PrimitiveType>)
			{
				m_stream << UInt8(1);
				m_stream << UInt32(arg);
			}
			else if constexpr (std::is_same_v<T, IdentifierType>)
			{
				m_stream << UInt8(2);
				m_stream << arg.name;
			}
			else if constexpr (std::is_same_v<T, MatrixType>)
			{
				m_stream << UInt8(3);
				m_stream << UInt32(arg.columnCount);
				m_stream << UInt32(arg.rowCount);
				m_stream << UInt32(arg.type);
			}
			else if constexpr (std::is_same_v<T, SamplerType>)
			{
				m_stream << UInt8(4);
				m_stream << UInt32(arg.dim);
				m_stream << UInt32(arg.sampledType);
			}
			else if constexpr (std::is_same_v<T, VectorType>)
			{
				m_stream << UInt8(5);
				m_stream << UInt32(arg.componentCount);
				m_stream << UInt32(arg.type);
			}
			*/

			case 0: //< NoType
				type = NoType{};
				break;

			case 1: //< PrimitiveType
			{
				PrimitiveType primitiveType;
				Enum(primitiveType);

				type = primitiveType;
				break;
			}

			case 2: //< Identifier
			{
				std::string identifier;
				Value(identifier);

				type = IdentifierType{ std::move(identifier) };
				break;
			}

			case 3: //< MatrixType
			{
				UInt32 columnCount, rowCount;
				PrimitiveType primitiveType;
				Value(columnCount);
				Value(rowCount);
				Enum(primitiveType);

				type = MatrixType {
					columnCount,
					rowCount,
					primitiveType
				};
				break;
			}

			case 4: //< SamplerType
			{
				ImageType dim;
				PrimitiveType sampledType;
				Enum(dim);
				Enum(sampledType);

				type = SamplerType {
					dim,
					sampledType
				};
				break;
			}

			case 5: //< StructType
			{
				UInt32 structIndex;
				Value(structIndex);

				type = StructType{
					structIndex
				};
				break;
			}

			case 6: //< UniformType
			{
				std::string containedType;
				Value(containedType);

				type = UniformType {
					IdentifierType {
						containedType
					}
				};
				break;
			}

			case 7: //< VectorType
			{
				UInt32 componentCount;
				PrimitiveType componentType;
				Value(componentCount);
				Enum(componentType);

				type = VectorType{
					componentCount,
					componentType
				};
				break;
			}

			default:
				break;
		}
	}

	void ShaderAstUnserializer::Value(bool& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(float& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(std::string& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Int32& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Vector2f& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Vector3f& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Vector4f& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Vector2i32& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Vector3i32& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(Vector4i32& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(UInt8& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(UInt16& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(UInt32& val)
	{
		m_stream >> val;
	}

	void ShaderAstUnserializer::Value(UInt64& val)
	{
		m_stream >> val;
	}
	

	ByteArray SerializeShader(StatementPtr& shader)
	{
		ByteArray byteArray;
		ByteStream stream(&byteArray, OpenModeFlags(OpenMode_WriteOnly));

		ShaderAstSerializer serializer(stream);
		serializer.Serialize(shader);

		return byteArray;
	}

	StatementPtr UnserializeShader(ByteStream& stream)
	{
		ShaderAstUnserializer unserializer(stream);
		return unserializer.Unserialize();
	}
}

