// This code was written by 康子秋
#include "PlayerCharacterBase.h"
#include "FPS_TESTGAME.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Weapon/WeaponBomb.h"
#include "Weapon/WeaponGun.h"
#include "Weapon/WeaponBase.h"
#include "Components/InputComponent.h"
#include "GameMode/FPS_TESTGAMEGameModeBase.h"
#include "Net/UnrealNetwork.h"
#include "Engine.h"
#include "AnimState/PawnAnimState/PawnAnimStateBase.h"
APlayerCharacterBase::APlayerCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
	GunTrenchNum = 2;	//默认生成2个武器槽位
	GunTrenchArray.SetNum(GunTrenchNum);
	GunTrenchArray[0].SetTrenchName("Weapon_A");
	GunTrenchArray[1].SetTrenchName("Weapon_B");
	bDied = false;
	bAim = false;
	//CurrentStateEnum = PlayerStateEnum::Idle;	//角色状态枚举
	CurrentWeaponAnimStateEnum = PlayerWeaponAnimStateEnum::GunComplete;	//武器动画枚举
	CurrentHandWeaponState = CurrentHandWeaponStateEnum::Hand;	//当前持有武器枚举
	MovementComp = GetCharacterMovement();
	bUseControllerRotationYaw = true;

	Wepone_Hand_name = "HandGun_R";

	SprintSpeed = 600;
	RunSpeed = 450;
	WalkSpeed = 190;
	CrouchSpeed = 190;

	DefaultFOV = 90.0f;
	ZoomedFOV = 60.0f;
	ZoomInterSpeed = 0.5f;


	//PlayerMeshStatic = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMeshStatic"));
	//PlayerMeshStatic->SetupAttachment(GetRootComponent());

	//PawnAnimState = &Idleing;

	//PawnAnimState->PawnAnimState(*this);

	CameraBoomComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoomComp"));
	CameraBoomComp->SetupAttachment(GetRootComponent());
	CameraBoomComp->SocketOffset = FVector(0, 35, 0);
	CameraBoomComp->TargetOffset = FVector(0, 0, 55);
	CameraBoomComp->bUsePawnControlRotation = true;	//允许跟随角色旋转
	CameraBoomComp->bEnableCameraRotationLag = true;


	ThrowWeaponScene = CreateDefaultSubobject<USceneComponent>(TEXT("ThrowWeaponScene"));
	ThrowWeaponScene->SetupAttachment(GetRootComponent());

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(CameraBoomComp);

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));

	GetCapsuleComponent()->SetCollisionResponseToChannel(COLLISION_WEAPON, ECR_Ignore);		//胶囊体忽略武器碰撞

	static ConstructorHelpers::FObjectFinder<UCurveFloat> FindTurnBackCurve(TEXT("/Game/PUBG_Assent/Animation/TurnBackCurve")); //加载TurnBackCurve
		TurnBackCurve = FindTurnBackCurve.Object;

	static ConstructorHelpers::FObjectFinder<UCurveFloat> FindAimSpringCurve(TEXT("/Game/PUBG_Assent/Animation/AimSpringCurve")); //加载AimSpringCurve
		AimSpringCurve = FindAimSpringCurve.Object;

		SetReplicates(true);
		SetReplicateMovement(true);
		GetMesh()->SetIsReplicated(true);
}

void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultWeaponClass)	//创建两把武器
	{
		//AddWeapon_Int_Implementation(GetWorld()->SpawnActor<AWeaponGun>(DefaultWeaponClass, FVector(0, 0, 0), FRotator(0, 0, 0)));

		//AddWeapon_Int_Implementation(GetWorld()->SpawnActor<AWeaponGun>(DefaultWeaponClass, FVector(0, 0, 0), FRotator(0, 0, 0)));

		//GunTrenchArray[0].Weapon->ClonWeapon(GetTransform());	//测试克隆函数
	}

	if (TurnBackCurve)
	{
		FOnTimelineFloat TurnBackTimelineCallBack;
		FOnTimelineEventStatic TurnBackTimelineFinishedCallback;	//绑定TimeLine播放完后调用的函数
		 
		TurnBackTimelineCallBack.BindUFunction(this, TEXT("UpdateControllerRotation"));//第一个调用函数对象 第二个为调用函数名
		TurnBackTimelineFinishedCallback.BindLambda([this]() {bUseControllerRotationYaw = true; });	//结束后执行Lambda表达式 将bUseControllerRotationYaw恢复为true

		TurnBackTimeLine.AddInterpFloat(TurnBackCurve, TurnBackTimelineCallBack);	
		TurnBackTimeLine.SetTimelineFinishedFunc(TurnBackTimelineFinishedCallback);	//设置TimeLine播放完后调用TurnBackTimelineFinishedCallback

	}

	if (AimSpringCurve)
	{
		FOnTimelineFloatStatic AimSpringTimelineCallBack;

		AimSpringTimelineCallBack.BindUFunction(this, TEXT("UpdateSpringLength"));

		AimSpringTimeLine.AddInterpFloat(AimSpringCurve, AimSpringTimelineCallBack);

	}


	AFPS_TESTGAMEGameModeBase * GM = Cast<AFPS_TESTGAMEGameModeBase>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		CurrentGameModeBase = GM;
	}

	HealthComp->OnHealthChanged.AddDynamic(this, &APlayerCharacterBase::OnHealthChanged);	//绑定血量响应事件

}

