// Fill out your copyright notice in the Description page of Project Settings.


#include "Prop/ABFountain.h"
#include "Components/StaticMeshComponent.h"
#include <Net/UnrealNetwork.h>
#include <ArenaBattle.h>

// Sets default values
AABFountain::AABFountain()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Water = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Water"));

	RootComponent = Body;
	Water->SetupAttachment(Body);
	Water->SetRelativeLocation(FVector(0.0f, 0.0f, 132.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/ArenaBattle/Environment/Props/SM_Plains_Castle_Fountain_01.SM_Plains_Castle_Fountain_01'"));
	if (BodyMeshRef.Object)
	{
		Body->SetStaticMesh(BodyMeshRef.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> WaterMeshRef(TEXT("/Script/Engine.StaticMesh'/Game/ArenaBattle/Environment/Props/SM_Plains_Fountain_02.SM_Plains_Fountain_02'"));
	if (WaterMeshRef.Object)
	{
		Water->SetStaticMesh(WaterMeshRef.Object);
	}

	// 리플리케이션 옵션 활성화. 
	SetReplicates(true);

	// 네트워크 전송 빈도 설정(1초에 1번)
	SetNetUpdateFrequency(1.0f);

	// 연관성 판정 거리값 설정(NetCullDistanceSquared)
	SetNetCullDistanceSquared(4000000.0f); // 20미터
}

// Called when the game starts or when spawned
void AABFountain::BeginPlay()
{
	Super::BeginPlay();

	// 타이머를 활용해 로직 반복 실행
	// 서버에서만 실행. 
	//if (HasAuthority())
	//{
	//	FTimerHandle Handle;
	//	GetWorld()->GetTimerManager().SetTimer(
	//		Handle,
	//		FTimerDelegate::CreateLambda([&]()
	//			{
	//				ServerRotationYaw += 1.0f;
	//			}), 1.0f, true
	//	);
	//}

}

void AABFountain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 리플리케이션 대상 속성 추가 (클래스, 변수)
	DOREPLIFETIME(AABFountain, ServerRotationYaw);
}

void AABFountain::OnActorChannelOpen(FInBunch& InBunch, UNetConnection* Connection)
{
	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("Begin"));

	Super::OnActorChannelOpen(InBunch, Connection);

	AB_LOG(LogABNetwork, Log, TEXT("%s"), TEXT("End"));
}

bool AABFountain::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	bool Result = Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
	if (!Result)
	{
		AB_LOG(LogABNetwork, Log, TEXT("Not Relavant! [%s] %s"),
			*RealViewer->GetName(), *SrcLocation.ToCompactString());
	}
	return false;
}

// Called every frame
void AABFountain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버/클라이언트 로직 구분해서 구현. 
	// -> 이 액터가 존재하는 위치를 구분해야 함 (서버에도있고 클라에도있음... )
	// HasAuthority로 서버인지 클라인지 확인. 
	if (HasAuthority())
	{
		// 서버일 때
		// 회전 적용
		AddActorLocalRotation(FRotator(0.0f, RotationRate * DeltaTime, 0.0f));

		//// 변경된 회전 값을 복제(리플리케이션)되는 변수에 설정. 
		ServerRotationYaw = RootComponent->GetComponentRotation().Yaw;
	}
	else
	{
		//// 클라일 때
		//// 서버로부터 전달받은 "변경된 회전 값"을 적용.(피치, 롤 값은 유지)
		//FRotator NewRotator = RootComponent->GetComponentRotation();
		//// 요 회전값은 서버로부터 전달받은 값 설정
		//NewRotator.Yaw = ServerRotationYaw;

		//// 루트 컴포넌트에 회전 설정. 
		//RootComponent->SetWorldRotation(NewRotator);

		// 서버로부터 데이터를 받은 이후에 경과한 시간. 
		//ClientTimeSinceUpdate += DeltaTime;

		//// 서버로부터 데이터를 받은 이후에 경과한 시간이 얼마 안되었으면 종료.
		//if (ClientTimeBetweenLastUpdate < KINDA_SMALL_NUMBER)
		//{
		//	return;
		//}

		//// 회전 예측 값 - 다음에 전달 받을 회전의 예측 값. 
		//const float EstimateRotationYaw = ServerRotationYaw + RotationRate * ClientTimeBetweenLastUpdate;

		//// 보간 시간 계산. (T또는  Alpha)
		//const float LerpRatio = ClientTimeSㅁinceUpdate / ClientTimeBetweenLastUpdate;

		//// 클라이언트 보간
		//const float ClientNewYaw 
		//	= FMath::Lerp(ServerRotationYaw, EstimateRotationYaw, LerpRatio);

		//FRotator ClinetRotator = RootComponent->GetComponentRotation();
		//ClinetRotator.Yaw = ClientNewYaw;

		//// 회전 값 설정
		//RootComponent->SetWorldRotation(ClinetRotator);
	}
}

void AABFountain::OnRep_ServerRotationYaw()
{
	// 로그 출력
	AB_LOG(LogABNetwork, Log, TEXT("Yaw: %f"), ServerRotationYaw);

	// 클라일 때
	// 서버로부터 전달받은 "변경된 회전 값"을 적용. (피치, 롤 값은 유지)
	FRotator NewRotator = RootComponent->GetComponentRotation();

	// 요 회전값은 서버로부터 전달받은 값 설정
	NewRotator.Yaw = ServerRotationYaw;

	// 루트 컴포넌트에 회전 설정. 
	RootComponent->SetWorldRotation(NewRotator);

	// 시간 갱신
	ClientTimeBetweenLastUpdate = ClientTimeSinceUpdate;

	// 경과 시간 초기화. 
	ClientTimeSinceUpdate = 0.0f;
}

