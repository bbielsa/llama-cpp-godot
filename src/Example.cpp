// Copied from godot-cpp/test/src and modified.

#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/label.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

#include "common/common.h"
#include "llama.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>


#include "Example.h"

// Used to mark unused parameters to indicate intent and suppress warnings.
#define UNUSED( expr ) (void)( expr )

namespace
{
    constexpr int MAGIC_NUMBER = 42;

    class MyCallableCustom : public godot::CallableCustom
    {
    public:
        virtual uint32_t hash() const
        {
            return 27;
        }

        virtual godot::String get_as_text() const
        {
            return "<MyCallableCustom>";
        }

        static bool compare_equal_func( const CallableCustom *inA, const CallableCustom *inB )
        {
            return inA == inB;
        }

        virtual CompareEqualFunc get_compare_equal_func() const
        {
            return &MyCallableCustom::compare_equal_func;
        }

        static bool compare_less_func( const CallableCustom *inA, const CallableCustom *inB )
        {
            return reinterpret_cast<const void *>( inA ) < reinterpret_cast<const void *>( inB );
        }

        virtual CompareLessFunc get_compare_less_func() const
        {
            return &MyCallableCustom::compare_less_func;
        }

        bool is_valid() const
        {
            return true;
        }

        virtual godot::ObjectID get_object() const
        {
            return godot::ObjectID();
        }

        virtual void call( const godot::Variant **inArguments, int inArgcount,
                           godot::Variant &outReturnValue,
                           GDExtensionCallError &outCallError ) const
        {
            UNUSED( inArguments );
            UNUSED( inArgcount );

            outReturnValue = "Hi";
            outCallError.error = GDEXTENSION_CALL_OK;
        }
    };
}

//// ExampleRef

int ExampleRef::sInstanceCount = 0;
int ExampleRef::sLastID = 0;

ExampleRef::ExampleRef()
{
    mID = ++sLastID;
    sInstanceCount++;

    godot::UtilityFunctions::print(
        "ExampleRef ", godot::itos( mID ),
        " created, current instance count: ", godot::itos( sInstanceCount ) );
}

ExampleRef::~ExampleRef()
{
    sInstanceCount--;
    godot::UtilityFunctions::print(
        "ExampleRef ", godot::itos( mID ),
        " destroyed, current instance count: ", godot::itos( sInstanceCount ) );
}

void ExampleRef::setId( int inID )
{
    mID = inID;
}

int ExampleRef::getID() const
{
    return mID;
}

void ExampleRef::_bind_methods()
{
    godot::ClassDB::bind_method( godot::D_METHOD( "set_id", "id" ), &ExampleRef::setId );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_id" ), &ExampleRef::getID );

    godot::ClassDB::bind_method( godot::D_METHOD( "was_post_initialized" ),
                                 &ExampleRef::wasPostInitialized );
}

void ExampleRef::_notification( int inWhat )
{
    if ( inWhat == NOTIFICATION_POSTINITIALIZE )
    {
        mPostInitialized = true;
    }
}

//// ExampleMin

void ExampleMin::_bind_methods()
{
}

//// Example

Example::Example()
{
    godot::UtilityFunctions::print( "Constructor." );
}

Example::~Example()
{
    godot::UtilityFunctions::print( "Destructor." );
}

// Methods.
void Example::simpleFunc()
{
    godot::UtilityFunctions::print( "  Simple func called." );
}

int32_t get_num_physical_cores() {
    return 1;
}

