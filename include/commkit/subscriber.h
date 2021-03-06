#pragma once

#include <chrono>
#include <string>

#include <commkit/callback.h>
#include <commkit/chrono.h>
#include <commkit/topic.h>
#include <commkit/types.h>
#include <commkit/visibility.h>

#ifndef COMMKIT_NO_CAPNP
#include <capnp/serialize.h>
#endif

namespace commkit
{

class NodeImpl;
class Subscriber;
class SubscriberImpl;

/*
 * Options to configure a Subscriber.
 */
struct COMMKIT_API SubscriptionOpts {
    bool reliable;
    int timeBasedFilterHere; // todo
    unsigned history;

    SubscriptionOpts() : reliable(false), timeBasedFilterHere(0), history(1)
    {
    }
};

/*
 * Type to return received data.
 * Subscriber::peek() and Subscriber::take() populate this with
 * a pointer to internally received data, in order to avoid an
 * additional copy step.
 */
struct COMMKIT_API Payload {
    uint8_t *bytes;
    size_t len;
    int64_t sequence;
    /*
     * Time the message was sent by the publisher (based on the
     * publisher's clock)
     */
    clock::time_point sourceTimestamp;

    Payload()
        : bytes(nullptr), len(0), sequence(SEQUENCE_NUMBER_INVALID),
          sourceTimestamp(TIME_POINT_INVALID)
    {
    }

#ifndef COMMKIT_NO_CAPNP
    capnp::FlatArrayMessageReader toReader(bool *ok = nullptr);
#endif // COMMKIT_NO_CAPNP
};

/*
 * Subscriber subscribes to a topic described by a name and datatype.
 * It reports new
 */
class COMMKIT_API Subscriber
{
public:
    ~Subscriber();

    bool init(const SubscriptionOpts &opts);

    bool peek(Payload *p);
    bool take(Payload *p);
    void waitForMessage();
    unsigned matchedPublishers() const;

    // XXX: make this const once TopicDataType.getName() is const
    std::string datatype();
    std::string name() const;

    Callback<void(const SubscriberPtr)> onPublisherConnected;
    Callback<void(const SubscriberPtr)> onPublisherDisconnected;
    Callback<void(SubscriberPtr)> onMessage;

private:
    Subscriber(const Topic &t, std::shared_ptr<NodeImpl> n);

    std::unique_ptr<SubscriberImpl> impl;

    friend class Node; // for private ctor
};

} // namespace commkit
