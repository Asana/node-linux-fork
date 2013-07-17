#include <node.h>
#include <v8.h>
#include <unistd.h>

v8::Handle<v8::Value> Fork(const v8::Arguments& args) {
  int res = -1;
  {
    v8::Unlocker unlocker;
    v8::V8::PrepareToFork();
    res = fork();
    v8::V8::AfterForking();
  }

  v8::HandleScope scope;
  return scope.Close(v8::Integer::New(res));
}

void init(v8::Handle<v8::Object> exports) {
  exports->Set(
    v8::String::NewSymbol("hello"),
    v8::FunctionTemplate::New(Fork)->GetFunction());
}

NODE_MODULE(fork, init);
