// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRUNKS_COMMAND_TRANSCEIVER_H_
#define TRUNKS_COMMAND_TRANSCEIVER_H_

#include <string>
#include <utility>

#include <base/functional/callback.h>

namespace trunks {

// CommandTransceiver is an interface that sends commands to a TPM device and
// receives responses. It can operate synchronously or asynchronously.
class CommandTransceiver {
 public:
  typedef base::OnceCallback<void(const std::string& response)>
      ResponseCallback;

  virtual ~CommandTransceiver() {}

  // Sends a TPM |command| asynchronously. When a |response| is received,
  // |callback| will be called with the |response| data from the TPM. If a
  // transmission error occurs |callback| will be called with a well-formed
  // error |response|.
  virtual void SendCommand(const std::string& command,
                           ResponseCallback callback) = 0;

  // Sends a TPM |command| synchronously (i.e. waits for a response) and returns
  // the response. If a transmission error occurs the response will be populated
  // with a well-formed error response.
  virtual std::string SendCommandAndWait(const std::string& command) = 0;

  // Similar to the SendCommand, but we add an extra sender information.
  // By default, it will fallback to the normal implementation.
  virtual void SendCommandWithSender(const std::string& command,
                                     uint64_t sender,
                                     ResponseCallback callback) {
    return SendCommand(command, std::move(callback));
  }

  // Similar to the SendCommandAndWait, but we add an extra sender information.
  // By default, it will fallback to the normal implementation.
  virtual std::string SendCommandWithSenderAndWait(const std::string& command,
                                                   uint64_t sender) {
    return SendCommandAndWait(command);
  }

  // Initializes the actual interface, replaced by the derived classes, where
  // needed.
  virtual bool Init() { return true; }
};

}  // namespace trunks

#endif  // TRUNKS_COMMAND_TRANSCEIVER_H_
