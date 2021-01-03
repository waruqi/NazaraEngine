#include <ShaderNode/ShaderGraph.hpp>
#include <Nazara/Core/StackArray.hpp>
#include <ShaderNode/DataModels/BinOp.hpp>
#include <ShaderNode/DataModels/BoolValue.hpp>
#include <ShaderNode/DataModels/BufferField.hpp>
#include <ShaderNode/DataModels/Cast.hpp>
#include <ShaderNode/DataModels/CompOp.hpp>
#include <ShaderNode/DataModels/ConditionalExpression.hpp>
#include <ShaderNode/DataModels/Discard.hpp>
#include <ShaderNode/DataModels/FloatValue.hpp>
#include <ShaderNode/DataModels/InputValue.hpp>
#include <ShaderNode/DataModels/Mat4BinOp.hpp>
#include <ShaderNode/DataModels/Mat4VecMul.hpp>
#include <ShaderNode/DataModels/OutputValue.hpp>
#include <ShaderNode/DataModels/PositionOutputValue.hpp>
#include <ShaderNode/DataModels/SampleTexture.hpp>
#include <ShaderNode/DataModels/ShaderNode.hpp>
#include <ShaderNode/DataModels/TextureValue.hpp>
#include <ShaderNode/DataModels/VecComposition.hpp>
#include <ShaderNode/DataModels/VecDecomposition.hpp>
#include <ShaderNode/DataModels/VecDot.hpp>
#include <ShaderNode/DataModels/VecFloatMul.hpp>
#include <ShaderNode/DataModels/VecValue.hpp>
#include <ShaderNode/Previews/QuadPreview.hpp>
#include <ShaderNode/DataTypes/BoolData.hpp>
#include <ShaderNode/DataTypes/FloatData.hpp>
#include <ShaderNode/DataTypes/Matrix4Data.hpp>
#include <ShaderNode/DataTypes/TextureData.hpp>
#include <ShaderNode/DataTypes/VecData.hpp>
#include <QtCore/QDebug>
#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/NodeGeometry>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/DataModelRegistry>
#include <unordered_set>

namespace
{
	template<typename T>
	void RegisterShaderNode(ShaderGraph& graph, std::shared_ptr<QtNodes::DataModelRegistry> registry, QString category = QString())
	{
		auto creator = [&] { return std::make_unique<T>(graph); };
		registry->registerModel<T>(category, std::move(creator));
	}
}

ShaderGraph::ShaderGraph() :
m_flowScene(BuildRegistry()),
m_type(ShaderType::NotSet)
{
	m_previewModel = std::make_unique<QuadPreview>();

	QObject::connect(&m_flowScene, &QGraphicsScene::selectionChanged, [&]
	{
		auto selectedNodes = m_flowScene.selectedNodes();
		if (selectedNodes.size() == 1)
			OnSelectedNodeUpdate(this, static_cast<ShaderNode*>(selectedNodes.front()->nodeDataModel()));
		else
			OnSelectedNodeUpdate(this, nullptr);
	});

	// Test
	AddInput("UV", PrimitiveType::Float2, InputRole::TexCoord, 0, 0);
	AddOutput("RenderTarget0", PrimitiveType::Float4, 0);
	AddTexture("Potato", TextureType::Sampler2D, 1);

	UpdateTexturePreview(0, QImage(R"(C:\Users\Lynix\Pictures\potatavril.png)"));

	auto& node1 = m_flowScene.createNode(std::make_unique<TextureValue>(*this));
	node1.nodeGraphicsObject().setPos(0, 200);

	auto& node2 = m_flowScene.createNode(std::make_unique<InputValue>(*this));
	node2.nodeGraphicsObject().setPos(50, 350);

	auto& node3 = m_flowScene.createNode(std::make_unique<SampleTexture>(*this));
	node3.nodeGraphicsObject().setPos(200, 200);

	auto& node4 = m_flowScene.createNode(std::make_unique<VecMul>(*this));
	node4.nodeGraphicsObject().setPos(400, 200);

	auto& node5 = m_flowScene.createNode(std::make_unique<OutputValue>(*this));
	node5.nodeGraphicsObject().setPos(600, 300);

	m_flowScene.createConnection(node3, 0, node1, 0);
	m_flowScene.createConnection(node3, 1, node2, 0);
	m_flowScene.createConnection(node4, 0, node3, 0);
	m_flowScene.createConnection(node4, 1, node3, 0);
	m_flowScene.createConnection(node5, 0, node4, 0);
}

