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


#include "epidemic-routing-protocol.h"
#include <vector>
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/random-variable-stream.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/udp-header.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include "ns3/ipv4-route.h"
#include "ns3/socket.h"
#include "ns3/log.h"


/**
 * \file
 * \ingroup epidemic
 * ns3::Epidemic::RoutingProtocol implementation.
 */


using namespace std;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpidemicRoutingProtocol");

namespace Epidemic {
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);



TypeId RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Epidemic::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HopCount","Maximum number of times "
                   "a packet will be flooded.",
                   UintegerValue (64),
                   MakeUintegerAccessor (&RoutingProtocol::m_hopCount),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueLength","Maximum number of "
                   "packets that a queue can hold.",
                   UintegerValue (64),
                   MakeUintegerAccessor (&RoutingProtocol::m_maxQueueLen),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("QueueEntryExpireTime","Maximum time a packet can live in "
                   "the epidemic queues since it's generated at the source.",
                   TimeValue (Seconds (100)),
                   MakeTimeAccessor (&RoutingProtocol::m_queueEntryExpireTime),
                   MakeTimeChecker ())
    .AddAttribute ("HostRecentPeriod","Time in seconds for host recent period"
                   ", in which hosts can not re-exchange summary vectors.",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&RoutingProtocol::m_hostRecentPeriod),
                   MakeTimeChecker ())
    .AddAttribute ("BeaconInterval","Time in seconds after which a "
                   "beacon packet is broadcast.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&RoutingProtocol::m_beaconInterval),
                   MakeTimeChecker ())
    .AddAttribute ("BeaconRandomness","Upper bound of the uniform distribution"
                   " random time added to avoid collisions. Measured in milliseconds",
                   UintegerValue (100),
                   MakeUintegerAccessor (&RoutingProtocol::m_beaconMaxJitterMs),
                   MakeUintegerChecker<uint32_t> ());

  return tid;
}




RoutingProtocol::RoutingProtocol ()
  : m_hopCount (0),
    m_maxQueueLen (0),
    m_queueEntryExpireTime (Seconds (0)),
    m_beaconInterval (Seconds (0)),
    m_hostRecentPeriod (Seconds (0)),
    m_beaconMaxJitterMs (0),
    m_dataPacketCounter (0),
    m_queue (m_maxQueueLen)
{
  NS_LOG_FUNCTION (this);
}

RoutingProtocol::~RoutingProtocol ()
{
  NS_LOG_FUNCTION (this);
}

void
RoutingProtocol::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_ipv4 = 0;
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator
       iter = m_socketAddresses.begin (); iter
       != m_socketAddresses.end (); iter++)
    {
      iter->first->Close ();
    }
  m_socketAddresses.clear ();
  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
  /*
   *  There is no routing table
   */
  *stream->GetStream () << "No Routing table ";
}




void
RoutingProtocol::Start ()
{
  NS_LOG_FUNCTION (this );
  m_queue.SetMaxQueueLen (m_maxQueueLen);
  m_beaconTimer.SetFunction (&RoutingProtocol::SendBeacons,this);
  m_beaconJitter = CreateObject<UniformRandomVariable> ();
  m_beaconJitter->SetAttribute ("Max", DoubleValue (m_beaconMaxJitterMs));
  m_beaconTimer.Schedule (m_beaconInterval + MilliSeconds
                            (m_beaconJitter->GetValue ()));
}

bool
RoutingProtocol::IsHostContactedRecently (Ipv4Address hostID)
{
  NS_LOG_FUNCTION (this << hostID);
  // If host is not in has table, record current time and return false
  HostContactMap::iterator hostID_pair =  m_hostContactTime.find (hostID);
  if (hostID_pair == m_hostContactTime.end ())
    {
      m_hostContactTime.insert (std::make_pair (hostID,Now ()));
      return false;
    }

  //if host is in hash table check time is less than the recent_period:
  if (Now () < (hostID_pair->second + m_hostRecentPeriod ))
    {
      // it means the host is recently contacted
      return true;
    }
  else
    {
      // update the recent contact value
      hostID_pair->second = Now ();
      // return false since it has exceeded the recent contact period
      return false;
    }

}

