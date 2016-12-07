/*
Copyright 2016 Krzysztof Lis

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "AugmentedUnreality.h"
#include "AURMarkerComponentBase.h"

const float UAURMarkerComponentBase::MARKER_DEFAULT_SIZE = 10.0;
const float UAURMarkerComponentBase::MARKER_TEXT_RELATIVE_SCALE = 0.5;

const int32 MATERIAL_INDEX = 0;

const FName TEXTURE_PARAM_NAME = FName("ContentTexture");
const int32 TEXTURE_SIZE = 128;
const FMatrix MATRIX_VERTICAL_MIRROR = FRotationTranslationMatrix(
	FRotator(0, 180, 0),
	FVector(TEXTURE_SIZE / 2, TEXTURE_SIZE / 2, 0)
);


/**
The order in ArUco is top-left, top-right, bottom-right, bottom-left,
but since OpenCV's vectors have different handedness, we need to have different order here.
The change swaps X with Y, so top-left with bottom-right:
bottom-right, top-right, top-left, bottom-left
**/
const std::vector<FVector> UAURMarkerComponentBase::LOCAL_CORNERS{
	FVector(0.5, -0.5, 0.0),
	FVector(0.5, 0.5, 0.0), 
	FVector(-0.5, 0.5, 0.0),
	FVector(-0.5, -0.5, 0.0) 
};

UAURMarkerComponentBase::UAURMarkerComponentBase()
	: Id(1)
	, BoardSizeCm(MARKER_DEFAULT_SIZE)
	, MarginCm(1.0)
	, MarkerIdFontColor(0.16, 0.5, 1.0, 1.0)
	, MarkerCenterColor(0.35, 0.35, 0.35, 0.6)
	, MarkerBackgroundColor(1.0, 1.0, 1.0, 0.6)
	, SurfaceDynamicMaterial(nullptr)
	, SurfaceDynamicTexture(nullptr)
{
	bWantsInitializeComponent = true;

	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UAURMarkerComponentBase::InitializeComponent()
{
	Super::InitializeComponent();

	//UE_LOG(LogAUR, Log, TEXT("UAURMarkerComponentBase::InitializeComponent"));
	//InitDynamicCanvas();
	//RedrawSurface();
}

void UAURMarkerComponentBase::InitDynamicCanvas()
{
	if (GetWorld() == nullptr)
	{
		UE_LOG(LogAUR, Error, TEXT("UAURMarkerComponentBase: Trying to init canvas but GetWorld is null"));
		return;
	}

	if (SurfaceDynamicMaterial == nullptr)
	{
		UMaterialInterface* material = this->GetMaterial(MATERIAL_INDEX);
		UTexture* texture_param_value = nullptr;
		if (material->GetTextureParameterValue(TEXTURE_PARAM_NAME, texture_param_value))
		{
			UMaterialInstanceDynamic* dynamic_material_instance = Cast<UMaterialInstanceDynamic>(material);
			if (!dynamic_material_instance)
			{
				dynamic_material_instance = UMaterialInstanceDynamic::Create(material, this);
				this->SetMaterial(MATERIAL_INDEX, dynamic_material_instance);
			}

			SurfaceDynamicMaterial = dynamic_material_instance;
		}
	}

	if (SurfaceDynamicMaterial && SurfaceDynamicTexture == nullptr)
	{
		SurfaceDynamicTexture = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this->GetWorld(), UCanvasRenderTarget2D::StaticClass(), 128, 128);

		if (!SurfaceDynamicMaterial)
		{
			UE_LOG(LogAUR, Error, TEXT("UAURMarkerComponentBase: CreateCanvasRenderTarget2D returned NULL"));
			return;
		}
		SurfaceDynamicTexture->OnCanvasRenderTargetUpdate.Clear();
		SurfaceDynamicTexture->OnCanvasRenderTargetUpdate.AddUniqueDynamic(this, &UAURMarkerComponentBase::OnTexturePaint);

		SurfaceDynamicMaterial->SetTextureParameterValue(TEXTURE_PARAM_NAME, SurfaceDynamicTexture);
	}
}

void UAURMarkerComponentBase::RedrawSurface()
{
	if (!SurfaceDynamicTexture)
	{
		InitDynamicCanvas();
	}

	if (SurfaceDynamicTexture)
	{
		SurfaceDynamicTexture->UpdateResource();
	}
	else
	{
		UE_LOG(LogAUR, Error, TEXT("UAURMarkerComponentBase: Dynamic texture not initialized"));
	}
}

