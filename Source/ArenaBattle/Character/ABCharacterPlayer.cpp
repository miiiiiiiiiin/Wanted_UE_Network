// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ABCharacterPlayer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include <GameFramework/CharacterMovementComponent.h>
#include <GameFramework/GameState.h>
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ABCharacterControlData.h"
#include "UI/ABHUDWidget.h"
#include "CharacterStat/ABCharacterStatComponent.h"
#include "Interface/ABGameInterface.h"
#include <Components/CapsuleComponent.h>
#include <Engine/DamageEvents.h>
#include <Net/UnrealNetwork.h>
#include <EngineUtils.h>

#include <ArenaBattle.h>
#include <Physics/ABCollision.h>

AABCharacterPlayer::AABCharacterPlayer()
{
	// Camera
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Input
	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionJumpRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Jump.IA_Jump'"));
	if (nullptr != InputActionJumpRef.Object)
	{
		JumpAction = InputActionJumpRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputChangeActionControlRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ChangeControl.IA_ChangeControl'"));
	if (nullptr != InputChangeActionControlRef.Object)
	{
		ChangeControlAction = InputChangeActionControlRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionShoulderMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ShoulderMove.IA_ShoulderMove'"));
	if (nullptr != InputActionShoulderMoveRef.Object)
	{
		ShoulderMoveAction = InputActionShoulderMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionShoulderLookRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_ShoulderLook.IA_ShoulderLook'"));
	if (nullptr != InputActionShoulderLookRef.Object)
	{
		ShoulderLookAction = InputActionShoulderLookRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionQuaterMoveRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_QuaterMove.IA_QuaterMove'"));
	if (nullptr != InputActionQuaterMoveRef.Object)
	{
		QuaterMoveAction = InputActionQuaterMoveRef.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> InputActionAttackRef(TEXT("/Script/EnhancedInput.InputAction'/Game/ArenaBattle/Input/Actions/IA_Attack.IA_Attack'"));
	if (nullptr != InputActionAttackRef.Object)
	{
		AttackAction = InputActionAttackRef.Object;
	}

	CurrentCharacterControlType = ECharacterControlType::Quater;

	// 시작할 때는 공격 가능하도록 설정
	bCanAttack = true;

	// 리플리케이션 활성화
	SetReplicates(true);

}

void AABCharacterPlayer::BeginPlay()
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

	Super::BeginPlay();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		EnableInput(PlayerController);
	}

	SetCharacterControl(CurrentCharacterControlType);
}

void AABCharacterPlayer::SetDead()
{
	Super::SetDead();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController)
	{
		DisableInput(PlayerController);
	}
}

void AABCharacterPlayer::PossessedBy(AController* NewController)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	// PossessedBy 호출 전에 소유 확인
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		AB_LOG(
			LogABNetwork, 
			Log, 
			TEXT("Owner: %s"), 
			*OwnerActor->GetName());

	}
	else
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("No Owner"));

	}
	// 호출 후 확인
	Super::PossessedBy(NewController);
	OwnerActor = GetOwner();
	if (OwnerActor)
	{
		AB_LOG(
			LogABNetwork,
			Log,
			TEXT("Owner: %s"),
			*OwnerActor->GetName());

	}
	else
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("No Owner"));

	}
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AABCharacterPlayer::OnRep_Owner()
{

	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	Super::OnRep_Owner();

	// OnRep_Owner 함수 호출 후에 소유 확인
	AActor* OwnerActor = GetOwner();
	if (OwnerActor)
	{
		AB_LOG(LogABNetwork, Log, TEXT("Owner: %s"), *OwnerActor->GetName());
	}
	else
	{
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("No Owner"));
	}
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));

}

void AABCharacterPlayer::PostNetInit()
{
	AB_LOG(LogABNetwork, Log, TEXT("%s %s"), TEXT("Begin"), *GetName());
	Super::PostNetInit();
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));
}

void AABCharacterPlayer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 리플리케이션 속성 랖탐 꼭 켜야함. 
	DOREPLIFETIME(AABCharacterPlayer, bCanAttack);
}

void AABCharacterPlayer::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	EnhancedInputComponent->BindAction(ChangeControlAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ChangeCharacterControl);
	EnhancedInputComponent->BindAction(ShoulderMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderMove);
	EnhancedInputComponent->BindAction(ShoulderLookAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::ShoulderLook);
	EnhancedInputComponent->BindAction(QuaterMoveAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::QuaterMove);
	EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Triggered, this, &AABCharacterPlayer::Attack);
}

