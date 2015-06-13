#pragma once

#include "Tools.h"

template<typename T>
T FromJsValue( v8::Local<v8::Value>& value )
{
    return typename T::Cast( value );
}

template<>
inline std::string FromJsValue<std::string>( v8::Local<v8::Value>& value )
{
    v8::String::Utf8Value str( value->ToString() );

    return *str;
}

template<>
inline unsigned FromJsValue<unsigned>( v8::Local<v8::Value>& value )
{
    return static_cast<unsigned>( v8::Local<v8::Integer>::Cast( value )->Value() );
}

template<>
inline int FromJsValue<int>( v8::Local<v8::Value>& value )
{
    return static_cast<int>( v8::Local<v8::Integer>::Cast( value )->Value() );
}

template<>
std::vector<std::string> FromJsValue<std::vector<std::string> >( v8::Local<v8::Value>& value )
{
    std::vector<std::string> result;

    if( value->IsArray() ) {
        v8::Local<v8::Array> jsArray = v8::Local<v8::Array>::Cast( value );

        for( unsigned i = 0 ; i < jsArray->Length(); ++i ) {
            v8::String::Utf8Value item( jsArray->Get(i)->ToString() );
            if( item.length() ) {
                result.emplace( result.end(), *item );
            }
        }
    }

    return std::move( result );
}

inline v8::Local<v8::Value> ToJsValue( v8::Local<v8::Value>& value )
{
    return value;
}

inline v8::Local<v8::Value> ToJsValue( int value )
{
    return v8::Number::New( v8::Isolate::GetCurrent(), value );
}

template<typename C, typename ... A, unsigned ... I >
void CallMethod( void ( C::* method) ( A ... ),
                 const v8::FunctionCallbackInfo<v8::Value>& info, StaticSequence<I ...> )
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope( isolate );

    C* instance = node::ObjectWrap::Unwrap<C>( info.Holder() );

    ( instance->*method ) ( FromJsValue<std::remove_const<std::remove_reference<A>::type>::type >( info[I] ) ... );
};

template<typename R, typename C, typename ... A, unsigned ... I >
void CallMethod( R ( C::* method) ( A ... ),
                 const v8::FunctionCallbackInfo<v8::Value>& info, StaticSequence<I ...> )
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope( isolate );

    C* instance = node::ObjectWrap::Unwrap<C>( info.Holder() );

    info.GetReturnValue().Set(
        ToJsValue(
            ( instance->*method ) (
                FromJsValue<std::remove_const<std::remove_reference<A>::type>::type >( info[I] ) ... ) ) );
};

template<typename R, typename C, typename ... A>
void CallMethod( R ( C::* method ) ( A ... ), const v8::FunctionCallbackInfo<v8::Value>& info )
{
    return CallMethod( method, info, MakeStaticSequence<sizeof ... ( A )>::Sequence() );
}

#define SET_METHOD( objTemplate, name, member )                  \
    NODE_SET_PROTOTYPE_METHOD( objTemplate, name,                \
        [] ( const v8::FunctionCallbackInfo<v8::Value>& info ) { \
            CallMethod( member, info );                          \
        }                                                        \
    )
