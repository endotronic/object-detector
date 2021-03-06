// predictor
#include "predictor.h"

namespace ObjectDetector {

  using v8::Array;
  using v8::Function;
  using v8::Exception;
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

  Persistent<Function> Predictor::constructor;

  void Predictor::Init(Local<Object> exports) {
    Isolate* isolate = exports->GetIsolate();

    // Prepare constructor template
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8(isolate, "Predictor"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    NODE_SET_PROTOTYPE_METHOD(tpl, "predictShapeInRect", PredictShapeInRect);
    NODE_SET_PROTOTYPE_METHOD(tpl, "saveToFile", SaveToFile);

    constructor.Reset(isolate, tpl->GetFunction());
    exports->Set(String::NewFromUtf8(isolate, "Predictor"), tpl->GetFunction());
  }

  void Predictor::New(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() > 1) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      if (args.IsConstructCall()) {
        // Invoked as constructor: `new Predictor(...)`
        Predictor* obj;
        if (args[0]->IsUndefined()) {
          obj = new Predictor();
        } else {
          v8::String::Utf8Value xmlPath(args[0]->ToString());
          obj = new Predictor(std::string(*xmlPath));
        }

        obj->Wrap(args.This());
        args.GetReturnValue().Set(args.This());
      } else {
        // Invoked as plain function `Predictor(...)`, turn into construct call.
        const int argc = 1;
        Local<Value> argv[argc] = { args[0] };
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        args.GetReturnValue().Set(cons->NewInstance(argc, argv));
      }
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Predictor::TrainFromXML(const FunctionCallbackInfo<Value>& args) {
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

      Predictor* obj = ObjectWrap::Unwrap<Predictor>(inst);
      v8::String::Utf8Value xmlPath(args[0]->ToString());

      double cascadeDepth = 10, oversamplingAmount = 20, nu = 0.1;
      bool verbose = true, includeMirrors = false;
      int treeDepth = 5, treesPerCascade = 500;
      if (args.Length() == 2 && !args[1]->IsUndefined() && args[1]->IsObject()) {
        Local<Object> options = args[1]->ToObject();

        Local<Value> optVerbose = options->Get(String::NewFromUtf8(isolate, "verbose"));
        if (!optVerbose->IsUndefined()) {
          verbose = optVerbose->BooleanValue();
        }

        Local<Value> optCascadeDepth = options->Get(String::NewFromUtf8(isolate, "cascadeDepth"));
        if (!optCascadeDepth->IsUndefined()) {
          cascadeDepth = optCascadeDepth->NumberValue();
        }

        Local<Value> optOversamplingAmount = options->Get(String::NewFromUtf8(isolate, "oversamplingAmount"));
        if (!optOversamplingAmount->IsUndefined()) {
          oversamplingAmount = optOversamplingAmount->NumberValue();
        }

        Local<Value> optNu = options->Get(String::NewFromUtf8(isolate, "nu"));
        if (!optNu->IsUndefined()) {
          nu = optNu->NumberValue();
        }

        Local<Value> optTreesPerCascade = options->Get(String::NewFromUtf8(isolate, "treesPerCascadeLevel"));
        if (!optTreesPerCascade->IsUndefined()) {
          treesPerCascade = optTreesPerCascade->IntegerValue();
        }

        Local<Value> optMirrors = options->Get(String::NewFromUtf8(isolate, "includeMirrors"));
        if (!optMirrors->IsUndefined()) {
          includeMirrors = optMirrors->BooleanValue();
        }

        Local<Value> optTreeDepth = options->Get(String::NewFromUtf8(isolate, "treeDepth"));
        if (!optTreeDepth->IsUndefined()) {
          treeDepth = optTreeDepth->IntegerValue();
        }
      }

      dlib::array<dlib::array2d<unsigned char> > images_train;
      std::vector<std::vector<dlib::full_object_detection> > shapes_train;
      dlib::load_image_dataset(images_train, shapes_train, *xmlPath);

      if (includeMirrors) {
        // Add mirror images
        dlib::add_image_left_right_flips(images_train, shapes_train);
      }

      dlib::shape_predictor_trainer trainer;
      trainer.set_cascade_depth(cascadeDepth);
      trainer.set_oversampling_amount(oversamplingAmount);
      trainer.set_nu(nu);
      trainer.set_tree_depth(treeDepth);
      trainer.set_num_trees_per_cascade_level(treesPerCascade);

      if (verbose) {
        std::cout << "Cascade depth set to " << cascadeDepth << std::endl;
        std::cout << "Trees per cascade level set to " << treesPerCascade << std::endl;
        std::cout << "Tree depth set to " << treeDepth << std::endl;
        std::cout << "Oversampling amount set to " << oversamplingAmount << std::endl;
        std::cout << "Nu set to " << nu << std::endl;
        std::cout << "Mirrors: " << (includeMirrors ? "on" : "off") << std::endl;
        std::cout << "Total samples: " << images_train.size() << std::endl;
        trainer.be_verbose();
      }

      obj->dlibShapePredictor = trainer.train(images_train, shapes_train);
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Predictor::PredictShapeInRect(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() != 2 || args[1]->IsUndefined() || !args[1]->IsObject()) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number/type of arguments")));
      return;
    }

    try {
      Predictor* obj = ObjectWrap::Unwrap<Predictor>(args.Holder());
      v8::String::Utf8Value imgPath(args[0]->ToString());

      Local<Object> rect = args[1]->ToObject();
      Local<Value> rectTop = rect->Get(String::NewFromUtf8(isolate, "top"));
      if (rectTop->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Rect top must be defined")));
        return;
      }

      Local<Value> rectLeft = rect->Get(String::NewFromUtf8(isolate, "left"));
      if (rectLeft->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Rect left must be defined")));
        return;
      }

      Local<Value> rectWidth = rect->Get(String::NewFromUtf8(isolate, "width"));
      if (rectWidth->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Rect width must be defined")));
        return;
      }

      Local<Value> rectHeight = rect->Get(String::NewFromUtf8(isolate, "height"));
      if (rectHeight->IsUndefined()) {
        isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Rect height must be defined")));
        return;
      }

      double top = rectTop->NumberValue(), left = rectLeft->NumberValue();
      double width = rectWidth->NumberValue(), height = rectHeight->NumberValue();
      dlib::array2d<unsigned char> img;
      dlib::load_image(img, std::string(*imgPath));

      dlib::rectangle detection(left, top, left + width, top + height);
      dlib::full_object_detection shape = obj->dlibShapePredictor(img, detection);

      HandleScope scope(isolate);
      Local<Array> points = Array::New(isolate);
      for (unsigned int i = 0; i < shape.num_parts(); ++i) {
        double x = shape.part(i).x();
        double y = shape.part(i).y();
        double xScaled = x / img.nc(), yScaled = y / img.nr();

        HandleScope scope(isolate);
        Local<Object> point = Object::New(isolate);
        point->Set(String::NewFromUtf8(isolate, "x"), Number::New(isolate, x));
        point->Set(String::NewFromUtf8(isolate, "y"), Number::New(isolate, y));
        point->Set(String::NewFromUtf8(isolate, "xScaled"), Number::New(isolate, xScaled));
        point->Set(String::NewFromUtf8(isolate, "yScaled"), Number::New(isolate, yScaled));
        points->Set(i, point);
      }

      args.GetReturnValue().Set(points);
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }

  void Predictor::SaveToFile(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();

    if (args.Length() != 1) {
      isolate->ThrowException(Exception::TypeError(
          String::NewFromUtf8(isolate, "Wrong number of arguments")));
      return;
    }

    try {
      Predictor* obj = ObjectWrap::Unwrap<Predictor>(args.Holder());
      v8::String::Utf8Value filePath(args[0]->ToString());
      dlib::serialize(std::string(*filePath)) << obj->dlibShapePredictor;
    } catch (std::exception& e) {
      isolate->ThrowException(Exception::Error(String::NewFromUtf8(isolate, e.what())));
    }
  }
}
