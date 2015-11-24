// predictor.h
#ifndef PREDICTOR_H
#define PREDICTOR_H

#include <node.h>
#include <node_object_wrap.h>

#include "dlib/image_processing.h"
#include "dlib/data_io.h"

namespace ObjectDetector {
  class Predictor : public node::ObjectWrap {
   public:
    static void Init(v8::Local<v8::Object> exports);
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void TrainFromXML(const v8::FunctionCallbackInfo<v8::Value>& args);

   private:
    explicit Predictor() {

    }

    explicit Predictor(std::string xmlFile) {
      dlib::deserialize(xmlFile) >> this->dlibShapePredictor;
    }

    static void PredictShapeInRect(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void SaveToFile(const v8::FunctionCallbackInfo<v8::Value>& args);
    static v8::Persistent<v8::Function> constructor;
    dlib::shape_predictor dlibShapePredictor;
  };
}

#endif
