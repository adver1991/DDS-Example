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

#include "RobotControlTypeSupportImpl.h"
#include "RplDataReaderListenerImpl.h"


void getSpeedASync(robot::RobotControl_RequestDataWriter_var message_writer) {
    robot::RobotControl_Request request;
    robot::RobotControl_getSpeed_In method;
    request.data.getSpeed(method);

    DDS::ReturnCode_t error = message_writer->write(request, DDS::HANDLE_NIL);

    if (error != DDS::RETCODE_OK) {
        std::cerr << "Write failed" << std::endl;
    }
}

void setSpeedASync(float speed, robot::RobotControl_RequestDataWriter_var message_writer) {
    robot::RobotControl_Request request;
    robot::RobotControl_setSpeed_In method;
    method.speed = speed;
    request.data.setSpeed(method);

    DDS::ReturnCode_t error = message_writer->write(request, DDS::HANDLE_NIL);

    if (error != DDS::RETCODE_OK) {
        std::cerr << "Write failed" << std::endl;
    }
}


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

        // Registering the Request Data Type
        robot::RobotControl_RequestTypeSupport_var qs=
            new robot::RobotControl_RequestTypeSupportImpl();
        if (DDS::RETCODE_OK != qs->register_type(participant, "")) {
            std::cerr << "register_type failed." << std::endl;
            return 1;
        }

        // Creating a Request Topic
        CORBA::String_var type_name = qs->get_type_name();
        DDS::Topic_var requestTopic =
            participant->create_topic(
                "RobotRequestTopic", // Topic Name
                type_name,
                TOPIC_QOS_DEFAULT,
                0,
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!requestTopic) {
            std::cerr << "create_topic failed." << std::endl;
            return 1;
        }


        // Registering the Reply Data type
        robot::RobotControl_ReplyTypeSupport_var rs=
            new robot::RobotControl_ReplyTypeSupportImpl();
        if (DDS::RETCODE_OK != rs->register_type(participant, "")) {
            std::cerr << "reply_type failed." << std::endl;
            return 1;
        }

        // Creating a Reply Topic
        CORBA::String_var rp_type_name = rs->get_type_name();
        DDS::Topic_var replyTopic =
            participant->create_topic(
                "RobotReplyTopic", // Topic Name
                rp_type_name,
                TOPIC_QOS_DEFAULT,
                0,
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
        if (!replyTopic) {
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
                requestTopic,  // Topic
                DATAWRITER_QOS_DEFAULT,
                0,
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        if (!writer) {
            std::cerr << "create_datawriter failed." << std::endl;
            return 1;
        }

        // Narrow the data wirter ref to a MessageDataWriter to use
        // type-specific publication
        robot::RobotControl_RequestDataWriter_var message_writer =
            robot::RobotControl_RequestDataWriter::_narrow(writer);

        if (!message_writer) {
            std::cerr << "message_writer failed." << std::endl;
            return 1;
        }

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
        DDS::DataReaderListener_var listener(new RplDataReaderListenerImpl);
        DDS::DataReaderQos reader_qos;
        subscriber->get_default_datareader_qos(reader_qos);
        reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

        DDS::DataReader_var reader =
            subscriber->create_datareader(
                replyTopic, // Topic
                reader_qos,
                listener,   // Litstener
                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

        //narrow
        robot::RobotControl_ReplyDataReader_var reader_i =
            robot::RobotControl_ReplyDataReader::_narrow(reader);
        if (!reader_i) {
            std::cerr << "reader_i failed" << std::endl;
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
        DDS::StatusCondition_var rq_condition = writer->get_statuscondition();
        rq_condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
        DDS::WaitSet_var rq_ws = new DDS::WaitSet;
        rq_ws->attach_condition(rq_condition);

        DDS::StatusCondition_var rp_condition = reader->get_statuscondition();
        rp_condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
        DDS::WaitSet_var rp_ws = new DDS::WaitSet;
        rp_ws->attach_condition(rp_condition);

        // while (true) {
        //     // 2)
            DDS::PublicationMatchedStatus rq_matches;
            DDS::SubscriptionMatchedStatus rp_matches;
            if (writer->get_publication_matched_status(rq_matches) != ::DDS::RETCODE_OK
                || reader->get_subscription_matched_status(rp_matches) != ::DDS::RETCODE_OK) {
                std::cerr << "get_publication_matched_status failed." << std::endl;
                return 1;
            }

            // if (matches.current_count >= 1) {
            //     break;
            // }
            // 3)
            DDS::ConditionSeq rq_conditions;
            DDS::ConditionSeq rp_conditions;
            DDS::Duration_t timeout = { 60, 0 };
            if (rq_ws->wait(rq_conditions, timeout) != DDS::RETCODE_OK) {
                std::cerr << "wait failed" << std::endl;
                return 1;
            }
            rq_ws->detach_condition(rq_condition);
            std::cout << "connet rq(writer)" << std::endl;

            if (rp_ws->wait(rp_conditions, timeout) != DDS::RETCODE_OK) {
                std::cerr << "wait failed" << std::endl;
                return 1;
            }
            rp_ws->detach_condition(rp_condition);
            std::cout << "connet rp(writer)" << std::endl;
        // }

        //***********************************

        // Write samples
        setSpeedASync(10, message_writer);
        sleep(10);
        getSpeedASync(message_writer);
        // // Wait for samples to be acknowledged
        // if (message_writer->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
        //     std::cerr << "wait_for_acknowledgments" << std::endl;
        // }

        while(true) {
            if (writer->get_publication_matched_status(rq_matches) != ::DDS::RETCODE_OK
                || reader->get_subscription_matched_status(rp_matches) != ::DDS::RETCODE_OK) {
                std::cerr << "get_publication_matched_status failed." << std::endl;
                return 1;
            }

            sleep(100);
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