void APlayerCharacterBase::OnHealthChanged(UHealthComponent * HealthComponent, float Health, float HealthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{
	if (Health <= 0.0f && !bDied)	//检查并执行死亡函数
	{
		PlayDieSet();

		bDied = true;

	}
}
void APlayerCharacterBase::PlayDieSet_Implementation()
{
	GetMovementComponent()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromControllerPendingDestroy();
	//SetLifeSpan(5.0f);  //设置自动销毁时间
	GetMesh()->SetSimulatePhysics(true);

	/*for (int32 index = 0; index < GunTrenchArray.Num(); index++)
	{
	if (GunTrenchArray[index].IsWeapon())
	{
	GunTrenchArray[index].GetWeapon()->SetCurrentMeshCollision(false);
	}
	}*/
}
bool APlayerCharacterBase::PlayDieSet_Validate()
{
	return true;
}

void APlayerCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TurnBackTimeLine.TickTimeline(DeltaTime); //tick中绑定TimeLine
	AimSpringTimeLine.TickTimeline(DeltaTime);

	if (bDied)
	{
		
	}

	/*if (bAim || CurrentHandWeaponState != CurrentHandWeaponStateEnum::Hand)
	{
		bUseControllerRotationYaw = true;
	}
	else
	{
		bUseControllerRotationYaw = false;
	}*/


}
void APlayerCharacterBase::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &APlayerCharacterBase::Jump);

	PlayerInputComponent->BindAction("Reloading", IE_Pressed, this, &APlayerCharacterBase::Reloadiong);

	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &APlayerCharacterBase::SprintPressed);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &APlayerCharacterBase::SprintReleased);

	PlayerInputComponent->BindAction("Freelook", IE_Pressed, this, &APlayerCharacterBase::FreelookPressed);
	PlayerInputComponent->BindAction("Freelook", IE_Released, this, &APlayerCharacterBase::FreelookReleased); 

	PlayerInputComponent->BindAction("Walk", IE_Pressed, this, &APlayerCharacterBase::WalkPressed);
	PlayerInputComponent->BindAction("Walk", IE_Released, this, &APlayerCharacterBase::WalkReleased);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &APlayerCharacterBase::CrouchPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &APlayerCharacterBase::CrouchReleased);

	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &APlayerCharacterBase::AimPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &APlayerCharacterBase::AimReleased);


	PlayerInputComponent->BindAction("Weapon_1", IE_Pressed, this, &APlayerCharacterBase::Weapon_1Pressed);
	PlayerInputComponent->BindAction("Weapon_2", IE_Pressed, this, &APlayerCharacterBase::Weapon_2Pressed);
	PlayerInputComponent->BindAction("Hand", IE_Pressed, this, &APlayerCharacterBase::HandPressed);
	PlayerInputComponent->BindAction("ThrowWeapon", IE_Pressed, this, &APlayerCharacterBase::ThrowWeapon);
}

FVector APlayerCharacterBase::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

FRotator APlayerCharacterBase::GetViewRotation() const
{
	//if (CameraComp)
	{
		//return CameraComp->GetComponentRotation();
	}
	return Super::GetViewRotation();
}

//////////////////////////////////////////////////////////////////////////TimeLine调用函数
void APlayerCharacterBase::UpdateControllerRotation(float Value)
{
	FRotator NewRotation = FMath::Lerp(CurrentContrtolRotation, TargetControlRotation, Value);
	Controller->SetControlRotation(NewRotation);


}

void APlayerCharacterBase::UpdateSpringLength(float Value)	//瞄准倍数
{
	float TargetFOV = bAim ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(CameraComp->FieldOfView, TargetFOV, Value, ZoomInterSpeed);

	CameraComp->SetFieldOfView(NewFOV);

}
//////////////////////////////////////////////////////////////////////////开火控制
void APlayerCharacterBase::AttackOn()
{
	if (CurrentHandWeapon && CurrentWeaponAnimStateEnum == PlayerWeaponAnimStateEnum::GunComplete)
	{
		CurrentHandWeapon->Execute_Fire_Int(CurrentHandWeapon, true, 0.0f);
	}
	
	//bUseControllerRotationYaw = true;
	
}