void
RoutingProtocol::SendPacket (Ptr<Packet> p,InetSocketAddress addr)
{
  NS_LOG_FUNCTION (this << p << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator
       j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (iface.GetLocal () == m_mainAddress)
        {
          Ptr<Socket> socket = j->first;
          NS_LOG_LOGIC ("Packet " << p << " is sent to" << addr );
          socket->SendTo (p,0, addr);
        }
    }
  NS_LOG_FUNCTION (this << *p);
}



void
RoutingProtocol::BroadcastPacket (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator
       j = m_socketAddresses.begin ();
       j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
      Ipv4Address destination;
      if (iface.GetMask () == Ipv4Mask::GetOnes ())
        {
          destination = Ipv4Address ("255.255.255.255");
        }
      else
        {
          destination = iface.GetBroadcast ();
        }
      NS_LOG_LOGIC ("Packet " << p << " is sent to" << destination );
      socket->SendTo (p, 0, InetSocketAddress (destination, EPIDEMIC_PORT));
    }
}



void
RoutingProtocol::SendPacketFromQueue (Ipv4Address dst,QueueEntry queueEntry)
{
  NS_LOG_FUNCTION (this << dst << queueEntry.GetPacketID ());
  Ptr<Packet> p = ConstCast<Packet> (queueEntry.GetPacket ());
  UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback ();
  Ipv4Header header = queueEntry.GetIpv4Header ();
  /*
   *  Since Epidemic routing has a control mechanism to drop packets based
   *  on hop count, IP TTL dropping mechanism is avoided by incrementing TTL.
   */
  header.SetTtl (header.GetTtl () + 1);
  header.SetPayloadSize (p->GetSize ());
  Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
  rt->SetSource (header.GetSource ());
  rt->SetDestination (header.GetDestination ());
  rt->SetGateway (dst);
  if (m_ipv4->GetInterfaceForAddress (m_mainAddress) != -1)
    {
      Ptr<Node> node = m_ipv4->GetObject<Node> ();
      rt->SetOutputDevice (m_ipv4->GetNetDevice
                             (m_ipv4->GetInterfaceForAddress (m_mainAddress)));

    }

  Ptr<Packet> copy = p->Copy ();
  /*
   *  The packet will not be sent if:
   *  The forward address is the source address of the packet.
   *  The forward address is the destination address of the packet.
   */
  if (dst != header.GetSource () && !IsMyOwnAddress (header.GetDestination ()))
    {
      ucb (rt, copy, header);
    }
}

void
RoutingProtocol::SendBeacons ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();
  EpidemicHeader header;
  // This number does not have any effect but it has to be more than
  // 1 to avoid dropping at the receiver
  header.SetHopCount (m_hopCount);
  packet->AddHeader (header);
  TypeHeader tHeader (TypeHeader::BEACON);
  packet->AddHeader (tHeader);
  ControlTag tempTag (ControlTag::CONTROL);
  // Packet tag is added and will be removed before local delivery in
  // RouteInput function
  packet->AddPacketTag (tempTag);


  BroadcastPacket (packet);
  m_beaconTimer.Schedule (m_beaconInterval + MilliSeconds
                            (m_beaconJitter->GetValue ()));
}


uint32_t
RoutingProtocol::FindOutputDeviceForAddress (Ipv4Address  dst)
{
  NS_LOG_FUNCTION (this << dst);
  Ptr<Node> mynode = m_ipv4->GetObject<Node> ();
  for (uint32_t i = 0; i < mynode->GetNDevices (); i++)
    {
      Ipv4InterfaceAddress iface = m_ipv4->GetAddress (
          m_ipv4->GetInterfaceForDevice (mynode->GetDevice (i)),0);
      if (dst.CombineMask (iface.GetMask ()) ==
          iface.GetLocal ().CombineMask (iface.GetMask ()))
        {
          return i;
        }
    }
  return -1;
}