void AABCharacterPlayer::ChangeCharacterControl()
{
	if (CurrentCharacterControlType == ECharacterControlType::Quater)
	{
		SetCharacterControl(ECharacterControlType::Shoulder);
	}
	else if (CurrentCharacterControlType == ECharacterControlType::Shoulder)
	{
		SetCharacterControl(ECharacterControlType::Quater);
	}
}

void AABCharacterPlayer::SetCharacterControl(ECharacterControlType NewCharacterControlType)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	UABCharacterControlData* NewCharacterControl = CharacterControlManager[NewCharacterControlType];
	check(NewCharacterControl);

	SetCharacterControlData(NewCharacterControl);

	APlayerController* PlayerController = CastChecked<APlayerController>(GetController());
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		UInputMappingContext* NewMappingContext = NewCharacterControl->InputMappingContext;
		if (NewMappingContext)
		{
			Subsystem->AddMappingContext(NewMappingContext, 0);
		}
	}

	CurrentCharacterControlType = NewCharacterControlType;
}

void AABCharacterPlayer::SetCharacterControlData(const UABCharacterControlData* CharacterControlData)
{
	Super::SetCharacterControlData(CharacterControlData);

	CameraBoom->TargetArmLength = CharacterControlData->TargetArmLength;
	CameraBoom->SetRelativeRotation(CharacterControlData->RelativeRotation);
	CameraBoom->bUsePawnControlRotation = CharacterControlData->bUsePawnControlRotation;
	CameraBoom->bInheritPitch = CharacterControlData->bInheritPitch;
	CameraBoom->bInheritYaw = CharacterControlData->bInheritYaw;
	CameraBoom->bInheritRoll = CharacterControlData->bInheritRoll;
	CameraBoom->bDoCollisionTest = CharacterControlData->bDoCollisionTest;
}

void AABCharacterPlayer::ShoulderMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.X);
	AddMovementInput(RightDirection, MovementVector.Y);
}

void AABCharacterPlayer::ShoulderLook(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AABCharacterPlayer::QuaterMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	float InputSizeSquared = MovementVector.SquaredLength();
	float MovementVectorSize = 1.0f;
	float MovementVectorSizeSquared = MovementVector.SquaredLength();
	if (MovementVectorSizeSquared > 1.0f)
	{
		MovementVector.Normalize();
		MovementVectorSizeSquared = 1.0f;
	}
	else
	{
		MovementVectorSize = FMath::Sqrt(MovementVectorSizeSquared);
	}

	FVector MoveDirection = FVector(MovementVector.X, MovementVector.Y, 0.0f);
	GetController()->SetControlRotation(FRotationMatrix::MakeFromX(MoveDirection).Rotator());
	AddMovementInput(MoveDirection, MovementVectorSize);
}

void AABCharacterPlayer::Attack()
{
	// 공격 가능한 상태인지 확인
	if (!bCanAttack)
	{
		return;
	}

	// 클라이언트인지 확인
	if (!HasAuthority())
	{
		// 클라이언트에서 공격 시작 처리. 
		bCanAttack = false;

		// 클라이언트에서 이미 값을 변경햇기땜에 
		// 서버가 리플리케이션을 통해 보내온 값과 동일함. 
		// 따라서 OnRep 함수가 호출되지 않음. 
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

		// 공격 종료를 위한 타이머 설정
		FTimerHandle Handle;
		GetWorld()->GetTimerManager().SetTimer(
			Handle,
			FTimerDelegate::CreateLambda([&]()
				{
					// 공격 상태 해제 처리
					bCanAttack = true;

					GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
				}), AttackTime, false

		);

		// 애니메이션 재생
		PlayAttackAnimation();

	}
	// 클라이언트에서 명령 처리를 한 뒤에 서버에 전달. 
	// 시간을 보낼 때 서버 기준 시간으로 전달. 
	ServerRPCAttack(GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
}

void AABCharacterPlayer::PlayAttackAnimation()
{
	// 공격 애님 몽타주 재생 처리
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->StopAllMontages(0.0f);
		AnimInstance->Montage_Play(ComboActionMontage);
	}
}

