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

#include "ExceptionState.hpp"

using namespace SlimRTTI;

namespace SlimExcLib
{

ExceptionState::ExceptionState(ExceptionState* previous) noexcept :
#ifndef __SLIM_EXC_ONLY_ONE_TYPE
		typeId(),
#ifndef __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
		destruct(nullptr),
#endif //__SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
#endif //__SLIM_EXC_ONLY_ONE_TYPE
		previousES(previous)
{
	setCurrentExceptionState(this);
}

ExceptionState::~ExceptionState() noexcept
{
	if(this->isExceptionThrowing())
	{
		this->propagateUp();
	}
#if (not defined __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES) && (not defined __SLIM_EXC_ONLY_ONE_TYPE)
	else if (this->destruct != nullptr)
	{
		this->destruct((const void*)&this->exceptionBuffer);
	}
#endif

	setCurrentExceptionState(this->previousES);
}



void ExceptionState::propagateUp() noexcept
{
	if((this->previousES == NULL) || this->previousES->isExceptionThrowing())
	{
		std::terminate();
	}

	//If the exception to rethrow is in the previous "exceptionState"-Instance
	if (this->isExceptionInState(State::RETHROW) && (this->previousES->isExceptionInState(State::HANDLETHROW)))
	{
		//Set it to throwing (it holds the exception)
		this->previousES->setToThrowingState();
		return;
	}

#if (not  defined __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES) && (not defined __SLIM_EXC_ONLY_ONE_TYPE)
	//Destruct previous exception if one exists there
	if (this->previousES->destruct != nullptr)
	{
		this->previousES->destruct((const void*)&this->previousES->exceptionBuffer);
	}
#endif

	this->previousES->takeInstance(this);
}

void ExceptionState::takeInstance(ExceptionState* source) noexcept
{
#ifndef __SLIM_EXC_ONLY_ONE_TYPE
	this->typeId = source->typeId;

#ifndef __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
	this->destruct = source->destruct;
#endif//__SLIM_EXC_ONLY_FUNDAMENTAL_TYPES
#endif//__SLIM_EXC_ONLY_ONE_TYPE

	this->state = source->state;

	for (size_t i = 0; i < sizeof(this->exceptionBuffer); i++)
	{
		this->exceptionBuffer[i] = source->exceptionBuffer[i];
	}

#if (not defined __SLIM_EXC_ONLY_ONE_TYPE) && (not defined __SLIM_EXC_ONLY_FUNDAMENTAL_TYPES)
	source->destruct = nullptr;
#endif

#ifndef __SLIM_EXC_ONLY_ONE_TYPE
#ifdef __SLIM_EXC_RTTI_STRATEGY_SLIM
	source->typeId.clear();
#else //__SLIM_EXC_RTTI_STRATEGY_SLIM
	source->typeId = nullptr;
#endif //__SLIM_EXC_RTTI_STRATEGY_SLIM
#endif //__SLIM_EXC_ONLY_ONE_TYPE
	source->state = State::CLEAR;
}

}