uint32_t
RoutingProtocol::FindLoopbackDevice ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Node> mynode = m_ipv4->GetObject<Node> ();
  for (uint32_t i = 0; i < mynode->GetNDevices (); i++)
    {
      Ipv4InterfaceAddress iface = m_ipv4->GetAddress (
          m_ipv4->GetInterfaceForDevice (mynode->GetDevice (i)),0);
      if (iface.GetLocal () == Ipv4Address ("127.0.0.1") )
        {
          return i;
        }
    }
  return -1;
}


bool
RoutingProtocol::IsMyOwnAddress (Ipv4Address src)
{
  NS_LOG_FUNCTION (this << src);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      if (src == iface.GetLocal ())
        {
          return true;
        }
    }
  return false;
}


Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p,
                              const Ipv4Header &header,
                              Ptr<NetDevice> oif,
                              Socket::SocketErrno &sockerr)
{

  NS_LOG_FUNCTION (this << p << header << oif << sockerr);
  NS_LOG_LOGIC (this << "Packet Size" << p->GetSize ()
                     << " Packet " << p->GetUid ()
                     << " reached node " << m_mainAddress << " source  " << header.GetSource ()
                     << " going to " << header.GetDestination ());


  if (IsMyOwnAddress (header.GetDestination ()))
    {
      NS_LOG_LOGIC ("Local delivery a packet" << p->GetUid ()
                                              << " has arrived destination "
                                              << " At node " << m_mainAddress << "  " << header);
      Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
      rt->SetSource (m_mainAddress);
      rt->SetDestination (header.GetDestination ());
      return rt;
    }
  else
    {
      Ptr<Ipv4Route> rt = Create<Ipv4Route> ();
      rt->SetSource (m_mainAddress);
      rt->SetDestination (header.GetDestination ());
      rt->SetGateway (header.GetDestination ());

      if (m_ipv4->GetInterfaceForAddress (m_mainAddress) != -1)
        {
          /*
           *  Control packets generated at this node, are
           *  tagged with ControlTag.
           *  They are removed before local delivery in RouteInput function.
           */
          ControlTag tag;
          p->PeekPacketTag (tag);
          if (  tag.GetTagType () == ControlTag::CONTROL)
            {
              /*
               * if the packet is not control, it means a data packet
               * Thus, data packet is supposed to be looped back to
               * store it in the epidemic queue.
               */
              rt->SetOutputDevice (m_ipv4->GetNetDevice (FindLoopbackDevice ()));
            }
          else
            {
              /*
               * if the packet is control packet
               * Thus, find the corresponding output device
               */
              NS_LOG_DEBUG ( "Epidemic triggered packets :" <<
                             header.GetDestination ()  << "  found " <<
                             FindOutputDeviceForAddress (header.GetDestination ()));
              rt->SetOutputDevice (m_ipv4->GetNetDevice (
                                     FindOutputDeviceForAddress (header.GetDestination ())));
            }
        }
      return rt;
    }

}


