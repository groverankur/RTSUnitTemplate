// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassCommonTypes.h"
#include "MassEntityQuery.h"
#include "ZeroStateTagRecoveryProcessor.generated.h"

struct FMassEntityManager;
struct FMassAIStateFragment;
struct FMassVelocityFragment;

/**
 * Backstop that guarantees no Mass unit persists with ZERO state tags.
 *
 * A unit with no FMassState* tag is invisible to every state processor (each requires its own tag with
 * EMassFragmentPresence::All) and its animation freezes on the last pose. This happens on the CLIENT
 * because Chase/Run/Attack have no replicated tag bit (UnitTagBits budget is exhausted), so when the
 * server enters one of those, ApplyReplicatedTagBits only removes the prior state tag and adds none.
 *
 * This processor matches EXACTLY the archetype that has NONE of the tags UnitClientTagSyncProcessor::
 * ComputeState maps to a real (non-None) state - i.e. the freeze condition - and re-injects a neutral
 * Idle (or Run when moving) so the entity re-enters the pipeline. FMassStateDeadTag is in the exclusion
 * set, so corpses are never recovered/resurrected. It runs in PostPhysics (after the PrePhysics
 * replication apply flushed) with a spawn-age gate, so it never fights a one-frame deferred tag swap.
 */
UCLASS()
class RTSUNITTEMPLATE_API UZeroStateTagRecoveryProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZeroStateTagRecoveryProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	/** Throttle (s) between recovery passes. */
	UPROPERTY(EditAnywhere, Category = RTSUnitTemplate)
	float ExecutionInterval = 0.2f;

	/** Minimum entity age (s) before recovery may act, to skip the spawn/init deferred-tag window. */
	UPROPERTY(EditAnywhere, Category = RTSUnitTemplate)
	float MinAgeSeconds = 1.0f;

	/** Speed^2 ((cm/s)^2) above which a recovered unit is given Run instead of Idle. */
	UPROPERTY(EditAnywhere, Category = RTSUnitTemplate)
	float MovingSpeedSq = 625.f; // (25 cm/s)^2

private:
	FMassEntityQuery EntityQuery;
	float TimeSinceLastRun = 0.f;
};