godot::String Example::generate( const godot::String &inBase )
{
    const int n_len = 32;
    gpt_params params;
    
    params.prompt = "Hello, world! My name is";
    params.model = "/Users/brandon/Downloads/llama-2-7b-chat.Q2_K.gguf";
    
    llama_backend_init();
    llama_numa_init(params.numa);
    
    llama_model_params model_params = llama_model_default_params();
    llama_model * model = llama_load_model_from_file(params.model.c_str(), model_params);
    
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.seed  = 1234;
    ctx_params.n_ctx = 2048;
    ctx_params.n_threads = params.n_threads;
    ctx_params.n_threads_batch = params.n_threads_batch == -1 ? params.n_threads : params.n_threads_batch;
    
    llama_context * ctx = llama_new_context_with_model(model, ctx_params);
    
    std::vector<llama_token> tokens_list;
    tokens_list = ::llama_tokenize(ctx, params.prompt, true);

//    const int n_ctx    = llama_n_ctx(ctx);
//    const int n_kv_req = tokens_list.size() + (n_len - tokens_list.size());

    // create a llama_batch with size 512
    // we use this object to submit token data for decoding

    llama_batch batch = llama_batch_init(512, 0, 1);

    // evaluate the initial prompt
    for (size_t i = 0; i < tokens_list.size(); i++) {
        llama_batch_add(batch, tokens_list[i], i, { 0 }, false);
    }

    // llama_decode will output logits only for the last token of the prompt
    batch.logits[batch.n_tokens - 1] = true;

    int n_cur    = batch.n_tokens;
//    int n_decode = 0;

    while (n_cur <= n_len) {
        // sample the next token
        {
            auto   n_vocab = llama_n_vocab(model);
            auto * logits  = llama_get_logits_ith(ctx, batch.n_tokens - 1);
            
            std::vector<llama_token_data> candidates;
            candidates.reserve(n_vocab);
            
            for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
                candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
            }
            
            llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };
            
            // sample the most likely token
            const llama_token new_token_id = llama_sample_token_greedy(ctx, &candidates_p);
            llama_batch_clear(batch);

            // push this new token for next evaluation
            llama_batch_add(batch, new_token_id, n_cur, { 0 }, true);

//            n_decode += 1;
        }
        
        n_cur += 1;

        llama_decode(ctx, batch);

    }
    return inBase;
}

void Example::simpleConstFunc() const
{
    godot::UtilityFunctions::print( "  Simple const func called." );
}

godot::String Example::returnSomething( const godot::String &inBase )
{
    godot::UtilityFunctions::print( "  Return something called." );

    return inBase;
}

godot::Viewport *Example::returnSomethingConst() const
{
    godot::UtilityFunctions::print( "  Return something const called." );

    if ( is_inside_tree() )
    {
        godot::Viewport *result = get_viewport();

        return result;
    }

    return nullptr;
}

godot::Ref<ExampleRef> Example::returnEmptyRef() const
{
    godot::Ref<ExampleRef> ref;
    return ref;
}

ExampleRef *Example::returnExtendedRef() const
{
    // You can instance and return a refcounted object like this, but keep in mind that refcounting
    // starts with the returned object and it will be destroyed when all references are destroyed.
    // If you store this pointer you run the risk of having a pointer to a destroyed object.
    return memnew( ExampleRef() );
}

godot::Ref<ExampleRef> Example::extendedRefChecks( godot::Ref<ExampleRef> inRef ) const
{
    // This is the preferred way of instancing and returning a refcounted object:
    godot::Ref<ExampleRef> ref;
    ref.instantiate();

    godot::UtilityFunctions::print(
        "  Example ref checks called with value: ", inRef->get_instance_id(),
        ", returning value: ", ref->get_instance_id() );

    return ref;
}

godot::Variant Example::varargsFunc( const godot::Variant **inArgs, GDExtensionInt inArgCount,
                                     GDExtensionCallError &outError )
{
    UNUSED( inArgs );
    UNUSED( outError );

    godot::UtilityFunctions::print( "  Varargs (Variant return) called with ",
                                    godot::String::num_int64( inArgCount ), " arguments" );

    return inArgCount;
}

int Example::varargsFuncNonVoidReturn( const godot::Variant **inArgs, GDExtensionInt inArgCount,
                                       GDExtensionCallError &outError )
{
    UNUSED( inArgs );
    UNUSED( outError );

    godot::UtilityFunctions::print( "  Varargs (int return) called with ",
                                    godot::String::num_int64( inArgCount ), " arguments" );

    return MAGIC_NUMBER;
}

void Example::varargsFuncVoidReturn( const godot::Variant **inArgs, GDExtensionInt inArgCount,
                                     GDExtensionCallError &outError )
{
    UNUSED( inArgs );
    UNUSED( outError );

    godot::UtilityFunctions::print( "  Varargs (no return) called with ",
                                    godot::String::num_int64( inArgCount ), " arguments" );
}

void Example::emitCustomSignal( const godot::String &inName, int inValue )
{
    emit_signal( "custom_signal", inName, inValue );
}

int Example::defArgs( int inA, int inB ) const
{
    return inA + inB;
}

godot::Array Example::testArray() const
{
    godot::Array arr;

    arr.resize( 2 );
    arr[0] = godot::Variant( 1 );
    arr[1] = godot::Variant( 2 );

    return arr;
}

