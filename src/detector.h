// detector.h
#ifndef DETECTOR_H
#define DETECTOR_H

#include <node.h>
#include <node_object_wrap.h>

#include "dlib/image_processing.h"
#include "dlib/image_processing/frontal_face_detector.h"
#include "dlib/data_io.h"

namespace ObjectDetector {
  typedef dlib::scan_fhog_pyramid<dlib::pyramid_down<6> > image_scanner_type;

  class Detector : public node::ObjectWrap {
   public:
    static void Init(v8::Local<v8::Object> exports);
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

   private:
    explicit Detector() {
      this->dlibObjectDetector = dlib::get_frontal_face_detector();
    }

    explicit Detector(std::string xmlFile) {
      dlib::deserialize(xmlFile) >> this->dlibObjectDetector;
    }

    static void DetectInImageFile(const v8::FunctionCallbackInfo<v8::Value>& args);
    static v8::Persistent<v8::Function> constructor;
    dlib::object_detector<image_scanner_type> dlibObjectDetector;
  };
}

#endif
