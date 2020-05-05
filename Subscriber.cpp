#include <iostream>
#include <ace/Log_Msg.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsSubscriptionC.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "DataReaderListenerImpl.h"
#include "MessengerTypeSupportImpl.h"

int main (int argc, char* argv[])
{
    try {
        // Initialize DomainParticipantFactory
        DDS::DomainParticipantFactory_var dpf =
          TheParticipantFactoryWithArgs(argc, argv);

        // Create DomainParticipant
        DDS::DomainParticipant_var participant =
          dpf->create_participant(42,
                                  PARTICIPANT_QOS_DEFAULT,
                                  0,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!participant) {
            std::cerr << "create_participant failed." << std::endl;
            return 1;
        }

        // Registering the Data Type
        Messenger::MessageTypeSupport_var ts =
            new Messenger::MessageTypeSupportImpl();
        if (DDS::RETCODE_OK != ts->register_type(participant, "")) {
            std::cerr << "register_type failed." << std::endl;
            return 1;
        }

        // Creating a Topic
        CORBA::String_var type_name = ts->get_type_name();
        DDS::Topic_var topic =
            participant->create_topic(
                "Movie Discussion List", // Topic Name
                type_name,
                TOPIC_QOS_DEFAULT,
                0,
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!topic) {
            std::cerr << "create_topic failed." << std::endl;
            return 1;
        }
//------------------------------------------------------------
        // Create Subscriber
        DDS::Subscriber_var subscriber =
            participant->create_subscriber(
                SUBSCRIBER_QOS_DEFAULT,
                0,
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!subscriber) {
            std::cerr << "create_subscriber failed." << std::endl;
            return 1;
        }

        // Create DataReder and Listener
        DDS::DataReaderListener_var listener(new DataReaderListenerImpl);

        DDS::DataReaderQos reader_qos;
        subscriber->get_default_datareader_qos(reader_qos);
        reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

        DDS::DataReader_var reader =
            subscriber->create_datareader(
                topic, // Topic
                reader_qos,
                listener,   // Litstener
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        //narrow
        Messenger::MessageDataReader_var reader_i =
            Messenger::MessageDataReader::_narrow(reader);
        if (!reader_i) {
            std::cerr << "reader_i failed" << std::endl;
            return 1;
        }

        // Block until Publisher completes
        DDS::StatusCondition_var condition = reader->get_statuscondition();
        condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);

        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        while (true) {
            DDS::SubscriptionMatchedStatus matches;
            if (reader->get_subscription_matched_status(matches) != DDS::RETCODE_OK) {
                std::cerr << "get_subscription_matched_status failed" << std::endl;
                return 1;
            }

            if (matches.current_count == 0 && matches.total_count > 0) {
                break;
            }

            DDS::ConditionSeq conditions;
            DDS::Duration_t timeout = { 60, 0 };
            if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
                std::cerr << "wait failed" << std::endl;
                return 1;
            }
        }

        // Clean-up!
        participant->delete_contained_entities();
        dpf->delete_participant(participant);

        TheServiceParticipant->shutdown();
    } catch (const CORBA::Exception& e) {
        e._tao_print_exception("Exception caught in main():");
        return 1;
    }

    return 0;
}