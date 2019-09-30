#include "relighter.h"

// helpers
bool hasEnding(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
  }
  else {
    return false;
  }
}

vector<float> hsvToRgb(float H, float S, float V) {
  // Normalize H
  H = H * 360.0f;

  float C = V * S;
  float Hp = H / 60;
  float X = C * (1 - abs(fmod(Hp, 2) - 1));

  float R, G, B;
  if (0 <= Hp && Hp < 1) {
    R = C;
    G = X;
    B = 0;
  }
  else if (1 <= Hp && Hp < 2) {
    R = X;
    G = C;
    B = 0;
  }
  else if (2 <= Hp && Hp < 3) {
    R = 0;
    G = C;
    B = X;
  }
  else if (3 <= Hp && Hp < 4) {
    R = 0;
    G = X;
    B = C;
  }
  else if (4 <= Hp && Hp < 5) {
    R = X;
    G = 0;
    B = C;
  }
  else if (5 <= Hp && Hp < 6) {
    R = C;
    G = 0;
    B = X;
  }

  float m = V - C;
  vector<float> rgb(3);
  rgb[0] = R + m;
  rgb[1] = G + m;
  rgb[2] = B + m;

  return rgb;
}

inline float clamp(float x, float min, float max) {
  if (x < min)
    return min;
  if (x > max)
    return max;
  return x;
}

vector<unsigned char> toneMap(vector<float>& img, float gamma = 2.2f, float level = 1.0f) {
  vector<unsigned char> tonedImg(img.size(), 0);

  for (int i = 0; i < img.size() / 4; i++) {
    tonedImg[i * 4] = (unsigned char)(clamp(pow(img[i * 4] * level, gamma), 0, 1) * 255);
    tonedImg[i * 4 + 1] = (unsigned char)(clamp(pow(img[i * 4 + 1] * level, gamma), 0, 1) * 255);
    tonedImg[i * 4 + 2] = (unsigned char)(clamp(pow(img[i * 4 + 2] * level, gamma), 0, 1) * 255);
    tonedImg[i * 4 + 3] = 255;
  }

  return tonedImg;
}

// nan module def
Nan::Persistent<v8::Function> Relighter::constructor;

