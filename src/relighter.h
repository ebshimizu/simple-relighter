#pragma once

#include <string>
#include <iostream>
#include <cmath>

#include <nan.h>
#include "third-party/lodepng/lodepng.h"

using namespace std;

class Relighter : public Nan::ObjectWrap {
public:
  static NAN_MODULE_INIT(Init);

  vector<unsigned char> _render(vector<float> paramas, float gamma = 2.2f, float level = 1.0f);

private: 
  explicit Relighter();
  ~Relighter();

  // internal
  void _load(string dir);
  void _renderToFile(vector<float> params, string file, float gamma = 2.2f, float level = 1.0f);
  // vector<float> _paramVec();
  // string _labelForIndex(int idx);
  vector<float> _decompress(vector<unsigned char>& img);
  v8::Local<v8::Object> _paramKey();

  // data
  vector<vector<float>> _imageData;
  unsigned int _width;
  unsigned int _height;

  // exposed
  static NAN_METHOD(New);
  static NAN_METHOD(load);
  static NAN_METHOD(renderToFile);
  static NAN_METHOD(renderToCanvas);
  static NAN_METHOD(transferToContext);
  static NAN_METHOD(renderAsync);
  static NAN_GETTER(getters);
  static Nan::Persistent<v8::Function> constructor;
};

// just a thing to let me wrap a buffer before returning
class ImageContainer {
public:
  ImageContainer(vector<unsigned char> buffer) : _buffer(buffer) {}

  vector<unsigned char> _buffer;
};

class RenderWorker : public Nan::AsyncWorker {
public:
  RenderWorker(Nan::Callback* cb, Relighter* rl, vector<float> params, float gamma, float level);
  ~RenderWorker();

  void Execute();
  void HandleOKCallback();

private:
  Relighter* _rl;
  vector<float> _params;
  float _gamma;
  float _level;
  ImageContainer* _result;
};