ShaderGraph::~ShaderGraph()
{
	m_flowScene.clearScene();
}

std::size_t ShaderGraph::AddBuffer(std::string name, BufferType bufferType, std::size_t structIndex, std::size_t bindingIndex)
{
	std::size_t index = m_buffers.size();
	auto& bufferEntry = m_buffers.emplace_back();
	bufferEntry.bindingIndex = bindingIndex;
	bufferEntry.name = std::move(name);
	bufferEntry.structIndex = structIndex;
	bufferEntry.type = bufferType;

	OnBufferListUpdate(this);

	return index;
}

std::size_t ShaderGraph::AddCondition(std::string name)
{
	std::size_t index = m_conditions.size();
	auto& conditionEntry = m_conditions.emplace_back();
	conditionEntry.name = std::move(name);

	OnConditionListUpdate(this);

	return index;
}

std::size_t ShaderGraph::AddInput(std::string name, PrimitiveType type, InputRole role, std::size_t roleIndex, std::size_t locationIndex)
{
	std::size_t index = m_inputs.size();
	auto& inputEntry = m_inputs.emplace_back();
	inputEntry.locationIndex = locationIndex;
	inputEntry.name = std::move(name);
	inputEntry.role = role;
	inputEntry.roleIndex = roleIndex;
	inputEntry.type = type;

	OnInputListUpdate(this);

	return index;
}

std::size_t ShaderGraph::AddOutput(std::string name, PrimitiveType type, std::size_t locationIndex)
{
	std::size_t index = m_outputs.size();
	auto& outputEntry = m_outputs.emplace_back();
	outputEntry.locationIndex = locationIndex;
	outputEntry.name = std::move(name);
	outputEntry.type = type;

	OnOutputListUpdate(this);

	return index;
}

std::size_t ShaderGraph::AddStruct(std::string name, std::vector<StructMemberEntry> members)
{
	std::size_t index = m_structs.size();
	auto& structEntry = m_structs.emplace_back();
	structEntry.name = std::move(name);
	structEntry.members = std::move(members);

	OnStructListUpdate(this);

	return index;
}

std::size_t ShaderGraph::AddTexture(std::string name, TextureType type, std::size_t bindingIndex)
{
	std::size_t index = m_textures.size();
	auto& textureEntry = m_textures.emplace_back();
	textureEntry.bindingIndex = bindingIndex;
	textureEntry.name = std::move(name);
	textureEntry.type = type;

	OnTextureListUpdate(this);

	return index;
}

void ShaderGraph::Clear()
{
	m_type = ShaderType::NotSet;

	m_flowScene.clearScene();
	m_flowScene.clear();

	m_buffers.clear();
	m_conditions.clear();
	m_inputs.clear();
	m_structs.clear();
	m_outputs.clear();
	m_textures.clear();

	OnBufferListUpdate(this);
	OnInputListUpdate(this);
	OnStructListUpdate(this);
	OnOutputListUpdate(this);
	OnTextureListUpdate(this);
}

void ShaderGraph::EnableCondition(std::size_t conditionIndex, bool enable)
{
	assert(conditionIndex < m_conditions.size());
	auto& conditionEntry = m_conditions[conditionIndex];
	conditionEntry.enabled = enable;

	OnConditionUpdate(this, conditionIndex);
}

