
#ifndef webx_engine_value_h_
#define webx_engine_value_h_

#include <functional>
#include <sstream>
#include <vector>
#include <map>
#include <stdint.h>

namespace webx {

  struct IValue;
  struct NullValue;
  struct BooleanValue;
  struct NumberValue;
  struct IntegerValue;
  struct C_StringValue;
  struct StringValue;
  struct StringRefValue;
  struct StringMapValue;
  struct StringArrayValue;

  enum tTypeId {
    undefinedId,
    stringId,
    booleanId,
    integerId,
    numberId,
    mapId,
    arrayId,
    nullId,
  };


  struct String {
    String() {
      this->ptr = 0;
    }
    String(String& value) {
      this->ptr = value.ptr;
      this->pfree = value.pfree;
      value.ptr = 0;
    }
    String(const char* value) {
      printf("New %s\n", value);
      this->ptr = _strdup(value);
      this->pfree = ::free;
    }
    String(std::string value) {
      printf("New %s\n", value.c_str());
      this->ptr = _strdup(value.c_str());
      this->pfree = ::free;
    }
    ~String() {
      if (this->ptr) {
        printf("Free %s\n", this->ptr);
        this->pfree(this->ptr);
      }
    }
    void operator = (String& value) {
      this->ptr = value.ptr;
      this->pfree = value.pfree;
      value.ptr = 0;
    }
    void operator = (const char* value) {
      if (this->ptr) {
        printf("Free %s\n", this->ptr);
        this->pfree(this->ptr);
      }
      printf("New %s\n", value);
      this->ptr = _strdup(value);
      this->pfree = ::free;
    }
    void operator = (std::string value) {
      if (this->ptr) {
        printf("Free %s\n", this->ptr);
        this->pfree(this->ptr);
      }
      printf("New %s\n", value.c_str());
      this->ptr = _strdup(value.c_str());
      this->pfree = ::free;
    }
    operator const char*() {
      return this->ptr;
    }
    operator bool() {
      return this->ptr != 0;
    }
  private:
    char* ptr;
    void(*pfree)(void*);
  };

  struct IForeachVisitor {
    template<class Ft> struct _lambda;
    virtual void operator() (const IValue& key, const IValue& value) = 0;
  };
  template<class Ft>
  struct IForeachVisitor::_lambda : IForeachVisitor {
    Ft cb;
    _lambda(Ft& data) : cb(data) { }
    virtual void operator() (const IValue& key, const IValue& value) override { return cb(key, value); }
  };

  struct IValue {
    typedef const std::function<void(const IValue& value)>& get_visitor_t;
    typedef IForeachVisitor& foreach_visitor_t;

    static IValue Undefined;
    static NullValue Null;

    virtual tTypeId getTypeId() const;
    virtual void foreach(foreach_visitor_t visitor) const;
    void print() const;

    // Map API
    virtual bool get(const char* key, get_visitor_t visitor) const;
    virtual String getString(const char* key) const;
    virtual double getNumber(const char* key) const;
    virtual int64_t getInteger(const char* key) const;

    // Array API
    virtual bool get(size_t key, get_visitor_t visitor) const;
    virtual String getString(size_t key) const;
    virtual double getNumber(size_t key) const;
    virtual int64_t getInteger(size_t key) const;

    // String API
    virtual void toJSON(std::stringstream& out) const;
    virtual const char* toString() const;

    // Number API
    virtual double toNumber() const;

    // Integer API
    virtual int64_t toInteger() const;

    // Boolean API
    virtual bool toBoolean() const;

    // Template helpers
    template<class Ft>
    void foreach(Ft visitor) const { return this->foreach((IForeachVisitor&)IForeachVisitor::_lambda<Ft>(visitor)); }

  };

  struct NullValue : IValue {
    virtual tTypeId getTypeId() const override {
      return nullId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << "null";
    }
    virtual const char* toString() const override {
      return "null";
    }
  };

  struct BooleanValue : IValue {
    bool value;
    BooleanValue(bool value) {
      this->value = value;
    }
    virtual tTypeId getTypeId() const override {
      return booleanId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << (this->value ? "true" : "false");
    }
    virtual const char* toString() const override {
      return this->value ? "true" : "false";
    }
    virtual double toNumber() const override {
      return (double)this->value;
    }
    virtual int64_t toInteger() const override {
      return this->value;
    }
  };

