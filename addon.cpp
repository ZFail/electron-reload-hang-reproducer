#include <assert.h>
#include <node_api.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <atomic>

struct Context {
	std::thread nativeThread;
	std::atomic<bool> stop = false;
	napi_threadsafe_function tsfn;
};

static void callJs(napi_env env, napi_value js_cb, void* _context, void* data) {
	// This parameter is not used.
	auto context = static_cast<Context*>(_context);
	napi_status status;

	// Retrieve the prime from the item created by the worker thread.
	int number = *(int*)data;

	// env and js_cb may both be NULL if Node.js is in its cleanup phase, and
	// items are left over from earlier thread-safe calls from the worker thread.
	// When env is NULL, we simply skip over the call into Javascript and free the
	// items.
	if (env != NULL) {
		napi_value undefined, js_number;

		// Convert the integer to a napi_value.
		status = napi_create_int32(env, number, &js_number);
		assert(status == napi_ok);

		// Retrieve the JavaScript `undefined` value so we can use it as the `this`
		// value of the JavaScript function call.
		status = napi_get_undefined(env, &undefined);
		assert(status == napi_ok);

		// Call the JavaScript function and pass it the prime that the secondary
		// thread found.
		status = napi_call_function(env,
			undefined,
			js_cb,
			1,
			&js_number,
			NULL);
		assert(status == napi_ok);
		// if (status == napi_pending_exception) {
		// 	std::cerr << "napi_get_and_clear_last_exception" << std::endl;
		// 	napi_value exception_obj;
		// 	status = napi_get_and_clear_last_exception(env, &exception_obj);
		// 	assert(status == napi_ok);
		// }
		// else if (status != napi_ok)
		// {
		// 	std::cerr << "napi_call_function status: " << status << std::endl;
		// 	const napi_extended_error_info* info;
		// 	status = napi_get_last_error_info(env, &info);
		// 	assert(status == napi_ok);
		// 	std::cerr << "napi_call_function error code: " << info->error_code << ", text: " << info->error_message << std::endl;
		// }
	}

	// Free the item created by the worker thread.
	free(data);
}

void threadEntry(Context* context);
void finalizerCallback(napi_env env,
	void* finalize_data,
	void* finalize_hint);

static napi_value createTSFN(napi_env env, napi_callback_info info) {
	napi_status status;

	size_t argc = 2;
	napi_value args[2];
	status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
	assert(status == napi_ok);

	if (argc < 1) {
		napi_throw_type_error(env, NULL, "Wrong number of arguments");
		return NULL;
	}

	napi_value work_name;

	status = napi_create_string_utf8(env,
		"tsfn",
		NAPI_AUTO_LENGTH,
		&work_name);
	assert(status == napi_ok);

	auto context = new Context;

	status = napi_create_threadsafe_function(
		env,
		args[0],
		NULL,
		work_name,
		0,
		1,
		context,
		finalizerCallback,
		context,
		callJs,
		&context->tsfn);
	assert(status == napi_ok);

	context->nativeThread = std::thread(threadEntry, context);

	napi_value result;
	napi_get_undefined(env, &result);
	return result;
}

void threadEntry(Context* context) {
	napi_status status = napi_acquire_threadsafe_function(context->tsfn);
	assert(status == napi_ok);

	int num = 0;

	// Find the first 1000 prime numbers using an extremely inefficient algorithm.
	while (!context->stop) {
		auto a = new int(num++);
		status = napi_call_threadsafe_function(context->tsfn,
			a,
			napi_tsfn_nonblocking);
		assert(status == napi_ok);

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		//std::this_thread::yield();
	}

	status = napi_release_threadsafe_function(context->tsfn,
		napi_tsfn_release);
	assert(status == napi_ok);
}
void finalizerCallback(napi_env env,
	void* finalize_data,
	void* finalize_hint)
{
	auto context = static_cast<Context*>(finalize_data);
	context->stop = true;
	context->nativeThread.join();
	delete context;
}

#define DECLARE_NAPI_METHOD(name, func)                                        \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Init(napi_env env, napi_value exports) {
	napi_status status;
	napi_property_descriptor addDescriptor = DECLARE_NAPI_METHOD("createTSFN", createTSFN);
	status = napi_define_properties(env, exports, 1, &addDescriptor);
	assert(status == napi_ok);
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)