void Example::testTypedArrayArg( const godot::TypedArray<int64_t> &inArray )
{
    for ( int i = 0; i < inArray.size(); ++i )
    {
        godot::UtilityFunctions::print( inArray[i] );
    }
}

godot::TypedArray<godot::Vector2> Example::testTypedArray() const
{
    godot::TypedArray<godot::Vector2> arr;

    arr.resize( 2 );
    arr[0] = godot::Vector2( 1, 2 );
    arr[1] = godot::Vector2( 2, 3 );

    return arr;
}

godot::Dictionary Example::testDictionary() const
{
    godot::Dictionary dict;

    dict["hello"] = "world";
    dict["foo"] = "bar";

    return dict;
}

Example *Example::testNodeArgument( Example *inNode ) const
{
    // This should use godot::String::num_uint64(), but it is currently broken:
    //  https://github.com/godotengine/godot-cpp/issues/1014
    godot::UtilityFunctions::print(
        "  Test node argument called with ",
        ( inNode != nullptr )
            ? godot::String::num_int64( static_cast<int64_t>( inNode->get_instance_id() ) )
            : "null" );
    return inNode;
}

godot::String Example::testStringOps() const
{
    godot::String s = godot::String( "A" );
    s += "B";
    s += "C";
    s += char32_t( 0x010E );
    s = s + "E";

    return s;
}

godot::String Example::testStrUtility() const
{
    return godot::UtilityFunctions::str( "Hello, ", "World", "! The answer is ", 42 );
}

bool Example::testStringIsFortyTwo( const godot::String &inString ) const
{
    return strcmp( inString.utf8().ptr(), "forty two" ) == 0;
}

godot::String Example::testStringResize( godot::String ioString ) const
{
    int64_t orig_len = ioString.length();

    // This cast is to fix an issue with the API.
    // See: https://github.com/godotengine/godot-cpp/issues/1338
    ioString.resize( static_cast<int>( orig_len ) + 3 );

    char32_t *data = ioString.ptrw();
    data[orig_len + 0] = '!';
    data[orig_len + 1] = '?';
    data[orig_len + 2] = '\0';

    return ioString;
}

int Example::testVectorOps() const
{
    godot::PackedInt32Array arr;
    arr.push_back( 10 );
    arr.push_back( 20 );
    arr.push_back( 30 );
    arr.push_back( 45 );

    int ret = 0;
    for ( const int32_t &E : arr )
    {
        ret += E;
    }

    return ret;
}

bool Example::testObjectCastToNode( godot::Object *inObject ) const
{
    return godot::Object::cast_to<Node>( inObject ) != nullptr;
}

bool Example::testObjectCastToControl( godot::Object *inObject ) const
{
    return godot::Object::cast_to<Control>( inObject ) != nullptr;
}

bool Example::testObjectCastToExample( godot::Object *inObject ) const
{
    return godot::Object::cast_to<Example>( inObject ) != nullptr;
}

godot::Vector2i Example::testVariantVector2iConversion( const godot::Variant &inVariant ) const
{
    return inVariant;
}

int Example::testVariantIntConversion( const godot::Variant &inVariant ) const
{
    return inVariant;
}

float Example::testVariantFloatConversion( const godot::Variant &inVariant ) const
{
    return inVariant;
}

godot::Variant Example::testVariantCall( godot::Variant inVariant )
{
    return inVariant.call( "test", "hello" );
}

godot::Variant Example::testVariantIterator( const godot::Variant &inVariant )
{
    godot::Array output;
    godot::Variant iter;

    bool is_init_valid = true;

    if ( !inVariant.iter_init( iter, is_init_valid ) )
    {
        if ( !is_init_valid )
        {
            return "iter_init: not valid";
        }

        return output;
    }

    bool is_iter_next_valid = true;
    bool is_iter_get_valid = true;

    do
    {
        if ( !is_iter_next_valid )
        {
            return "iter_next: not valid";
        }

        godot::Variant value = inVariant.iter_get( iter, is_iter_get_valid );

        if ( !is_iter_get_valid )
        {
            return "iter_get: not valid";
        }

        output.push_back( ( static_cast<int>( value ) ) + 5 );

    } while ( inVariant.iter_next( iter, is_iter_next_valid ) );

    if ( !is_iter_next_valid )
    {
        return "iter_next: not valid";
    }

    return output;
}

void Example::testAddChild( godot::Node *inNode )
{
    add_child( inNode );
}