bool
RoutingProtocol::RouteInput (Ptr<const Packet> p,
                             const Ipv4Header &header,
                             Ptr<const NetDevice> idev,
                             UnicastForwardCallback ucb,
                             MulticastForwardCallback mcb,
                             LocalDeliverCallback lcb,
                             ErrorCallback ecb)
{
  NS_LOG_FUNCTION (this << header << *p );
  NS_ASSERT (m_ipv4 != 0);
  NS_ASSERT (p != 0);
  // Check if input device supports IP
  NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
  /*
  If there are no interfaces, ignore the packet and return false
  */
  if (m_socketAddresses.empty ())
    {
      NS_LOG_ERROR ("No  interfaces");
      return false;
    }





  if (header.GetTtl () < 1)
    {
      NS_LOG_DEBUG ("TTL expired, Packet is dropped " << p->GetUid ());
      return false;
    }


  if (header.GetProtocol () == 1)
    {
      NS_LOG_DEBUG ("Does not deliver ICMP packets " << p->GetUid ());
      return false;
    }

  /*
   *  Check all the interfaces local addresses for local delivery
   */

  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ipv4InterfaceAddress iface = j->second;
      int32_t iif = m_ipv4->GetInterfaceForDevice (idev);
      if (m_ipv4->GetInterfaceForAddress (iface.GetLocal ()) == iif)
        {
          if (header.GetDestination () == iface.GetBroadcast ()
              || header.GetDestination () == m_mainAddress )
            {
              ControlTag tag;
              p->PeekPacketTag (tag);
              Ptr<Packet> local_copy = p->Copy ();
              bool duplicatePacket = false;
              /*
               * If this is a data packet, add it to the epidemic
               * queue in order to avoid
               * receiving duplicates of the same packet.
               */
              if (tag.GetTagType () == ControlTag::NOT_SET)
                {
                  Ptr<Packet> copy = p->Copy ();
                  QueueEntry newEntry (copy, header, ucb, ecb);
                  EpidemicHeader current_Header;
                  copy->PeekHeader (current_Header);
                  newEntry.SetExpireTime (m_queueEntryExpireTime +
                                          current_Header.GetTimeStamp ());
                  newEntry.SetPacketID (current_Header.GetPacketID ());
                  EpidemicHeader header;
                  local_copy->RemoveHeader (header);
                  // Try to see the packet has been
                  // delivered i.e. in the epidemic buffer
                  if (m_queue.Find (
                        current_Header.GetPacketID ()).GetPacketID ()
                      == 0)
                    {
                      m_queue.Enqueue (newEntry);
                    }
                  else
                    {
                      duplicatePacket = true;
                    }
                }
              /*
              Deliver the packet locally
              */
              if (!duplicatePacket)
                {
                  local_copy->RemovePacketTag (tag);
                  lcb (local_copy, header, iif);
                }
              return true;

            }
        }
    }

  /*
  If the packet does not have an epidemic header,
  create one and attach it to the packet.
  This condition occurs when the packet is originated locally and does not have
  an Epidemic header.
  */

  Ptr<Packet> copy = p->Copy ();
  NS_LOG_LOGIC ("Creating Epidemic packet " << p->GetUid () <<  " Src " <<
                header.GetSource () <<  " Dest " <<
                header.GetDestination () << " Size before" << copy->GetSize ());
  /*
   * The global packet id format: 16bit(Source IP):16bit(source packet counter)
   */
  uint16_t hostID = header.GetSource ().Get () & 0xFFFF;
  m_dataPacketCounter++;
  uint32_t global_packet_ID = hostID;
  global_packet_ID = global_packet_ID << 16 | m_dataPacketCounter;




  // Adding the data packet to the queue
  QueueEntry newEntry (copy, header, ucb, ecb);
  newEntry.SetPacketID (global_packet_ID);

  if (IsMyOwnAddress (header.GetSource ()))
    {
      NS_LOG_DEBUG ("Adding Epidemic packet header " << p->GetUid () );
      //ADD EPIDEMIC HEADER
      EpidemicHeader new_Header;
      new_Header.SetPacketID (global_packet_ID);
      new_Header.SetTimeStamp (Simulator::Now ());
      new_Header.SetHopCount (m_hopCount);
      // If the packet is generated in this node, add the epidemic header
      copy->AddHeader (new_Header);
      // If the packet is generated in this node,
      // make the Expire time start from now + the user specified period
      newEntry.SetExpireTime (m_queueEntryExpireTime + Simulator::Now ());

    }
  else
    {
      // If the packet is generated in another node, read the epidemic header
      EpidemicHeader current_Header;
      copy->RemoveHeader (current_Header);
      if (current_Header.GetHopCount () <= 1
          || (current_Header.GetTimeStamp ()
              + m_queueEntryExpireTime) < Simulator::Now () )
        {
          // Exit  the function to not add the packet to the queue
          // since the flood count limit is reached
          NS_LOG_DEBUG ("Exit the function  and not add the "
                        "packet to the queue since the flood count limit is reached");
          return true;
        }
      // If the packet is generated in another node,
      // use the timestamp from the epidemic header
      newEntry.SetExpireTime (m_queueEntryExpireTime +
                              current_Header.GetTimeStamp ());
      // If the packet is generated in another node,
      // use the PacketID from the epidemic header
      newEntry.SetPacketID (current_Header.GetPacketID ());
      //Decrease the packet flood counter
      current_Header.SetHopCount (
        current_Header.GetHopCount () - 1);
      // Add the updated header
      copy->AddHeader (current_Header);
    }

  m_queue.Enqueue (newEntry);
  return true;

}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_LOG_FUNCTION (this << ipv4);
  m_ipv4 = ipv4;
  Simulator::ScheduleNow (&RoutingProtocol::Start,this);
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
  NS_LOG_FUNCTION (this << i);
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ipv4InterfaceAddress iface = l3->GetAddress (i,0);
  if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
    {
      return;
    }
  if (m_mainAddress == Ipv4Address ())
    {
      m_mainAddress = iface.GetLocal ();
    }

  /*
  Create a socket to be used for epidemic routing port
  */
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),tid);
  socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvEpidemic,this));
  socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), EPIDEMIC_PORT));
  socket->BindToNetDevice (l3->GetNetDevice (i));
  socket->SetAllowBroadcast (true);
  m_socketAddresses.insert (std::make_pair (socket,iface));
}

