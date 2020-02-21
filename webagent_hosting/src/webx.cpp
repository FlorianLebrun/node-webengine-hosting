#include "webagent-hosting/webx-releasables.h"
#include "webagent-hosting/webx-value.h"
#include <iostream>
#include <unordered_map>
#include <mutex>

using namespace webx;

#ifdef _RELEASABLE_DEBUG
static std::unordered_map<size_t, int> _s_object_counts;
static std::mutex _s_object_counts_lock;

int webx::_report_releasable_count(const type_info& infos) {
  int count = 0;
  _s_object_counts_lock.lock();
  count = _s_object_counts[infos.hash_code()];
  _s_object_counts_lock.unlock();
  return count;
}
int webx::_report_releasable_new(const type_info& infos) {
  int count = 0;
  _s_object_counts_lock.lock();
  count = ++_s_object_counts[infos.hash_code()];
  _s_object_counts_lock.unlock();
  return count;
}
int webx::_report_releasable_delete(const type_info& infos) {
  int count = 0;
  _s_object_counts_lock.lock();
  count = --_s_object_counts[infos.hash_code()];
  _s_object_counts_lock.unlock();
  return count;
}

#endif

IValue IValue::Undefined;
NullValue IValue::Null;

tTypeId IValue::getTypeId() const {
  return undefinedId;
}
void IValue::foreach(foreach_visitor_t visitor) const {
}
void IValue::print() const {
  std::stringstream out;
  this->toJSON(out);
  std::cout << out.str() << "\n";
}

// Map API
bool IValue::get(const char* key, get_visitor_t visitor) const {
  if (this->getTypeId() == mapId) {
    bool found = false;
    this->foreach([key, visitor, &found](const IValue& ckey, const IValue& cvalue) {
      if (ckey.toString() == key) {
        visitor(cvalue);
        found = true;
      }
    });
    return found;
  }
  return false;
}
String IValue::getString(const char* key) const {
  String x;
  this->get(key, [&x](const IValue& value) {
    x = value.toString();
  });
  return x;
}
double IValue::getNumber(const char* key) const {
  double x = 0.0; this->get(key, [&x](const IValue& value) {x = value.toNumber(); });
  return x;
}
int64_t IValue::getInteger(const char* key) const {
  int64_t x = 0; this->get(key, [&x](const IValue& value) {x = value.toInteger(); });
  return x;
}

// Array API
bool IValue::get(size_t key, get_visitor_t visitor) const {
  if (this->getTypeId() == arrayId) {
    bool found = false;
    this->foreach([key, visitor, &found](const IValue& ckey, const IValue& cvalue) {
      if (ckey.toInteger() == key) {
        visitor(cvalue);
        found = true;
      }
    });
    return found;
  }
  return false;
}
String IValue::getString(size_t key) const {
  String x;
  this->get(key, [&x](const IValue& value) {
    x = value.toString();
  });
  return x;
}
double IValue::getNumber(size_t key) const {
  double x = 0.0; this->get(key, [&x](const IValue& value) {x = value.toNumber(); });
  return x;
}
int64_t IValue::getInteger(size_t key) const {
  int64_t x = 0; this->get(key, [&x](const IValue& value) {x = value.toInteger(); });
  return x;
}

// String API
void IValue::toJSON(std::stringstream& out) const {
  switch (this->getTypeId()) {
  case tTypeId::mapId:
  {
    bool first = true;
    out << '{';
    this->foreach([&first, &out](const IValue& key, const IValue& value) {
      if (first) first = false;
      else out << ',';
      out << '"' << key.toString() << '"' << ':';
      value.toJSON(out);
    });
    out << '}';
  } break;
  case tTypeId::arrayId:
  {
    bool first = true;
    out << '[';
    this->foreach([&first, &out](const IValue& key, const IValue& value) {
      if (first) first = false;
      else out << ',';
      value.toJSON(out);
    });
    out << ']';
  } break;
  case tTypeId::stringId:
    out << '"' << this->toString() << '"';
    break;
  case tTypeId::integerId:
    out << this->toInteger();
    break;
  case tTypeId::numberId:
    out << this->toNumber();
    break;
  case tTypeId::booleanId:
    out << (this->toBoolean() ? "true" : "false");
    break;
  case tTypeId::nullId:
    out << "null";
    break;
  }
}
const char* IValue::toString() const {
  return "";
}

// Number API
double IValue::toNumber() const {
  return 0.0;
}

// Integer API
int64_t IValue::toInteger() const {
  return 0;
}

// Boolean API
bool IValue::toBoolean() const {
  return this->toInteger() != 0;
}
