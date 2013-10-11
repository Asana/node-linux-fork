#include <node.h>
#include <uv.h>
#include <openssl/rand.h>
#include <v8.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

uv_signal_t* sigchld_handler_ptr = NULL;
uv_signal_t sigchld_handler;

static void sigchld_callback(uv_signal_t* handler, int signum) {
  pid_t pid;
  int status;
  do {
    pid = waitpid(-1, &status, WNOHANG);
  } while (pid > 0);
}

static void start_sigchld_handler() {
  if (sigchld_handler_ptr == NULL) {
    sigchld_handler_ptr = &sigchld_handler;
    if (uv_signal_init(uv_default_loop(), &sigchld_handler)) {
      abort();
    }
    if (uv_signal_start(&sigchld_handler, sigchld_callback, SIGCHLD)) {
      abort();
    }
  }
}

static void stop_sigchld_handler() {
  if (sigchld_handler_ptr != NULL) {
    uv_signal_stop(&sigchld_handler);
    sigchld_handler_ptr = NULL;
  }
}

static void make_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    perror("F_GETFL failed");
    abort();
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
    perror("F_SETFL failed");
    abort();
  }
}

v8::Handle<v8::Value> Fork(const v8::Arguments& args) {
  v8::HandleScope scope;
  int res = -1;
  if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
    v8::ThrowException(v8::Exception::TypeError(v8::String::New(
      "Ivalid arguments for fork: (stdin_filename, stout_filename) expected")));
    return scope.Close(v8::Undefined());
  }
  v8::String::Utf8Value stdin_filename(args[0]);
  v8::String::Utf8Value stdout_filename(args[1]);
  {
    v8::Unlocker unlocker;
    start_sigchld_handler();
    v8::V8::PrepareToFork();
    uv_suspend_worker_threads();
    res = fork();
    setsid();

    if (res == 0) {
      stop_sigchld_handler();
      int stdin_fd = open(*stdin_filename, O_RDONLY);
      if (stdin_fd == -1) {
        perror("Cannot open child stdin");
        abort();
      }
      int stdout_fd = open(*stdout_filename, O_WRONLY);
      if (stdout_fd == -1) {
        perror("Cannot open child stdout");
        abort();
      }
      make_nonblocking(stdin_fd);
      make_nonblocking(stdout_fd);
      if (dup2(stdin_fd, STDIN_FILENO) == -1 || dup2(stdout_fd, STDOUT_FILENO) == -1) {
        abort();
      }  
      close(stdin_fd);
      close(stdout_fd);
      uv_after_fork();
      RAND_cleanup();
    }
    v8::V8::AfterForking();
  }

  return scope.Close(v8::Integer::New(res));
}

v8::Handle<v8::Value> Getpid(const v8::Arguments& args) {
  v8::HandleScope scope;
  return scope.Close(v8::Integer::New(getpid()));
}

void init(v8::Handle<v8::Object> exports) {
  exports->Set(
    v8::String::NewSymbol("forkAndReopenStdio"),
    v8::FunctionTemplate::New(Fork)->GetFunction());
  exports->Set(
    v8::String::NewSymbol("getpid"),
    v8::FunctionTemplate::New(Getpid)->GetFunction());
}

NODE_MODULE(fork, init);