void ShaderGraph::Load(const QJsonObject& data)
{
	Clear();

	if (auto typeOpt = DecodeEnum<ShaderType>(data["type"].toString().toStdString()))
		m_type = typeOpt.value();

	QJsonArray bufferArray = data["buffers"].toArray();
	for (const auto& bufferDocRef : bufferArray)
	{
		QJsonObject bufferDoc = bufferDocRef.toObject();

		BufferEntry& buffer = m_buffers.emplace_back();
		buffer.bindingIndex = static_cast<std::size_t>(bufferDoc["bindingIndex"].toInt(0));
		buffer.name = bufferDoc["name"].toString().toStdString();
		buffer.structIndex = bufferDoc["structIndex"].toInt();
		buffer.type = DecodeEnum<BufferType>(bufferDoc["type"].toString().toStdString()).value();
	}

	OnBufferListUpdate(this);

	QJsonArray conditionArray = data["conditions"].toArray();
	for (const auto& conditionDocRef : conditionArray)
	{
		QJsonObject conditionDoc = conditionDocRef.toObject();

		ConditionEntry& condition = m_conditions.emplace_back();
		condition.name = conditionDoc["name"].toString().toStdString();
	}

	OnConditionListUpdate(this);

	QJsonArray inputArray = data["inputs"].toArray();
	for (const auto& inputDocRef : inputArray)
	{
		QJsonObject inputDoc = inputDocRef.toObject();

		InputEntry& input = m_inputs.emplace_back();
		input.locationIndex = static_cast<std::size_t>(inputDoc["locationIndex"].toInt(0));
		input.name = inputDoc["name"].toString().toStdString();
		input.role = DecodeEnum<InputRole>(inputDoc["role"].toString().toStdString()).value();
		input.roleIndex = static_cast<std::size_t>(inputDoc["roleIndex"].toInt(0));
		input.type = DecodeEnum<PrimitiveType>(inputDoc["type"].toString().toStdString()).value();
	}

	OnInputListUpdate(this);

	QJsonArray outputArray = data["outputs"].toArray();
	for (const auto& outputDocRef : outputArray)
	{
		QJsonObject outputDoc = outputDocRef.toObject();

		OutputEntry& output = m_outputs.emplace_back();
		output.locationIndex = static_cast<std::size_t>(outputDoc["locationIndex"].toInt(0));
		output.name = outputDoc["name"].toString().toStdString();
		output.type = DecodeEnum<PrimitiveType>(outputDoc["type"].toString().toStdString()).value();
	}

	OnOutputListUpdate(this);

	QJsonArray structArray = data["structs"].toArray();
	for (const auto& structDocRef : structArray)
	{
		QJsonObject structDoc = structDocRef.toObject();

		StructEntry& structInfo = m_structs.emplace_back();
		structInfo.name = structDoc["name"].toString().toStdString();

		QJsonArray memberArray = structDoc["members"].toArray();
		for (const auto& memberDocRef : memberArray)
		{
			QJsonObject memberDoc = memberDocRef.toObject();

			auto& memberInfo = structInfo.members.emplace_back();
			memberInfo.name = memberDoc["name"].toString().toStdString();

			const auto& typeDocRef = memberDoc["type"];
			if (typeDocRef.isString())
				memberInfo.type = DecodeEnum<PrimitiveType>(typeDocRef.toString().toStdString()).value();
			else
				memberInfo.type = typeDocRef.toInt();
		}
	}

	OnStructListUpdate(this);

	QJsonArray textureArray = data["textures"].toArray();
	for (const auto& textureDocRef : textureArray)
	{
		QJsonObject textureDoc = textureDocRef.toObject();

		TextureEntry& texture = m_textures.emplace_back();
		texture.bindingIndex = static_cast<std::size_t>(textureDoc["bindingIndex"].toInt(0));
		texture.name = textureDoc["name"].toString().toStdString();
		texture.type = DecodeEnum<TextureType>(textureDoc["type"].toString().toStdString()).value();
	}

	OnTextureListUpdate(this);

	for (QJsonValueRef node : data["nodes"].toArray())
		m_flowScene.restoreNode(node.toObject());

	for (QJsonValueRef connection : data["connections"].toArray())
		m_flowScene.restoreConnection(connection.toObject());
}

