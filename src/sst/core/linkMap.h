// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_LINKMAP_H
#define SST_CORE_LINKMAP_H

#include "sst/core/component.h"
#include "sst/core/link.h"
#include "sst/core/sst_types.h"

#include <map>
#include <string>

namespace SST {

LinkMap* newLinkMap(int onComponentId);

struct ConstLinkMapIterator
{
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = const std::pair<const std::string, Link*>;
    using pointer           = value_type*;
    using reference         = value_type&;

    virtual reference operator*() const = 0;
    virtual ConstLinkMapIterator& operator++() = 0;
    virtual bool operator!=(const ConstLinkMapIterator& other) const = 0;
};

struct ConstLinkMapIteratorOverMap : ConstLinkMapIterator
{
    std::map<std::string, Link*>::const_iterator linkMapIter;

    ConstLinkMapIteratorOverMap(std::map<std::string, Link*>::const_iterator linkMapIter)
        : linkMapIter(linkMapIter)
    { }

    virtual reference operator*() const { 
      reference ref = *linkMapIter;
      return ref;
    }

    virtual ConstLinkMapIterator& operator++() { linkMapIter++; }
    virtual bool operator!=(const ConstLinkMapIterator& other) const {
        return linkMapIter !=
            dynamic_cast<const ConstLinkMapIteratorOverMap&>(other).linkMapIter;
    }
};

struct ConstLinkMapIteratorForVirtualLinkMap : ConstLinkMapIterator {
    ConstLinkMapIteratorForVirtualLinkMap(ComponentId_t id, int onLink);
    virtual reference operator*() const;
    virtual ConstLinkMapIterator& operator++();
    virtual bool operator!=(const ConstLinkMapIterator& other) const;

  private:
    ComponentId_t id;
    int onLink;
};

struct WrapConstLinkMapIterator
{
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = const std::pair<const std::string, Link*>;
    using pointer           = value_type*;
    using reference         = value_type&;

    ConstLinkMapIterator *iter;

    WrapConstLinkMapIterator(ConstLinkMapIterator *iter) : iter(iter) {}
    ~WrapConstLinkMapIterator() {
      delete iter;
    }

    reference operator*() const { return *(*iter); }
    ConstLinkMapIterator& operator++() { return (*iter).operator++(); }
    bool operator!=(const WrapConstLinkMapIterator& other) const { return *iter != *(other.iter); }
};



/**
 * Maps port names to the Links that are connected to it
 */
class LinkMap
{

private:
    std::map<std::string, Link*> linkMap;
    // const std::vector<std::string> * allowedPorts;
    std::vector<std::string>     selfPorts;

    friend class SST::Core::Serialization::serialize_impl<LinkMap*>;

    virtual void serialize_order(SST::Core::Serialization::serializer& ser)
    {
        ser& linkMap;
        ser& selfPorts;
    }


    // bool checkPort(const char *def, const char *offered) const
    // {
    //     const char * x = def;
    //     const char * y = offered;

    //     /* Special case.  Name of '*' matches everything */
    //     if ( *x == '*' && *(x+1) == '\0' ) return true;

    //     do {
    //         if ( *x == '%' && (*(x+1) == '(' || *(x+1) == 'd') ) {
    //             // We have a %d or %(var)d to eat
    //             x++;
    //             if ( *x == '(' ) {
    //                 while ( *x && (*x != ')') ) x++;
    //                 x++;  /* *x should now == 'd' */
    //             }
    //             if ( *x != 'd') /* Malformed string.  Fail all the things */
    //                 return false;
    //             x++; /* Finish eating the variable */
    //             /* Now, eat the corresponding digits of y */
    //             while ( *y && isdigit(*y) ) y++;
    //         }
    //         if ( *x != *y ) return false;
    //         if ( *x == '\0' ) return true;
    //         x++;
    //         y++;
    //     } while ( *x && *y );
    //     if ( *x != *y ) return false; // aka, both nullptr
    //     return true;
    // }

    // bool checkPort(const std::string& name) const
    // {
    //     // First check to see if this is a self port
    //     for ( std::vector<std::string>::const_iterator i = selfPorts.begin() ; i != selfPorts.end() ; ++i ) {
    //         /* Compare name with stored name, which may have wildcards */
    //         // if ( checkPort(i->c_str(), x) ) {
    //         if ( name == *i ) {
    //             return true;
    //         }
    //     }

