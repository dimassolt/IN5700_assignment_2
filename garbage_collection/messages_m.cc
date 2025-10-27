//
// Generated file, do not edit! Created by opp_msgtool 6.2 from garbage_collection/messages.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "messages_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace garbage_collection {

Register_Class(GarbagePacket)

GarbagePacket::GarbagePacket(const char *name, short kind) : ::omnetpp::cPacket(name, kind)
{
}

GarbagePacket::GarbagePacket(const GarbagePacket& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

GarbagePacket::~GarbagePacket()
{
}

GarbagePacket& GarbagePacket::operator=(const GarbagePacket& other)
{
    if (this == &other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void GarbagePacket::copy(const GarbagePacket& other)
{
    this->command = other.command;
    this->canId = other.canId;
    this->isFull_ = other.isFull_;
    this->travelTime = other.travelTime;
    this->note = other.note;
}

void GarbagePacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->command);
    doParsimPacking(b,this->canId);
    doParsimPacking(b,this->isFull_);
    doParsimPacking(b,this->travelTime);
    doParsimPacking(b,this->note);
}

void GarbagePacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->command);
    doParsimUnpacking(b,this->canId);
    doParsimUnpacking(b,this->isFull_);
    doParsimUnpacking(b,this->travelTime);
    doParsimUnpacking(b,this->note);
}

const char * GarbagePacket::getCommand() const
{
    return this->command.c_str();
}

void GarbagePacket::setCommand(const char * command)
{
    this->command = command;
}

int GarbagePacket::getCanId() const
{
    return this->canId;
}

void GarbagePacket::setCanId(int canId)
{
    this->canId = canId;
}

bool GarbagePacket::isFull() const
{
    return this->isFull_;
}

void GarbagePacket::setIsFull(bool isFull)
{
    this->isFull_ = isFull;
}

double GarbagePacket::getTravelTime() const
{
    return this->travelTime;
}

void GarbagePacket::setTravelTime(double travelTime)
{
    this->travelTime = travelTime;
}

const char * GarbagePacket::getNote() const
{
    return this->note.c_str();
}

void GarbagePacket::setNote(const char * note)
{
    this->note = note;
}

class GarbagePacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_command,
        FIELD_canId,
        FIELD_isFull,
        FIELD_travelTime,
        FIELD_note,
    };
  public:
    GarbagePacketDescriptor();
    virtual ~GarbagePacketDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(GarbagePacketDescriptor)

GarbagePacketDescriptor::GarbagePacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(garbage_collection::GarbagePacket)), "omnetpp::cPacket")
{
    propertyNames = nullptr;
}

GarbagePacketDescriptor::~GarbagePacketDescriptor()
{
    delete[] propertyNames;
}

bool GarbagePacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<GarbagePacket *>(obj)!=nullptr;
}

const char **GarbagePacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *GarbagePacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int GarbagePacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 5+base->getFieldCount() : 5;
}

unsigned int GarbagePacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_command
        FD_ISEDITABLE,    // FIELD_canId
        FD_ISEDITABLE,    // FIELD_isFull
        FD_ISEDITABLE,    // FIELD_travelTime
        FD_ISEDITABLE,    // FIELD_note
    };
    return (field >= 0 && field < 5) ? fieldTypeFlags[field] : 0;
}

const char *GarbagePacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "command",
        "canId",
        "isFull",
        "travelTime",
        "note",
    };
    return (field >= 0 && field < 5) ? fieldNames[field] : nullptr;
}

int GarbagePacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "command") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "canId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "isFull") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "travelTime") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "note") == 0) return baseIndex + 4;
    return base ? base->findField(fieldName) : -1;
}

const char *GarbagePacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "string",    // FIELD_command
        "int",    // FIELD_canId
        "bool",    // FIELD_isFull
        "double",    // FIELD_travelTime
        "string",    // FIELD_note
    };
    return (field >= 0 && field < 5) ? fieldTypeStrings[field] : nullptr;
}

const char **GarbagePacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *GarbagePacketDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int GarbagePacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void GarbagePacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'GarbagePacket'", field);
    }
}

const char *GarbagePacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string GarbagePacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        case FIELD_command: return oppstring2string(pp->getCommand());
        case FIELD_canId: return long2string(pp->getCanId());
        case FIELD_isFull: return bool2string(pp->isFull());
        case FIELD_travelTime: return double2string(pp->getTravelTime());
        case FIELD_note: return oppstring2string(pp->getNote());
        default: return "";
    }
}

void GarbagePacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        case FIELD_command: pp->setCommand((value)); break;
        case FIELD_canId: pp->setCanId(string2long(value)); break;
        case FIELD_isFull: pp->setIsFull(string2bool(value)); break;
        case FIELD_travelTime: pp->setTravelTime(string2double(value)); break;
        case FIELD_note: pp->setNote((value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'GarbagePacket'", field);
    }
}

omnetpp::cValue GarbagePacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        case FIELD_command: return pp->getCommand();
        case FIELD_canId: return pp->getCanId();
        case FIELD_isFull: return pp->isFull();
        case FIELD_travelTime: return pp->getTravelTime();
        case FIELD_note: return pp->getNote();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'GarbagePacket' as cValue -- field index out of range?", field);
    }
}

void GarbagePacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        case FIELD_command: pp->setCommand(value.stringValue()); break;
        case FIELD_canId: pp->setCanId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_isFull: pp->setIsFull(value.boolValue()); break;
        case FIELD_travelTime: pp->setTravelTime(value.doubleValue()); break;
        case FIELD_note: pp->setNote(value.stringValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'GarbagePacket'", field);
    }
}

const char *GarbagePacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr GarbagePacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void GarbagePacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    GarbagePacket *pp = omnetpp::fromAnyPtr<GarbagePacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'GarbagePacket'", field);
    }
}

}  // namespace garbage_collection

namespace omnetpp {

}  // namespace omnetpp