void UAURMarkerComponentBase::SetId(int32 new_id)
{
	Id = new_id;
	//RedrawSurface();
}

void UAURMarkerComponentBase::SetBoardSize(float new_board_size)
{
	BoardSizeCm = new_board_size;

	const float scale = BoardSizeCm / MARKER_DEFAULT_SIZE;
	this->SetWorldScale3D(FVector(scale, scale, scale));
}

void UAURMarkerComponentBase::OnTexturePaint(UCanvas* CanvasWrapper, int32 Width, int32 Height)
{
	FCanvas* const canvas = CanvasWrapper->Canvas;
	const FVector2D canvas_size(Width, Height);
	const FVector2D center = canvas_size * 0.5;

	// whole background
	canvas->Clear(MarkerBackgroundColor);

	// center (inside margin)
	const FVector2D margin_offset = canvas_size * (MarginCm / BoardSizeCm);
	FCanvasTileItem TileItem(
		margin_offset, 
		GWhiteTexture, 
		canvas_size - margin_offset*2, 
		MarkerCenterColor
	);
	//TileItem.BlendMode = SE_BLEND_Translucent;
	canvas->DrawItem(TileItem);

	// marker ID
	canvas->PushRelativeTransform(MATRIX_VERTICAL_MIRROR);

	FCanvasTextItem text_item(
		FVector2D::ZeroVector, //center,
		FText::Format(NSLOCTEXT("AUR", "MarkerEditorText", "{0}"), FText::AsNumber(Id)),
		MarkerIdFontInfo,
		MarkerIdFontColor
	);
	text_item.bCentreX = true;
	text_item.bCentreY = true;
	//text_item.
	canvas->DrawItem(text_item);

}

FMarkerDefinitionData UAURMarkerComponentBase::GetDefinition() const
{
	//	UE_LOG(LogAUR, Log, TEXT("Marker %d (%s)"), Id, *this->GetName());

	// Determine the transform from board actor to this component
	FTransform transform_this_to_actor = this->GetRelativeTransform();

	//TArray<USceneComponent*> parent_components;
	//this->GetParentComponents(parent_components);

	USceneComponent* root_component = GetAttachmentRoot();
	USceneComponent* parent_comp = GetAttachParent();

	while (parent_comp && parent_comp != root_component)
	{
		// Runtime/Core/Public/Math/Transform.h:
		// C = A * B will yield a transform C that logically first applies A then B to any subsequent transformation.
		// (we want the parent-most transform done first)
		transform_this_to_actor *= parent_comp->GetRelativeTransform();
		parent_comp = parent_comp->GetAttachParent();
	}

	// Transform the corners
	FMarkerDefinitionData def_data(Id);

	// Don't multiply by this marker's size becuase the transform's scale will do it.
	// But calculate what part of the board is taken by the margin
	const float pattern_size_relative = (BoardSizeCm - 2 * MarginCm) * MARKER_DEFAULT_SIZE/BoardSizeCm;

	for (int idx = 0; idx < 4; idx++)
	{
		def_data.Corners[idx] = transform_this_to_actor.TransformPosition(LOCAL_CORNERS[idx] * pattern_size_relative);
	}

	return def_data;
}


#if WITH_EDITOR
void UAURMarkerComponentBase::PostEditChangeProperty(FPropertyChangedEvent & property_change_event)
{
	Super::PostEditChangeProperty(property_change_event);

	const FName property_name = (property_change_event.Property != nullptr) ? property_change_event.Property->GetFName() : NAME_None;
	
	//UE_LOG(LogAUR, Warning, TEXT("UAURMarkerComponentBase::PostEditChangeProperty %s"), *property_name.ToString());
	if (property_name == GET_MEMBER_NAME_CHECKED(UAURMarkerComponentBase, BoardSizeCm))
	{
		SetBoardSize(BoardSizeCm);
	}
/*	
	else if (property_name == GET_MEMBER_NAME_CHECKED(UAURMarkerComponentBase, Id))
	{
		SetId(Id);
	}
	else if (property_name == GET_MEMBER_NAME_CHECKED(UAURMarkerComponentBase, MarginCm))
	{
		RedrawSurface();
	}
*/
}
#endif

