#include "MixologyPresentation.h"

FTransform MixologyPresentation::PourTransform(const FVector& Home,const FVector& Mouth,const FVector& Scale,float LocalOpening,float Blend){
 const float T=FMath::Clamp(Blend,0.f,1.f);
 const FQuat Rotation=FRotationMatrix::MakeFromZ(FVector(.20f,.86f,-.47f).GetSafeNormal()).ToQuat();
 const FVector End=Mouth-Rotation.RotateVector(FVector(0,0,LocalOpening)*Scale);
 return FTransform(FQuat::Slerp(FQuat::Identity,Rotation,T),FMath::Lerp(Home,End,T),Scale);
}

int32 MixologyPresentation::Volume(const FMixCatalog& C,const TMap<FString,int32>& A){
 int32 Ml=0;for(const auto& M:C.Materials)if(M.Unit==TEXT("ml")&&M.Id!=TEXT("ice")&&M.Id!=TEXT("citrus_garnish"))Ml+=FMath::Max(0,A.FindRef(M.Id));return Ml;
}
FLinearColor MixologyPresentation::LiquidColor(const FMixCatalog& C,const TMap<FString,int32>& A){
 FLinearColor Sum(0,0,0,0);float Total=0;
 for(const auto& M:C.Materials){
  if(M.Unit!=TEXT("ml")||M.Id==TEXT("ice")||M.Id==TEXT("citrus_garnish"))continue;
  float Amount=FMath::Max(0,A.FindRef(M.Id));if(!Amount)continue;
  FLinearColor Color(.70f,.87f,.84f,1); // Clear spirit/mixer: stylized, not physical absorption.
  if(M.Id==TEXT("bourbon"))Color=FLinearColor(.62f,.24f,.045f,1);
  else if(M.Id==TEXT("red_bitter_aperitif"))Color=FLinearColor(.70f,.025f,.018f,1);
  else if(M.Id==TEXT("sweet_red_vermouth"))Color=FLinearColor(.36f,.08f,.028f,1);
  else if(M.Id==TEXT("lemon_juice"))Color=FLinearColor(.94f,.82f,.26f,1);
  else if(M.Id==TEXT("lime_juice"))Color=FLinearColor(.72f,.83f,.38f,1);
  else if(M.Id==TEXT("cola"))Color=FLinearColor(.12f,.045f,.012f,1);
  else if(M.Id==TEXT("oolong_tea"))Color=FLinearColor(.48f,.22f,.055f,1);
  else if(M.Id==TEXT("cranberry_drink"))Color=FLinearColor(.62f,.018f,.07f,1);
  // Increase pigment influence so a colored ingredient is legible at tabletop scale.
  // Deliberately stylized: these weights are not real optical absorption values.
  float Pigment=M.Id==TEXT("cranberry_drink")||M.Id==TEXT("red_bitter_aperitif")?6.f:
   M.Id==TEXT("cola")||M.Id==TEXT("sweet_red_vermouth")?5.f:
   M.Id==TEXT("bourbon")||M.Id==TEXT("oolong_tea")?3.f:1.f;
  Sum+=Color*(Amount*Pigment);Total+=Amount*Pigment;
 }
 if(Total<=0)return FLinearColor(.70f,.87f,.84f,1);
 FLinearColor Result=Sum/Total;Result.A=1;return Result;
}
FString MixologyPresentation::MethodLabel(const FString& M){
 if(M.Contains(TEXT("shake")))return TEXT("SHAKE");
 if(M.Contains(TEXT("stir")))return TEXT("STIR");
 if(M.Contains(TEXT("muddle")))return TEXT("MUDDLE / BUILD");
 if(M.Contains(TEXT("build")))return TEXT("BUILD IN GLASS");
 return TEXT("MIX");
}
float MixologyPresentation::LiquidOpacity(const FMixCatalog& C,const TMap<FString,int32>& A){
 const int32 Ml=Volume(C,A);if(Ml<=0)return 0;
 const float Citrus=FMath::Max(0,A.FindRef(TEXT("lemon_juice")))+FMath::Max(0,A.FindRef(TEXT("lime_juice")));
 const float Red=FMath::Max(0,A.FindRef(TEXT("cranberry_drink")))+FMath::Max(0,A.FindRef(TEXT("red_bitter_aperitif")));
 const float Dark=FMath::Max(0,A.FindRef(TEXT("cola")))+FMath::Max(0,A.FindRef(TEXT("sweet_red_vermouth")));
 return FMath::Clamp(.16f+(Citrus*1.15f+Red*.22f+Dark*.36f)/Ml,.16f,.72f);
}
float MixologyPresentation::LiquidHeight(int32 Ml){return FMath::Clamp(FMath::Max(0,Ml)/32.6f,0.f,11.2f);}