void Example::testSetTileset( godot::TileMap *inTilemap,
                              const godot::Ref<godot::TileSet> &inTileset ) const
{
    inTilemap->set_tileset( inTileset );
}

godot::Callable Example::testCallableMP()
{
    return callable_mp( this, &Example::unboundMethod1 );
}

godot::Callable Example::testCallableMPRet()
{
    return callable_mp( this, &Example::unboundMethod2 );
}

godot::Callable Example::testCallableMPRetC() const
{
    return callable_mp( this, &Example::unboundMethod3 );
}

godot::Callable Example::testCallableMPStatic() const
{
    return callable_mp_static( &Example::unboundStaticMethod1 );
}

godot::Callable Example::testCallableMPStaticRet() const
{
    return callable_mp_static( &Example::unboundStaticMethod2 );
}

godot::Callable Example::testCustomCallable() const
{
    return godot::Callable( memnew( MyCallableCustom ) );
}

void Example::callableBind()
{
    godot::Callable c = godot::Callable( this, "emit_custom_signal" ).bind( "bound", 11 );
    c.call();
}

void Example::unboundMethod1( godot::Object *inObject, godot::String inString, int inInt )
{
    godot::String test = "unbound_method1: ";
    test += inObject->get_class();
    test += " - " + inString;
    emitCustomSignal( test, inInt );
}

godot::String Example::unboundMethod2( godot::Object *inObject, godot::String inString, int inInt )
{
    godot::String test = "unbound_method2: ";
    test += inObject->get_class();
    test += " - " + inString;
    test += " - " + godot::itos( inInt );
    return test;
}

godot::String Example::unboundMethod3( godot::Object *inObject, godot::String inString,
                                       int inInt ) const
{
    godot::String test = "unbound_method3: ";
    test += inObject->get_class();
    test += " - " + inString;
    test += " - " + godot::itos( inInt );
    return test;
}

void Example::unboundStaticMethod1( Example *inObject, godot::String inString, int inInt )
{
    godot::String test = "unbound_static_method1: ";
    test += inObject->get_class();
    test += " - " + inString;
    inObject->emitCustomSignal( test, inInt );
}

godot::String Example::unboundStaticMethod2( godot::Object *inObject, godot::String inString,
                                             int inInt )
{
    godot::String test = "unbound_static_method2: ";
    test += inObject->get_class();
    test += " - " + inString;
    test += " - " + godot::itos( inInt );
    return test;
}

godot::BitField<Example::Flags> Example::testBitfield( godot::BitField<Flags> inFlags )
{
    godot::UtilityFunctions::print( "  Got BitField: ", godot::String::num_int64( inFlags ) );
    return inFlags;
}

// RPC
void Example::testRPC( int inValue )
{
    mLastRPCArg = inValue;
}

void Example::testSendRPC( int inValue )
{
    rpc( "test_rpc", inValue );
}

int Example::returnLastRPCArg()
{
    return mLastRPCArg;
}

// Properties
void Example::setCustomPosition( const godot::Vector2 &inPos )
{
    mCustomPosition = inPos;
}

godot::Vector2 Example::getCustomPosition() const
{
    return mCustomPosition;
}

godot::Vector4 Example::getV4() const
{
    return { 1.2f, 3.4f, 5.6f, 7.8f };
}

bool Example::testPostInitialize() const
{
    godot::Ref<ExampleRef> new_example_ref;

    new_example_ref.instantiate();

    return new_example_ref->wasPostInitialized();
}

// Static methods
int Example::testStatic( int inA, int inB )
{
    return inA + inB;
}

void Example::testStatic2()
{
    godot::UtilityFunctions::print( "  void static" );
}

// Virtual function override.
bool Example::_has_point( const godot::Vector2 &inPoint ) const
{
    auto *label = godot::Control::get_node<godot::Label>( "Label" );

    label->set_text( "Got point: " + godot::Variant( inPoint ).stringify() );

    return false;
}

