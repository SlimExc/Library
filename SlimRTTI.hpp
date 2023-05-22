/*
 * 	Copyright (C) 2023 Philipp Rimmele & Valentin Felder
 *
 *	Redistribution and use in source and binary forms, with or without modification, are
 *	permitted provided that the following conditions are met:
 *
 * 	1. Redistributions of source code must retain the above copyright notice, this list
 *     of conditions and the following disclaimer.
 *
 * 	2. Redistributions in binary form must reproduce the above copyright notice, this
 *     list of conditions and the following disclaimer in the documentation and/or
 *     other materials provided with the distribution.
 *
 *	3. Neither the name of the copyright holder nor the names of its contributors
 * 	   may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *	“AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EXCEPTIONSYSTEM_SLIMRTTI_H_
#define EXCEPTIONSYSTEM_SLIMRTTI_H_

/*
 * Defines utility functions to use the TypeSystem easily.
 * This simplified System does NOT fully support dynamic polymorphism:
 * - It does not support multiple and virtual inheritance.
 * - It only supports a maximal pointer level of 7.
 * - It does not support runtime reflection and dynamic casting.
 */

#include <cstdint>
#include <type_traits>
#include <tr2/type_traits>

//#define __SLIM_EXC_SLIM_RTTI_QUALIFIER

namespace SlimRTTI
{

template<typename T> inline void** getTypeId() noexcept
{
	if constexpr(std::is_reference<T>::value)
	{
		typedef typename std::remove_reference<T>::type type;
		return getTypeId<type>();
	}
	else if constexpr(std::is_const<T>::value)
	{
		typedef typename std::remove_const<T>::type type;
		return getTypeId<type>();
	}
	else if constexpr(std::is_volatile<T>::value)
	{
		typedef typename std::remove_volatile<T>::type type;
		return getTypeId<type>();
	}
	else if constexpr(std::is_pointer<T>::value)
	{
		typedef typename std::remove_pointer<T>::type type;
		return getTypeId<type>();
	}
	else if constexpr (std::is_void<T>::value)
	{
		static void** const id = nullptr;
		return reinterpret_cast<void**>(const_cast<void***>(&id)); //(void** const)&id;
	}
	else
	{
		if constexpr (std::tr2::direct_bases<T>::type::empty::value) //has no bases?
		{
			static void** const id = nullptr;
			return reinterpret_cast<void**>(const_cast<void***>(&id)); //(void** const)&id;
		}
		else
		{
			typedef typename std::tr2::direct_bases<T>::type::first::type BaseType0; //use first baseclass
			static void** const id = getTypeId<BaseType0>();
			return reinterpret_cast<void**>(const_cast<void***>(&id));
		}
	}
}

	template <typename T> inline constexpr uint8_t getPointerLevel() noexcept
	{
		typedef typename std::remove_reference<T>::type derefType;
		typedef typename std::remove_cv<derefType>::type dequalifiedType;
		if constexpr(std::is_pointer<dequalifiedType>::value)
		{
			typedef typename std::remove_pointer<dequalifiedType>::type depointType;
			return getPointerLevel<depointType>()+1;
		}
		else
		{
			return 0;
		}
	}

	template <class T> static inline constexpr bool isInherited() noexcept
	{
		if constexpr(std::is_reference<T>::value)
		{
			typedef typename std::remove_reference<T>::type type;
			return isInherited<type>();
		}
		else if constexpr(std::is_const<T>::value)
		{
			typedef typename std::remove_const<T>::type type;
			return isInherited<type>();
		}
		else if constexpr(std::is_volatile<T>::value)
		{
			typedef typename std::remove_volatile<T>::type type;
			return isInherited<type>();
		}
		else if constexpr(std::is_pointer<T>::value)
		{
			typedef typename std::remove_pointer<T>::type type;
			return isInherited<type>();
		}
		else
		{
			return !std::tr2::direct_bases<T>::type::empty::value;
		}
	}


