// Copyright 2014 The ChromiumOS Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Note: These tests are not generated. They test generated code.

#include <iterator>
#include <utility>

#include <gtest/gtest.h>

#include "mock_authorization_delegate.h"
#include "mock_command_transceiver.h"
#include "tpm_generated.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;
using testing::WithArg;

namespace trunks {

// This test is designed to get good coverage of the different types of code
// generated for serializing and parsing structures / unions / typedefs.
TEST(GeneratorTest, SerializeParseStruct) {
  TPM2B_CREATION_DATA data;
  memset(&data, 0, sizeof(TPM2B_CREATION_DATA));
  data.size = sizeof(TPMS_CREATION_DATA);
  data.creation_data.pcr_select.count = 1;
  data.creation_data.pcr_select.pcr_selections[0].hash = TPM_ALG_SHA256;
  data.creation_data.pcr_select.pcr_selections[0].sizeof_select = 1;
  data.creation_data.pcr_select.pcr_selections[0].pcr_select[0] = 0;
  data.creation_data.pcr_digest.size = 2;
  data.creation_data.locality = 0;
  data.creation_data.parent_name_alg = TPM_ALG_SHA256;
  data.creation_data.parent_name.size = 3;
  data.creation_data.parent_qualified_name.size = 4;
  data.creation_data.outside_info.size = 5;
  std::string buffer;
  TPM_RC rc = Serialize_TPM2B_CREATION_DATA(data, &buffer);
  ASSERT_EQ(TPM_RC_SUCCESS, rc);
  EXPECT_EQ(35u, buffer.size());
  TPM2B_CREATION_DATA data2;
  memset(&data2, 0, sizeof(TPM2B_CREATION_DATA));
  std::string buffer_before = buffer;
  std::string buffer_parsed;
  rc = Parse_TPM2B_CREATION_DATA(&buffer, &data2, &buffer_parsed);
  ASSERT_EQ(TPM_RC_SUCCESS, rc);
  EXPECT_EQ(0u, buffer.size());
  EXPECT_EQ(buffer_before, buffer_parsed);
  EXPECT_EQ(data.size, data2.size);
  EXPECT_EQ(0, memcmp(&data.creation_data, &data2.creation_data,
                      sizeof(TPMS_CREATION_DATA)));
}

// This tests serializing and parsing TPM2B_ structures with zero |size|, in
// which case the enclosed structure isn't marshalled.
TEST(GeneratorTest, SerializeParseEmptyStruct) {
  TPM2B_CREATION_DATA data;
  memset(&data, 0, sizeof(TPM2B_CREATION_DATA));
  std::string buffer;
  TPM_RC rc = Serialize_TPM2B_CREATION_DATA(data, &buffer);
  ASSERT_EQ(TPM_RC_SUCCESS, rc);
  EXPECT_EQ(2u, buffer.size());
  TPM2B_CREATION_DATA data2;
  memset(&data2, 0, sizeof(TPM2B_CREATION_DATA));
  std::string buffer_before = buffer;
  std::string buffer_parsed;
  rc = Parse_TPM2B_CREATION_DATA(&buffer, &data2, &buffer_parsed);
  ASSERT_EQ(TPM_RC_SUCCESS, rc);
  EXPECT_EQ(0u, buffer.size());
  EXPECT_EQ(buffer_before, buffer_parsed);
  EXPECT_EQ(data.size, data2.size);
  EXPECT_EQ(0, memcmp(&data.creation_data, &data2.creation_data,
                      sizeof(TPMS_CREATION_DATA)));
}

TEST(GeneratorTest, SerializeBufferOverflow) {
  TPM2B_MAX_BUFFER value;
  value.size = std::size(value.buffer) + 1;
  std::string tmp;
  EXPECT_EQ(TPM_RC_INSUFFICIENT, Serialize_TPM2B_MAX_BUFFER(value, &tmp));
}

TEST(GeneratorTest, ParseBufferOverflow) {
  TPM2B_MAX_BUFFER tmp;
  // Case 1: Sufficient source but overflow the destination.
  std::string malformed1 = "\x10\x00";
  malformed1 += std::string(0x1000, 'A');
  ASSERT_GT(0x1000u, sizeof(tmp.buffer));
  EXPECT_EQ(TPM_RC_INSUFFICIENT,
            Parse_TPM2B_MAX_BUFFER(&malformed1, &tmp, nullptr));
  // Case 2: Sufficient destination but overflow the source.
  std::string malformed2 = "\x00\x01";
  EXPECT_EQ(TPM_RC_INSUFFICIENT,
            Parse_TPM2B_MAX_BUFFER(&malformed2, &tmp, nullptr));
}

TEST(GeneratorTest, SynchronousCommand) {
  // A hand-rolled TPM2_Startup command.
  std::string expected_command(
      "\x80\x01"          // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0C"  // size=12
      "\x00\x00\x01\x44"  // code=TPM_CC_Startup
      "\x00\x00",         // param=TPM_SU_CLEAR
      12);
  std::string command_response(
      "\x80\x01"           // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0A"   // size=10
      "\x00\x00\x00\x00",  // code=TPM_RC_SUCCESS
      10);
  StrictMock<MockCommandTransceiver> transceiver;
  EXPECT_CALL(transceiver, SendCommandAndWait(expected_command))
      .WillOnce(Return(command_response));
  StrictMock<MockAuthorizationDelegate> authorization;
  EXPECT_CALL(authorization, GetCommandAuthorization(_, _, _, _))
      .WillOnce(Return(true));
  Tpm tpm(&transceiver);
  EXPECT_EQ(TPM_RC_SUCCESS, tpm.StartupSync(TPM_SU_CLEAR, &authorization));
}

TEST(GeneratorTest, SynchronousCommandWithError) {
  // A hand-rolled TPM2_Startup command.
  std::string expected_command(
      "\x80\x01"          // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0C"  // size=12
      "\x00\x00\x01\x44"  // code=TPM_CC_Startup
      "\x00\x00",         // param=TPM_SU_CLEAR
      12);
  std::string command_response(
      "\x80\x01"           // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0A"   // size=10
      "\x00\x00\x01\x01",  // code=TPM_RC_FAILURE
      10);
  StrictMock<MockCommandTransceiver> transceiver;
  EXPECT_CALL(transceiver, SendCommandAndWait(expected_command))
      .WillOnce(Return(command_response));
  StrictMock<MockAuthorizationDelegate> authorization;
  EXPECT_CALL(authorization, GetCommandAuthorization(_, _, _, _))
      .WillOnce(Return(true));
  Tpm tpm(&transceiver);
  EXPECT_EQ(TPM_RC_FAILURE, tpm.StartupSync(TPM_SU_CLEAR, &authorization));
}

TEST(GeneratorTest, SynchronousCommandResponseTest) {
  std::string auth_in(10, 'A');
  std::string auth_out(10, 'B');
  std::string auth_size("\x00\x00\x00\x0A", 4);
  std::string handle_in("\x40\x00\x00\x07", 4);  // primary_handle = TPM_RH_NULL
  std::string handle_out("\x80\x00\x00\x01", 4);  // out_handle
  std::string sensitive(
      "\x00\x05"   // sensitive.size = 5
      "\x00\x01"   // sensitive.auth.size = 1
      "\x61"       // sensitive.auth.buffer[0] = 0x65
      "\x00\x00",  // sensitive.data.size = 0
      7);
  std::string public_data(
      "\x00\x12"  // public.size = 18
      "\x00\x25"  // public.type = TPM_ALG_SYMCIPHER
      "\x00\x0B"  // public.name_alg = SHA256
      "\x00\x00\x00\x00"
      "\x00\x00"   // public.auth_policy.size = 0
      "\x00\x06"   // public.sym.alg = TPM_ALG_AES
      "\x00\x80"   // public.sym.key_bits = 128
      "\x00\x43"   // public.sym.mode = TPM_ALG_CFB
      "\x00\x00",  // public.unique.size = 0
      20);
  std::string outside("\x00\x00", 2);             // outside_info.size = 0
  std::string pcr_select("\x00\x00\x00\x00", 4);  // pcr_select.size = 0

  std::string data(
      "\x00\x0F"          // creation_data.size = 15
      "\x00\x00\x00\x00"  // creation.pcr = 0
      "\x00\x00"          // creation.digest.size = 0
      "\x00"              // creation.locality = 0
      "\x00\x00"          // creation.parent_alg = 0
      "\x00\x00"          // creation.parent_name.size = 0
      "\x00\x00"
      "\x00\x00",  // creation.outside.size = 0
      17);
  std::string hash(
      "\x00\x01"
      "\x62",
      3);
  std::string ticket(
      "\x80\x02"          // tag = TPM_ST_SESSIONS
      "\x40\x00\x00\x07"  // parent = TPM_RH_NULL
      "\x00\x00",
      8);
  std::string name(
      "\x00\x03"
      "KEY",
      5);
  std::string parameter_size("\x00\x00\x00\x35", 4);  // param_size = 38

  std::string command_tag(
      "\x80\x02"           // tag = TPM_ST_SESSIONS
      "\x00\x00\x00\x3D"   // size = 61
      "\x00\x00\x01\x31",  // code = TPM_CC_CreatePrimary
      10);
  std::string response_tag(
      "\x80\x02"           // tag = TPM_ST_SESSIONS
      "\x00\x00\x00\x51"   // size = 79
      "\x00\x00\x00\x00",  // rc = TPM_RC_SUCCESS
      10);

  std::string expected_command = command_tag + handle_in + auth_size + auth_in +
                                 sensitive + public_data + outside + pcr_select;
  std::string command_response = response_tag + handle_out + parameter_size +
                                 public_data + data + hash + ticket + name +
                                 auth_out;

  StrictMock<MockCommandTransceiver> transceiver;
  EXPECT_CALL(transceiver, SendCommandAndWait(expected_command))
      .WillOnce(Return(command_response));
  StrictMock<MockAuthorizationDelegate> authorization;
  EXPECT_CALL(authorization, GetCommandAuthorization(_, _, _, _))
      .WillOnce(DoAll(SetArgPointee<3>(auth_in), Return(true)));
  EXPECT_CALL(authorization, CheckResponseAuthorization(_, auth_out))
      .WillOnce(Return(true));
  EXPECT_CALL(authorization, EncryptCommandParameter(_)).WillOnce(Return(true));
  EXPECT_CALL(authorization, DecryptResponseParameter(_))
      .WillOnce(Return(true));

  TPM2B_SENSITIVE_CREATE in_sensitive;
  in_sensitive.size = sizeof(TPMS_SENSITIVE_CREATE);
  in_sensitive.sensitive.user_auth.size = 1;
  in_sensitive.sensitive.user_auth.buffer[0] = 'a';
  in_sensitive.sensitive.data.size = 0;
  TPM2B_PUBLIC in_public;
  in_public.size = sizeof(TPMT_PUBLIC);
  in_public.public_area.type = TPM_ALG_SYMCIPHER;
  in_public.public_area.name_alg = TPM_ALG_SHA256;
  in_public.public_area.object_attributes = 0;
  in_public.public_area.auth_policy.size = 0;
  in_public.public_area.parameters.sym_detail.sym.algorithm = TPM_ALG_AES;
  in_public.public_area.parameters.sym_detail.sym.key_bits.aes = 128;
  in_public.public_area.parameters.sym_detail.sym.mode.aes = TPM_ALG_CFB;
  in_public.public_area.unique.sym.size = 0;
  TPM2B_DATA outside_info;
  outside_info.size = 0;
  TPML_PCR_SELECTION create_pcr;
  create_pcr.count = 0;

  TPM_HANDLE key_handle;
  TPM2B_PUBLIC out_public;
  TPM2B_CREATION_DATA creation_data;
  TPM2B_DIGEST creation_hash;
  TPMT_TK_CREATION creation_ticket;
  TPM2B_NAME key_name;

  Tpm tpm(&transceiver);
  TPM_RC rc = tpm.CreatePrimarySync(
      trunks::TPM_RH_NULL, "", in_sensitive, in_public, outside_info,
      create_pcr, &key_handle, &out_public, &creation_data, &creation_hash,
      &creation_ticket, &key_name, &authorization);
  ASSERT_EQ(rc, TPM_RC_SUCCESS);
  EXPECT_EQ(key_handle, 0x80000001);
  EXPECT_EQ(out_public.size, sizeof(TPMT_PUBLIC));
  EXPECT_EQ(creation_data.size, sizeof(TPMS_CREATION_DATA));
  EXPECT_EQ(creation_hash.size, 1);
  EXPECT_EQ(creation_hash.buffer[0], 'b');
  EXPECT_EQ(creation_ticket.tag, 0x8002);
  EXPECT_EQ(creation_ticket.hierarchy, 0x40000007u);
  EXPECT_EQ(creation_ticket.digest.size, 0);
  EXPECT_EQ(key_name.size, 3);
  EXPECT_EQ(key_name.name[0], 'K');
  EXPECT_EQ(key_name.name[1], 'E');
  EXPECT_EQ(key_name.name[2], 'Y');
}

// A fixture for asynchronous command flow tests.
class CommandFlowTest : public testing::Test {
 public:
  CommandFlowTest() : response_code_(TPM_RC_SUCCESS) {}
  ~CommandFlowTest() override {}

