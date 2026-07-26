// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.
#include "Mass/States/ZeroStateTagRecoveryProcessor.h"

#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"                    // FTransformFragment (not required, kept for parity)
#include "MassMovementFragments.h"                  // FMassVelocityFragment
#include "Mass/UnitMassTag.h"                       // FMassAIStateFragment + all FMassState* tags
#include "Mass/Replication/ReplicationSettings.h"   // RTSReplicationSettings

UZeroStateTagRecoveryProcessor::UZeroStateTagRecoveryProcessor(): EntityQuery()
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;          // client (primary) + server + standalone
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;            // after the PrePhysics replication apply flushed
	bAutoRegisterWithProcessingPhases = true;
	bRequiresGameThreadExecution = false;
}

void UZeroStateTagRecoveryProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FMassAIStateFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);

	// Not-yet-initialized / transient exclusions.
	EntityQuery.AddTagRequirement<FMassStateFrozenTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateNeedsInitialKickTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassIsEffectAreaTag>(EMassFragmentPresence::None);

	// Match ONLY the freeze archetype: NONE of the tags ComputeState maps to a real (non-None) state.
	// FMassStateDeadTag is included -> corpses are never recovered/resurrected.
	EntityQuery.AddTagRequirement<FMassStateDeadTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateChaseTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateRunTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateAttackTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStatePauseTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateIdleTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStatePatrolRandomTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStatePatrolIdleTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStatePatrolTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateCastingTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateChargingTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateIsAttackedTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateRootedTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateEvasionTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateContinuousAttackTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassRotateToMouseTag>(EMassFragmentPresence::None);        // ComputeState -> Aim
	EntityQuery.AddTagRequirement<FRunAnimationTag>(EMassFragmentPresence::None);             // ComputeState -> AnimationState
	EntityQuery.AddTagRequirement<FMassStateBuildTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateRepairTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateResourceExtractionTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateGoToBaseTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateGoToBuildTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateGoToRepairTag>(EMassFragmentPresence::None);
	EntityQuery.AddTagRequirement<FMassStateGoToResourceExtractionTag>(EMassFragmentPresence::None);

	EntityQuery.RegisterWithProcessor(*this);
}

void UZeroStateTagRecoveryProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Only relevant to the Mass replication path (mirrors UnitClientTagSyncProcessor / ClientReplicationProcessor).
	if (RTSReplicationSettings::GetReplicationMode() != RTSReplicationSettings::Mass)
	{
		return;
	}

	TimeSinceLastRun += Context.GetDeltaTimeSeconds();
	if (TimeSinceLastRun < ExecutionInterval)
	{
		return;
	}
	TimeSinceLastRun = 0.f;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	const float MinAge = MinAgeSeconds;
	const float MoveSq = MovingSpeedSq;

	EntityQuery.ForEachEntityChunk(Context,
		[Now, MinAge, MoveSq](FMassExecutionContext& ChunkContext)
	{
		const int32 Num = ChunkContext.GetNumEntities();
		const TConstArrayView<FMassAIStateFragment> States = ChunkContext.GetFragmentView<FMassAIStateFragment>();
		const TConstArrayView<FMassVelocityFragment> Vels = ChunkContext.GetFragmentView<FMassVelocityFragment>();
		const bool bHasVel = (Vels.Num() == Num);

		for (int32 i = 0; i < Num; ++i)
		{
			// Skip the spawn/init window so we never race a one-frame deferred tag swap.
			if (Now - States[i].BirthTime < MinAge)
			{
				continue;
			}

			const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
			const bool bMoving = bHasVel && (Vels[i].Value.SizeSquared() > MoveSq);
			if (bMoving)
			{
				ChunkContext.Defer().AddTag<FMassStateRunTag>(Entity);
			}
			else
			{
				ChunkContext.Defer().AddTag<FMassStateIdleTag>(Entity);
			}
		}
	});
}