void APlayerCharacterBase::AttackOff()
{
	if (CurrentHandWeapon && CurrentWeaponAnimStateEnum == PlayerWeaponAnimStateEnum::GunComplete)
	{
		CurrentHandWeapon->Execute_Fire_Int(CurrentHandWeapon, false, 0.0f);
	}

// 	if (!bAim)
// 	{
// 		bUseControllerRotationYaw = false;
// 	}
}

//////////////////////////////////////////////////////////////////////////行走控制
void APlayerCharacterBase::SprintPressed()
{
	if (!bAim)	
	{
		MovementComp->MaxWalkSpeed = SprintSpeed;
	}
	else   
	{
		MovementComp->MaxWalkSpeed = RunSpeed;   //瞄准状态下限制最大速度
	}
}

void APlayerCharacterBase::SprintReleased()
{
	if (!bAim)
	{
		MovementComp->MaxWalkSpeed = RunSpeed;
	}
	else
	{
		MovementComp->MaxWalkSpeed = WalkSpeed;
	}
}

void APlayerCharacterBase::WalkPressed()
{
	MovementComp->MaxWalkSpeed = WalkSpeed;
}

void APlayerCharacterBase::WalkReleased()
{

	MovementComp->MaxWalkSpeed = RunSpeed;

}
void APlayerCharacterBase::CrouchPressed()
{

	Crouch(true);
	MovementComp->MaxWalkSpeed = CrouchSpeed;
}
void APlayerCharacterBase::CrouchReleased()
{

	UnCrouch();
	MovementComp->MaxWalkSpeed = RunSpeed;

}
//////////////////////////////////////////////////////////////////////////视角控制
void APlayerCharacterBase::FreelookPressed()
{
	TargetControlRotation = GetControlRotation();
	bUseControllerRotationYaw = false;
}

void APlayerCharacterBase::FreelookReleased()
{
	CurrentContrtolRotation = GetControlRotation();
	TurnBackTimeLine.PlayFromStart();	//播放TimeLine

}
void APlayerCharacterBase::AimPressed()
{
	if (CurrentHandWeapon)
	{
		ZoomedFOV = CurrentHandWeapon->ZoomedFOV;
	}
	AimSpringTimeLine.PlayFromStart();
	bAim = true;

	if (CurrentHandWeaponState != CurrentHandWeaponStateEnum::Hand)		//如果现在的武器不是拳头则限制走路速度
	{
		MovementComp->MaxWalkSpeed = WalkSpeed;
	}
}

void APlayerCharacterBase::AimReleased()
{
	ZoomedFOV = 60.0f;

	AimSpringTimeLine.PlayFromStart();	//倒叙播放AimSpringTimeLine

	bAim = false;
	
	MovementComp->MaxWalkSpeed = RunSpeed;
	
}

//////////////////////////////////////////////////////////////////////////武器控制
void APlayerCharacterBase::Weapon_1Pressed()
{

	TakeWeapon(CurrentHandWeaponStateEnum::Weapon_1);
	
}
void APlayerCharacterBase::Weapon_2Pressed()
{
	
	TakeWeapon(CurrentHandWeaponStateEnum::Weapon_2);
	
}

void APlayerCharacterBase::HandPressed()
{
	
	TakeWeapon(CurrentHandWeaponStateEnum::Hand);
	
}
void APlayerCharacterBase::Reloadiong()
{
	if (CurrentHandWeapon)
	{
		CurrentHandWeapon->Execute_Fire_Int(CurrentHandWeapon, false, 0.0f);
		CurrentHandWeapon->TryReloading();
	}
}

void APlayerCharacterBase::ThrowWeapon()
{
	if (CurrentHandWeapon)
	{
		//设置碰撞参数，Tag为一个字符串用于以后识别，true是TraceParams.bTraceComplex，this是InIgnoreActor    
		FCollisionQueryParams TraceParams(FName(TEXT("TraceUsableActor")), true, this);
		TraceParams.bTraceAsyncScene = true;
		TraceParams.bReturnPhysicalMaterial = false;    //使用复杂Collision判定，逐面判定，更加准确        
		TraceParams.bTraceComplex = true;    /* FHitResults负责接受结果 */

		FHitResult Hit(ForceInit);
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), ThrowWeaponScene->GetComponentLocation(), ECollisionChannel::ECC_Camera,TraceParams))
		{
			CurrentHandWeapon->ClonWeapon(FTransform(Hit.Location));
		}
		else
		{
			CurrentHandWeapon->ClonWeapon(FTransform(ThrowWeaponScene->GetComponentLocation()));
		}
		CurrentHandWeapon->Destroy(true);
		CurrentHandWeapon = nullptr;
		TakeWeapon(CurrentHandWeaponStateEnum::Hand);
	}

}



