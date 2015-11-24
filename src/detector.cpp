// detector
#include "detector.h"

namespace ObjectDetector {

  using v8::Array;
  using v8::Exception;
  using v8::Function;
  using v8::FunctionCallbackInfo;
  using v8::FunctionTemplate;
  using v8::HandleScope;
  using v8::Isolate;
  using v8::Local;
  using v8::Number;
  using v8::Object;
  using v8::Persistent;
  using v8::String;
  using v8::Value;

  Persistent<Function> Detector::constructor;

  void Detector::Init(Local<Object> exports) {
    Isolate* isolate = exports->GetIsolate();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Detector"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "detectInImageFile", DetectInImageFile);
    NODE_SET_PROTOTYPE_METHOD(tpl, "saveToFile", SaveToFile);
    NODE_SET_PROTOTYPE_METHOD(tpl, "saveImageRepresentation", SaveImageRepresentation);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "Detector"), tpl->GetFunction());
  }

  void Detector::New(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() > 1) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      if (args.IsConstructCall()) {
        // Invoked as constructor: `new Detector(...)`
        Detector* obj;
        if (args[0]->IsUndefined()) {
          obj = new Detector();
        } else {
          v8::String::Utf8Value xmlPath(args[0]->ToString());
          obj = new Detector(std::string(*xmlPath));
        }

        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      } else {
        // Invoked as plain function `Detector(...)`, turn into construct call.
        const int argc = 1;
        Local<Value> argv[argc] = { args[0] };
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(argc, argv));
      }
    } catch (std::exception& e) {
        isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Detector::DetectInImageFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() != 1) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      Detector* obj = ObjectWrap::Unwrap<Detector>(args.Holder());
      v8::String::Utf8Value imgPath(args[0]->ToString());

      dlib::array2d<unsigned char> img;
      dlib::load_image(img, std::string(*imgPath));

      std::vector<dlib::rectangle> dets = obj->dlibObjectDetector(img);

      HandleScope scope(isolate);
      Local<Array> rectangles = Array::New(isolate);
      for (unsigned int i = 0; i < dets.size(); ++i) {
        double left = dets[i].left();
        double top = dets[i].top();
        double width = dets[i].right() - dets[i].left();
        double height = dets[i].bottom() - dets[i].top();

        HandleScope scope(isolate);
        Local<Object> rectangle = Object::New(isolate);
        rectangle->Set(String::NewFromUtf8(isolate, "left"), Number::New(isolate, left));
        rectangle->Set(String::NewFromUtf8(isolate, "top"), Number::New(isolate, top));
        rectangle->Set(String::NewFromUtf8(isolate, "width"), Number::New(isolate, width));
        rectangle->Set(String::NewFromUtf8(isolate, "height"), Number::New(isolate, height));
        rectangles->Set(i, rectangle);
      }

      args.GetReturnValue().Set(rectangles);
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Detector::TrainFromXML(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() < 1 || args.Length() > 2) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      Local<Function> cons = Local<Function>::New(isolate, constructor);
      Local<Object> inst = cons->NewInstance(0, 0);
      args.GetReturnValue().Set(inst);

      Detector* obj = ObjectWrap::Unwrap<Detector>(inst);
      v8::String::Utf8Value xmlPath(args[0]->ToString());

      double c = 1, windowSize = 80, epsilon = 0.01;
      bool verbose = true;
      int nThreads = 4;
      if (args.Length() == 2 && !args[1]->IsUndefined() && args[1]->IsObject()) {
        Local<Object> options = args[1]->ToObject();

        Local<Value> optC = options->Get(String::NewFromUtf8(isolate, "c"));
        if (!optC->IsUndefined()) {
          c = optC->NumberValue();
        }

        Local<Value> optWindowSize = options->Get(String::NewFromUtf8(isolate, "windowSize"));
        if (!optWindowSize->IsUndefined()) {
          windowSize = optWindowSize->NumberValue();
        }

        Local<Value> optEpsilon = options->Get(String::NewFromUtf8(isolate, "epsilon"));
        if (!optEpsilon->IsUndefined()) {
          epsilon = optEpsilon->NumberValue();
        }

        Local<Value> optVerbose = options->Get(String::NewFromUtf8(isolate, "verbose"));
        if (!optVerbose->IsUndefined()) {
          verbose = optVerbose->BooleanValue();
        }

        Local<Value> optThreads = options->Get(String::NewFromUtf8(isolate, "threads"));
        if (!optThreads->IsUndefined()) {
          nThreads = optThreads->IntegerValue();
        }
      }

      dlib::array<dlib::array2d<unsigned char> > images_train;
      std::vector<std::vector<dlib::rectangle> > face_boxes_train;
      dlib::load_image_dataset(images_train, face_boxes_train, *xmlPath);

      // Add mirror images
      dlib::add_image_left_right_flips(images_train, face_boxes_train);

      image_scanner_type scanner;
      scanner.set_detection_window_size(windowSize, windowSize);
      dlib::structural_object_detection_trainer<image_scanner_type> trainer(scanner);
      trainer.set_num_threads(nThreads);
      trainer.set_c(c);
      trainer.set_epsilon(epsilon);

      if (verbose) {
        trainer.be_verbose();
      }

      obj->dlibObjectDetector = trainer.train(images_train, face_boxes_train);
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Detector::SaveToFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() != 1) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      Detector* obj = ObjectWrap::Unwrap<Detector>(args.Holder());
      v8::String::Utf8Value filePath(args[0]->ToString());
      dlib::serialize(std::string(*filePath)) << obj->dlibObjectDetector;
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Detector::SaveImageRepresentation(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() != 1) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      Detector* obj = ObjectWrap::Unwrap<Detector>(args.Holder());
      v8::String::Utf8Value filePath(args[0]->ToString());
      dlib::save_png(draw_fhog(obj->dlibObjectDetector), std::string(*filePath));
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }
}