    //     // If no a self port, check against info in library manifest
    //     Component::isValidPortForComponent(
    //     const char *x = name.c_str();
    //     bool found = false;
    //     if ( nullptr != allowedPorts ) {
    //         for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i
    //         ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             if ( checkPort(i->c_str(), x) ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }
    //     return found;
    // }

    // bool checkPort(const std::string& name) const
    // {
    //     const char *x = name.c_str();
    //     bool found = false;
    //     if ( nullptr != allowedPorts ) {
    //         for ( std::vector<std::string>::const_iterator i = allowedPorts->begin() ; i != allowedPorts->end() ; ++i
    //         ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             if ( checkPort(i->c_str(), x) ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }

    //     if ( !found ) { // Check self ports
    //         for ( std::vector<std::string>::const_iterator i = selfPorts.begin() ; i != selfPorts.end() ; ++i ) {
    //             /* Compare name with stored name, which may have wildcards */
    //             // if ( checkPort(i->c_str(), x) ) {
    //             if ( name == *i ) {
    //                 found = true;
    //                 break;
    //             }
    //         }
    //     }

    //     return found;
    // }

public:
    LinkMap() /*: allowedPorts(nullptr)*/ {}

    ~LinkMap()
    {
        // Delete all the links in the map
        for ( std::map<std::string, Link*>::iterator it = linkMap.begin(); it != linkMap.end(); ++it ) {
            delete it->second;
        }
        linkMap.clear();
    }

    // /**
    //  * Set the list of allowed port names from the ElementInfoPort
    //  */
    // void setAllowedPorts(const std::vector<std::string> *p)
    // {
    //     allowedPorts = p;
    // }

    /**
     * Add a port name to the list of allowed ports.
     * Used by SelfLinks, as these are undocumented.
     */
    virtual void addSelfPort(const std::string& name) { selfPorts.push_back(name); }

    virtual bool isSelfPort(const std::string& name) const
    {
        for ( std::vector<std::string>::const_iterator i = selfPorts.begin(); i != selfPorts.end(); ++i ) {
            /* Compare name with stored name, which may have wildcards */
            // if ( checkPort(i->c_str(), x) ) {
            if ( name == *i ) { return true; }
        }
        return false;
    }

    /** Inserts a new pair of name and link into the map */
    virtual void insertLink(const std::string& name, Link* link) { linkMap.insert(std::pair<std::string, Link*>(name, link)); }

    virtual void removeLink(const std::string& name) { linkMap.erase(name); }

    /** Returns a Link pointer for a given name */
    virtual Link* getLink(const std::string& name)
    {

        //         if ( !checkPort(name) ) {
        // #ifdef USE_PARAM_WARNINGS
        //             std::cerr << "Warning:  Using undocumented port '" << name << "'." << std::endl;
        // #endif
        //         }
        std::map<std::string, Link*>::iterator it = linkMap.find(name);
        if ( it == linkMap.end() )
            return nullptr;
        else
            return it->second;
    }

    /**
       Checks to see if LinkMap is empty.
       @return True if Link map is empty, false otherwise
    */
    virtual bool empty() { return linkMap.empty(); }

    virtual WrapConstLinkMapIterator begin() const { return WrapConstLinkMapIterator(new ConstLinkMapIteratorOverMap(linkMap.begin())); }
    virtual WrapConstLinkMapIterator end()   const { return WrapConstLinkMapIterator(new ConstLinkMapIteratorOverMap(linkMap.end()));   }

    // FIXME: Kludge for now, fix later.  Need to make LinkMap look
    // like a regular map instead.
    /** Return a reference to the internal map */
//    std::map<std::string, Link*>& getLinkMap() { return linkMap; }
};


class VirtualLinkMap : public LinkMap
{
private:
    ComponentId_t fromComponentId;
    virtual void serialize_order(SST::Core::Serialization::serializer& ser);

public:
    VirtualLinkMap(ComponentId_t fromComponentId);

    virtual void addSelfPort(const std::string& name);
    virtual bool isSelfPort(const std::string& name) const;
    virtual void insertLink(const std::string& name, Link* link);
    virtual void removeLink(const std::string& name);
    virtual Link* getLink(const std::string& name);
    virtual bool empty();
    virtual WrapConstLinkMapIterator begin() const;
    virtual WrapConstLinkMapIterator end() const;
};

} // namespace SST

#endif // SST_CORE_LINKMAP_H