void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
  NS_LOG_FUNCTION (this << m_ipv4->GetAddress (i, 0).GetLocal ());
  // Disable layer 2 link state monitoring (if possible)
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  Ptr<NetDevice> dev = l3->GetNetDevice (i);
  // Close socket
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (
      m_ipv4->GetAddress (i, 0));
  NS_ASSERT (socket);
  socket->Close ();
  m_socketAddresses.erase (socket);
}

void
RoutingProtocol::NotifyAddAddress (uint32_t i,Ipv4InterfaceAddress address)
{
  NS_LOG_FUNCTION (this << i << address);
  Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
  if (!l3->IsUp (i))
    {
      return;
    }
  if (l3->GetNAddresses (i) == 1)
    {
      Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
      Ptr<Socket> socket = FindSocketWithInterfaceAddress (iface);
      if (!socket)
        {
          if (iface.GetLocal () == Ipv4Address ("127.0.0.1"))
            {
              return;
            }
          // Create a socket to listen only on this interface
          Ptr<Socket> socket =
            Socket::CreateSocket (GetObject<Node> (),
                                  UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (
            MakeCallback (&RoutingProtocol::RecvEpidemic,this));
          socket->Bind (InetSocketAddress (Ipv4Address::GetAny (),
                                           EPIDEMIC_PORT));
          socket->BindToNetDevice (l3->GetNetDevice (i));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));
        }
    }
  else
    {
      NS_LOG_LOGIC ("Epidemic does not work with more then "
                    "one address per each interface. Ignore added address");
    }



}

void
RoutingProtocol::NotifyRemoveAddress (uint32_t i,Ipv4InterfaceAddress address)
{

  NS_LOG_FUNCTION (this << i << address);
  Ptr<Socket> socket = FindSocketWithInterfaceAddress (address);
  if (socket)
    {
      m_socketAddresses.erase (socket);
      Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol> ();
      if (l3->GetNAddresses (i))
        {
          Ipv4InterfaceAddress iface = l3->GetAddress (i, 0);
          // Create a socket to listen only on this interface
          Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                     UdpSocketFactory::GetTypeId ());
          NS_ASSERT (socket != 0);
          socket->SetRecvCallback (
            MakeCallback (&RoutingProtocol::RecvEpidemic,this));
          // Bind to any IP address so that broadcasts can be received
          socket->Bind (InetSocketAddress (
                          Ipv4Address::GetAny (), EPIDEMIC_PORT));
          socket->SetAllowBroadcast (true);
          m_socketAddresses.insert (std::make_pair (socket, iface));

        }

    }
  else
    {
      NS_LOG_LOGIC ("Remove address not participating in Epidemic operation");
    }
}