void APlayerCharacterBase::TakeWeapon_Implementation(CurrentHandWeaponStateEnum HandWeaponEnum)
{
	if (CurrentWeaponAnimStateEnum == PlayerWeaponAnimStateEnum::GunComplete)
	{
		switch (HandWeaponEnum)
		{
		case CurrentHandWeaponStateEnum::Weapon_1:
			if (GunTrenchArray[0].IsWeapon())
			{
				CurrentHandWeaponState = HandWeaponEnum;
				CurrentWeaponAnimStateEnum = PlayerWeaponAnimStateEnum::Take_Gun;
			}
			break;
		case CurrentHandWeaponStateEnum::Weapon_2:
			if (GunTrenchArray[1].IsWeapon())
			{
				CurrentHandWeaponState = HandWeaponEnum;
				CurrentWeaponAnimStateEnum = PlayerWeaponAnimStateEnum::Take_Gun;
			}
			break;
		case CurrentHandWeaponStateEnum::Hand:
			if (CurrentHandWeaponState != CurrentHandWeaponStateEnum::Hand)
			{
				CurrentHandWeaponState = HandWeaponEnum;
				CurrentWeaponAnimStateEnum = PlayerWeaponAnimStateEnum::Down_Gun;
			}
			break;
		default:
			break;
		}
	}
	UpdateWeapon();
}
bool APlayerCharacterBase::TakeWeapon_Validate(CurrentHandWeaponStateEnum HandWeaponEnum)
{
	return true;
}

void APlayerCharacterBase::UpdateWeapon()
{
	switch (CurrentHandWeaponState)
	{
	case CurrentHandWeaponStateEnum::Weapon_1:
		GetGunWeapon(0);
		break;
	case CurrentHandWeaponStateEnum::Weapon_2:

		GetGunWeapon(1);
		break;
	case CurrentHandWeaponStateEnum::Hand:
	
		AddWeapon_Int_Implementation(CurrentHandWeapon);
		CurrentHandWeapon = nullptr;
		
		break;
	default:
		break;
	}
	CurrentWeaponAnimStateEnum = PlayerWeaponAnimStateEnum::GunComplete;

}


bool APlayerCharacterBase::AddWeapon_Int_Implementation(AWeaponBase * Gun)
{
	if (Gun == nullptr)
	{
		return false;
	}
	
	for (int32 index = 0; index < GunTrenchArray.Num(); index++)
	{
		if (!GunTrenchArray[index].IsWeapon())
		{
			Gun->Execute_Fire_Int(Gun, false, 0.0f);
			Gun->SetOwner(nullptr);
			Gun->SetCurrentMeshCollision(false);
			Gun->WeaponHitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			GunTrenchArray[index].SetWeapon(GetMesh(), Gun);

			if (ChangeWeaponSound)	//播放武器更换音效
			{
				UGameplayStatics::SpawnSoundAttached(ChangeWeaponSound, GetMesh());
			}
			return true;
		}
	}
	return false;
}

AWeaponBase * APlayerCharacterBase::GetGunWeapon(int32 TrenchID)
{
	if (CurrentHandWeapon)
	{
		if (!AddWeapon_Int_Implementation(CurrentHandWeapon))
		{
			ThrowWeapon();
		}
	}

	if (GunTrenchArray.IsValidIndex(TrenchID))
	{
		CurrentHandWeapon = GunTrenchArray[TrenchID].GetWeapon();
	}

	if (CurrentHandWeapon)
	{
		CurrentHandWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules(EAttachmentRule::KeepRelative, true), Wepone_Hand_name);
		CurrentHandWeapon->SetOwner(this);
		AimReleased();
		return CurrentHandWeapon;
	}
	
	return nullptr;
}

void APlayerCharacterBase::RecoilRifleFire_Implementation(FVector2D RandomRecoilPith, FVector2D RandomRecoilYaw)
{
}

void APlayerCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const	//成员复制
{

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APlayerCharacterBase, CurrentHandWeapon);
	DOREPLIFETIME(APlayerCharacterBase, CurrentWeaponAnimStateEnum);
	DOREPLIFETIME(APlayerCharacterBase, CurrentHandWeaponState);
	DOREPLIFETIME(APlayerCharacterBase, bDied);
}
