// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// 로컬 역할 가져오는 매크로.
#define LOG_LOCALROLEINFO *(UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetLocalRole()))

// 리모트 역할 가져오는 매크로.
#define LOG_REMOTEROLEINFO *(UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetRemoteRole()))      

// 로컬 역할 가져오는 매크로. 컴포넌트에서 부모 액터의 로컬 롤을 가져온다. 
#define LOG_SUBLOCALROLEINFO *(UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetOwner()->GetLocalRole()))

// 리모트 역할 가져오는 매크로. 컴포넌트에서 부모 액터의 로컬 롤을 가져온다. 
#define LOG_SUBREMOTEROLEINFO *(UEnum::GetValueAsString(TEXT("Engine.ENetRole"), GetOwner()->GetRemoteRole()))     

// 로그 매크로 작성
#define LOG_NETMODEINFO ((GetNetMode() == ENetMode::NM_Client) ? \
		*FString::Printf(TEXT("CLIENT%d"), UE::GetPlayInEditorID()) : \
		((GetNetMode() == ENetMode::NM_Standalone) ? TEXT("STANDALONE") : TEXT("SERVER")))

#define LOG_CALLINFO ANSI_TO_TCHAR(__FUNCTION__)

#define AB_LOG(LogCat, Verbosity, Format, ...) \
	UE_LOG(LogCat, Verbosity, TEXT("[%s][%s/%s] %s %s"), \
		LOG_NETMODEINFO, LOG_LOCALROLEINFO, LOG_REMOTEROLEINFO, LOG_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

#define AB_SUBLOG(LogCat, Verbosity, Format, ...) \
	UE_LOG(LogCat, Verbosity, TEXT("[%s][%s/%s] %s %s"), \
		LOG_NETMODEINFO, LOG_SUBLOCALROLEINFO, LOG_SUBREMOTEROLEINFO, LOG_CALLINFO, *FString::Printf(Format, ##__VA_ARGS__))

// 로그 카테고리 추가
DECLARE_LOG_CATEGORY_EXTERN(LogABNetwork, Log, All);

