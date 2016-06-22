/*
 * Copyright (c) 2016, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string>
#include <set>

#include "application_manager/commands/mobile/perform_interaction_request.h"

#include "gtest/gtest.h"
#include "utils/shared_ptr.h"
#include "utils/helpers.h"
#include "utils/make_shared.h"
#include "smart_objects/smart_object.h"
#include "utils/custom_string.h"
#include "application_manager/commands/command_request_test.h"
#include "application_manager/smart_object_keys.h"
#include "application_manager/mock_application.h"
#include "application_manager/mock_application_manager.h"
#include "application_manager/mock_message_helper.h"
#include "application_manager/event_engine/event.h"
#include "application_manager/mock_hmi_interface.h"

namespace test {
namespace components {
namespace commands_test {

namespace am = application_manager;
using am::commands::CommandImpl;
using am::ApplicationManager;
using am::commands::MessageSharedPtr;
using am::ApplicationSharedPtr;
using am::MockMessageHelper;
using am::MockHmiInterfaces;
using ::testing::_;
using ::utils::SharedPtr;
using ::testing::Return;
using ::testing::ReturnRef;
using am::commands::PerformInteractionRequest;
using ::test::components::application_manager_test::MockApplication;

namespace strings = ::application_manager::strings;
namespace hmi_response = ::application_manager::hmi_response;

namespace {
const int32_t kCommandId = 1;
const uint32_t kCmdId = 1u;
const uint32_t kConnectionKey = 2u;
}  // namespace

class PerformInteractionRequestTest
    : public CommandRequestTest<CommandsTestMocks::kIsNice> {
 public:
  sync_primitives::Lock lock_;
};

TEST_F(PerformInteractionRequestTest, OnTimeout_VR_GENERIC_ERROR) {
  MessageSharedPtr msg_vr = CreateMessage(smart_objects::SmartType_Map);

  (*msg_vr)[strings::msg_params][strings::interaction_mode] =
      mobile_apis::InteractionMode::BOTH;
  (*msg_vr)[strings::msg_params][strings::result_code] =
      am::mobile_api::Result::GENERIC_ERROR;
  (*msg_vr)[strings::msg_params][strings::success] = false;
  utils::SharedPtr<PerformInteractionRequest> req_vr =
      CreateCommand<PerformInteractionRequest>(msg_vr);
  MockAppPtr mock_app;
  ON_CALL(app_mngr_, application(_)).WillByDefault(Return(mock_app));
  MockHmiInterfaces hmi_interfaces;
  ON_CALL(app_mngr_, hmi_interfaces()).WillByDefault(ReturnRef(hmi_interfaces));
  Event event(hmi_apis::FunctionID::VR_PerformInteraction);
  event.set_smart_object(*msg_vr);

  req_vr->on_event(event);
  req_vr->Init();

  MockMessageHelper* mock_message_helper =
      MockMessageHelper::message_helper_mock();
  EXPECT_CALL(
      *mock_message_helper,
      CreateNegativeResponse(_, _, _, am::mobile_api::Result::GENERIC_ERROR))
      .WillOnce(Return(msg_vr));

  MessageSharedPtr vr_command_result;
  EXPECT_CALL(
      app_mngr_,
      ManageMobileCommand(_, am::commands::Command::CommandOrigin::ORIGIN_SDL))
      .WillOnce(DoAll(SaveArg<0>(&vr_command_result), Return(true)));

  req_vr->onTimeOut();

  EXPECT_EQ(
      (*vr_command_result)[strings::msg_params][strings::success].asBool(),
      false);
  EXPECT_EQ(
      (*vr_command_result)[strings::msg_params][strings::result_code].asInt(),
      static_cast<int32_t>(am::mobile_api::Result::GENERIC_ERROR));
}

TEST_F(PerformInteractionRequestTest, OnEvent_VR_UNSUPPORTED_RESOURCE) {
  MessageSharedPtr msg_vr = CreateMessage(smart_objects::SmartType_Map);
  (*msg_vr)[strings::params][strings::connection_key] = kConnectionKey;
  (*msg_vr)[strings::msg_params][strings::interaction_mode] =
      mobile_apis::InteractionMode::VR_ONLY;
  utils::SharedPtr<PerformInteractionRequest> req_vr =
      CreateCommand<PerformInteractionRequest>(msg_vr);
  req_vr->Init();
  MockAppPtr mock_app = CreateMockApp();
  ON_CALL(app_mngr_, application(_)).WillByDefault(Return(mock_app));
  ON_CALL(*mock_app, app_id()).WillByDefault(Return(1));
  MockHmiInterfaces hmi_interfaces;
  ON_CALL(app_mngr_, hmi_interfaces()).WillByDefault(ReturnRef(hmi_interfaces));
  ON_CALL(hmi_interfaces, GetInterfaceFromFunction(_))
      .WillByDefault(
          Return(am::HmiInterfaces::HMI_INTERFACE_BasicCommunication));
  ON_CALL(hmi_interfaces, GetInterfaceState(_))
      .WillByDefault(Return(am::HmiInterfaces::STATE_AVAILABLE));

  MessageSharedPtr msg = CreateMessage(smart_objects::SmartType_Map);
  (*msg)[strings::params][hmi_response::code] =
      hmi_apis::Common_Result::UNSUPPORTED_RESOURCE;
  (*msg)[strings::params][strings::info] = "info";
  (*msg)[strings::msg_params][strings::cmd_id] = kCommandId;

  am::event_engine::Event event(hmi_apis::FunctionID::VR_PerformInteraction);
  event.set_smart_object(*msg);

  smart_objects::SmartObject* ptr = NULL;
  ON_CALL(*mock_app, FindCommand(kCmdId)).WillByDefault(Return(ptr));
  EXPECT_EQ(NULL, ptr);

  am::CommandsMap commands_map;
  ON_CALL(*mock_app, commands_map())
      .WillByDefault(
          Return(DataAccessor<am::CommandsMap>(commands_map, lock_)));

  MessageSharedPtr vr_command_result;
  EXPECT_CALL(
      app_mngr_,
      ManageMobileCommand(_, am::commands::Command::CommandOrigin::ORIGIN_SDL))
      .WillOnce(Return(true))
      .WillOnce(DoAll(SaveArg<0>(&vr_command_result), Return(true)));

  req_vr->on_event(event);

  EXPECT_EQ(
      (*vr_command_result)[strings::msg_params][strings::success].asBool(),
      true);
  EXPECT_EQ(
      (*vr_command_result)[strings::msg_params][strings::result_code].asInt(),
      static_cast<int32_t>(hmi_apis::Common_Result::SUCCESS));
  if ((*vr_command_result)[strings::msg_params].keyExists(strings::info)) {
    EXPECT_EQ(
        (*vr_command_result)[strings::msg_params][strings::info].asString(),
        (*msg)[strings::msg_params][strings::info].asString());
  }
}

}  // namespace commands_test
}  // namespace components
}  // namespace tests
