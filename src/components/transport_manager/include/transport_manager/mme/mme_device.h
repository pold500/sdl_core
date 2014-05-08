/*
 * Copyright (c) 2014, Ford Motor Company
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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_DEVICE_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_DEVICE_H_

#include "utils/shared_ptr.h"
#include "transport_manager/transport_adapter/device.h"
#include "utils/threads/thread_delegate.h"
#include "utils/threads/thread.h"
#include "map"
#include "vector"
#include <iap2/iap2.h>

namespace transport_manager {
namespace transport_adapter {

typedef int appId;

void fillIap2Protocols();

class MmeDevice : public Device {
 public:
  typedef enum {UnknownProtocol, IAP, IAP2} Protocol;

  MmeDevice(const std::string& mount_point,
            Protocol protocol,
            const std::string& name,
            const DeviceUID& unique_device_id);

  const std::string& mount_point() const {
    return mount_point_;
  }

  Protocol protocol() const {
    return protocol_;
  }

 protected:
  virtual bool IsSameAs(const Device* other_device) const;
  virtual ApplicationList GetApplicationList() const;

 private:
  void on_iap2SessionReady(iap2ea_hdl_t* handler);

  std::string mount_point_;
  Protocol protocol_;
  int last_used_app_id_;
  std::map<appId, iap2ea_hdl_t*> iap2ea_handlers_;
  std::vector<utils::SharedPtr<threads::Thread> > threads_;

  class iap2_connect_thread : public threads::ThreadDelegate {
    public:
      iap2_connect_thread(MmeDevice* parent, std::string protocol_name): parent_(parent), protocol_name_(protocol_name) {}
      void threadMain();
    private:
      MmeDevice* parent_;
      std::string protocol_name_;
  };
};

typedef utils::SharedPtr<MmeDevice> MmeDevicePtr;

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_DEVICE_H_
