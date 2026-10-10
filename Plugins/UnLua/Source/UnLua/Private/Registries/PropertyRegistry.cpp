#include "UnLuaCompatibility.h"
#include "PropertyRegistry.h"
#include "Binding.h"
#include "ClassRegistry.h"
#include "EnumRegistry.h"
#include "LowLevel.h"
#include "LuaEnv.h"
#include "ReflectionUtils/PropertyDesc.h"

namespace UnLua
{
    FPropertyRegistry::FPropertyRegistry(FLuaEnv* Env)
        : Env(Env)
    {
        PropertyCollector = FindFirstObject<UScriptStruct>(TEXT("PropertyCollector"));
        check(PropertyCollector);
    }

    void FPropertyRegistry::NotifyUObjectDeleted(UObject* Object)
    {
        FieldProperties.Remove(static_cast<UField*>(Object));
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::CreateTypeInterface(lua_State* L, int32 Index)
    {
        Index = LowLevel::AbsIndex(L, Index);

        TSharedPtr<ITypeInterface> TypeInterface;
        int32 Type = lua_type(L, Index);
        switch (Type)
        {
        case LUA_TBOOLEAN:
            TypeInterface = GetBoolProperty();
            break;
        case LUA_TNUMBER:
            TypeInterface = lua_isinteger(L, Index) > 0 ? GetIntProperty() : GetFloatProperty();
            break;
        case LUA_TSTRING:
            TypeInterface = GetStringProperty();
            break;
        case LUA_TTABLE:
            {
                lua_pushstring(L, "__name");
                Type = lua_rawget(L, Index);
                if (Type == LUA_TSTRING)
                {
                    const char* Name = lua_tostring(L, -1);
                    auto ClassDesc = Env->GetClassRegistry()->Find(Name);
                    if (ClassDesc)
                    {
                        TypeInterface = GetFieldProperty(ClassDesc->AsStruct());
                    }
                    else
                    {
                        auto EnumDesc = Env->GetEnumRegistry()->Find(Name);
                        if (EnumDesc)
                            TypeInterface = GetFieldProperty(EnumDesc->GetEnum());
                        else
                            TypeInterface = FindTypeInterface(lua_tostring(L, -1));
                    }
                }
                lua_pop(L, 1);
            }
            break;
        case LUA_TUSERDATA:
            {
                // mt/nil
                lua_getmetatable(L, Index);
                if (lua_istable(L, -1))
                {
                    // mt,mt.__name/nil
                    lua_getfield(L, -1, "__name");
                    if (lua_isstring(L, -1))
                    {
                        const char* Name = lua_tostring(L, -1);
                        FClassDesc* ClassDesc = Env->GetClassRegistry()->Find(Name);
                        if (ClassDesc)
                            TypeInterface = GetFieldProperty(ClassDesc->AsStruct());
                    }
                    // mt
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }
            break;
        default:
            break;
        }

        return TypeInterface;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetBoolProperty()
    {
        if (!BoolProperty)
        {
            constexpr auto Params = UECodeGen_Private::FBoolPropertyParams
            {
                nullptr,
                nullptr,
                CPF_None,
                UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool,
                nullptr,
                nullptr,
                1,
                sizeof(bool),
                sizeof(FPropertyCollector),
                nullptr,
                METADATA_PARAMS(0, nullptr)
            };
            const auto Property = new FBoolProperty(PropertyCollector, Params);
            BoolProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return BoolProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetIntProperty()
    {
        if (!IntProperty)
        {
            constexpr auto Params = UECodeGen_Private::FIntPropertyParams
            {
                nullptr,
                nullptr,
                CPF_HasGetValueTypeHash,
                UECodeGen_Private::EPropertyGenFlags::Int,
                nullptr,
                nullptr,
                1,
                0,
                METADATA_PARAMS(0, nullptr)
            };
            const auto Property = new FIntProperty(PropertyCollector, Params);
            IntProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return IntProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetFloatProperty()
    {
        if (!FloatProperty)
        {
            constexpr auto Params = UECodeGen_Private::FFloatPropertyParams
            {
                nullptr,
                nullptr,
                CPF_HasGetValueTypeHash,
                UECodeGen_Private::EPropertyGenFlags::Float,
                nullptr,
                nullptr,
                1,
                0,
                METADATA_PARAMS(0, nullptr)
            };
            const auto Property = new FFloatProperty(PropertyCollector, Params);
            FloatProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return FloatProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetStringProperty()
    {
        if (!StringProperty)
        {
            constexpr auto Params = UECodeGen_Private::FStrPropertyParams
            {
                nullptr,
                nullptr,
                CPF_HasGetValueTypeHash,
                UECodeGen_Private::EPropertyGenFlags::Str,
                nullptr,
                nullptr,
                1,
                0,
                METADATA_PARAMS(0, nullptr)
            };
            const auto Property = new FStrProperty(PropertyCollector, Params);
            StringProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return StringProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetNameProperty()
    {
        if (!NameProperty)
        {
            constexpr auto Params = UECodeGen_Private::FNamePropertyParams
            {
                nullptr,
                nullptr,
                CPF_HasGetValueTypeHash,
                UECodeGen_Private::EPropertyGenFlags::Name,
                nullptr,
                nullptr,
                1,
                0,
                METADATA_PARAMS(0, nullptr)
            };
            const auto Property = new FNameProperty(PropertyCollector, Params);
            NameProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return NameProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetTextProperty()
    {
        if (!TextProperty)
        {
            // FTextProperty cannot be built from FTextPropertyParams: the generated-code path expects a
            // text namespace/key pair that we have no meaningful value for here.
            const auto Property = new FTextProperty(PropertyCollector, NAME_None);
            Property->PropertyFlags |= CPF_HasGetValueTypeHash;
            TextProperty = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        }
        return TextProperty;
    }

    TSharedPtr<ITypeInterface> FPropertyRegistry::GetFieldProperty(UField* Field)
    {
        if (const auto Exists = FieldProperties.Find(Field))
            return *Exists;

        FProperty* Property;
        if (const auto Class = Cast<UClass>(Field))
        {
            constexpr auto Params = UECodeGen_Private::FObjectPropertyParams
            {
                nullptr,
                nullptr,
                CPF_HasGetValueTypeHash,
                UECodeGen_Private::EPropertyGenFlags::Object,
                nullptr,
                nullptr,
                1,
                0,
                nullptr,
                METADATA_PARAMS(0, nullptr)
            };
            const auto ObjectProperty = new FObjectProperty(PropertyCollector, Params);
            ObjectProperty->PropertyClass = Class;
            Property = ObjectProperty;
        }
        else if (const auto ScriptStruct = Cast<UScriptStruct>(Field))
        {
            const auto Params = UECodeGen_Private::FStructPropertyParams
            {
                nullptr,
                nullptr,
                ScriptStruct->GetCppStructOps()
                    ? ScriptStruct->GetCppStructOps()->GetComputedPropertyFlags() | CPF_HasGetValueTypeHash
                    : CPF_HasGetValueTypeHash,
                UECodeGen_Private::EPropertyGenFlags::Struct,
                nullptr,
                nullptr,
                1,
                0,
                nullptr,
                METADATA_PARAMS(0, nullptr)
            };
            const auto StructProperty = new FStructProperty(PropertyCollector, Params);
            StructProperty->Struct = ScriptStruct;
            StructProperty->SetElementSize(ScriptStruct->PropertiesSize);
            Property = StructProperty;
        }
        else if (const auto Enum = Cast<UEnum>(Field))
        {
            const auto EnumProperty = new FEnumProperty(PropertyCollector, NAME_None);
            EnumProperty->SetEnum(Enum);
            EnumProperty->PropertyFlags |= CPF_HasGetValueTypeHash;
            const auto UnderlyingProperty = new FByteProperty(EnumProperty, TEXT("UnderlyingType"));
            Property = EnumProperty;
            Property->AddCppProperty(UnderlyingProperty);
            Property->SetElementSize(UnderlyingProperty->GetElementSize());
            Property->PropertyFlags |= CPF_IsPlainOldData | CPF_NoDestructor | CPF_ZeroConstructor;
        }
        else
        {
            Property = nullptr;
        }

        const auto Ret = TSharedPtr<ITypeInterface>(FPropertyDesc::Create(Property));
        FieldProperties.Add(Field, Ret);
        return Ret;
    }
}