QJsonObject ShaderGraph::Save()
{
	QJsonObject sceneJson;
	sceneJson["type"] = QString(EnumToString(m_type));

	QJsonArray bufferArray;
	{
		for (const auto& buffer : m_buffers)
		{
			QJsonObject bufferDoc;
			bufferDoc["bindingIndex"] = int(buffer.bindingIndex);
			bufferDoc["name"] = QString::fromStdString(buffer.name);
			bufferDoc["structIndex"] = int(buffer.structIndex);
			bufferDoc["type"] = QString(EnumToString(buffer.type));

			bufferArray.append(bufferDoc);
		}
	}
	sceneJson["buffers"] = bufferArray;

	QJsonArray conditionArray;
	{
		for (const auto& condition : m_conditions)
		{
			QJsonObject inputDoc;
			inputDoc["name"] = QString::fromStdString(condition.name);

			conditionArray.append(inputDoc);
		}
	}
	sceneJson["conditions"] = conditionArray;

	QJsonArray inputArray;
	{
		for (const auto& input : m_inputs)
		{
			QJsonObject inputDoc;
			inputDoc["locationIndex"] = int(input.locationIndex);
			inputDoc["name"] = QString::fromStdString(input.name);
			inputDoc["role"] = QString(EnumToString(input.role));
			inputDoc["roleIndex"] = int(input.roleIndex);
			inputDoc["type"] = QString(EnumToString(input.type));

			inputArray.append(inputDoc);
		}
	}
	sceneJson["inputs"] = inputArray;

	QJsonArray outputArray;
	{
		for (const auto& output : m_outputs)
		{
			QJsonObject outputDoc;
			outputDoc["locationIndex"] = int(output.locationIndex);
			outputDoc["name"] = QString::fromStdString(output.name);
			outputDoc["type"] = QString(EnumToString(output.type));

			outputArray.append(outputDoc);
		}
	}
	sceneJson["outputs"] = outputArray;

	QJsonArray structArray;
	{
		for (const auto& s : m_structs)
		{
			QJsonObject structDoc;
			structDoc["name"] = QString::fromStdString(s.name);

			QJsonArray memberArray;
			for (const auto& member : s.members)
			{
				QJsonObject memberDoc;
				memberDoc["name"] = QString::fromStdString(member.name);

				std::visit([&](auto&& arg)
				{
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, PrimitiveType>)
						memberDoc["type"] = QString(EnumToString(arg));
					else if constexpr (std::is_same_v<T, std::size_t>)
						memberDoc["type"] = int(arg);
					else
						static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
				}, member.type);

				memberArray.append(std::move(memberDoc));
			}
			structDoc["members"] = memberArray;

			structArray.append(structDoc);
		}
	}
	sceneJson["structs"] = structArray;

	QJsonArray textureArray;
	{
		for (const auto& texture : m_textures)
		{
			QJsonObject textureDoc;
			textureDoc["bindingIndex"] = int(texture.bindingIndex);
			textureDoc["name"] = QString::fromStdString(texture.name);
			textureDoc["type"] = QString(EnumToString(texture.type));

			textureArray.append(textureDoc);
		}
	}
	sceneJson["textures"] = textureArray;

	QJsonArray nodesJsonArray;
	{
		for (auto&& [uuid, node] : m_flowScene.nodes())
			nodesJsonArray.append(node->save());
	}
	sceneJson["nodes"] = nodesJsonArray;

	QJsonArray connectionJsonArray;
	{
		for (auto&& [uuid, connection] : m_flowScene.connections())
		{
			QJsonObject connectionJson = connection->save();

			if (!connectionJson.isEmpty())
				connectionJsonArray.append(connectionJson);
		}
	}
	sceneJson["connections"] = connectionJsonArray;

	return sceneJson;
}

