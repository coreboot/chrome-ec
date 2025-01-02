// Copyright 2024 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TPM_GENERATED_CALLBACK_H_
#define TPM_GENERATED_CALLBACK_H_

// tpm_generated was designed to use libchrome, but we can't use libchrome in
// AOSP. This implements just enough of libchrome's callback functionality to
// make tpm_generated work.

#include <functional>

namespace trunks {

template <typename Signature>
class OnceCallback;

template <typename R, typename... Args>
class OnceCallback<R(Args...)> {
 public:
  OnceCallback(std::function<R(Args...)> function) : inner_(function) {}

  R Run(Args... args) && { return inner_(args...); }

 private:
  std::function<R(Args...)> inner_;
};

template <typename R, typename Arg1, typename... Remaining>
OnceCallback<R(Remaining...)> BindOnce(R (*callback)(Arg1, Remaining...),
                                       Arg1 arg1) {
  return OnceCallback<R(Remaining...)>(
      [callback, arg1](Remaining... remaining) {
        return callback(arg1, remaining...);
      });
}

template <typename R, typename Arg1, typename Arg2, typename... Remaining>
OnceCallback<R(Remaining...)> BindOnce(R (*callback)(Arg1, Arg2, Remaining...),
                                       Arg1 arg1, Arg2 arg2) {
  return OnceCallback<R(Remaining...)>(
      [callback, arg1, arg2](Remaining... remaining) {
        return callback(arg1, arg2, remaining...);
      });
}

}  // namespace trunks

#endif  // TPM_GENERATED_CALLBACK_H_
