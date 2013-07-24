#include <node.h>
#include <uv.h>
#include <v8.h>
#include <unistd.h>

v8::Handle<v8::Value> Fork(const v8::Arguments& args) {
  int res = -1;
  {
    v8::Unlocker unlocker;
    v8::V8::PrepareToFork();
    uv_suspend_worker_threads();
    res = fork();
    v8::V8::AfterForking();
  }

  v8::HandleScope scope;
  return scope.Close(v8::Integer::New(res));
}

v8::Handle<v8::Value> Dup2(const v8::Arguments& args) {
  v8::HandleScope scope;

  if (args.Length() < 2 || !args[0]->IsNumber() || !args[1]->IsNumber()) {
    v8::ThrowException(v8::Exception::TypeError(v8::String::New(
      "Ivalid arguments for dup2: int, int expected")));
    return scope.Close(v8::Undefined());
  }

  int fd1 = args[0]->NumberValue();
  int fd2 = args[1]->NumberValue();
  return scope.Close(v8::Integer::New(dup2(fd1, fd2)));
}

void init(v8::Handle<v8::Object> exports) {
  exports->Set(
    v8::String::NewSymbol("fork"),
    v8::FunctionTemplate::New(Fork)->GetFunction());
  exports->Set(
    v8::String::NewSymbol("dup2"),
    v8::FunctionTemplate::New(Dup2)->GetFunction());
}

NODE_MODULE(fork, init);