Nz::ShaderNodes::StatementPtr ShaderGraph::ToAst()
{
	std::vector<Nz::ShaderNodes::StatementPtr> statements;

	using Key = QPair<QUuid, std::size_t>;
	auto BuildKey = [](QUuid uuid, std::size_t index)
	{
		return Key(uuid, index);
	};

	QHash<Key, unsigned int> usageCount;

	std::function<void(QtNodes::Node*, std::size_t)> DetectVariables;
	DetectVariables = [&](QtNodes::Node* node, std::size_t outputIndex)
	{
		auto it = usageCount.find(BuildKey(node->id(), outputIndex));
		if (it == usageCount.end())
		{
			for (const auto& connectionSet : node->nodeState().getEntries(QtNodes::PortType::In))
			{
				for (const auto& [uuid, conn] : connectionSet)
				{
					DetectVariables(conn->getNode(QtNodes::PortType::Out), conn->getPortIndex(QtNodes::PortType::Out));
				}
			}

			it = usageCount.insert(BuildKey(node->id(), outputIndex), 0);
		}

		(*it)++;
	};

	std::vector<QtNodes::Node*> outputNodes;

	m_flowScene.iterateOverNodes([&](QtNodes::Node* node)
	{
		if (node->nodeDataModel()->nPorts(QtNodes::PortType::Out) == 0)
		{
			DetectVariables(node, 0);
			outputNodes.push_back(node);
		}
	});

	QHash<Key, Nz::ShaderNodes::ExpressionPtr> variableExpressions;

	unsigned int varCount = 0;
	std::unordered_set<std::string> usedVariableNames;

	std::function<Nz::ShaderNodes::NodePtr(QtNodes::Node*, std::size_t portIndex)> HandleNode;
	HandleNode = [&](QtNodes::Node* node, std::size_t portIndex) -> Nz::ShaderNodes::NodePtr
	{
		ShaderNode* shaderNode = static_cast<ShaderNode*>(node->nodeDataModel());
		if (shaderNode->validationState() != QtNodes::NodeValidationState::Valid)
			throw std::runtime_error(shaderNode->validationMessage().toStdString());

		qDebug() << shaderNode->name() << node->id();
		if (auto it = variableExpressions.find(BuildKey(node->id(), portIndex)); it != variableExpressions.end())
			return *it;

		auto it = usageCount.find(BuildKey(node->id(), portIndex));
		assert(it != usageCount.end());

		std::size_t inputCount = shaderNode->nPorts(QtNodes::PortType::In);
		Nz::StackArray<Nz::ShaderNodes::ExpressionPtr> expressions = NazaraStackArray(Nz::ShaderNodes::ExpressionPtr, inputCount);
		std::size_t i = 0;

		for (const auto& connectionSet : node->nodeState().getEntries(QtNodes::PortType::In))
		{
			for (const auto& [uuid, conn] : connectionSet)
			{
				assert(i < expressions.size());
				Nz::ShaderNodes::NodePtr inputNode = HandleNode(conn->getNode(QtNodes::PortType::Out), conn->getPortIndex(QtNodes::PortType::Out));
				if (inputNode->IsStatement())
					throw std::runtime_error("unexpected statement");

				expressions[i] = std::static_pointer_cast<Nz::ShaderNodes::Expression>(inputNode);
				i++;
			}
		}

		auto astNode = shaderNode->BuildNode(expressions.data(), expressions.size(), portIndex);

		const std::string& variableName = shaderNode->GetVariableName();
		if (*it > 1 || !variableName.empty())
		{
			if (astNode->IsStatement())
				throw std::runtime_error("unexpected statement");

			auto expression = std::static_pointer_cast<Nz::ShaderNodes::Expression>(astNode);

			Nz::ShaderNodes::ExpressionPtr varExpression;
			if (expression->GetExpressionCategory() == Nz::ShaderNodes::ExpressionCategory::RValue)
			{
				std::string name;
				if (variableName.empty())
					name = "var" + std::to_string(varCount++);
				else
					name = variableName;

				if (usedVariableNames.find(name) != usedVariableNames.end())
					throw std::runtime_error("duplicate variable found: " + name);

				usedVariableNames.insert(name);

				auto variable = Nz::ShaderBuilder::Local(std::move(name), expression->GetExpressionType());
				statements.emplace_back(Nz::ShaderBuilder::DeclareVariable(variable, std::move(expression)));

				varExpression = Nz::ShaderBuilder::Identifier(variable);
			}
			else
				varExpression = std::move(expression);

			variableExpressions.insert(BuildKey(node->id(), portIndex), varExpression);

			return varExpression;
		}
		else
			return astNode;
	};

	std::sort(outputNodes.begin(), outputNodes.end(), [](QtNodes::Node* lhs, QtNodes::Node* rhs)
	{
		ShaderNode* leftNode = static_cast<ShaderNode*>(lhs->nodeDataModel());
		ShaderNode* rightNode = static_cast<ShaderNode*>(rhs->nodeDataModel());

		return leftNode->GetOutputOrder() < rightNode->GetOutputOrder();
	});

	for (QtNodes::Node* node : outputNodes)
	{
		auto astNode = HandleNode(node, 0);
		if (!astNode->IsStatement())
			statements.emplace_back(Nz::ShaderBuilder::ExprStatement(std::static_pointer_cast<Nz::ShaderNodes::Expression>(astNode)));
		else
			statements.emplace_back(std::static_pointer_cast<Nz::ShaderNodes::Statement>(astNode));
	}

	return Nz::ShaderNodes::StatementBlock::Build(std::move(statements));
}