NAN_MODULE_INIT(Relighter::Init) {
  v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("Relighter").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New("filecount").ToLocalChecked(), Relighter::getters);
  Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New("paramKey").ToLocalChecked(), Relighter::getters);
  Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New("width").ToLocalChecked(), Relighter::getters);
  Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New("height").ToLocalChecked(), Relighter::getters);

  Nan::SetPrototypeMethod(tpl, "load", load);
  Nan::SetPrototypeMethod(tpl, "renderToFile", renderToFile);
  Nan::SetPrototypeMethod(tpl, "renderToCanvas", renderToCanvas);

  constructor.Reset(Nan::GetFunction(tpl).ToLocalChecked());
  Nan::Set(target, Nan::New("Relighter").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

// internal
// errors should be thrown up to the javascript context

Relighter::Relighter() {}

Relighter::~Relighter() {
  // free image memory
}

void Relighter::_load(string dir) {
  // clear previous vector
  _imageData = vector<vector<float>>();

  // list files in directory
  for (const auto& entry : fs::directory_iterator(dir)) {
    string file = entry.path().string();

    // load .png files
    if (hasEnding(file, ".png")) {
      vector<unsigned char> img;
      unsigned width, height;

      unsigned err = lodepng::decode(img, width, height, file);

      if (err) {
        throw exception(lodepng_error_text(err));
      }

      // decompress to float
      _imageData.push_back(this->_decompress(img));
      _width = width;
      _height = height;
    }
  }
}

vector<float> Relighter::_decompress(vector<unsigned char>& img) {
  vector<float> newImg(img.size());

  // leave alpha channel alone
  for (int i = 0; i < img.size(); i++) {
    if (i % 4 == 3) {
      newImg[i] = img[i] / 255.f;
    }
    else {
      newImg[i] = pow(img[i] / 255.0f, 2.2f);
    }
  }

  return newImg;
}

v8::Local<v8::Object> Relighter::_paramKey() {
  auto params = Nan::New<v8::Array>();

  for (int i = 0; i < this->_imageData.size(); i++) {
    // params are Hue, Sat, Val
    // hue
    auto hParam = Nan::New<v8::Object>();
    Nan::Set(hParam, Nan::New("name").ToLocalChecked(), Nan::New(to_string(i) + "-hue").ToLocalChecked());
    Nan::Set(hParam, Nan::New("parent").ToLocalChecked(), Nan::New(to_string(i)).ToLocalChecked());
    Nan::Set(hParam, Nan::New("default").ToLocalChecked(), Nan::New(0));

    // sat
    auto sParam = Nan::New<v8::Object>();
    Nan::Set(sParam, Nan::New("name").ToLocalChecked(), Nan::New(to_string(i) + "-saturation").ToLocalChecked());
    Nan::Set(sParam, Nan::New("parent").ToLocalChecked(), Nan::New(to_string(i)).ToLocalChecked());
    Nan::Set(sParam, Nan::New("default").ToLocalChecked(), Nan::New(0));

    // val
    auto vParam = Nan::New<v8::Object>();
    Nan::Set(vParam, Nan::New("name").ToLocalChecked(), Nan::New(to_string(i) + "-value").ToLocalChecked());
    Nan::Set(vParam, Nan::New("parent").ToLocalChecked(), Nan::New(to_string(i)).ToLocalChecked());
    Nan::Set(vParam, Nan::New("default").ToLocalChecked(), Nan::New(1));

    Nan::Set(params, Nan::New(i * 3), hParam);
    Nan::Set(params, Nan::New(i * 3 + 1), sParam);
    Nan::Set(params, Nan::New(i * 3 + 2), vParam);
  }

  return params;
}

vector<unsigned char> Relighter::_render(vector<float> params, float gamma, float level) {
  if (_imageData.size() == 0)
    throw exception("Need at least one input image to use render command");

  if (params.size() != _imageData.size() * 3)
    throw exception(string("Parameter input vector does not match expected params. Got " + to_string(params.size()) + ". Expected " + to_string(_imageData.size() * 3)).c_str());

  // per layer operations, render to float buffer first, alpha is 1 during all of this
  vector<float> renderBuffer(_width * _height * 4, 0);

  for (int i = 0; i < _imageData.size(); i++) {
    auto& img = _imageData[i];
    // convert param to hsv
    auto mod = hsvToRgb(params[i * 3], params[i * 3 + 1], params[i * 3 + 2]);
    float rMod = mod[0];
    float gMod = mod[1];
    float bMod = mod[2];

    for (int px = 0; px < _width * _height; px++) {
      // get pixel
      const int idx = px * 4;
      
      // modulate and add to render buffer
      renderBuffer[idx] += img[idx] * rMod;
      renderBuffer[idx + 1] += img[idx + 1] * gMod;
      renderBuffer[idx + 2] += img[idx + 2] * bMod;
      renderBuffer[idx + 3] = 1;
    }
  }

  vector<unsigned char> finalImg = toneMap(renderBuffer, gamma, level);

  return finalImg;
}

void Relighter::_renderToFile(vector<float> params, string file, float gamma, float level) {
  auto img = _render(params, gamma, level);

  unsigned err = lodepng::encode(file, img, _width, _height);
  if (err) {
    throw exception(lodepng_error_text(err));
  }
}

// exposed methods

NAN_METHOD(Relighter::New) {
  // nothing is copied at the moment
  Relighter* obj = new Relighter();
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(Relighter::load) {
  // load files from the specified directory
  Relighter* obj = Nan::ObjectWrap::Unwrap<Relighter>(info.This());
  Nan::Utf8String dirval(info[0]);
  if (dirval.length() <= 0) {
    return Nan::ThrowTypeError("directory path (arg 0) must be a non-empty string");
  }

  string dir(*dirval, dirval.length());
  try {
    obj->_load(dir);
  }
  catch (exception e) {
    return Nan::ThrowError(e.what());
  }
}

NAN_METHOD(Relighter::renderToFile) {
  Relighter *self = Nan::ObjectWrap::Unwrap<Relighter>(info.This());

  vector<float> params;
  v8::Local<v8::Array> args = info[0].As<v8::Array>();
  for (unsigned int i = 0; i < args->Length(); i++) {
    params.push_back((float)Nan::To<double>(Nan::Get(args, i).ToLocalChecked().As<v8::Number>()).ToChecked());
  }

  string file = string(*Nan::Utf8String(info[1]));

  // check exist other params
  float gamma = 2.2f;
  if (info[2]->IsNumber()) {
    gamma = (float)Nan::To<double>(info[2]).ToChecked();
  }

  float level = 1.0f;
  if (info[3]->IsNumber()) {
    level = (float)Nan::To<double>(info[3]).ToChecked();
  }
  
  try {
    self->_renderToFile(params, file, gamma, level);
  }
  catch (exception e) {
    return Nan::ThrowError(e.what());
  }
}

NAN_METHOD(Relighter::renderToCanvas) {
  Relighter *self = Nan::ObjectWrap::Unwrap<Relighter>(info.This());

  vector<float> params;
  v8::Local<v8::Array> args = info[0].As<v8::Array>();
  for (unsigned int i = 0; i < args->Length(); i++) {
    params.push_back((float)Nan::To<double>(Nan::Get(args, i).ToLocalChecked().As<v8::Number>()).ToChecked());
  }

  // check exist other params
  float gamma = 2.2f;
  if (info[2]->IsNumber()) {
    gamma = (float)Nan::To<double>(info[2]).ToChecked();
  }

  float level = 1.0f;
  if (info[3]->IsNumber()) {
    level = (float)Nan::To<double>(info[3]).ToChecked();
  }

  try {
    vector<unsigned char> imData = self->_render(params, gamma, level);

    // the black magic part where you grab the canvas buffer directly
    v8::Local<v8::Uint8ClampedArray> arr = Nan::Get(info[1].As<v8::Object>(), Nan::New("data").ToLocalChecked()).ToLocalChecked().As<v8::Uint8ClampedArray>();
    unsigned char *data = (unsigned char*)arr->Buffer()->GetContents().Data();

    memcpy(data, &imData[0], imData.size());
  }
  catch (exception e) {
    return Nan::ThrowError(e.what());
  }
}

NAN_GETTER(Relighter::getters) {
  Relighter *self = Nan::ObjectWrap::Unwrap<Relighter>(info.This());

  string prop = string(*Nan::Utf8String(property));

  if (prop == "filecount") {
    info.GetReturnValue().Set((int)self->_imageData.size());
  }
  else if (prop == "width") {
    info.GetReturnValue().Set((int)self->_width);
  }
  else if (prop == "height") {
    info.GetReturnValue().Set((int)self->_height);
  }
  else if (prop == "paramKey") {
    info.GetReturnValue().Set(self->_paramKey());
  }
  else {
    info.GetReturnValue().Set(Nan::Undefined());
  }
}

void InitAll(v8::Local<v8::Object> exports) {
  Relighter::Init(exports);
}

NODE_MODULE(SimpleRelighter, InitAll);