/*
 * Copyright (c) 2013, Ford Motor Company
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

#ifndef CONNECTION_TEST_H
#define CONNECTION_TEST_H

#include <gtest/gtest.h>
#include <algorithm>
#include "connection_handler/connection.h"
#include "connection_handler/connection_handler_impl.h"
#include "protocol_handler/service_type.h"
#include "utils/shared_ptr.h"

using namespace connection_handler;

namespace test {
namespace components {
namespace connection_handle {

class ConnectionTest: public ::testing::Test {
 protected:
  void SetUp() OVERRIDE {
    connection_handler_ = ConnectionHandlerImpl::instance();
    const ConnectionHandle connectionHandle = 0;
    const DeviceHandle device_handle = 0;
    connection_.reset(new Connection(connectionHandle, device_handle,
                                     connection_handler_, 10000));
  }

  void TearDown() OVERRIDE {
    connection_.reset();
    ConnectionHandlerImpl::destroy();
  }
  void StartSession() {
    session_id = connection_->AddNewSession();
    EXPECT_NE(session_id, -1);
    const SessionMap sessionMap = connection_->session_map();
    EXPECT_FALSE(sessionMap.empty());
    const ServiceList serviceList = sessionMap.begin()->second.service_list;
    EXPECT_FALSE(serviceList.empty());
    const ServiceList::const_iterator it =
        std::find(serviceList.begin(), serviceList.end(), protocol_handler::kRpc);
    EXPECT_NE(it, serviceList.end());
  }
  void AddNewService(const protocol_handler::ServiceType service_type,
                    const bool protection,
                    const bool expect_add_new_service_call_result,
                    const bool expect_exist_service) {
    const bool result = connection_->
        AddNewService(session_id, service_type, protection);
    EXPECT_EQ(result, expect_add_new_service_call_result);

    const SessionMap newSessionMap = connection_->session_map();
    EXPECT_FALSE(newSessionMap.empty());
    const ServiceList newServiceList = newSessionMap.begin()->second.service_list;
    EXPECT_FALSE(newServiceList.empty());
    const ServiceList::const_iterator it =
        std::find(newServiceList.begin(), newServiceList.end(), service_type);
    const bool found_result = it != newServiceList.end();
    EXPECT_EQ(expect_exist_service, found_result);
    if(found_result) {
      const Service& service = *it;
      EXPECT_EQ(service.is_protected_, protection);
      }
  }

  void RemoveService(const protocol_handler::ServiceType service_type,
                     const bool expect_remove_service_result,
                     const bool expect_exist_service) {
    const bool result = connection_->
        RemoveService(session_id, service_type);
    EXPECT_EQ(result, expect_remove_service_result);

    const SessionMap newSessionMap = connection_->session_map();
    EXPECT_FALSE(newSessionMap.empty());
    const ServiceList newServiceList = newSessionMap.begin()->second.service_list;
    EXPECT_FALSE(newServiceList.empty());
    const ServiceList::const_iterator it =
        std::find(newServiceList.begin(), newServiceList.end(), service_type);
    const bool found_result = it != newServiceList.end();
    EXPECT_EQ(expect_exist_service, found_result);
  }

  ::utils::SharedPtr<Connection> connection_;
  ConnectionHandlerImpl* connection_handler_;
  int32_t session_id;
  const bool encrypted = true;
  const bool unencrypted = false;
  const bool return_result_true = true;
  const bool return_result_false = false;
  const bool expect_service_exist = true;
  const bool expect_service_not_exist = false;
};

// Try to add service without session
TEST_F(ConnectionTest, Session_AddNewServiceWithoySession) {
  EXPECT_EQ(connection_->
            AddNewService(session_id, protocol_handler::kAudio, true),
            return_result_false);
  EXPECT_EQ(connection_->
            AddNewService(session_id, protocol_handler::kAudio, false),
            return_result_false);
  EXPECT_EQ(connection_->
            AddNewService(session_id, protocol_handler::kMobileNav, true),
            return_result_false);
  EXPECT_EQ(connection_->
            AddNewService(session_id, protocol_handler::kMobileNav, false),
            return_result_false);
}
// Try to remove service without session
TEST_F(ConnectionTest, Session_RemoveServiceWithoutSession) {
  EXPECT_EQ(connection_->
            RemoveService(session_id, protocol_handler::kAudio),
            return_result_false);
  EXPECT_EQ(connection_->
            RemoveService(session_id, protocol_handler::kMobileNav),
            return_result_false);
}
// Try to remove RPC
TEST_F(ConnectionTest, Session_RemoveRPCBulk) {
  StartSession();
  EXPECT_EQ(connection_->
            RemoveService(session_id, protocol_handler::kRpc),
            return_result_false);
  EXPECT_EQ(connection_->
            RemoveService(session_id, protocol_handler::kBulk),
            return_result_false);
}
// Control Service couldnot be started anyway
TEST_F(ConnectionTest, Session_AddControlService) {
  StartSession();

  AddNewService(protocol_handler::kControl, unencrypted,
               return_result_false,
               expect_service_not_exist);
  AddNewService(protocol_handler::kControl, encrypted,
               return_result_false,
               expect_service_not_exist);
}

// Invalid Services couldnot be started anyway
TEST_F(ConnectionTest, Session_AddInvalideService) {
  StartSession();

  AddNewService(protocol_handler::kInvalidServiceType, unencrypted,
               return_result_false,
               expect_service_not_exist);
  AddNewService(protocol_handler::kInvalidServiceType, encrypted,
               return_result_false,
               expect_service_not_exist);
}

// RPC and Bulk Services could be only delay protected
TEST_F(ConnectionTest, Session_AddRPCBulkServices) {
  StartSession();

  AddNewService(protocol_handler::kRpc, unencrypted,
               return_result_false,
               expect_service_exist);

  //Bulk shall not be added and shall be unencrypted
  AddNewService(protocol_handler::kBulk, unencrypted,
               return_result_false,
               expect_service_exist);

  AddNewService(protocol_handler::kRpc, encrypted,
               return_result_true,
               expect_service_exist);

  //Bulk shall not be added and shall be encrypted
  AddNewService(protocol_handler::kBulk, encrypted,
               return_result_false,
               expect_service_exist);
}
TEST_F(ConnectionTest, Session_AddAllOtherService_Unprotected) {
  StartSession();

  AddNewService(protocol_handler::kAudio, unencrypted,
               return_result_true,
               expect_service_exist);
  AddNewService(protocol_handler::kMobileNav, unencrypted,
               return_result_true,
               expect_service_exist);
}

TEST_F(ConnectionTest, Session_AddAllOtherService_Protected) {
  StartSession();

  AddNewService(protocol_handler::kAudio, encrypted,
               return_result_true,
               expect_service_exist);
  AddNewService(protocol_handler::kMobileNav, encrypted,
               return_result_true,
               expect_service_exist);
}

TEST_F(ConnectionTest, Session_AddAllOtherService_DelayProtected1) {
  StartSession();

  AddNewService(protocol_handler::kAudio, unencrypted,
               return_result_true,
               expect_service_exist);
  AddNewService(protocol_handler::kMobileNav, unencrypted,
               return_result_true,
               expect_service_exist);


  AddNewService(protocol_handler::kAudio, encrypted,
               return_result_true,
               expect_service_exist);
  AddNewService(protocol_handler::kMobileNav, encrypted,
               return_result_true,
               expect_service_exist);
}
//Use other order
TEST_F(ConnectionTest, Session_AddAllOtherService_DelayProtected2) {
  StartSession();

  AddNewService(protocol_handler::kAudio, unencrypted,
               return_result_true,
               expect_service_exist);
  AddNewService(protocol_handler::kAudio, encrypted,
               return_result_true,
               expect_service_exist);

  AddNewService(protocol_handler::kMobileNav, unencrypted,
               return_result_true,
               expect_service_exist);
  AddNewService(protocol_handler::kMobileNav, encrypted,
               return_result_true,
               expect_service_exist);
}

} // connection_handle
} // namespace components
} // namespace test

#endif // CONNECTION_TEST_H