Nz::ShaderExpressionType ShaderGraph::ToShaderExpressionType(const std::variant<PrimitiveType, std::size_t>& type) const
{
	return std::visit([&](auto&& arg) -> Nz::ShaderExpressionType
	{
		using T = std::decay_t<decltype(arg)>;
		if constexpr (std::is_same_v<T, PrimitiveType>)
			return ToShaderExpressionType(arg);
		else if constexpr (std::is_same_v<T, std::size_t>)
		{
			assert(arg < m_structs.size());
			const auto& s = m_structs[arg];
			return s.name;
		}
		else
			static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
	}, type);
};

void ShaderGraph::UpdateBuffer(std::size_t bufferIndex, std::string name, BufferType bufferType, std::size_t structIndex, std::size_t bindingIndex)
{
	assert(bufferIndex < m_buffers.size());
	auto& bufferEntry = m_buffers[bufferIndex];
	bufferEntry.bindingIndex = bindingIndex;
	bufferEntry.name = std::move(name);
	bufferEntry.structIndex = structIndex;
	bufferEntry.type = bufferType;

	OnBufferUpdate(this, bufferIndex);
}

void ShaderGraph::UpdateCondition(std::size_t conditionIndex, std::string condition)
{
	assert(conditionIndex < m_conditions.size());
	auto& conditionEntry = m_conditions[conditionIndex];
	conditionEntry.name = std::move(condition);

	OnConditionUpdate(this, conditionIndex);
}

void ShaderGraph::UpdateInput(std::size_t inputIndex, std::string name, PrimitiveType type, InputRole role, std::size_t roleIndex, std::size_t locationIndex)
{
	assert(inputIndex < m_inputs.size());
	auto& inputEntry = m_inputs[inputIndex];
	inputEntry.locationIndex = locationIndex;
	inputEntry.name = std::move(name);
	inputEntry.role = role;
	inputEntry.roleIndex = roleIndex;
	inputEntry.type = type;

	OnInputUpdate(this, inputIndex);
}

