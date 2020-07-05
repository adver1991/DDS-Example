#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include "ReqDataReaderListenerImpl.h"
#include "RobotControlTypeSupportC.h"
#include "RobotControlTypeSupportImpl.h"

#include <iostream>

float My_Local_Robot::m_speed = 50;

void My_Local_Robot::setSpeed(float speed)
{
  std::cout << "call SetSpeed:" << speed << std::endl;
  m_speed = speed;
}

float My_Local_Robot::getSpeed()
{
  std::cout << "call getSpeed:" << std::endl;
  return m_speed;
}

void
ReqDataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedDeadlineMissedStatus& /*status*/)
{
}

void
ReqDataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr /*reader*/,
  const DDS::RequestedIncompatibleQosStatus& /*status*/)
{
}

void
ReqDataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleRejectedStatus& /*status*/)
{
}

void
ReqDataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr /*reader*/,
  const DDS::LivelinessChangedStatus& /*status*/)
{
}

void
ReqDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  robot::RobotControl_RequestDataReader_var reader_i =
    robot::RobotControl_RequestDataReader::_narrow(reader);

  if (!reader_i) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(1);
  }

  robot::RobotControl_Request request;
  DDS::SampleInfo info;

  DDS::ReturnCode_t error = reader_i->take_next_sample(request, info);

  if (error == DDS::RETCODE_OK) {
    // std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
    // std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

    if (info.valid_data) {
        ::CORBA::Long funID = request.data._d();
        switch(funID) {
            case robot::RobotControl_setSpeed_Hash:
                std::cout << "call setSpeed" << std::endl;
                My_Local_Robot::setSpeed(request.data.setSpeed().speed);
                break;
            case robot::RobotControl_getSpeed_Hash:
                {
                  std::cout << "call getSpeed" << std::endl;
                  float speed =  My_Local_Robot::getSpeed();

                  robot::RobotControl_Reply reply;
                  robot::RobotControl_getSpeed_Result result;
                  robot::RobotControl_getSpeed_Out out;
                  out.return_ = speed;
                  result.result(out);
                  reply.data.getSpeed(result);
                  std::cout << "sendMessage" << std::endl;
                  DDS::ReturnCode_t error = m_messgeWriter->write(reply, DDS::HANDLE_NIL);
                  if (error != DDS::RETCODE_OK) {
                      std::cerr << "Write failed" << std::endl;
                  }

                  std::cout << "sendMessage End" << std::endl;
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
ReqDataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SubscriptionMatchedStatus& /*status*/)
{
}

void
ReqDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr /*reader*/,
  const DDS::SampleLostStatus& /*status*/)
{
}

void ReqDataReaderListenerImpl::setMessageWriter(robot::RobotControl_ReplyDataWriter_var writer)
{
  m_messgeWriter = writer;
}