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

#ifndef EXCEPTIONSYSTEM_EXCEPTIONSTATE_H_
#define EXCEPTIONSYSTEM_EXCEPTIONSTATE_H_

/*
 * This file implements class "ExceptionState" which represents the entire state of the current exception,
 * including the Exception object. It also provides convenient methods for checking the current state,
 * accessing the exception-object and throwing.
 */

#include <cstddef>

#include "SlimRTTI.hpp"

#ifdef __GXX_RTTI
#include <typeinfo>
#include <typeindex>
#endif


//Only to prevent warnings from IDEs, defines will be set by Plugin
#ifndef __SLIM_EXC_THROWABLETYPE
#define __SLIM_EXC_THROWABLETYPE unsigned int
#endif

#ifndef __SLIM_EXC_BUFFER_SIZE
#define __SLIM_EXC_BUFFER_SIZE 10
#endif

//use this in your code if the config is set to "onlyOneType"
typedef __SLIM_EXC_THROWABLETYPE throwable_t;


using namespace SlimRTTI;

extern void* operator new (size_t, void* ptr) noexcept;
extern void  operator delete  (void*, void*) noexcept;

namespace std
{
extern void terminate() noexcept;
}


namespace SlimExcLib
{

//std::move-replacement to not be dependend on std-library
template<typename T> constexpr std::remove_reference_t<T>&& move(T&& t) noexcept
{
    return static_cast<std::remove_reference_t<T>&&>(t);
}



// This class represents the entire state of the current exception, including the Exception object itself.
class ExceptionState final {
public:
	//Version of the GCC-Exception-Library
	static const uint8_t versionMajor = 0;	//Checked by plugin to be compatible
	static const uint8_t versionMinor = 9;	//Checked by plugin to be compatible
	static const uint8_t versionPatch = 0;	//Not checked

private:

#ifndef __SLIM_EXC_ONLY_ONE_TYPE
	// The raw buffer for the Exception object.
	unsigned char exceptionBuffer[__SLIM_EXC_BUFFER_SIZE] alignas(alignof(std::max_align_t));
#else
	unsigned char exceptionBuffer[sizeof(__SLIM_EXC_THROWABLETYPE)] alignas(alignof(__SLIM_EXC_THROWABLETYPE));
#endif

#ifndef __SLIM_EXC_ONLY_ONE_TYPE
//When class-instances can be thrown, we need a complex typeId
#ifdef __SLIM_EXC_RTTI_STRATEGY_SLIM
//typeId with SlimRTTI
	InstanceType typeId;
#else
//typeId with normal RTTI
	const std::type_info* typeId;
#endif

#ifndef __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
//The destructor ist only needed when class-instances can be thrown
	void(*destruct)(const void*) noexcept; // A pointer to the Destructor of the currently active Exception, if it isn't a fundamental type.
#endif //__SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
#endif //__SLIM_EXC_ONLY_ONE_TYPE


	ExceptionState* previousES = NULL;

	enum State : uint8_t {
		CLEAR = 0,				//empty (no active exceptions exists in this ExceptionState-Object)
		HANDLERETHROW = 1,		//handling the exception contained in an ExceptionState-Object lower down the list
		HANDLETHROW = 2,		//handling the exception contained in this ExceptionState-Object
		THROW = 3,				//throwing the exception contained in this ExcpetionState-Object
		RETHROW = 4				//rethrowing the exception contained in an ExceptionState-Object lower down the list
	} state = CLEAR;

	//Helper-Fuction to comare adresses with lvalue
	template <class T> static inline bool compareAdresses(T& exception, void* bufferAdr) noexcept
	{
		return (void*)(&exception) == bufferAdr;
	}

	// Wrapper-Function for the Destructor
	template<class T> static void destructorInvoker(const void* obj) noexcept
	{
	  reinterpret_cast<const T*>(obj)->~T();
	}

	void propagateUp() noexcept;

	void takeInstance(ExceptionState* source) noexcept;

	inline ExceptionState* getLatestHandlingExceptionState(void)noexcept
	{
		ExceptionState* tmpState = this->previousES;
		while(tmpState != NULL)
		{
			if(tmpState->state == State::HANDLETHROW)
			{
				return tmpState;
			}
			tmpState = tmpState->previousES;
		}
		return NULL;
	}

