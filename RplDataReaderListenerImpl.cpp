#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "RplDataReaderListenerImpl.h"
#include "RobotControlTypeSupportC.h"
#include "RobotControlTypeSupportImpl.h"

#include <iostream>

void
RplDataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
RplDataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
RplDataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& /*status*/)
{
}

void
RplDataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/)
{
}

void
RplDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{    robot::RobotControl_ReplyDataReader_var reader_i =
    robot::RobotControl_ReplyDataReader::_narrow(reader);

    if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
        ACE_OS::exit(1);
    }

    robot::RobotControl_Reply reply;
    DDS::SampleInfo info;

    DDS::ReturnCode_t error = reader_i->take_next_sample(reply, info);

    if (error == DDS::RETCODE_OK) {
        // std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
        // std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;
        if (info.valid_data) {
            // std::cout << "RobotControl_Request:" << request.data._d() << std::endl;
            ::CORBA::Long funID = reply.data._d();
            switch(funID) {
            case robot::RobotControl_setSpeed_Hash:
                std::cout << "setSpeed" << std::endl;
                break;
            case robot::RobotControl_getSpeed_Hash:
                {
                  std::cout << "getSpeed" << std::endl;
                  if (DDS::RPC::REMOTE_EX_OK == reply.data.getSpeed()._d()) {
                      float speedR =  reply.data.getSpeed().result().return_;
                      std::cout << "Speed is:" << speedR << std::endl;
                  }
                }
                break;
            default:
                break;
            }
        }

    } else {
        ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" take_next_sample failed!\n")));
    }
}

void
RplDataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& /*status*/)
{
}

void
RplDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}