	class InstanceType
	{
	private:
		void** typeId = nullptr;
#ifdef __SLIM_EXC_SLIM_RTTI_POINTER
#ifdef __SLIM_EXC_SLIM_RTTI_QUALIFIER
		union MetaData {
			uint16_t data;
			struct Fields {
				uint8_t constMask;
				uint8_t ptrDepth;
				/* alternativ implementation with more metadata (and worse performance)
				uint16_t constMask : 8;
				uint16_t ptrDepth : 3;
				uint16_t isReference : 1;
				uint16_t isInherited : 1;
				uint16_t unused : 3;
				*/
			} fields;
		} metaData = {};

		template<typename T> static inline MetaData initializeMetadata() noexcept
		{
			MetaData tmp;
			tmp.fields.constMask = getConstMask<T>();
			auto constexpr ptrDepth = getPointerLevel<T>();
			static_assert(ptrDepth < 8, "SlimRTTI only support a Pointer Level < 8");
			tmp.fields.ptrDepth = ptrDepth;
			/* alternativ implementation with more metadata (and worse performance)
			tmp.fields.isReference = std::is_reference<T>::value ? 1 : 0;
			tmp.fields.isInherited =isInherited<T>() ? 1 : 0;
			*/
			return tmp;
		}
#else //__SLIM_EXC_SLIM_RTTI_QUALIFIER
		uint8_t ptrDepth = 0;
#endif//__SLIM_EXC_SLIM_RTTI_QUALIFIER
#endif


		template <typename T> static inline constexpr uint8_t getConstMask() noexcept
		{
			if constexpr(std::is_reference<T>::value)
			{
				typedef typename std::remove_reference<T>::type type;
				return getConstMask<type>();
			}
			else if constexpr(std::is_volatile<T>::value)
			{
				typedef typename std::remove_volatile<T>::type type;
				return getConstMask<type>();
			}
			else
			{
				if constexpr (std::is_const<T>::value)
				{
					typedef typename std::remove_const<T>::type type;
					if constexpr(std::is_pointer<type>::value)
					{
						typedef typename std::remove_pointer<T>::type type;
						return 1 | (getConstMask<type>() << 1);
					}
					else
					{
						return 1;
					}
				}
				else
				{
					if constexpr(std::is_pointer<T>::value)
					{
						typedef typename std::remove_pointer<T>::type type;
						return getConstMask<type>() << 1;
					}
					else
					{
						return 0;
					}
				}
			}
		}


		template <class T> static inline constexpr bool isPotentialBasetype() noexcept
		{
			if constexpr(std::is_reference<T>::value)
			{
				typedef typename std::remove_reference<T>::type type;
				return isPotentialBasetype<type>();
			}
			else if constexpr(std::is_const<T>::value)
			{
				typedef typename std::remove_const<T>::type type;
				return isPotentialBasetype<type>();
			}
			else if constexpr(std::is_volatile<T>::value)
			{
				typedef typename std::remove_volatile<T>::type type;
				return isPotentialBasetype<type>();
			}
			else if constexpr(std::is_pointer<T>::value)
			{
				typedef typename  std::remove_pointer<T>::type type;
				return isPotentialBasetype<type>();
			}
			else
			{
				if(std::is_class<T>::value)
				{
					return true;
				}
				return false;
			}
		}

		template <class T> inline bool isPtrDepthCatchCompatible() noexcept
		{
#ifdef __SLIM_EXC_SLIM_RTTI_POINTER
#ifdef __SLIM_EXC_SLIM_RTTI_QUALIFIER
			if(this->metaData.fields.ptrDepth != getPointerLevel<T>())
			{
				return false;
			}
#else //SLIM_EXC_SLIM_RTTI_QUALIFIER
			if(this->ptrDepth != getPointerLevel<T>())
			{
				return false;
			}
#endif //SLIM_EXC_SLIM_RTTI_QUALIFIER
#else //__SLIM_EXC_SLIM_RTTI_POINTER
			static_assert(std::is_pointer<T>::value == false, "Pointer-handling is disabled in SlimRTTI. Enable it in configuration 'RTTI/PointerTypes'!");
#endif//__SLIM_EXC_SLIM_RTTI_POINTER
			return true;

		}