void ShaderGraph::UpdateOutput(std::size_t outputIndex, std::string name, PrimitiveType type, std::size_t locationIndex)
{
	assert(outputIndex < m_outputs.size());
	auto& outputEntry = m_outputs[outputIndex];
	outputEntry.locationIndex = locationIndex;
	outputEntry.name = std::move(name);
	outputEntry.type = type;

	OnOutputUpdate(this, outputIndex);
}

void ShaderGraph::UpdateStruct(std::size_t structIndex, std::string name, std::vector<StructMemberEntry> members)
{
	assert(structIndex < m_structs.size());
	auto& structEntry = m_structs[structIndex];
	structEntry.name = std::move(name);
	structEntry.members = std::move(members);

	OnStructUpdate(this, structIndex);
}

void ShaderGraph::UpdateTexture(std::size_t textureIndex, std::string name, TextureType type, std::size_t bindingIndex)
{
	assert(textureIndex < m_textures.size());
	auto& textureEntry = m_textures[textureIndex];
	textureEntry.bindingIndex = bindingIndex;
	textureEntry.name = std::move(name);
	textureEntry.type = type;

	OnTextureUpdate(this, textureIndex);
}

void ShaderGraph::UpdateTexturePreview(std::size_t textureIndex, QImage preview)
{
	assert(textureIndex < m_textures.size());
	auto& textureEntry = m_textures[textureIndex];
	textureEntry.preview = std::move(preview);
	textureEntry.preview.convertTo(QImage::Format_RGBA8888);

	OnTexturePreviewUpdate(this, textureIndex);
}

void ShaderGraph::UpdateType(ShaderType type)
{
	if (m_type != type)
	{
		m_type = type;
		OnTypeUpdated(this);
	}
}

QtNodes::NodeDataType ShaderGraph::ToNodeDataType(PrimitiveType type)
{
	switch (type)
	{
		case PrimitiveType::Bool:
			return BoolData::Type();

		case PrimitiveType::Float1:
			return FloatData::Type();

		case PrimitiveType::Float2:
		case PrimitiveType::Float3:
		case PrimitiveType::Float4:
			return VecData::Type();

		case PrimitiveType::Mat4x4:
			return Matrix4Data::Type();
	}

	assert(false);
	throw std::runtime_error("Unhandled input type");
}

Nz::ShaderExpressionType ShaderGraph::ToShaderExpressionType(PrimitiveType type)
{
	switch (type)
	{
		case PrimitiveType::Bool:   return Nz::ShaderNodes::BasicType::Boolean;
		case PrimitiveType::Float1: return Nz::ShaderNodes::BasicType::Float1;
		case PrimitiveType::Float2: return Nz::ShaderNodes::BasicType::Float2;
		case PrimitiveType::Float3: return Nz::ShaderNodes::BasicType::Float3;
		case PrimitiveType::Float4: return Nz::ShaderNodes::BasicType::Float4;
		case PrimitiveType::Mat4x4: return Nz::ShaderNodes::BasicType::Mat4x4;
	}

	assert(false);
	throw std::runtime_error("Unhandled primitive type");
}

Nz::ShaderExpressionType ShaderGraph::ToShaderExpressionType(TextureType type)
{
	switch (type)
	{
		case TextureType::Sampler2D:   return Nz::ShaderNodes::BasicType::Sampler2D;
	}

	assert(false);
	throw std::runtime_error("Unhandled texture type");
}

Nz::ShaderStageType ShaderGraph::ToShaderStageType(ShaderType type)
{
	switch (type)
	{
		case ShaderType::NotSet:
			throw std::runtime_error("Invalid shader type");

		case ShaderType::Fragment: return Nz::ShaderStageType::Fragment;
		case ShaderType::Vertex: return Nz::ShaderStageType::Vertex;
	}

	assert(false);
	throw std::runtime_error("Unhandled shader type");
}

