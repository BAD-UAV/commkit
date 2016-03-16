#include "subscriberimpl.h"
#include "nodeimpl.h"

#include <assert.h>

#include <fastrtps/Domain.h>
#include <fastrtps/attributes/SubscriberAttributes.h>

namespace commkit
{

SubscriberImpl::SubscriberImpl(const Topic &t, std::shared_ptr<NodeImpl> n)
    : frsub(nullptr), matchedPubs(0), node(n), topicName(t.name)
{
    /*
     * NB: we require 'sub' to be initialized separately via setSubscriber(),
     * since we expect to be constructed from the Subscriber() ctor, at which point
     * a shared_ptr to the about to be created Subscriber does not yet exist.
     */
    topicDataType.setName(t.datatype.c_str());

    // ugh, payload size must be defined
    assert(t.maxPayloadSize > 0 && "Topic::maxPayloadSize must be specified");
    topicDataType.setSize(t.maxPayloadSize);
}

SubscriberImpl::~SubscriberImpl()
{
    if (frsub != nullptr) {
        eprosima::fastrtps::Domain::removeSubscriber(frsub);
    }
}

bool SubscriberImpl::init(const SubscriptionOpts &opts)
{
    /*
     * Create subscription from the given opts.
     */

    eprosima::fastrtps::SubscriberAttributes sa;
    sa.topic.topicKind = NO_KEY;
    sa.topic.topicName = name();
    sa.topic.topicDataType = datatype();

    if (opts.reliable) {
        sa.qos.m_reliability.kind = eprosima::fastrtps::RELIABLE_RELIABILITY_QOS;
    } else {
        sa.qos.m_reliability.kind = eprosima::fastrtps::BEST_EFFORT_RELIABILITY_QOS;
    }

    // fast-rtps requires datatype to be regsitered before we can
    // create the publisher.

    eprosima::fastrtps::TopicDataType *tdt;
    if (!eprosima::fastrtps::Domain::getRegisteredType(node->part, datatype().c_str(), &tdt)) {
        eprosima::fastrtps::Domain::registerType(node->part, &topicDataType);
    }

    frsub = eprosima::fastrtps::Domain::createSubscriber(node->part, sa, this);
    return (frsub != nullptr);
}

bool SubscriberImpl::peek(Payload *p)
{
    /*
     * Reads the next sample without removing it from the buffer.
     * Data returned via 'p' is only valid until next call to peek() or take().
     */

    eprosima::fastrtps::SampleInfo_t si;
    if (frsub->readNextData(&topicData, &si) && (si.sampleKind == ALIVE)) {
        p->bytes = topicData.buf;
        p->len = topicData.len;
        return true;
    }

    return false;
}

bool SubscriberImpl::take(Payload *p)
{
    /*
     * Reads the next sample and removes it from the buffer.
     * Data returned via 'p' is only valid until next call to peek() or take().
     */

    eprosima::fastrtps::SampleInfo_t si;
    if (frsub->takeNextData(&topicData, &si) && (si.sampleKind == ALIVE)) {
        p->bytes = topicData.buf;
        p->len = topicData.len;
        return true;
    }

    return false;
}

void SubscriberImpl::onSubscriptionMatched(eprosima::fastrtps::Subscriber *,
                                           eprosima::fastrtps::rtps::MatchingInfo &info)
{
    /*
     * callback from fastrtps indicating our matched subscriber has changed.
     */

    if (info.status == MATCHED_MATCHING) {
        matchedPubs++;
        if (auto sharedSub = sub.lock()) {
            sharedSub->onPublisherConnected(sharedSub);
        }
    } else {
        matchedPubs--;
        if (auto sharedSub = sub.lock()) {
            sharedSub->onPublisherDisconnected(sharedSub);
        }
    }
}

void SubscriberImpl::onNewDataMessage(eprosima::fastrtps::Subscriber *)
{
    /*
     * New data has arrived - inform our delegate
     * They can then either peek() or take() as appropriate.
     */

    if (auto sharedSub = sub.lock()) {
        sharedSub->onMessage(sharedSub);
    }
}

} // namespace commkit
