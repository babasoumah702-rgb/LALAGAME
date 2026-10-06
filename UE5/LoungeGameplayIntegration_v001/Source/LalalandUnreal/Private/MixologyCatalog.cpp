#include "MixologyCatalog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace {
 FString Label(const TSharedPtr<FJsonObject>& O,const FString& Lang){auto N=O->GetObjectField(TEXT("name"));FString Result;if(!N->TryGetStringField(Lang,Result))Result=N->GetStringField(TEXT("en"));return Result;}
 void Profile(FMixRecipe& R,const TSharedPtr<FJsonObject>& P){double V; if(P->TryGetNumberField(TEXT("sweetness"),V))R.Sweet=V;if(P->TryGetNumberField(TEXT("sourness"),V))R.Sour=V;if(P->TryGetNumberField(TEXT("bitterness"),V))R.Bitter=V;if(P->TryGetNumberField(TEXT("alcoholFeel"),V))R.Spirit=V;}
 void Ingredients(FMixRecipe& R,const TArray<TSharedPtr<FJsonValue>>& A){for(const auto& V:A){auto I=V->AsObject();R.Amounts.Add(I->GetStringField(TEXT("id")),I->GetIntegerField(TEXT("amount")));}}
}
bool FMixCatalog::Load(FString& Error,const FString& InLanguage){
 Language=InLanguage==TEXT("zh-CN")||InLanguage==TEXT("ko")?InLanguage:TEXT("en");
 Materials.Reset();Recipes.Reset();FString Text;TSharedPtr<FJsonObject> Root;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Data/drink_catalog_v0.3.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root.IsValid()){Error=Language==TEXT("zh-CN")?TEXT("酒单文件缺失或格式无效"):TEXT("Catalog missing or invalid");return false;}
 if(Root->GetStringField(TEXT("schemaVersion"))!=TEXT("lalaland-drink-catalog-v0.3")){Error=TEXT("Unexpected catalog version");return false;}
 for(auto V:Root->GetArrayField(TEXT("materials"))){auto O=V->AsObject();FMixMaterial M;M.Id=O->GetStringField(TEXT("id"));M.Name=Label(O,Language);M.Cost=O->GetNumberField(TEXT("cashPerRecipePortion"));M.Alcoholic=O->GetBoolField(TEXT("alcoholic"));if(M.Id==TEXT("mint"))M.Unit=TEXT("leaf");Materials.Add(M);}
 TMap<FString,TSharedPtr<FJsonObject>> Families;for(auto V:Root->GetArrayField(TEXT("families"))){auto F=V->AsObject();Families.Add(F->GetStringField(TEXT("id")),F);}
 for(auto V:Root->GetArrayField(TEXT("outcomes"))){auto O=V->AsObject();auto F=Families.FindRef(O->GetStringField(TEXT("familyId")));if(!F){Error=TEXT("Unknown family");return false;}FMixRecipe R;R.Id=O->GetStringField(TEXT("id"));R.Name=Label(F,Language)+TEXT(" / ")+Root->GetObjectField(TEXT("ui"))->GetObjectField(O->GetStringField(TEXT("option")))->GetStringField(Language);R.Kind=TEXT("normal");R.CraftCost=O->GetIntegerField(TEXT("craftCash"));R.OrderCost=O->GetIntegerField(TEXT("orderCash"));Ingredients(R,F->GetArrayField(TEXT("fixedIngredients")));int32 Syrup=O->GetIntegerField(TEXT("syrupMl"));if(Syrup>0)R.Amounts.Add(TEXT("simple_syrup"),Syrup);Profile(R,F->GetObjectField(TEXT("profile")));Profile(R,O->GetObjectField(TEXT("profileOverrides")));Recipes.Add(R);}
 for(const FString Key:{TEXT("craftOnlySpecials"),TEXT("craftOnlyExperiments")})for(auto V:Root->GetArrayField(Key)){auto O=V->AsObject();FMixRecipe R;R.Id=O->GetStringField(TEXT("id"));R.Name=Label(O,Language);R.Kind=Key==TEXT("craftOnlySpecials")?TEXT("special"):TEXT("failed");R.CraftCost=O->GetIntegerField(TEXT("craftCash"));Ingredients(R,O->GetArrayField(TEXT("ingredients")));Profile(R,O->GetObjectField(TEXT("profile")));Recipes.Add(R);}
 for(auto& R:Recipes){
  if(R.Kind==TEXT("normal")){for(auto V:Root->GetArrayField(TEXT("outcomes"))){auto O=V->AsObject();if(O->GetStringField(TEXT("id"))==R.Id){R.Method=Families.FindRef(O->GetStringField(TEXT("familyId")))->GetStringField(TEXT("method"));break;}}}
  else for(auto V:Root->GetArrayField(R.Kind==TEXT("special")?TEXT("craftOnlySpecials"):TEXT("craftOnlyExperiments"))){auto O=V->AsObject();if(O->GetStringField(TEXT("id"))==R.Id){R.Method=O->GetStringField(TEXT("method"));break;}}
 }
 Error.Empty();return true;
}
const FMixRecipe* FMixCatalog::Resolve(const TMap<FString,int32>& A)const{
 for(const auto& R:Recipes){bool Equal=true;int32 Count=0;for(const auto& I:A){if(I.Value<0){Equal=false;break;}if(I.Value==0)continue;Count++;const int32* V=R.Amounts.Find(I.Key);if(!V||*V!=I.Value){Equal=false;break;}}if(Equal&&Count==R.Amounts.Num())return &R;}return nullptr;
}
int32 FMixCatalog::Quote(const TMap<FString,int32>& A)const{float Cost=1;for(const auto& I:A)if(I.Value>0){const auto* M=Materials.FindByPredicate([&](const auto& V){return V.Id==I.Key;});if(!M)return -1;Cost+=M->Cost;}return FMath::CeilToInt(Cost);}