void AABCharacterPlayer::AttackHitCheck()
{
	// 공격 판정은 서버에서만 진행
	if (IsLocallyControlled())
	{
		// 로그 출력
		AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

		FHitResult OutHitResult;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(Attack), false, this);

		const float AttackRange = Stat->GetTotalStat().AttackRange;
		const float AttackRadius = Stat->GetAttackRadius();
		const float AttackDamage = Stat->GetTotalStat().Attack;
		const FVector Forward = GetActorForwardVector();
		const FVector Start = GetActorLocation() + Forward * GetCapsuleComponent()->GetScaledCapsuleRadius();
		const FVector End = Start + Forward * AttackRange;

		bool HitDetected = GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, CCHANNEL_ABACTION, FCollisionShape::MakeSphere(AttackRadius), Params);
		
		// 시간 가져오기
		float HitCheckTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

		// 클라이언트
		if (!HasAuthority())
		{
			if (HitDetected)
			{// 맞앗을 때 
				ServerRPCNotifyHit(OutHitResult, HitCheckTime);
			}
			else
			{// 미스낫을때
				ServerRPCNotifyMiss(Start, End, Forward, HitCheckTime);
			}
		}
		// 서버
		else
		{
			FColor DebugColor = HitDetected ? FColor::Green : FColor::Red;

			DrawDebugAttackRange(
				DebugColor,
				Start,
				End,
				Forward
			);

			// 서버의 경우 판정 결과 확인하지 않고 진행
			if (HitDetected)
			{
				AttackHitConfirm(OutHitResult.GetActor());
			}
		}
	}
}

void AABCharacterPlayer::AttackHitConfirm(AActor* HitActor)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

	// 서버에서 실행
	if (HasAuthority())
	{
		// 대미지 스탯
		const float AttackDamage = Stat->GetTotalStat().Attack;

		// 대미지 이벤트 변수
		FDamageEvent DamageEvent;

		// 대미지 전달
		HitActor->TakeDamage(AttackDamage, DamageEvent, GetController(), this);
	}
}

void AABCharacterPlayer::DrawDebugAttackRange(
	const FColor& DrawColor, FVector TraceStart, FVector TraceEnd, FVector Forward)
{
#if ENABLE_DRAW_DEBUG

	const float AttackRange = Stat->GetTotalStat().AttackRange;
	const float AttackRadius = Stat->GetAttackRadius();

	FVector CapsuleOrigin = TraceStart + (TraceEnd - TraceStart) * 0.5f;
	float CapsuleHalfHeight = AttackRange * 0.5f;

	DrawDebugCapsule(
		GetWorld(), 
		CapsuleOrigin, 
		CapsuleHalfHeight, 
		AttackRadius, 
		FRotationMatrix::MakeFromZ(Forward).ToQuat(), 
		DrawColor, 
		false, 
		5.0f
	);

#endif
}

void AABCharacterPlayer::ClientRPCPlayAnimation_Implementation(AABCharacterPlayer* CharacterToPlay)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));
	if (CharacterToPlay)
	{
		CharacterToPlay->PlayAttackAnimation();
	}
}

void AABCharacterPlayer::ServerRPCNotifyHit_Implementation(const FHitResult& HitResult, float HitCheckTime)
{
	// 맞은 액터 정보 얻어오기
	AActor* HitActor = HitResult.GetActor();

	// 맞은 액터 정보가 유효한지 확인
	if (HitActor)
	{
		// 맞은 위치 정보 가져오기
		const FVector HitLocation = HitResult.Location;

		// 바운딩 박스 정보 가져오기
		const FBox HitBox = HitActor->GetComponentsBoundingBox();

		// 바운딩 박스의 중심 위치. 
		const FVector ActorBoxCenter = HitBox.GetCenter();

		// 충돌 판정에 문제가 없는지 거리 비교
		if (FVector::DistSquared(HitLocation, ActorBoxCenter) <= AcceptCheckDistance * AcceptCheckDistance)
		{
			AttackHitConfirm(HitActor);
		}

#if ENABLE_DRAW_DEBUG
		// 맞은 액터를 점으로 표시
		DrawDebugPoint(
			GetWorld(),
			ActorBoxCenter,
			50.0f, // 점크기
			FColor::Cyan,
			false,
			5.0f
		);

		// 맞은 위치를 점으로 표시
		DrawDebugPoint(
			GetWorld(),
			ActorBoxCenter,
			50.0f, // 점크기
			FColor::Magenta,
			false,
			5.0f
		);
#endif
		// 이미 맞앗다고 판정한 상태
		DrawDebugAttackRange(
			FColor::Green,
			HitResult.TraceStart,
			HitResult.TraceEnd,
			HitActor->GetActorForwardVector()
		);
	}
}

bool AABCharacterPlayer::ServerRPCNotifyHit_Validate(const FHitResult& HitResult, float HitCheckTime)
{
	return true;
}

