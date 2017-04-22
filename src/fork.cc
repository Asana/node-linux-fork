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

void Fork(const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  int res = -1;
  if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsString()) {
    isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(
      isolate,
      "Ivalid arguments for fork: (stdin_filename, stout_filename) expected")));
    return;
  }
  v8::String::Utf8Value stdin_filename(info[0]);
  v8::String::Utf8Value stdout_filename(info[1]);
  {
    v8::Unlocker unlocker(isolate);
    start_sigchld_handler();
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
  }

  info.GetReturnValue().Set(res);
}

void Getpid(const v8::FunctionCallbackInfo<v8::Value>& info) {
  return info.GetReturnValue().Set(getpid());
}

void init(v8::Local<v8::Object> exports) {
  NODE_SET_METHOD(exports, "forkAndReopenStdio", Fork);
  NODE_SET_METHOD(exports, "getpid", Getpid);
}

NODE_MODULE(fork, init);
