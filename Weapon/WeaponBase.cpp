
#include "WeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Interface/AddWeaponInterface/I_AddWeapon.h"
#include "Engine.h"

AWeaponBase::AWeaponBase()
{
	AttackHP_Value = 25.f;
	AttackHP_Multiple = 2.0f;
	AttackTimeInterval = 1.5f;
	PrimaryActorTick.bCanEverTick = true;
	TrenchName = "NULL";
	MaxReserveBullet = 120;
	MaxCurrentBullet = 30;
	WeaponTime = 2.0f;
	WeaponHitTime = 1.5f;
	ReloadingTime = 2.4f;
	ZoomedFOV = 60.0f;
	isHit = false;
	isAttackFire = false;
	isReloading = false;
	ThisWeaponSpeciesEnum = WeaponSpeciesEnum::SK_KA74U;	//当前武器的种类


	//WeaponRootScene = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRootScene"));

	//WeaponStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponStaticMesh"));
	//WeaponStaticMesh->SetSimulatePhysics(true);
	//WeaponStaticMesh->SetupAttachment(GetRootComponent());

	WeaponHitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("WeaponHitSphere"));
	WeaponHitSphere->SetSphereRadius(55.0f);
	WeaponHitSphere->SetupAttachment(GetRootComponent());
	WeaponHitSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponHitSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponHitSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::BeginHit);

	SetReplicates(true);

	//WeaponBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponBox"));
	//WeaponBox->SetSimulatePhysics(true);

}

float AWeaponBase::GetAttackHP() const
{
	return AttackHP_Value;
}

void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	ReserveBullet = MaxReserveBullet;
	CurrentBullet = MaxCurrentBullet;
	//this->SetActorRotation(FRotator(-90, 0, 0));


	//设置碰撞参数，Tag为一个字符串用于以后识别，true是TraceParams.bTraceComplex，this是InIgnoreActor    
	FCollisionQueryParams TraceParams(FName(TEXT("TraceUsableActor")), true, this);    
	TraceParams.bTraceAsyncScene = true;    
	TraceParams.bReturnPhysicalMaterial = false;    //使用复杂Collision判定，逐面判定，更加准确        
	TraceParams.bTraceComplex = true;    /* FHitResults负责接受结果 */ 
	TraceParams.AddIgnoredActor(this);

	FHitResult Hit(ForceInit);
	if (GetWorld()->LineTraceSingleByChannel(Hit,GetActorLocation(), FVector(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z - 1000), ECollisionChannel::ECC_Camera, TraceParams))
	{
		//SetActorLocation(FVector(Hit.Location.X, Hit.Location.Y, Hit.Location.Z+2));
	}
	SetCurrentMeshCollision(true);
}

void AWeaponBase::SetCurrentMeshCollision(bool bCollision)
{
}

void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	WeaponTime += DeltaTime;
	if (WeaponTime >= WeaponHitTime && WeaponTime <= WeaponHitTime + DeltaTime)
	{
		isHit = true;
	}

}

bool AWeaponBase::Fire_Int_Implementation(bool isFire, float Time)
{

	if (isFire)
	{
		if (IsCurrentBullet())
		{
			isAttackFire = true;
			
		}
	}
	else 
	{
		isAttackFire = false;
	}
	return false;
}

void AWeaponBase::BeginHit(UPrimitiveComponent * OverlappedComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (isHit)
	{
		II_AddWeapon * AddWeaponInter = Cast<II_AddWeapon>(OtherActor);
		if (AddWeaponInter)
		{
			SetCurrentMeshCollision(false);
			if (AddWeaponInter->Execute_AddWeapon_Int(OtherActor, this))
			{
				isHit = false;
			}
			else
			{
				SetCurrentMeshCollision(true);

			}
		}
	}

}

bool AWeaponBase::SetWeaponBullet(int32 Currentbullet, int32 Reservebullet)
{
	if (Currentbullet >= 0 && Currentbullet <= MaxCurrentBullet && Reservebullet >= 0 && Reservebullet <= MaxReserveBullet)
	{
		CurrentBullet = Currentbullet;
		ReserveBullet = Reservebullet;
		return true;
	}

	return false;
}

void AWeaponBase::AddWeaponBullet(int32 Reservebullet)
{
	if (Reservebullet >= 0)
	{
		int32 addNum = Reservebullet + ReserveBullet;
		if (addNum > MaxReserveBullet)
		{
			ReserveBullet = MaxReserveBullet;
		}
		else
		{
			ReserveBullet = addNum;
		}
	}
}
bool AWeaponBase::Reload()
{
	return false;
}

void AWeaponBase::OnAttack()
{
	OnAttackEvent(FVector(0,0,0),FRotator(0,0,0));
}

void AWeaponBase::OffAttack()
{
}


AWeaponBase * AWeaponBase::ClonWeapon(FTransform ClonWeaponTransform)
{
	AWeaponBase * newWeapon = GetWorld()->SpawnActor<AWeaponBase>(GetClass(), ClonWeaponTransform);
	if (newWeapon)
	{
		newWeapon->SetWeaponBullet(CurrentBullet, ReserveBullet);
		return newWeapon;
	}
	return nullptr;
}

bool AWeaponBase::IsReserveBullet()
{
	if (ReserveBullet > 0)
	{
		return true;
	}
	return false;
}

bool AWeaponBase::IsCurrentBullet()
{
	if (CurrentBullet > 0)
	{
		return true;
	}
	return false;
}

void AWeaponBase::TryReloading()	//尝试延时调用换弹逻辑
{
	if (CurrentBullet == MaxCurrentBullet || ReserveBullet <= 0 || isReloading)
	{
		return;
	}
	isReloading = true;
	GetWorldTimerManager().SetTimer(TimeHandle_Reloading, this, &AWeaponBase::Reloading, ReloadingTime);

	ReloadingEvent(WeaponHitSphere);	//换弹事件在蓝图中用于播放声音等内容
}

void AWeaponBase::Reloading()
{
	if (ReserveBullet <= 0)
	{
		return;
	}

	int32 A = MaxCurrentBullet - CurrentBullet;

	if (A >= ReserveBullet)
	{
		CurrentBullet += ReserveBullet;
		ReserveBullet = 0;
	}
	else
	{
		CurrentBullet += A;
		ReserveBullet -= A;
	}

	isReloading = false;
	GetWorldTimerManager().ClearTimer(TimeHandle_Reloading);
}