std::shared_ptr<QtNodes::DataModelRegistry> ShaderGraph::BuildRegistry()
{
	auto registry = std::make_shared<QtNodes::DataModelRegistry>();

	// Casts
	RegisterShaderNode<CastToVec2>(*this, registry, "Casts");
	RegisterShaderNode<CastToVec3>(*this, registry, "Casts");
	RegisterShaderNode<CastToVec4>(*this, registry, "Casts");

	// Constants
	RegisterShaderNode<BoolValue>(*this, registry, "Constants");
	RegisterShaderNode<FloatValue>(*this, registry, "Constants");
	RegisterShaderNode<Vec2Value>(*this, registry, "Constants");
	RegisterShaderNode<Vec3Value>(*this, registry, "Constants");
	RegisterShaderNode<Vec4Value>(*this, registry, "Constants");

	// Inputs
	RegisterShaderNode<BufferField>(*this, registry, "Inputs");
	RegisterShaderNode<InputValue>(*this, registry, "Inputs");

	// Outputs
	RegisterShaderNode<Discard>(*this, registry, "Outputs");
	RegisterShaderNode<OutputValue>(*this, registry, "Outputs");
	RegisterShaderNode<PositionOutputValue>(*this, registry, "Outputs");

	// Float comparison
	RegisterShaderNode<FloatEq>(*this, registry, "Float comparisons");
	RegisterShaderNode<FloatGe>(*this, registry, "Float comparisons");
	RegisterShaderNode<FloatGt>(*this, registry, "Float comparisons");
	RegisterShaderNode<FloatLe>(*this, registry, "Float comparisons");
	RegisterShaderNode<FloatLt>(*this, registry, "Float comparisons");
	RegisterShaderNode<FloatNe>(*this, registry, "Float comparisons");

	// Float operations
	RegisterShaderNode<FloatAdd>(*this, registry, "Float operations");
	RegisterShaderNode<FloatDiv>(*this, registry, "Float operations");
	RegisterShaderNode<FloatMul>(*this, registry, "Float operations");
	RegisterShaderNode<FloatSub>(*this, registry, "Float operations");

	// Matrix operations
	RegisterShaderNode<Mat4Add>(*this, registry, "Matrix operations");
	RegisterShaderNode<Mat4Mul>(*this, registry, "Matrix operations");
	RegisterShaderNode<Mat4Sub>(*this, registry, "Matrix operations");
	RegisterShaderNode<Mat4VecMul>(*this, registry, "Matrix operations");

	// Shader
	RegisterShaderNode<ConditionalExpression>(*this, registry, "Shader");

	// Texture
	RegisterShaderNode<SampleTexture>(*this, registry, "Texture");
	RegisterShaderNode<TextureValue>(*this, registry, "Texture");

	// Vector comparison
	RegisterShaderNode<VecEq>(*this, registry, "Vector comparisons");
	RegisterShaderNode<VecGe>(*this, registry, "Vector comparisons");
	RegisterShaderNode<VecGt>(*this, registry, "Vector comparisons");
	RegisterShaderNode<VecLe>(*this, registry, "Vector comparisons");
	RegisterShaderNode<VecLt>(*this, registry, "Vector comparisons");
	RegisterShaderNode<VecNe>(*this, registry, "Vector comparisons");

	// Vector operations
	RegisterShaderNode<VecAdd>(*this, registry, "Vector operations");
	RegisterShaderNode<Vec2Composition>(*this, registry, "Vector operations");
	RegisterShaderNode<Vec3Composition>(*this, registry, "Vector operations");
	RegisterShaderNode<Vec4Composition>(*this, registry, "Vector operations");
	RegisterShaderNode<VecDecomposition>(*this, registry, "Vector operations");
	RegisterShaderNode<VecDiv>(*this, registry, "Vector operations");
	RegisterShaderNode<VecDot>(*this, registry, "Vector operations");
	RegisterShaderNode<VecFloatMul>(*this, registry, "Vector operations");
	RegisterShaderNode<VecMul>(*this, registry, "Vector operations");
	RegisterShaderNode<VecSub>(*this, registry, "Vector operations");

	return registry;
}