// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_LIB_UI_PAINTING_MUTLI_FRAME_CODEC_H_
#define FLUTTER_LIB_UI_PAINTING_MUTLI_FRAME_CODEC_H_

#include "flutter/fml/macros.h"
#include "flutter/lib/ui/painting/codec.h"
#include "flutter/lib/ui/painting/image_generator.h"

#include <functional>
#include <memory>
#include <utility>

using tonic::DartPersistentValue;

namespace flutter {

class MultiFrameCodec : public Codec {
 public:
  explicit MultiFrameCodec(std::shared_ptr<ImageGenerator> generator);

  ~MultiFrameCodec() override;

  // |Codec|
  int frameCount() const override;

  // |Codec|
  int repetitionCount() const override;

  // |Codec|
  Dart_Handle getNextFrame(Dart_Handle args) override;

 private:
  // Captures the state shared between the IO and UI task runners.
  //
  // The state is initialized on the UI task runner when the Dart object is
  // created. Decoding occurs on the IO task runner. Since it is possible for
  // the UI object to be collected independently of the IO task runner work,
  // it is not safe for this state to live directly on the MultiFrameCodec.
  // Instead, the MultiFrameCodec creates this object when it is constructed,
  // shares it with the IO task runner's decoding work, and sets the live_
  // member to false when it is destructed.
  struct State : public std::enable_shared_from_this<State> {
    explicit State(std::shared_ptr<ImageGenerator> generator);

    const std::shared_ptr<ImageGenerator> generator_;
    const int frameCount_;
    const int repetitionCount_;
    bool is_impeller_enabled_ = false;

    // The non-const members and functions below here are only read or written
    // to on the IO thread. They are not safe to access or write on the UI
    // thread.
    int nextFrameIndex_;
    // The last decoded frame that's required to decode any subsequent frames.
    std::optional<SkBitmap> lastRequiredFrame_;

    // The rectangle that should be cleared if the previous frame's disposal
    // method was kRestoreBGColor.
    std::optional<SkIRect> restoreBGColorRect_;

    [[nodiscard]] std::pair<sk_sp<DlImage>, std::string> GetNextFrameImage(
        SkBitmap bitmap,
        const fml::WeakPtr<IOManager>& io_manager) const;

    using DecodeCallback =
        std::function<void(std::optional<SkBitmap>, std::string)>;

    void DecodeImage(const fml::RefPtr<fml::TaskRunner>& io_task_runner,
                     DecodeCallback callback);

    void GetNextFrameAndInvokeCallback(
        std::unique_ptr<DartPersistentValue> callback,
        const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
        const fml::RefPtr<fml::TaskRunner>& io_task_runner,
        const fml::WeakPtr<IOManager>& io_manager);

    void OnGetImageAndInvokeCallback(
        const fml::RefPtr<fml::TaskRunner>& ui_task_runner,
        sk_sp<DlImage> dl_image,
        std::string decode_error,
        std::unique_ptr<DartPersistentValue> callback);
  };

  // Shared across the UI and IO task runners.
  std::shared_ptr<State> state_;

  FML_FRIEND_MAKE_REF_COUNTED(MultiFrameCodec);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MultiFrameCodec);
};

}  // namespace flutter

#endif  // FLUTTER_LIB_UI_PAINTING_MUTLI_FRAME_CODEC_H_
