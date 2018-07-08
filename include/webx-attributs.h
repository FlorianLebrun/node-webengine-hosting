

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

  // Template for attributs visitor
  template <class CAttributsVisitor>
  class StringAttributsVisitor; // For visitor which support only strings

  // Template for attributs object
  template <class CAttributs>
  class BuiltinAttributs; // For object with hard coded attributs
  template <class CAttributs>
  class NoAttributs; // For object with no attributs
  template <class CAttributs>
  class StringMapBasedAttributs; // For object with std::map attributs

  // Interface of attributs visitor
  class IAttributsVisitor
  {
  public:
    virtual void visitInt(const char *name, int64_t value) = 0;
    virtual void visitFloat(const char *name, double value) = 0;
    virtual void visitString(const char *name, const char *value) = 0;
    virtual void visitObject(const char *name, IAttributs *value) = 0;
  };

  // Interface of attributs object
  class IAttributs : public IReleasable
  {
  public:
    virtual int32_t getAttributCount() = 0;
    virtual void visitAttributs(IAttributsVisitor *visitor) = 0;
    virtual bool hasAttribut(const char *name) = 0;
    virtual bool removeAttribut(const char *name) = 0;

    virtual bool getAttributInt(const char *name, int64_t& value) = 0;
    virtual bool setAttributInt(const char *name, int64_t value) = 0;

    virtual bool getAttributFloat(const char *name, double& value) = 0;
    virtual bool setAttributFloat(const char *name, double value) = 0;

    virtual bool getAttributString(const char *name, const char*& value) = 0;
    virtual bool setAttributString(const char *name, const char *value, int size = -1) = 0;

    template <int defaultValue>
    int64_t getAttributInt(const char *name);

    template <int defaultValue>
    double getAttributFloat(const char *name);

    const char* getAttributString(const char *name);

    void printAttributs();
  };

  // -------------------------------------------------
  // Implementations
  // -------------------------------------------------

  inline void IAttributs::printAttributs()
  {
    struct Visitor : public IAttributsVisitor
    {
      virtual void visitInt(const char *name, int64_t value) override
      {
        printf("%s: %lld\n", name, value);
      }
      virtual void visitFloat(const char *name, double value) override
      {
        printf("%s: %lg\n", name, value);
      }
      virtual void visitString(const char *name, const char *value) override
      {
        printf("%s: %s\n", name, value);
      }
      virtual void visitObject(const char *name, IAttributs *value) override
      {
        if(value) {
          Visitor visitor;
          printf("%s: {", name);
          value->visitAttributs(&visitor);
          printf("}");
        }
      }
    };
    Visitor visitor;
    this->visitAttributs(&visitor);
  }
  
  template <class CAttributsVisitor = IAttributsVisitor>
  class StringAttributsVisitor : public CAttributsVisitor
  {
    virtual void visitObject(const char *name, IAttributs* value) override
    {
    }
    virtual void visitInt(const char *name, int64_t value) override
    {
      char tmp[64];
      this->visitString(name, _i64toa(value, tmp, 10));
    }
    virtual void visitFloat(const char *name, double value) override
    {
      char tmp[64];
      _gcvt_s(tmp, 64, value, 64);
      this->visitString(name, tmp);
    }
  };

  template <class CAttributs>
  class NoAttributs : public CAttributs
  {
  public:
    virtual int32_t getAttributCount() override
    {
      return 0;
    }
    virtual void visitAttributs(IAttributsVisitor *visitor) override
    {
    }
    virtual bool hasAttribut(const char *name) override
    {
      return false;
    }
    virtual bool removeAttribut(const char *name) override
    {
      return false;
    }
    virtual bool getAttributInt(const char *name, int64_t& value) override
    {
      return false;
    }
    virtual bool setAttributInt(const char *name, int64_t value) override
    {
      return false;
    }
    virtual bool getAttributFloat(const char *name, double& value) override
    {
      return false;
    }
    virtual bool setAttributFloat(const char *name, double value) override
    {
      return false;
    }
    virtual bool getAttributString(const char *name, const char*& value) override
    {
      return false;
    }
    virtual bool setAttributString(const char *name, const char *value, int size = -1) override
    {
      return false;
    }
  };

  template <class CAttributs>
  class BuiltinAttributs : public NoAttributs<CAttributs>
  {
  public:
    virtual int32_t getAttributCount() override
    {
      struct Visitor : public IAttributsVisitor {
        int count;
        Visitor():count(0){}
        virtual void visitInt(const char *name, int64_t value) override { this->count++; }
        virtual void visitFloat(const char *name, double value) override { this->count++; }
        virtual void visitString(const char *name, const char *value) override { this->count++; }
        virtual void visitObject(const char *name, IAttributs *value) override { this->count++; }
      };
      Visitor visitor;
      this->visitAttributs(&visitor);
      return visitor.count;
    }
    virtual bool hasAttribut(const char *name) override
    {
      struct Visitor : public IAttributsVisitor {
        bool hasAttribut;
        const char* attributName;
        Visitor(const char* name):attributName(name),hasAttribut(false){}
        void match(const char* name) {if(!strcmp(name, this->attributName)) this->hasAttribut=true;}
        virtual void visitInt(const char *name, int64_t value) override {this->match(name);}
        virtual void visitFloat(const char *name, double value) override {this->match(name);}
        virtual void visitString(const char *name, const char *value) override { this->match(name); }
        virtual void visitObject(const char *name, IAttributs *value) override { this->match(name); }
      };
      Visitor visitor(name);
      this->visitAttributs(&visitor);
      return visitor.hasAttribut;
    }
  };

  template <class CAttributs>
  class StringMapBasedAttributs : public NoAttributs<CAttributs>
  {
  public:
    typedef std::map<std::string, std::string> tAttributs;

    tAttributs attributs;

    virtual bool hasAttribut(const char *name) override
    {
      return this->attributs.find(name) != this->attributs.end();
    }
    virtual int getAttributCount() override
    {
      return this->attributs.size();
    }
    virtual void visitAttributs(webx::IAttributsVisitor *visitor) override
    {
      tAttributs::const_iterator end = this->attributs.end();
      for (tAttributs::const_iterator it = this->attributs.begin(); it != end; ++it)
      {
        visitor->visitString(it->first.c_str(), it->second.c_str());
      }
    }
    virtual bool getAttributInt(const char *name, int64_t& value) override
    {
      const char* s;
      if (this->getAttributString(name, s)) {
        value = _atoi64(s);
        return true;
      }
      return false;
    }
    virtual bool setAttributInt(const char *name, int64_t value) override
    {
      char tmp[32];
      _i64toa_s(value, tmp, 32, 10);
      return this->setAttributString(name, tmp, -1);
    }
    virtual bool getAttributFloat(const char *name, double& value) override
    {
      const char* s;
      if (this->getAttributString(name, s)) {
        _CRT_DOUBLE result;
        _atodbl(&result, (char *)s);
        value = result.x;
        return true;
      }
      return false;
    }
    virtual bool setAttributFloat(const char *name, double value) override
    {
      char tmp[64];
      _gcvt_s(tmp, 64, value, 64);
      return this->setAttributString(name, tmp, -1);
    }
    virtual bool getAttributString(const char *name, const char*& value) override
    {
      tAttributs::iterator it = this->attributs.find(name);
      if (it != this->attributs.end()) {
        value = this->attributs[name].c_str();
        return true;
      }
      return false;
    }
    virtual bool setAttributString(const char *name, const char *value, int size) override
    {
      if(value) {
        if (size < 0)
          this->attributs[name] = value;
        else
          this->attributs[name] = std::string(value, size);
        return true;
      }
      return false;
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

  template <int defaultValue>
  inline int64_t IAttributs::getAttributInt(const char *name)
  {
    int64_t value = defaultValue;
    this->getAttributInt(name, value);
    return value;
  }
  template <int defaultValue>
  inline double IAttributs::getAttributFloat(const char *name)
  {
    double value = defaultValue;
    this->getAttributFloat(name, value);
    return value;
  }
  inline const char* IAttributs::getAttributString(const char *name)
  {
    const char* value = 0;
    this->getAttributString(name, value);
    return value;
  }
} // namespace webx
#endif