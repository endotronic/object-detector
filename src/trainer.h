// trainer.h
#ifndef TRAINER_H
#define TRAINER_H

#include <node.h>
#include <node_object_wrap.h>

#include "dlib/image_processing.h"
#include "dlib/data_io.h"

#include "detector.h"

namespace ObjectDetector {
  typedef dlib::scan_fhog_pyramid<dlib::pyramid_down<6> > image_scanner_type;

  class Trainer : public node::ObjectWrap {
   public:
    static void Init(v8::Local<v8::Object> exports);
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

   private:
    explicit Trainer() {

    }

    static v8::Persistent<v8::Function> constructor;
  };
}

#endif
