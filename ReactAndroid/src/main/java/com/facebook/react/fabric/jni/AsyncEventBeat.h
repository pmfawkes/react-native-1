/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <jsi/jsi.h>
#include <react/core/EventBeat.h>
#include <react/uimanager/primitives.h>

#include "EventBeatManager.h"

namespace facebook {
namespace react {

namespace {

class AsyncEventBeat : public EventBeat {
 private:
  EventBeatManager* eventBeatManager_;
  RuntimeExecutor runtimeExecutor_;
  jni::global_ref<jobject> javaUIManager_;

 public:
  friend class EventBeatManager;

  AsyncEventBeat(
      EventBeat::SharedOwnerBox const &ownerBox,
      EventBeatManager* eventBeatManager,
      RuntimeExecutor runtimeExecutor,
      jni::global_ref<jobject> javaUIManager) :
      EventBeat(ownerBox),
      eventBeatManager_(eventBeatManager),
      runtimeExecutor_(std::move(runtimeExecutor)),
      javaUIManager_(javaUIManager) {
    eventBeatManager->registerEventBeat(this);
  }

  ~AsyncEventBeat() {
    eventBeatManager_->unregisterEventBeat(this);
  }

  void induce() const override {
    runtimeExecutor_([this, ownerBox = ownerBox_](jsi::Runtime &runtime) {
      auto owner = ownerBox->owner.lock();
      if (!owner) {
        return;
      }

      this->beat(runtime);
    });
  }

  void request() const override {
    bool alreadyRequested = isRequested_;
    EventBeat::request();
    if (!alreadyRequested) {
      // Notifies java side that an event will be dispatched (e.g. LayoutEvent)
      static auto onRequestEventBeat =
          jni::findClassStatic("com/facebook/react/fabric/FabricUIManager")
              ->getMethod<void()>("onRequestEventBeat");
      onRequestEventBeat(javaUIManager_);
    }
  }
};

} // namespace

} // namespace react
} // namespace facebook
