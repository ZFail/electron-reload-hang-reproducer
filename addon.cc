#include <chrono>
#include <thread>
#include <iostream>
#include <atomic>
#include "napi.h"

constexpr size_t ARRAY_LENGTH = 10;

// Data structure representing our thread-safe function context.
struct TsfnContext {
  TsfnContext(Napi::Env env) {
  };

  // Native thread
  std::thread nativeThread;

  std::atomic<bool> stop = false;

  Napi::ThreadSafeFunction tsfn;
};

void threadEntry(TsfnContext* context);

void FinalizerCallback(Napi::Env env, void* finalizeData, TsfnContext* context);

Napi::Value CreateTSFN(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // Construct context data
  auto testData = new TsfnContext(env);

  // Create a new ThreadSafeFunction.
  testData->tsfn = Napi::ThreadSafeFunction::New(
      env,                           // Environment
      info[0].As<Napi::Function>(),  // JS function from caller
      "TSFN",                        // Resource name
      0,                             // Max queue size (0 = unlimited).
      1,                             // Initial thread count
      testData,                      // Context,
      FinalizerCallback,             // Finalizer
      (void*)nullptr                 // Finalizer data
  );
  testData->nativeThread = std::thread(threadEntry, testData);

  return env.Undefined();
}

void threadEntry(TsfnContext* context) {
      auto callback = [](Napi::Env env, Napi::Function jsCallback, int* data) {
        try {
            jsCallback.Call({Napi::Number::New(env, *data)});
        } catch (const Napi::Error& e) {
            std::cout << "Caught JavaScript exception: " << e.what() << std::endl;
        }
        delete data;
    };

  int num = 0;

  while (!context->stop) {
    auto a = new int(num++);
    napi_status status =
        context->tsfn.NonBlockingCall(a, callback);

    if (status != napi_ok) {
      // Napi::Error::Fatal(
      //     "ThreadEntry",
      //     "Napi::ThreadSafeNapi::Function.BlockingCall() failed");
          std::cout << "Napi::ThreadSafeNapi::Function.BlockingCall() failed\n";
    }
    // Sleep for some time.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // Release the thread-safe function. This decrements the internal thread
  // count, and will perform finalization since the count will reach 0.
  context->tsfn.Release();
}

void FinalizerCallback(Napi::Env env,
                       void* finalizeData,
                       TsfnContext* context) {
  context->stop = true;
  context->nativeThread.join();
  delete context;
}

// Addon entry point
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports["createTSFN"] = Napi::Function::New(env, CreateTSFN);
  return exports;
}

NODE_API_MODULE(addon, Init)
