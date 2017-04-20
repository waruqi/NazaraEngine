// Copyright (C) 2017 Jérôme Leclercq
// This file is part of the "Nazara Development Kit"
// For conditions of distribution and use, see copyright notice in Prerequesites.hpp

#pragma once

#ifndef NDK_ENTITYLIST_HPP
#define NDK_ENTITYLIST_HPP

#include <Nazara/Core/Bitset.hpp>
#include <NDK/Prerequesites.hpp>
#include <NDK/Entity.hpp>

namespace Ndk
{
	class NDK_API EntityList
	{
		public:
			class iterator;
			friend iterator;
			using size_type = std::size_t;

			inline EntityList();
			~EntityList() = default;

			inline void Clear();

			inline bool Has(const Entity* entity) const;
			inline bool Has(EntityId entity) const;

			inline void Insert(Entity* entity);

			inline void Remove(Entity* entity);

			// STL API
			inline iterator begin();
			inline bool empty() const;
			inline iterator end();
			inline size_type size() const;

		private:
			inline std::size_t FindNext(std::size_t currentId) const;
			inline World* GetWorld() const;

			Nz::Bitset<Nz::UInt64> m_entityBits;
			World* m_world;
	};

	class EntityList::iterator : public std::iterator<std::forward_iterator_tag, const EntityHandle>
	{
		friend EntityList;

		public:
			inline iterator(const iterator& iterator);

			const EntityHandle& operator*() const;

			inline iterator& operator=(const iterator& iterator);
			inline iterator& operator++();
			inline iterator operator++(int);

			friend inline bool operator==(const iterator& lhs, const iterator& rhs);
			friend inline bool operator!=(const iterator& lhs, const iterator& rhs);

			friend inline void swap(iterator& lhs, iterator& rhs);

		private:
			inline iterator(const EntityList* world, std::size_t nextId);

			std::size_t m_nextEntityId;
			const EntityList* m_list;
	};
}

#include <NDK/EntityList.inl>

#endif // NDK_ENTITYLIST_HPP