  void StartupCallback(TPM_RC response_code) { response_code_ = response_code; }

  void CertifyCallback(TPM_RC response_code,
                       const TPM2B_ATTEST& certify_info,
                       const TPMT_SIGNATURE& signature) {
    response_code_ = response_code;
    signed_data_ = StringFrom_TPM2B_ATTEST(certify_info);
    signature_ =
        StringFrom_TPM2B_PUBLIC_KEY_RSA(signature.signature.rsassa.sig);
  }

 protected:
  void Run() {}

  TPM_RC response_code_;
  std::string signature_;
  std::string signed_data_;
};

// A functor for posting command responses.
class PostResponse {
 public:
  explicit PostResponse(const std::string& response) : response_(response) {}
  void operator()(base::OnceCallback<void(const std::string&)> callback) {
    std::move(callback).Run(response_);
  }

 private:
  std::string response_;
};

// A functor to handle fake encryption / decryption of parameters.
class Encryptor {
 public:
  Encryptor(const std::string& expected_input, const std::string& output)
      : expected_input_(expected_input), output_(output) {}
  bool operator()(std::string* value) {
    EXPECT_EQ(expected_input_, *value);
    value->assign(output_);
    return true;
  }

 private:
  std::string expected_input_;
  std::string output_;
};

TEST_F(CommandFlowTest, SimpleCommandFlow) {
  // A hand-rolled TPM2_Startup command.
  std::string expected_command(
      "\x80\x01"          // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0C"  // size=12
      "\x00\x00\x01\x44"  // code=TPM_CC_Startup
      "\x00\x00",         // param=TPM_SU_CLEAR
      12);
  std::string command_response(
      "\x80\x01"           // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0A"   // size=10
      "\x00\x00\x00\x00",  // code=TPM_RC_SUCCESS
      10);
  StrictMock<MockCommandTransceiver> transceiver;
  EXPECT_CALL(transceiver, SendCommand(expected_command, _))
      .WillOnce(WithArg<1>(Invoke(PostResponse(command_response))));
  StrictMock<MockAuthorizationDelegate> authorization;
  EXPECT_CALL(authorization, GetCommandAuthorization(_, _, _, _))
      .WillOnce(Return(true));
  Tpm tpm(&transceiver);
  response_code_ = TPM_RC_FAILURE;
  tpm.Startup(TPM_SU_CLEAR, &authorization,
              std::function<void(TPM_RC)>([this](TPM_RC response_code) {
                this->StartupCallback(response_code);
              }));
  Run();
  EXPECT_EQ(TPM_RC_SUCCESS, response_code_);
}

TEST_F(CommandFlowTest, SimpleCommandFlowWithError) {
  // A hand-rolled TPM2_Startup command.
  std::string expected_command(
      "\x80\x01"          // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0C"  // size=12
      "\x00\x00\x01\x44"  // code=TPM_CC_Startup
      "\x00\x00",         // param=TPM_SU_CLEAR
      12);
  std::string command_response(
      "\x80\x01"           // tag=TPM_ST_NO_SESSIONS
      "\x00\x00\x00\x0A"   // size=10
      "\x00\x00\x01\x01",  // code=TPM_RC_FAILURE
      10);
  StrictMock<MockCommandTransceiver> transceiver;
  EXPECT_CALL(transceiver, SendCommand(expected_command, _))
      .WillOnce(WithArg<1>(Invoke(PostResponse(command_response))));
  StrictMock<MockAuthorizationDelegate> authorization;
  EXPECT_CALL(authorization, GetCommandAuthorization(_, _, _, _))
      .WillOnce(Return(true));
  Tpm tpm(&transceiver);
  tpm.Startup(TPM_SU_CLEAR, &authorization,
              std::function<void(TPM_RC)>([this](TPM_RC response_code) {
                this->StartupCallback(response_code);
              }));
  Run();
  EXPECT_EQ(TPM_RC_FAILURE, response_code_);
}

// This test is designed to get good coverage of the different types of code
// generated for command / response processing. It covers:
// - input handles
// - authorization
// - multiple input and output parameters
// - parameter encryption and decryption
TEST_F(CommandFlowTest, FullCommandFlow) {
  // A hand-rolled TPM2_Certify command.
  std::string auth_in(10, 'A');
  std::string auth_out(20, 'B');
  std::string user_data(
      "\x00\x0C"
      "ct_user_data",
      14);
  std::string scheme("\x00\x10", 2);  // scheme=TPM_ALG_NULL
  std::string signed_data(
      "\x00\x0E"
      "ct_signed_data",
      16);
  std::string signature(
      "\x00\x14"    // sig_scheme=RSASSA
      "\x00\x0B"    // hash_scheme=SHA256
      "\x00\x09"    // signature size
      "signature",  // signature bytes
      15);
  std::string expected_command(
      "\x80\x02"           // tag=TPM_ST_SESSIONS
      "\x00\x00\x00\x30"   // size=48
      "\x00\x00\x01\x48"   // code=TPM_CC_Certify
      "\x11\x22\x33\x44"   // @objectHandle
      "\x55\x66\x77\x88"   // @signHandle
      "\x00\x00\x00\x0A",  // auth_size=10
      22);
  expected_command += auth_in + user_data + scheme;
  std::string command_response(
      "\x80\x02"           // tag=TPM_ST_SESSIONS
      "\x00\x00\x00\x41"   // size=65
      "\x00\x00\x00\x00"   // code=TPM_RC_SUCCESS
      "\x00\x00\x00\x1F",  // param_size=31
      14);
  command_response += signed_data + signature + auth_out;

  StrictMock<MockCommandTransceiver> transceiver;
  EXPECT_CALL(transceiver, SendCommand(expected_command, _))
      .WillOnce(WithArg<1>(Invoke(PostResponse(command_response))));
  StrictMock<MockAuthorizationDelegate> authorization;
  EXPECT_CALL(authorization, GetCommandAuthorization(_, _, _, _))
      .WillOnce(DoAll(SetArgPointee<3>(auth_in), Return(true)));
  EXPECT_CALL(authorization, CheckResponseAuthorization(_, auth_out))
      .WillOnce(Return(true));
  EXPECT_CALL(authorization, EncryptCommandParameter(_))
      .WillOnce(Invoke(Encryptor("pt_user_data", "ct_user_data")));
  EXPECT_CALL(authorization, DecryptResponseParameter(_))
      .WillOnce(Invoke(Encryptor("ct_signed_data", "pt_signed_data")));

  TPMT_SIG_SCHEME null_scheme;
  null_scheme.scheme = TPM_ALG_NULL;
  null_scheme.details.rsassa.hash_alg = TPM_ALG_SHA256;
  Tpm tpm(&transceiver);
  tpm.Certify(
      0x11223344u, "object_handle", 0x55667788u, "sign_handle",
      Make_TPM2B_DATA("pt_user_data"), null_scheme, &authorization,
      std::function<void(TPM_RC, const TPM2B_ATTEST&, const TPMT_SIGNATURE&)>(
          [this](TPM_RC response_code, const TPM2B_ATTEST& certify_info,
                 const TPMT_SIGNATURE& signature) {
            this->CertifyCallback(response_code, certify_info, signature);
          }));
  Run();
  ASSERT_EQ(TPM_RC_SUCCESS, response_code_);
  EXPECT_EQ("pt_signed_data", signed_data_);
  EXPECT_EQ("signature", signature_);
}

}  // namespace trunks
