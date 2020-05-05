#include <iostream>
#include <ace/Log_Msg.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>

#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "MessengerTypeSupportImpl.h"

int main(int argc, char* argv[]) {

    try {
        // Init DomainParticipantFactory
        DDS::DomainParticipantFactory_var dpf =
          TheParticipantFactoryWithArgs(argc, argv);
        DDS::DomainParticipant_var participant =
            dpf->create_participant(42, //domainID
                PARTICIPANT_QOS_DEFAULT,  // Qos
                0,  // No Listener
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
        // Create Publisher
        DDS::Publisher_var publisher =
            participant->create_publisher(
                PUBLISHER_QOS_DEFAULT,  // Qos
                0, // No Listener
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!publisher) {
            std::cerr << "create_publisher failed" << std::endl;
            return 1;
        }

        // Create DataWriter
        DDS::DataWriter_var writer =
            publisher->create_datawriter(
                topic,  // Topic
                DATAWRITER_QOS_DEFAULT,
                0,
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!writer) {
            std::cerr << "create_datawriter failed." << std::endl;
            return 1;
        }

        // Narrow the data wirter ref to a MessageDataWriter to use
        // type-specific publication
        Messenger::MessageDataWriter_var message_writer =
            Messenger::MessageDataWriter::_narrow(writer);

        if (!message_writer) {
            std::cerr << "message_writer failed." << std::endl;
            return 1;
        }

        /************************************
        1) Get the status condition from the data writer we created
        2) Enable the Publication Matched status in the condition
        3) Create a wait set
        4) Attach the status condition to the wait set
        5) Get the publication matched status
        6) If the current count of matches is one or more, detach the condition from the wait set
        and proceed to publication
        7) Wait on the wait set (can be bounded by a specifed period of time)
        8) Loop back around to step 5
        */

        // Block until Subscriber is available
        DDS::StatusCondition_var condition = writer->get_statuscondition();
        condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
        DDS::WaitSet_var ws = new DDS::WaitSet;
        ws->attach_condition(condition);

        while (true) {
            // 2)
            DDS::PublicationMatchedStatus matches;
            if (writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
                std::cerr << "get_publication_matched_status failed." << std::endl;
                return 1;
            }

            if (matches.current_count >= 1) {
                break;
            }
            // 3)
            DDS::ConditionSeq conditions;
            DDS::Duration_t timeout = { 60, 0 };
            if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
                std::cerr << "wait failed" << std::endl;
                return 1;
            }
        }

        ws->detach_condition(condition);
        //***********************************

        // Write samples
        Messenger::Message message;
        message.subject_id = 99;
        message.from       = "Comic Book Guy";
        message.subject    = "Review";
        message.text       = "Worst. Movie. Ever.";
        message.count      = 0;
        for (int i = 0; i < 10; ++i) {
            DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
            ++message.count;
            ++message.subject_id;

            if (error != DDS::RETCODE_OK) {
                std::cerr << "Write failed" << std::endl;
                return 1;
            }
        }

        // Wait for samples to be acknowledged
        DDS::Duration_t timeout = { 30, 0 };
        if (message_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
            std::cerr << "wait_for_acknowledgments" << std::endl;
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