void
RoutingProtocol::SendDisjointPackets (SummaryVectorHeader packet_SMV,
                                      Ipv4Address dest)
{
  NS_LOG_FUNCTION (this << dest);
  /*
  This function is used to find send the packets listed in the vector list
  */
  SummaryVectorHeader list = m_queue.FindDisjointPackets (packet_SMV);
  for (std::vector<uint32_t>::iterator
       i = list.m_packets.begin ();
       i  != list.m_packets.end ();
       ++i)
    {
      QueueEntry newEntry = m_queue.Find (*i);
      if (newEntry.GetPacket ())
        {

          Simulator::Schedule (Time (0),
                               &RoutingProtocol::SendPacketFromQueue,
                               this,dest, newEntry);
        }
    }
}



Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress
  (Ipv4InterfaceAddress addr ) const
{
  NS_LOG_FUNCTION (this << addr);
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
         m_socketAddresses.begin (); j != m_socketAddresses.end (); ++j)
    {
      Ptr<Socket> socket = j->first;
      Ipv4InterfaceAddress iface = j->second;
      if (iface == addr)
        {
          return socket;
        }
    }
  Ptr<Socket> socket;
  return socket;
}


void
RoutingProtocol::SendSummaryVector (Ipv4Address dest,bool firstNode)
{
  NS_LOG_FUNCTION (this << dest << firstNode);
  // Creating the packet
  Ptr<Packet> packet_summary = Create<Packet> ();
  SummaryVectorHeader header_summary = m_queue.GetSummaryVector ();
  packet_summary->AddHeader (header_summary);
  TypeHeader tHeader;
  if (firstNode)
    {
      tHeader.SetMessageType (TypeHeader::REPLY);
    }
  else
    {
      tHeader.SetMessageType (TypeHeader::REPLY_BACK);
    }

  packet_summary->AddHeader (tHeader);
  ControlTag tempTag (ControlTag::CONTROL);
  packet_summary->AddPacketTag (tempTag);
  // Send the summary vector
  NS_LOG_INFO ("Sending the summary vector 2 packet " << header_summary );
  InetSocketAddress addr = InetSocketAddress (dest, EPIDEMIC_PORT);
  SendPacket (packet_summary, addr);

}



void
RoutingProtocol::RecvEpidemic (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_queue.DropExpiredPackets ();
  Address address;
  Ptr<Packet> packet = socket->RecvFrom (address);
  TypeHeader tHeader (TypeHeader::BEACON);
  packet->RemoveHeader (tHeader);

  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (address);
  Ipv4Address sender = inetSourceAddr.GetIpv4 ();
  if (tHeader.GetMessageType () == TypeHeader::BEACON)
    {
      NS_LOG_LOGIC ("Got a beacon from " << sender << " " << packet->GetUid ()
                                         << " " << m_mainAddress);
      // Anti-entropy session
      // Check if you have the smaller address and the host has not been
      // contacted recently
      if (m_mainAddress.Get () < sender.Get ()
          && !IsHostContactedRecently (sender))
        {
          SendSummaryVector (sender,true);
        }
    }
  else if (tHeader.GetMessageType () == TypeHeader::REPLY)
    {
      NS_LOG_LOGIC ("Got a A reply from " << sender << " "
                                          << packet->GetUid () << " " << m_mainAddress);
      SummaryVectorHeader packet_SMV;
      packet->RemoveHeader (packet_SMV);
      SendDisjointPackets (packet_SMV, sender);
      SendSummaryVector (sender,false);
    }
  else if (tHeader.GetMessageType () == TypeHeader::REPLY_BACK)
    {
      NS_LOG_LOGIC ("Got a A reply back from " << sender
                                               << " " << packet->GetUid () << " " << m_mainAddress);
      SummaryVectorHeader packet_SMV;
      packet->RemoveHeader (packet_SMV);
      SendDisjointPackets (packet_SMV, sender);

    }
  else
    {
      NS_LOG_LOGIC ("Unknown MessageType packet ");
    }
}
} //end namespace epidemic
} //end namespace ns3
