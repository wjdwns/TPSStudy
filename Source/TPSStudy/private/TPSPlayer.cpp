// Fill out your copyright notice in the Description page of Project Settings.


#include "TPSPlayer.h"
#include <GameFramework/SpringArmComponent.h>
#include <Camera/CameraComponent.h>
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "Bullet.h"

// Sets default values
ATPSPlayer::ATPSPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh(TEXT("/SkeletalMesh'/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple'"));
	if (TempMesh.Succeeded()) {
		GetMesh()->SetSkeletalMesh(TempMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -90), FRotator(0, -90, 0));
	}

	springArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	springArmComp->SetupAttachment(RootComponent);
	springArmComp->SetRelativeLocation(FVector(0, 70, 90));
	springArmComp->TargetArmLength = 400;
	springArmComp->bUsePawnControlRotation = true;
	tpsCamComp = CreateDefaultSubobject<UCameraComponent>(TEXT("TpsCamComp"));
	tpsCamComp->SetupAttachment(springArmComp);
	tpsCamComp->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = true;
	
	JumpMaxCount = 2;
	
	//4. 총 스켈레탈메시 컴포넌트 등록
	gunMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GunMeshComp"));
	//4-1 부모 컴포넌트를 Mesh 컴포넌트로 설정
	gunMeshComp->SetupAttachment(GetMesh());
	//4-2 스켈레탈메시 데이터 로드
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempGunMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Weapons/Pistol/Meshes/SKM_Pistol.SKM_Pistol'"));
	//4-3 데이터 로드 성공
	if (TempGunMesh.Succeeded())
	{
		//4-4 스켈레탈메시 데이터 할당
		gunMeshComp->SetSkeletalMesh((TempGunMesh.Object));
		//4-5 위치 조정하기
		gunMeshComp->SetRelativeLocation(FVector(-14, 52, 120));
	}
	//5. 스나이퍼 컴포넌트 등록
	sniperGunComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SniperGunComp"));
	//5-1 부모 컴포넌트를 Mesh 컴포넌트로 설정
	sniperGunComp->SetupAttachment(GetMesh());
	//5-2 스태틱메시 데이터 로드
	ConstructorHelpers::FObjectFinder<UStaticMesh> TempSniperMesh(TEXT("/Script/Engine.StaticMesh'/Game/SniperGun/sniper11.sniper11'"));
	//5-3 데이터로드가 성공했다면
	if (TempSniperMesh.Succeeded())
	{
		//5-4 스태틱메시 데이터 할당
		sniperGunComp->SetStaticMesh(TempSniperMesh.Object);
		//5-5 위치 조정하기
		sniperGunComp->SetRelativeLocation(FVector(-22, 55, 120));
		//5-6 크기 조장하기
		sniperGunComp->SetRelativeScale3D(FVector(0.15f));
	}
}

// Called when the game starts or when spawned
void ATPSPlayer::BeginPlay()
{
	Super::BeginPlay();
	
	auto pc = Cast<APlayerController>(Controller);
	if (pc)
	{
		auto subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(pc->GetLocalPlayer());
		if (subsystem) {
			subsystem->AddMappingContext(imc_TPS, 0);
		}
	}
	ChangeToSniperGun(FInputActionValue());
}

// Called every frame
void ATPSPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	direction = FTransform(GetControlRotation()).TransformVector(direction);
	//FVector P0 = GetActorLocation();
	//FVector vt = direction * walkSpeed * DeltaTime;
	//FVector P = P0 + vt;
	//SetActorLocation(P);
	AddMovementInput(direction);
	direction = FVector::ZeroVector;
	PlayerMove();
}

void ATPSPlayer::PlayerMove()
{
	direction = FTransform(GetControlRotation()).TransformVector(direction);
	AddMovementInput(direction);
	direction = FVector::ZeroVector;
}

// Called to bind functionality to input
void ATPSPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	auto PlayerInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (PlayerInput) {
		PlayerInput->BindAction(ia_Turn, ETriggerEvent::Triggered, this, &ATPSPlayer::Turn);
		PlayerInput->BindAction(ia_LookUp, ETriggerEvent::Triggered, this, &ATPSPlayer::LookUp);
		PlayerInput->BindAction(ia_Move, ETriggerEvent::Triggered, this, &ATPSPlayer::Move);
		PlayerInput->BindAction(ia_Jump, ETriggerEvent::Triggered, this, &ATPSPlayer::InputJump);
		PlayerInput->BindAction(ia_Fire, ETriggerEvent::Started, this, &ATPSPlayer::InputFire);
		PlayerInput->BindAction(ia_GrenadeGun, ETriggerEvent::Started, this, &ATPSPlayer::ChangeToGrenadeGun);
		PlayerInput->BindAction(ia_SniperGun, ETriggerEvent::Started, this, &ATPSPlayer::ChangeToSniperGun);
		PlayerInput->BindAction(ia_Sniper,ETriggerEvent::Started, this, &ATPSPlayer::SniperAim);
		PlayerInput->BindAction(ia_Sniper,ETriggerEvent::Completed,this,&ATPSPlayer::SniperAim);
		
	}
}

void ATPSPlayer::SniperAim(const struct FInputActionValue& inputValue)
{
	
}

void ATPSPlayer::InputFire(const struct FInputActionValue& inputValue)
{
	//총알 발사처리
	FTransform firePosition = gunMeshComp->GetSocketTransform(TEXT("FirePosition"));
	GetWorld()->SpawnActor<ABullet>(bulletFactory, firePosition);
}

void ATPSPlayer::Turn(const FInputActionValue& inputValue)
{
	float value = inputValue.Get<float>();
	AddControllerYawInput(value);
}

void ATPSPlayer::LookUp(const FInputActionValue& inputValue)
{
	float value = inputValue.Get<float>();
	AddControllerPitchInput(value);
}

void ATPSPlayer::Move(const struct FInputActionValue& inputValue)
{
	FVector2D value = inputValue.Get<FVector2D>();
	direction.X = value.X;
	direction.Y = value.Y;
}

void ATPSPlayer::InputJump(const struct FInputActionValue& inputValue)
{
		Jump();
}

//유탄총 교체
void ATPSPlayer::ChangeToGrenadeGun(const struct FInputActionValue& inputValue)
{
	bUsingGrenadeGun = true;
	sniperGunComp->SetVisibility(false);
	gunMeshComp->SetVisibility(true);
}

//스나이퍼건 교체
void ATPSPlayer::ChangeToSniperGun(const struct FInputActionValue& inputValue)
{
	bUsingGrenadeGun = false;
	sniperGunComp->SetVisibility(true);
	gunMeshComp->SetVisibility(false);
}