  struct IntegerValue : IValue {
    int64_t value;
    mutable char tmp[32];
    IntegerValue(int64_t value) {
      this->value = value;
    }
    virtual tTypeId getTypeId() const override {
      return integerId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << this->value;
    }
    virtual const char* toString() const override {
      if (_i64toa_s(this->value, this->tmp, sizeof(tmp), 10)) return "NaN";
      return tmp;
    }
    virtual double toNumber() const override {
      return (double)this->value;
    }
    virtual int64_t toInteger() const override {
      return this->value;
    }
  };

  struct NumberValue : IValue {
    double value;
    mutable char tmp[32];
    NumberValue(double value) {
      this->value = value;
    }
    virtual tTypeId getTypeId() const override {
      return numberId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << this->value;
    }
    virtual const char* toString() const override {
      if (_gcvt_s(this->tmp, this->value, 12)) return "NaN";
      return this->tmp;
    }
    virtual double toNumber() const override {
      return this->value;
    }
    virtual int64_t toInteger() const override {
      return int64_t(this->value);
    }
  };

  struct C_StringValue : IValue {
    const char* value;
    C_StringValue(const char* value) {
      this->value = value;
    }
    virtual tTypeId getTypeId() const override {
      return stringId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << "\"" << this->toString() << "\"";
    }
    virtual const char* toString() const override {
      return this->value;
    }
    virtual double toNumber() const override {
      return strtod((char*)this->value, 0);
    }
    virtual int64_t toInteger() const override {
      return _atoi64(this->value);
    }
  };

  struct StringValue : IValue {
    std::string value;
    StringValue() : value() {
    }
    StringValue(std::string& _value) : value(_value) {
    }
    virtual tTypeId getTypeId() const override {
      return stringId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << "\"" << this->value << "\"";
    }
    virtual const char* toString() const override {
      return this->value.c_str();
    }
    virtual double toNumber() const override {
      return std::stod(this->value);
    }
    virtual int64_t toInteger() const override {
      return std::stoll(this->value);
    }
  };

  struct StringRefValue : IValue {
    const std::string& value;
    StringRefValue(const std::string& _value) : value(_value) {
    }
    virtual tTypeId getTypeId() const override {
      return stringId;
    }
    virtual void toJSON(std::stringstream& out) const override {
      out << "\"" << this->value << "\"";
    }
    virtual const char* toString() const override {
      return this->value.c_str();
    }
    virtual double toNumber() const override {
      return std::stod(this->value);
    }
    virtual int64_t toInteger() const override {
      return std::stoll(this->value);
    }
  };

  struct StringMapValue : IValue {
    struct compare_t {
      bool operator() (const std::string& lhs, const std::string& rhs) const {
        return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
      }
    };
    typedef std::map<std::string, std::string, compare_t> map_t;
    map_t values;

    virtual tTypeId getTypeId() const override {
      return mapId;
    }
    virtual bool get(const char* key, get_visitor_t visitor) const override {
      auto const& it = this->values.find(key);
      if (it != this->values.end()) {
        visitor(StringRefValue(it->second));
        return true;
      }
      return false;
    }
    virtual void foreach(foreach_visitor_t visitor) const override {
      for (auto const &item : this->values) {
        visitor(StringRefValue(item.first), StringRefValue(item.second));
      }
    }
    virtual void toJSON(std::stringstream& out) const override {
      bool first = true;
      out << '{';
      for (auto const &item : this->values) {
        if (first) first = false;
        else out << ',';
        out << '"' << item.first << '"' << ':';
        StringRefValue(item.second).toJSON(out);
      }
      out << '}';
    }
  };

  struct StringArrayValue : IValue {
    typedef std::vector<std::string> array_t;
    array_t values;

    virtual tTypeId getTypeId() const override {
      return arrayId;
    }
    virtual bool get(size_t key, get_visitor_t visitor) const override {
      if (key >= 0 && key <= this->values.size()) {
        visitor(StringRefValue(this->values[key]));
        return true;
      }
      return false;
    }
    virtual void foreach(foreach_visitor_t visitor) const override {
      IntegerValue index(0);
      for (auto const &item : this->values) {
        visitor(index, StringRefValue(item));
        index.value++;
      }
    }
    virtual void toJSON(std::stringstream& out) const override {
      bool first = true;
      out << '[';
      for (auto const &item : this->values) {
        if (first) first = false;
        else out << ',';
        StringRefValue(item).toJSON(out);
      }
      out << ']';
    }
  };

}

#endif