void Example::_bind_methods()
{
    // Methods.
    godot::ClassDB::bind_method( godot::D_METHOD( "simple_func" ), &Example::simpleFunc );
    godot::ClassDB::bind_method( godot::D_METHOD( "simple_const_func" ),
                                 &Example::simpleConstFunc );
    godot::ClassDB::bind_method( godot::D_METHOD( "generate" ), &Example::generate );
    godot::ClassDB::bind_method( godot::D_METHOD( "return_something" ), &Example::returnSomething );
    godot::ClassDB::bind_method( godot::D_METHOD( "return_something_const" ),
                                 &Example::returnSomethingConst );
    godot::ClassDB::bind_method( godot::D_METHOD( "return_empty_ref" ), &Example::returnEmptyRef );
    godot::ClassDB::bind_method( godot::D_METHOD( "return_extended_ref" ),
                                 &Example::returnExtendedRef );
    godot::ClassDB::bind_method( godot::D_METHOD( "extended_ref_checks", "ref" ),
                                 &Example::extendedRefChecks );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_array" ), &Example::testArray );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_tarray_arg", "array" ),
                                 &Example::testTypedArrayArg );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_tarray" ), &Example::testTypedArray );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_dictionary" ), &Example::testDictionary );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_node_argument" ),
                                 &Example::testNodeArgument );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_string_ops" ), &Example::testStringOps );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_str_utility" ), &Example::testStrUtility );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_string_is_forty_two" ),
                                 &Example::testStringIsFortyTwo );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_string_resize" ),
                                 &Example::testStringResize );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_vector_ops" ), &Example::testVectorOps );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_object_cast_to_node", "object" ),
                                 &Example::testObjectCastToNode );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_object_cast_to_control", "object" ),
                                 &Example::testObjectCastToControl );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_object_cast_to_example", "object" ),
                                 &Example::testObjectCastToExample );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_variant_vector2i_conversion", "variant" ),
                                 &Example::testVariantVector2iConversion );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_variant_int_conversion", "variant" ),
                                 &Example::testVariantIntConversion );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_variant_float_conversion", "variant" ),
                                 &Example::testVariantFloatConversion );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_variant_call", "variant" ),
                                 &Example::testVariantCall );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_variant_iterator", "input" ),
                                 &Example::testVariantIterator );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_add_child", "node" ),
                                 &Example::testAddChild );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_set_tileset", "tilemap", "tileset" ),
                                 &Example::testSetTileset );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_callable_mp" ), &Example::testCallableMP );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_callable_mp_ret" ),
                                 &Example::testCallableMPRet );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_callable_mp_retc" ),
                                 &Example::testCallableMPRetC );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_callable_mp_static" ),
                                 &Example::testCallableMPStatic );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_callable_mp_static_ret" ),
                                 &Example::testCallableMPStaticRet );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_custom_callable" ),
                                 &Example::testCustomCallable );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_bitfield", "flags" ),
                                 &Example::testBitfield );

    godot::ClassDB::bind_method( godot::D_METHOD( "test_rpc", "value" ), &Example::testRPC );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_send_rpc", "value" ),
                                 &Example::testSendRPC );
    godot::ClassDB::bind_method( godot::D_METHOD( "return_last_rpc_arg" ),
                                 &Example::returnLastRPCArg );

    godot::ClassDB::bind_method( godot::D_METHOD( "def_args", "a", "b" ), &Example::defArgs,
                                 DEFVAL( 100 ), DEFVAL( 200 ) );
    godot::ClassDB::bind_method( godot::D_METHOD( "callable_bind" ), &Example::callableBind );
    godot::ClassDB::bind_method( godot::D_METHOD( "test_post_initialize" ),
                                 &Example::testPostInitialize );

    godot::ClassDB::bind_static_method( "Example", godot::D_METHOD( "test_static", "a", "b" ),
                                        &Example::testStatic );
    godot::ClassDB::bind_static_method( "Example", godot::D_METHOD( "test_static2" ),
                                        &Example::testStatic2 );

    {
        godot::MethodInfo mi;
        mi.arguments.emplace_back( godot::Variant::STRING, "some_argument" );
        mi.name = "varargs_func";

        godot::ClassDB::bind_vararg_method( godot::METHOD_FLAGS_DEFAULT, "varargs_func",
                                            &Example::varargsFunc, mi );
    }

    {
        godot::MethodInfo mi;
        mi.arguments.emplace_back( godot::Variant::STRING, "some_argument" );
        mi.name = "varargs_func_nv";

        godot::ClassDB::bind_vararg_method( godot::METHOD_FLAGS_DEFAULT, "varargs_func_nv",
                                            &Example::varargsFuncNonVoidReturn, mi );
    }

    {
        godot::MethodInfo mi;
        mi.arguments.emplace_back( godot::Variant::STRING, "some_argument" );
        mi.name = "varargs_func_void";

        godot::ClassDB::bind_vararg_method( godot::METHOD_FLAGS_DEFAULT, "varargs_func_void",
                                            &Example::varargsFuncVoidReturn, mi );
    }

    // Properties.
    ADD_GROUP( "Test group", "group_" );
    ADD_SUBGROUP( "Test subgroup", "group_subgroup_" );

    godot::ClassDB::bind_method( godot::D_METHOD( "get_custom_position" ),
                                 &Example::getCustomPosition );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_v4" ), &Example::getV4 );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_custom_position", "position" ),
                                 &Example::setCustomPosition );
    ADD_PROPERTY( godot::PropertyInfo( godot::Variant::VECTOR2, "group_subgroup_custom_position" ),
                  "set_custom_position", "get_custom_position" );

    // Signals.
    ADD_SIGNAL( godot::MethodInfo( "custom_signal",
                                   godot::PropertyInfo( godot::Variant::STRING, "name" ),
                                   godot::PropertyInfo( godot::Variant::INT, "value" ) ) );
    godot::ClassDB::bind_method( godot::D_METHOD( "emit_custom_signal", "name", "value" ),
                                 &Example::emitCustomSignal );

    // Constants.
    BIND_ENUM_CONSTANT( FIRST )
    BIND_ENUM_CONSTANT( ANSWER_TO_EVERYTHING )

    BIND_BITFIELD_FLAG( FLAG_ONE );
    BIND_BITFIELD_FLAG( FLAG_TWO );

    BIND_CONSTANT( CONSTANT_WITHOUT_ENUM );
    BIND_ENUM_CONSTANT( OUTSIDE_OF_CLASS );
}

