

#ifndef webx_engine_interfaces_impl_h_
#define webx_engine_interfaces_impl_h_

#include <map>
#include <stdint.h>
#include <string>

#include "./webx-releasables.h"

namespace webx
{
  class IAttributs;
  class IAttributsVisitor;

  // Template for attributs object
  template <class CAttributs>
  class BuiltinAttributs; // For object with hard coded attributs
  template <class CAttributs>
  class NoAttributs; // For object with no attributs
  struct CaseInsensitive;
  struct CaseSensitive;
  template <class CAttributs, class KeyCompare = CaseInsensitive>
  class StringMapBasedAttributs; // For object with std::map attributs

  // Attribut value structure
  struct AttributValue {
    enum tTypeId {
      stringId,
      integerId,
      floatId,
      undefinedId,
    };
    union {
      const char* _string;
      int64_t _integer;
      double _float;
      LPVOID _undefined;
    };
    int typeId;
    int size;

    AttributValue() {this->typeId = undefinedId;this->_undefined = 0;}
    AttributValue(const char* value, int size = -1) {this->typeId = 0;this->_string = value;this->size = size;}
    AttributValue(int8_t value) {this->typeId = 1;this->_integer = value;}
    AttributValue(int16_t value) {this->typeId = 1;this->_integer = value;}
    AttributValue(int32_t value) {this->typeId = 1;this->_integer = value;}
    AttributValue(int64_t value) {this->typeId = 1;this->_integer = value;}
    AttributValue(long value) {this->typeId = 1;this->_integer = value;}
    AttributValue(float value) {this->typeId = 2;this->_float = value;}
    AttributValue(double value) {this->typeId = 2;this->_float = value;}

    bool undefined() {
      return this->typeId == undefinedId;
    }
    const char* getStringPtr() {
      if(this->typeId ==  stringId) return this->_string;
      return 0;
    }
    int getStringLen() {
      if(this->typeId ==  stringId) return this->size<0?strlen(this->_string):this->size;
      return 0;
    }
    std::string toString() {
      char tmp[32];
      switch(this->typeId) {
      case stringId: return this->_string;
      case integerId: return _i64toa(this->_integer, tmp, 10);
      case floatId: return gcvt(this->_float, 12, tmp);
      }
      return "";
    }
    int64_t toInt() {
      switch(this->typeId) {
      case stringId: return _atoi64(this->_string);
      case integerId: return this->_integer;
      case floatId: return int64_t(this->_float);
      }
      return 0;
    }
    double toFloat() {
      switch(this->typeId) {
      case stringId: return _atoi64(this->_string);
      case integerId: return double(this->_integer);
      case floatId: return this->_float;
      }
      return 0.0;
    }
  };

  // Interface of attributs visitor
  class IAttributsVisitor
  {
  public:
    virtual void visit(const char *name, AttributValue value) = 0;
  };

  // Interface of attributs object
  class IAttributs : public IReleasable
  {
  public:
    virtual AttributValue getAttribut(const char *name) = 0;
    virtual bool setAttribut(const char *name, AttributValue value) = 0;
    virtual bool removeAttribut(const char *name) = 0;

    virtual void visitAttributs(IAttributsVisitor *visitor) = 0;
    void printAttributs();
  };

  // -------------------------------------------------
  // Implementations
  // -------------------------------------------------

  inline void IAttributs::printAttributs()
  {
    struct Visitor : public IAttributsVisitor
    {
      virtual void visit(const char *name, AttributValue value) override
      {
        std::cout<<name<<": "<<value.toString()<<"\n";
      }
    };
    Visitor visitor;
    this->visitAttributs(&visitor);
  }

  template <class CAttributs>
  class NoAttributs : public CAttributs
  {
  public:
    virtual void visitAttributs(IAttributsVisitor *visitor) override
    {
    }
    virtual bool removeAttribut(const char *name) override
    {
      return false;
    }
    virtual AttributValue getAttribut(const char *name) override
    {
      return AttributValue();
    }
    virtual bool setAttribut(const char *name, AttributValue value) override
    {
      return false;
    }
  };

  template <class CAttributs>
  class BuiltinAttributs : public NoAttributs<CAttributs>
  {
  public:
    virtual AttributValue getAttribut(const char *name) override
    {
      struct Visitor : public IAttributsVisitor {
        AttributValue attributValue;
        const char* attributName;
        Visitor(const char* name):attributName(name),attributValue(){}
        virtual void visit(const char *name, AttributValue value) override { 
          if(!strcmp(name, this->attributName)) this->attributValue = value;
        }
      };
      Visitor visitor(name);
      this->visitAttributs(&visitor);
      return visitor.attributValue;
    }
  };

  struct CaseInsensitive { 
    bool operator() (const std::string& lhs, const std::string& rhs) const {
      return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
    }
  };
  struct CaseSensitive { 
    bool operator() (const std::string& lhs, const std::string& rhs) const {
      return strcmp(lhs.c_str(), rhs.c_str()) < 0;
    }
  };

  template <class CAttributs, class KeyCompare>
  class StringMapBasedAttributs : public NoAttributs<CAttributs>
  {
  public:
    typedef std::map<std::string, std::string, KeyCompare> tAttributs;

    tAttributs attributs;

    virtual void visitAttributs(webx::IAttributsVisitor *visitor) override
    {
      tAttributs::const_iterator end = this->attributs.end();
      for (tAttributs::const_iterator it = this->attributs.begin(); it != end; ++it)
      {
        visitor->visit(it->first.c_str(), it->second.c_str());
      }
    }
    virtual AttributValue getAttribut(const char *name) override
    {
      tAttributs::iterator it = this->attributs.find(name);
      if (it != this->attributs.end()) {
        return this->attributs[name].c_str();
      }
      return AttributValue();
    }
    virtual bool setAttribut(const char *name, AttributValue value) override
    {
      this->attributs[name] = value.toString();
      return true;
    }
    void print()
    {
      tAttributs::const_iterator end = this->attributs.end();
      for (tAttributs::const_iterator it = this->attributs.begin(); it != end;
        ++it)
      {
        std::cout << it->first << ": " << it->second << "\n";
      }
    }
  };

} // namespace webx
#endif