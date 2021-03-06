// This code was written by 康子秋

#include "Zombie_Base.h"
#include "Components/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine.h"

AZombie_Base::AZombie_Base()
{
	PrimaryActorTick.bCanEverTick = true;

	HealthComp = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &AZombie_Base::OnHealthChanged);	//绑定血量响应事件
	BloodScene = CreateDefaultSubobject<USceneComponent>(TEXT("BloodScene"));
	BloodScene->SetupAttachment(GetMesh());
	AutoDestoryTime = 20.0f;
	DefaultAttack_HP = 20.0f;
	BloodDecalSize = FVector(50, 100, 100);
}

void AZombie_Base::BeginPlay()
{
	Super::BeginPlay();
	
	

}

void AZombie_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AZombie_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AZombie_Base::OnHealthChanged(UHealthComponent * HealthComponent, float Health, float HealthDelta, const UDamageType * DamageType, AController * InstigatedBy, AActor * DamageCauser)
{

	UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BloodImpactDecal, this);	//得到印花材质并创建动态材质
	if (DynMaterial)
	{
		FVector DecalSize = BloodDecalSize * FMath::FRandRange(0.75, 1.9);

		FRotator DecalDirection = FRotator(-90,0 , FMath::FRandRange(-180, 180));
		FVector DecalLocation = FVector(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z);
		UGameplayStatics::SpawnDecalAtLocation(GetWorld(), DynMaterial, DecalSize, BloodScene->GetComponentLocation(), DecalDirection,35.f);	//创建印花
	}
	if (Health <= 0.0f)
	{
		DieSet();
	}
}

void AZombie_Base::DieSet_Implementation()
{
	GetMovementComponent()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DetachFromControllerPendingDestroy();
	GetMesh()->SetSimulatePhysics(true);

	SetLifeSpan(AutoDestoryTime);  //设置自动销毁时间
}
bool AZombie_Base::DieSet_Validate()
{
	return true;
}