void Example::_notification( int inWhat )
{
    godot::UtilityFunctions::print( "Notification: ", godot::String::num( inWhat ) );
}

bool Example::_set( const godot::StringName &inName, const godot::Variant &inValue )
{
    godot::String name = inName;

    if ( name.begins_with( "dproperty" ) )
    {
        int64_t index = name.get_slicec( '_', 1 ).to_int();
        mDProp[index] = inValue;

        return true;
    }

    if ( name == "property_from_list" )
    {
        mPropertyFromList = inValue;

        return true;
    }

    return false;
}

bool Example::_get( const godot::StringName &inName, godot::Variant &outReturn ) const
{
    godot::String name = inName;

    if ( name.begins_with( "dproperty" ) )
    {
        int64_t index = name.get_slicec( '_', 1 ).to_int();
        outReturn = mDProp[index];

        return true;
    }

    if ( name == "property_from_list" )
    {
        outReturn = mPropertyFromList;

        return true;
    }

    return false;
}

void Example::_get_property_list( godot::List<godot::PropertyInfo> *outList ) const
{
    outList->push_back( godot::PropertyInfo( godot::Variant::VECTOR3, "property_from_list" ) );

    for ( int i = 0; i < 3; ++i )
    {
        outList->push_back(
            godot::PropertyInfo( godot::Variant::VECTOR2, "dproperty_" + godot::itos( i ) ) );
    }
}

bool Example::_property_can_revert( const godot::StringName &inName ) const
{
    if ( inName == godot::StringName( "property_from_list" ) &&
         mPropertyFromList != godot::Vector3( MAGIC_NUMBER, MAGIC_NUMBER, MAGIC_NUMBER ) )
    {
        return true;
    }

    return false;
};

bool Example::_property_get_revert( const godot::StringName &inName,
                                    godot::Variant &outProperty ) const
{
    if ( inName == godot::StringName( "property_from_list" ) )
    {
        outProperty = godot::Vector3( MAGIC_NUMBER, MAGIC_NUMBER, MAGIC_NUMBER );

        return true;
    }

    return false;
};

void Example::_validate_property( godot::PropertyInfo &inProperty ) const
{
    godot::String name = inProperty.name;

    // Test hiding the "mouse_filter" property from the editor.
    if ( name == "mouse_filter" )
    {
        inProperty.usage = godot::PROPERTY_USAGE_NO_EDITOR;
    }
}

godot::String Example::_to_string() const
{
    return "[ GDExtension::Example <--> Instance ID:" + godot::uitos( get_instance_id() ) + " ]";
}

//// ExampleVirtual

void ExampleVirtual::_bind_methods()
{
}

//// ExampleAbstract

void ExampleAbstract::_bind_methods()
{
}