	template <class T> bool throwExceptionHelper(T& exc) noexcept
	{
 		if (this->isExceptionThrowing())
		{
			std::terminate(); //multiple exceptions cannot coexist! This could happen when a unhandled throw occurs (nested) inside a catch-block, or within an Exception-object's Destructor/Move-Constructor.
		}
		else
		{
			if(compareAdresses(exc, this->exceptionBuffer)) //Explicite rethrow?
			{
				this->setToThrowingState();
				return false;
			}

#ifndef __SLIM_EXC_ONLY_ONE_TYPE
#ifndef __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
			if(this->destruct != nullptr)
			{//Call destructor for the old object
				this->destruct((const void*)&this->exceptionBuffer);
			}



			if constexpr(std::is_destructible<T>()) //is T destructible?
			{
				destruct = &ExceptionState::destructorInvoker<T>;
			}
			else
			{
				destruct = nullptr;
			}
#endif //__SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
#ifdef __SLIM_EXC_RTTI_STRATEGY_SLIM
			this->typeId.set<T>();
#else
#ifdef __SLIM_EXC_PLUGIN	//Only to prevent compiler-error if SlimExceptions are disabled
			this->typeId = &typeid(T);
#endif//__SLIM_EXC_PLUGIN
#endif //__SLIM_EXC_RTTI_STRATEGY_SLIM
#endif //__SLIM_EXC_ONLY_ONE_TYPE
			this->state = State::THROW;
		}
 		return true;
	}

public:

	// Default Constructor
	ExceptionState(ExceptionState* previous) noexcept;

	// Default Destructor
	~ExceptionState() noexcept;

	// Returns a pointer to the current ExceptionState-Object. Has to be implemented by the user, in order to allow for multithreading implementations.
	static ExceptionState* getCurrentExceptionState() noexcept;

	static void setCurrentExceptionState(ExceptionState* newInstance) noexcept;

	inline bool isExceptionInState(State state) noexcept { return this->state == state; }
	inline bool isExceptionThrowing() noexcept { return this->state >= State::THROW; }
	inline void setToHandlingState() noexcept { this->state = State::HANDLETHROW; }
	inline void setToThrowingState() noexcept { this->state = State::THROW; }
	inline void setToHandleRethrowState() noexcept { this->state = State::HANDLERETHROW; }
	inline void setToRethrowingState() noexcept { this->state = State::RETHROW; }

	inline void rethrow(void) noexcept
	{
		if(this->state == State::HANDLETHROW)
		{
			this->setToThrowingState();
			return;
		}

		//Search in the older (outer) exceptions for the correct reference
		ExceptionState* tmpState = this->previousES;
		while(tmpState != NULL)
		{
			if(tmpState->state == State::HANDLETHROW)
			{
				this->setToRethrowingState();
				return;
			}
			tmpState = tmpState->previousES;
		}

		std::terminate();
	}



	// Checks if the given template parameter type is equal to OR a basetype of the currently active Exception.
	template <class T> inline bool holdsExceptionOfTypeT() noexcept
	{
#ifdef __SLIM_EXC_ONLY_ONE_TYPE
		return true;
#else //__SLIM_EXC_ONLY_ONE_TYPE
		ExceptionState* tmpState = this;

		if(this->state == State::RETHROW)
		{
			tmpState = getLatestHandlingExceptionState();
		}
#if (defined __SLIM_EXC_RTTI_STRATEGY_SLIM)
		return tmpState->typeId.do_catch<T>();
#else
#ifdef __SLIM_EXC_PLUGIN	//Only to prevent compiler-error if SlimExceptions are disabled
		return typeid(T).__do_catch(tmpState->typeId, (void**)&this->exceptionBuffer, SlimRTTI::getPointerLevel<T>());
#else
		return false;	//Only to prevent compiler-Warning
#endif//__SLIM_EXC_PLUGIN
#endif //__SLIM_EXC_RTTI_STRATEGY_SLIM
#endif //__SLIM_EXC_ONLY_ONE_TYPE
	}


	// Returns a reference to the currently active Exception of type T. Should only be called after the type was successfully checked with "holdsExceptionOfTypeT()".
	template <class T> inline void* getExceptionReference() noexcept
	{
		ExceptionState* tmpState = this;

		if(this->state == State::RETHROW)
		{
			//Search in the older exceptions for the correct reference
			tmpState = getLatestHandlingExceptionState();
			this->setToHandleRethrowState();
		}
		else
		{
			this->setToHandlingState();
		}

		if constexpr(std::is_pointer<T>::value)
		{
			return (void*)(*reinterpret_cast<void**>(tmpState->exceptionBuffer));
		}
		else
		{
			return reinterpret_cast<void*>(tmpState->exceptionBuffer);
		}
	}

	template <class T> inline void throwException(T& exc) noexcept
	{
		if(throwExceptionHelper<T>(exc))
		{
			//Move new exception in Buffer
			new(this->exceptionBuffer) T(exc);
		}
	}

	// Throws the Exception which was preconstructed via placement-new into the internal ExceptionBuffer. Use together with "getExceptionBuffer()" and placement-new!
	template <class T> inline void throwException(T&& exc) noexcept
	{
		if(throwExceptionHelper<T>(exc))
		{
			//Move new exception in Buffer
			new(this->exceptionBuffer) T(move(exc));
		}
	}


};

}//Endnamespace SlimExcLib

#endif /* EXCEPTIONSYSTEM_EXCEPTIONSTATE_H_ */
