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

#ifndef SST_CORE_LINK_H
#define SST_CORE_LINK_H

#include "sst/core/event.h"
#include "sst/core/serialization/serialize_impl_fwd.h"
#include "sst/core/sst_types.h"
#include "sst/core/timeConverter.h"

namespace SST {

class Link;

extern bool gUseVirtualLinks;
Link* newLink(LinkId_t tag);

#define _LINK_DBG(fmt, args...) __DBG(DBG_LINK, Link, fmt, ##args)

class ActivityQueue;
class BaseComponent;
class TimeConverter;
class LinkPair;
class Simulation_impl;

class UnitAlgebra;
class LinkSendProfileToolList;

namespace Profile {
class EventHandlerProfileTool;
}

template <>
class SST::Core::Serialization::serialize_impl<Link*>
{
    template <class A>
    friend class serialize;
    // Function implemented in link.cc
    void operator()(Link*& s, SST::Core::Serialization::serializer& ser);
    void operator()(Link*& s, SST::Core::Serialization::serializer& ser, const char* name);
};

int getLinksCount();

/** Link between two components. Carries events */
class alignas(64) Link
{
    enum Type_t : uint16_t { POLL, HANDLER, SYNC, UNINITIALIZED };
    enum Mode_t : uint16_t { INIT, RUN, COMPLETE };

    friend class SST::Core::Serialization::serialize_impl<Link*>;

public:
    friend class LinkPair;
    friend class RankSync;
    friend class ThreadSync;
    friend class Simulation_impl;
    friend class SyncManager;
    friend class ComponentInfo;
    friend Link* newLink(LinkId_t tag);

    virtual ~Link();

    /** Set additional Latency to be added to events being sent out of this link
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    virtual void addSendLatency(int cycles, const std::string& timebase);

    /** Set additional Latency to be added to events being sent out of this link
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    virtual void addSendLatency(SimTime_t cycles, TimeConverter* timebase);

    /** Set additional Latency to be added on to events coming in on this link.
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    virtual void addRecvLatency(int cycles, const std::string& timebase);

    /** Set additional Latency to be added on to events coming in on this link.
     * @param cycles Number of Cycles to be added
     * @param timebase Base Units of cycles
     */
    virtual void addRecvLatency(SimTime_t cycles, TimeConverter* timebase);

    /** Set the callback function to be called when a message is
     * delivered. Not available for Polling links.
     * @param functor Functor to call when message is delivered
     */
    virtual void setFunctor(Event::HandlerBase* functor);

    /** Replace the callback function to be called when a message is
     * delivered. Any previous handler will be deleted.
     * Not available for Polling links.
     * @param functor Functor to call when message is delivered
     */
    virtual void replaceFunctor(Event::HandlerBase* functor);

    /** Send an event over the link with additional delay. Sends an event
     * over a link with an additional delay specified with a
     * TimeConverter. I.e. the total delay is the link's delay + the
     * additional specified delay.
     * @param delay - additional delay
     * @param tc - time converter to specify units for the additional delay
     * @param event - the Event to send
     */
    virtual void send(SimTime_t delay, TimeConverter* tc, Event* event)
    {
        send_impl(tc->convertToCoreTime(delay), event);
    }

    /** Send an event with additional delay. Sends an event over a link
     * with additional delay specified by the Link's default
     * timebase.
     * @param delay The additional delay, in units of the default Link timebase
     * @param event The event to send
     */
    virtual void send(SimTime_t delay, Event* event) { send_impl(delay * defaultTimeBase, event); }

    /** Send an event with the Link's default delay
     * @param event The event to send
     */
    virtual void send(Event* event) { send_impl(0, event); }


    /** Retrieve a pending event from the Link. For links which do not
     * have a set event handler, they can be polled with this function.
     * Returns nullptr if there is no pending event.
     * Not available for HANDLER-type links.
     * @return Event if one is available
     * @return nullptr if no Event is available
     */
    virtual Event* recv();

    /** Manually set the default detaulTimeBase
     * @param tc TimeConverter object for the timebase
     */
    virtual void setDefaultTimeBase(TimeConverter* tc);

    /** Return the default Time Base for this link
     * @return the default Time Base for this link
     */
    virtual TimeConverter* getDefaultTimeBase();

    /** Return the default Time Base for this link
     * @return the default Time Base for this link
     */
    virtual const TimeConverter* getDefaultTimeBase() const;

    /** Return the ID of this link
     * @return the unique ID for this link
     */
    virtual LinkId_t getId() { return tag; }

    /** Send data during the init() or complete() phase.
     * @param data event to send
     */
    virtual void sendUntimedData(Event* data);

    /** Receive an event (if any) during the init() or complete() phase.
     * @return Event if one is available
     * @return nullptr if no Event is available
     */
    virtual Event* recvUntimedData();

    /** Return whether link has been configured
     * @return whether link is configured
     */
    virtual bool isConfigured() { return type != UNINITIALIZED; }

#ifdef __SST_DEBUG_EVENT_TRACKING__
    void setSendingComponentInfo(const std::string& comp_in, const std::string& type_in, const std::string& port_in)
    {
        comp  = comp_in;
        ctype = type_in;
        port  = port_in;
    }

    const std::string& getSendingComponentName() { return comp; }
    const std::string& getSendingComponentType() { return ctype; }
    const std::string& getSendingPort() { return port; }

#endif