		template <class T> inline bool areQualifiersCatchCompatible() noexcept
		{
#if (defined __SLIM_EXC_SLIM_RTTI_POINTER) and (defined __SLIM_EXC_SLIM_RTTI_QUALIFIER)
			if(this->metaData.fields.ptrDepth > 0)
			{
				//It is allowed to add const-qualifiers, but not to remove any
				uint8_t invertedCatchMask = ~getConstMask<T>();
				if((invertedCatchMask & this->metaData.fields.constMask) != 0)
				{
					return false;
				}
			}
#endif //(defined __SLIM_EXC_SLIM_RTTI_POINTER) and (defined __SLIM_EXC_SLIM_RTTI_QUALIFIER)
			return true;
		}


	public:
		inline InstanceType() noexcept
		{
			clear();
		}

		void inline clear() noexcept
		{
			typeId = getTypeId<void>();
#ifdef __SLIM_EXC_SLIM_RTTI_POINTER
#ifdef __SLIM_EXC_SLIM_RTTI_QUALIFIER
			metaData.data = 0;
#else
			ptrDepth = 0;
#endif
#endif
		}



		InstanceType& operator=(const InstanceType& other) noexcept
		{
		    // Guard self assignment
		    if (this == &other)
		        return *this;

		    this->typeId = other.typeId;
#ifdef __SLIM_EXC_SLIM_RTTI_POINTER
#ifdef __SLIM_EXC_SLIM_RTTI_QUALIFIER
			metaData.data = other.metaData.data;
#else
			this->ptrDepth = other.ptrDepth;
#endif

#endif
			return *this;
		}

		template <class T> void set() noexcept
		{
			this->typeId = getTypeId<T>();
#ifdef __SLIM_EXC_SLIM_RTTI_POINTER
#ifdef __SLIM_EXC_SLIM_RTTI_QUALIFIER
			this->metaData = initializeMetadata<T>();
#else
			this->ptrDepth = getPointerLevel<T>();
#endif
#else
			static_assert(std::is_pointer<T>::value == false, "Pointer-handling is disabled in SlimRTTI. Enable it in configuration 'RTTI/PointerTypes'!");
#endif
		}

		template <class T> inline bool isEqualTo() noexcept
		{
			void** tmpTypeID = getTypeId<T>();
			if(tmpTypeID == this->typeId)
			{
				return true;
			}
			return false;
		}

		template <class T> inline bool isDerivedOf() noexcept
		{
			if constexpr(isPotentialBasetype<T>())
			{
				//Class-Instance
				void** ppTmpId = (void**)*(this->typeId);
				while(ppTmpId != nullptr)
				{
					//Vergleiche die Adressen
					if(getTypeId<T>() == ppTmpId)
					{
						return true;
					}
					//Verfolge Pointer eine Ebene nach oben
					ppTmpId = (void**)*ppTmpId;
				}
			}
			return false;
		}

		template <class T> inline bool isBaseOf() noexcept
		{
			void** ppTId =  (void**)*(getTypeId<T>());
			while(ppTId != nullptr)
			{
				//Vergleiche die Adressen
				if(this->typeId == ppTId)
				{
					return true;
				}
				//Verfolge Pointer eine Ebene nach oben
				ppTId = (void**)*ppTId;
			}

			return false;
		}

		template <class T> bool do_catch() noexcept
		{
			if(!isPtrDepthCatchCompatible<T>() || !areQualifiersCatchCompatible<T>())
			{
				return false;
			}

			if(this->isEqualTo<T>() || this->isDerivedOf<T>())
			{
				return true;
			}

			return false;
		}
	};
}

#endif /* EXCEPTIONSYSTEM_SLIMRTTI_H_ */
