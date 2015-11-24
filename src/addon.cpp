// addon.cpp
#include <node.h>
#include "detector.h"
#include "predictor.h"

namespace ObjectDetector {

  using v8::FunctionCallbackInfo;
  using v8::Isolate;
  using v8::Local;
  using v8::Object;
  using v8::String;
  using v8::Value;

  void CreateDetector(const FunctionCallbackInfo<Value>& args) {
    Detector::New(args);
  }

  void TrainFromXML(const FunctionCallbackInfo<Value>& args) {
    Detector::TrainFromXML(args);
  }

  void TrainPredictorFromXML(const FunctionCallbackInfo<Value>& args) {
    Predictor::TrainFromXML(args);
  }

  void InitAll(Local<Object> exports) {
    Detector::Init(exports);

    NODE_SET_METHOD(exports, "createDetector", CreateDetector);
    NODE_SET_METHOD(exports, "trainFromXML", TrainFromXML);
    NODE_SET_METHOD(exports, "trainPredictorFromXML", TrainPredictorFromXML);
  }

  NODE_MODULE(ObjectDetector, InitAll)
}
