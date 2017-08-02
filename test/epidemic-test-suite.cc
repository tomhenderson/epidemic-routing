/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Mohammed J.F. Alenazi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mohammed J.F. Alenazi  <malenazi@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported by King Saud University and
 * the ITTC at The University of Kansas.
 */

#include "ns3/epidemic-routing-protocol.h"
#include <vector>
#include "ns3/ptr.h"
#include "ns3/boolean.h"
#include "ns3/test.h"
#include "ns3/ipv4-route.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/epidemic-helper.h"
#include "ns3/socket.h"
#include "ns3/log.h"


using namespace ns3;
using namespace Epidemic;


class EpidemicHeaderTestCase : public TestCase
{
public:
  EpidemicHeaderTestCase ();
  virtual ~EpidemicHeaderTestCase ();

private:
  virtual void DoRun (void);
};


EpidemicHeaderTestCase::EpidemicHeaderTestCase ()
  : TestCase ("Verifying the Epidemic header")
{
}


EpidemicHeaderTestCase::~EpidemicHeaderTestCase ()
{
}

void
EpidemicHeaderTestCase::DoRun (void)
{
  Ptr<Packet> packet = Create<Packet> ();
  Time ts1 = Simulator::Now ();

  EpidemicHeader header1;
  header1.SetPacketID (12345);
  header1.SetTimeStamp (ts1);
  header1.SetHopCount (10);
  packet->AddHeader (header1);

  EpidemicHeader header2;
  packet->RemoveHeader (header2);

  NS_TEST_ASSERT_MSG_EQ (header2.GetSerializedSize (),16,
                         "Checking Header Size");
  NS_TEST_ASSERT_MSG_EQ (header2.GetPacketID (),12345,
                         "Checking Packet ID");
  NS_TEST_ASSERT_MSG_EQ (header2.GetTimeStamp (),ts1,
                         "Checking TimeStamp");
  NS_TEST_ASSERT_MSG_EQ (header2.GetHopCount (),10,
                         "Checking hop count");

}






struct EpidemicRqueueTest : public TestCase
{
  EpidemicRqueueTest () : TestCase ("EpidemicQueue"),
                          q (64)
  {
  }
  virtual void DoRun ();
  void Unicast (Ptr<Ipv4Route> route,
                Ptr<const Packet> packet, const Ipv4Header & header)
  {
  }
  void Error (Ptr<const Packet>, const Ipv4Header &, Socket::SocketErrno)
  {
  }
  void CheckSizeLimit ();
  void CheckTimeout ();
  PacketQueue q;
};

void
EpidemicRqueueTest::DoRun ()
{
  NS_TEST_EXPECT_MSG_EQ (q.GetMaxQueueLen (), 64,
                         "Checking the default size for queue length");
  q.SetMaxQueueLen (32);
  NS_TEST_EXPECT_MSG_EQ (q.GetMaxQueueLen (), 32,
                         "Checking the set function for queue length");


  Ptr<const Packet> packet = Create<Packet> ();
  Ipv4Header h;
  h.SetDestination (Ipv4Address ("1.2.3.4"));
  h.SetSource (Ipv4Address ("4.3.2.1"));
  Ipv4RoutingProtocol::UnicastForwardCallback ucb =
    MakeCallback (&EpidemicRqueueTest::Unicast, this);
  Ipv4RoutingProtocol::ErrorCallback ecb =
    MakeCallback (&EpidemicRqueueTest::Error, this);

  QueueEntry e1 (packet, h, ucb, ecb, Seconds (1),1111);
  q.Enqueue (e1);
  q.Enqueue (e1);
  q.Enqueue (e1);
  NS_TEST_EXPECT_MSG_EQ (q.GetSize (), 1,
                         "Checking if enqueue  removes duplicates");

  h.SetDestination (Ipv4Address ("2.2.2.2"));
  QueueEntry e2 (packet, h, ucb, ecb, Seconds (1),2222);
  q.Enqueue (e1);
  q.Enqueue (e2);
  Ptr<Packet> packet2 = Create<Packet> ();
  QueueEntry e3 (packet2, h, ucb, ecb, Seconds (1),3333);
  q.Enqueue (e2);
  q.Enqueue (e3);
  NS_TEST_EXPECT_MSG_EQ (q.GetSize (), 3,
                         "Checking the queue size function after multiple enqueue");
  QueueEntry e4;
  q.Dequeue (e4);
  NS_TEST_EXPECT_MSG_EQ (q.GetSize (), 2,
                         "Checking the queue size function after dequeue function is called");
  NS_TEST_EXPECT_MSG_EQ (e4.GetPacketID (), e1.GetPacketID (), "trivial");
  q.Dequeue (e4);
  NS_TEST_EXPECT_MSG_EQ (q.GetSize (), 1,
                         "Checking the queue size function after dequeue function is called");
  NS_TEST_EXPECT_MSG_EQ (e4.GetPacketID (), e2.GetPacketID (),
                         "Checking the packetID for two entries");
  q.Dequeue (e4);
  NS_TEST_EXPECT_MSG_EQ (q.GetSize (), 0,
                         "Checking if the queue is empty");
  NS_TEST_EXPECT_MSG_EQ (e4.GetPacketID (), e3.GetPacketID (),
                         "Checking the packetID for two entries");
  NS_TEST_EXPECT_MSG_EQ (q.Dequeue (e4), false,
                         "Checking the dequeue function with empty queue");
}



class EpidemicTestSuite : public TestSuite
{
public:
  EpidemicTestSuite ();
};

EpidemicTestSuite::EpidemicTestSuite ()
  : TestSuite ("epidemic", UNIT)
{
  AddTestCase (new EpidemicHeaderTestCase, TestCase::QUICK);
  AddTestCase (new EpidemicRqueueTest, TestCase::QUICK);
}


static EpidemicTestSuite epidemicTestSuite;