void AABCharacterPlayer::ServerRPCNotifyMiss_Implementation(
	FVector_NetQuantize TraceStart,
	FVector_NetQuantize TraceEnd,
	FVector_NetQuantizeNormal TraceDir, float HitCheckTime)
{
	// 이미 맞앗다고 판정한 상태
	DrawDebugAttackRange(
		FColor::Green,
		TraceStart,
		TraceEnd,
		TraceDir
	);
}

bool AABCharacterPlayer::ServerRPCNotifyMiss_Validate(
	FVector_NetQuantize TraceStart,
	FVector_NetQuantize TraceEnd,
	FVector_NetQuantizeNormal TraceDir, float HitCheckTime)
{
	// 공격을 처음 했을 때는 패스.
	if (LastAttackStartTime == 0.0f)
	{
		return true;
	}

	return (HitCheckTime - LastAttackStartTime) > AcceptMinCheckTime;
}

void AABCharacterPlayer::OnRep_CanAttack()
{
	// 공격중
	if (!bCanAttack)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);
	}
}


bool AABCharacterPlayer::ServerRPCAttack_Validate(float AttackStartTime)
{
	// 처음 공격하는 경우에는 패스
	if (LastAttackStartTime == 0.0f)
	{
		return true;
	}

	// 클라이언트가 보내온 시간 값이 문제가 없는지 확인. 
	// 공격 애니메이션을 고려했을 때 충분한 시간이 지나고 보내왔는지 검증
	return (AttackStartTime - LastAttackStartTime) > AttackTime;
}

void AABCharacterPlayer::ServerRPCAttack_Implementation(float AttackStartTime)
{
	// 로그 출력
	//AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

	// 공격 관련 처리
	bCanAttack = false;
	OnRep_CanAttack();

	// 현재 서버 시간 - 클라가 보낸 시간
	AttackTimeDifference = GetWorld()->GetTimeSeconds() - AttackStartTime;

	// 로그 - 랙 시간 출력
	AB_LOG(LogABNetwork, Log, TEXT("LagTime: %f"), AttackTimeDifference);

	// 공격 종료 타이머 설정
	// -> 공격 시간 전체로 타이머를 설정하면 랙을 반영하지 않음
	// 따라서 랙을 반영한 시간으로 타이머 설정. 
	AttackTimeDifference = FMath::Clamp(AttackTimeDifference, 0.0f, AttackTime - 0.01f); // 0이 되지 않도록

	// 클라이언트가 보내온 시간 저장
	LastAttackStartTime = AttackStartTime;

	// 타이머 설정
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateLambda([&]()
			{
				bCanAttack = true;
				OnRep_CanAttack();
			}), AttackTime - AttackTimeDifference, false
	);

	// 서버에서도 애니메이션 재생
	PlayAttackAnimation();

	// 서버에 있는 액터에서 실행
	//MulticastRPCAttack();

	// 클라이언트 RPC
	// Range- Based Loop 활용
	for (APlayerController* PlayerController :
		TActorRange<APlayerController>(GetWorld()))
	{
		// 컨트롤러가 Null이거나 서버 제어 컨트롤러일때는 예외처리
		if (PlayerController && GetController() != PlayerController)
		{
			// Local인지확인하는이유: 
			if (!PlayerController->IsLocalController())
			{
				AABCharacterPlayer* OtherPlayer
					= Cast<AABCharacterPlayer>(PlayerController->GetPawn());

				if (OtherPlayer)
				{
					// ClientRPC 호출
					OtherPlayer->ClientRPCPlayAnimation(this);
				}
			}
		}
	}
}

void AABCharacterPlayer::MulticastRPCAttack_Implementation()
{
	// 자기 캐릭터에 대해서는 이미 처리를 했기 때문에
	// 나머지 클라이언트에 대해서만 애니메이션 재생 처리. 
	if (!IsLocallyControlled())
	{
		PlayAttackAnimation();
	}
}

void AABCharacterPlayer::SetupHUDWidget(UABHUDWidget* InHUDWidget)
{
	if (InHUDWidget)
	{
		InHUDWidget->UpdateStat(Stat->GetBaseStat(), Stat->GetModifierStat());
		InHUDWidget->UpdateHpBar(Stat->GetCurrentHp());

		Stat->OnStatChanged.AddUObject(InHUDWidget, &UABHUDWidget::UpdateStat);
		Stat->OnHpChanged.AddUObject(InHUDWidget, &UABHUDWidget::UpdateHpBar);
	}
}
