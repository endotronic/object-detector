// addon.cpp
#include <node.h>
#include "detector.h"

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
  Detector::New(args);
}

void InitAll(Local<Object> exports, Local<Object> module) {
  Detector::Init(exports);

  //NODE_SET_METHOD(module, "exports", CreateDetector);
  //NODE_SET_METHOD(module, "exports", TrainFromXML);
}

NODE_MODULE(ObjectDetector, InitAll)

}
