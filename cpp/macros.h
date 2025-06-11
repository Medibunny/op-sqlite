#pragma once

#define HOST_STATIC_FN(name)                                                   \
jsi::Function::createFromHostFunction( \
rt, \
jsi::PropNameID::forAscii(rt, name), \
0, \
[&](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *args, size_t count) -> jsi::Value

#define JS_THROW_IF_NOT(rt, cond, msg)                                         \
  if (!(cond)) {                                                               \
    throw jsi::JSError(rt, msg);                                               \
  }