    virtual void report() const;

protected:
    Link();

    virtual void setAsSyncLink() { type = SYNC; }

    /**
       Set the delivery_info for the link
     */
    virtual void setDeliveryInfo(uintptr_t info) { delivery_info = info; }

    /** Send an event over the link with additional delay. Sends an event
     * over a link with an additional delay specified with a
     * TimeConverter. I.e. the total delay is the link's delay + the
     * additional specified delay.
     * @param delay - additional total delay to add
     * @param event - the Event to send
     */
    virtual void send_impl(SimTime_t delay, Event* event);

    // Since Links are found in pairs, I will keep all the information
    // needed for me to send and deliver an event to the other side of
    // the link.  That means, that I mostly keep my pair's
    // information.  The one consequence, is that polling links will
    // have to pull the data from the pair, but since this is a less
    // common case, that's okay (this decision makes the common case
    // faster and the less common case slower).

    /** Queue of events to be received by the owning component */
    ActivityQueue* send_queue;

    /** Holds the delivery information.  This is stored as a
      uintptr_t, but is actually a pointer converted using
      reinterpret_cast.  For links connected to a
      Component/SubComponent, this holds a pointer to the delivery
      functor.  For links connected to a Sync object, this holds a
      pointer to the remote link to send the event on after
      synchronization.
    */
    uintptr_t delivery_info;

    /** Timebase used if no other timebase is specified. Used to specify
      the units for added delays when sending, such as in
      Link::send(). Often set by the Component::registerClock()
      function if the regAll argument is true.
      */
    SimTime_t defaultTimeBase;

    /** Latency of the link. It is used by the partitioner as the
      weight. This latency is added to the delay put on the event by
      the component.
    */
    SimTime_t latency;

    /** Pointer to the opposite side of this link */
    Link* pair_link;

private:
    friend class BaseComponent;

    SimTime_t& current_time;
    Type_t     type;
    Mode_t     mode;
    LinkId_t   tag;

protected:
    /** Create a new link with a given tag

        The tag is used for two different things depending on where
        this link sends data:

        If it sends it to a Sync object, then it represents the
        remote_tag used to lookup the correct link on the other side.

        If it sends to a TimeVortex (or DirectLinkQueue), it is the
        value used for enforce_link_order (if that feature is
        enabled).
     */
    Link(LinkId_t tag);

private:
    Link(const Link& l);

    /** Specifies that this link has no callback, and is poll-based only */
    virtual void setPolling();

    /** Causes an event to be delivered to the registered callback */
    virtual void deliverEvent(Event* event) const { (*reinterpret_cast<Event::HandlerBase*>(delivery_info))(event); }

    /** Set minimum link latency */
    virtual void setLatency(Cycle_t lat);

    virtual void sendUntimedData_sync(Event* data);
    virtual void finalizeConfiguration();
    virtual void prepareForComplete();

    virtual std::string
    createUniqueGlobalLinkName(RankInfo local_rank, uintptr_t local_ptr, RankInfo remote_rank, uintptr_t remote_ptr);


    virtual void addProfileTool(SST::Profile::EventHandlerProfileTool* tool, const EventHandlerMetaData& mdata);


    LinkSendProfileToolList* profile_tools;


#ifdef __SST_DEBUG_EVENT_TRACKING__
    std::string comp;
    std::string ctype;
    std::string port;
#endif
};

/** Self Links are links from a component to itself */
class SelfLink : public Link
{
public:
    SelfLink() : Link()
    {
        pair_link = this;
        latency   = 0;
    }
};

class VirtualLink : public Link
{
public:
    VirtualLink(int tag);

    virtual void addSendLatency(int cycles, const std::string& timebase);
    virtual void addSendLatency(SimTime_t cycles, TimeConverter* timebase);
    virtual void addRecvLatency(int cycles, const std::string& timebase);
    virtual void addRecvLatency(SimTime_t cycles, TimeConverter* timebase);
    virtual void setFunctor(Event::HandlerBase* functor);
    virtual void replaceFunctor(Event::HandlerBase* functor);
    virtual void send(SimTime_t delay, TimeConverter* tc, Event* event);
    virtual void send(SimTime_t delay, Event* event);
    virtual void send(Event* event);
    virtual Event* recv();
    virtual void setDefaultTimeBase(TimeConverter* tc);
    virtual TimeConverter* getDefaultTimeBase();
    virtual const TimeConverter* getDefaultTimeBase() const;
    virtual LinkId_t getId();
    virtual void sendUntimedData(Event* data);
    virtual Event* recvUntimedData();
    virtual bool isConfigured();
    virtual void report() const;

protected:
    virtual void setAsSyncLink();
    virtual void setDeliveryInfo(uintptr_t info);
    virtual void send_impl(SimTime_t delay, Event* event);

private:
    virtual void setPolling();
    virtual void deliverEvent(Event* event) const;
    virtual void setLatency(Cycle_t lat);
    virtual void sendUntimedData_sync(Event* data);
    virtual void finalizeConfiguration();
    virtual void prepareForComplete();
    virtual std::string createUniqueGlobalLinkName(RankInfo local_rank, uintptr_t local_ptr, RankInfo remote_rank, uintptr_t remote_ptr);
    virtual void addProfileTool(SST::Profile::EventHandlerProfileTool* tool, const EventHandlerMetaData& mdata);
};

} // namespace SST

#endif // SST_CORE_LINK_H
