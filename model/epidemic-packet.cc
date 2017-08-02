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

#include "epidemic-packet.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/packet.h"

/**
 * \file
 * \ingroup epidemic
 * ns3::Epidemic::TypeHeader, ns3::Epidemic::EpidemicSummaryVectorHeader
 * and ns3::Epidemic::EpidemicHeader implementations.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpidemicPacket");

namespace Epidemic {




NS_OBJECT_ENSURE_REGISTERED (TypeHeader);


TypeHeader::TypeHeader (MessageType t)
  : m_type (t),
    m_valid (true)
{
}

TypeHeader::~TypeHeader ()
{
}
TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid =
    TypeId ("ns3::Epidemic::TypeHeader")
    .SetParent<Header> ()
    .AddConstructor<TypeHeader> ();
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

void TypeHeader::SetMessageType (MessageType type)
{
  NS_LOG_FUNCTION (this << type);
  m_type = type;
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return sizeof(uint8_t);
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case BEACON:
    case REPLY:
    case REPLY_BACK:
      {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
      break;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case BEACON:
      {
        os << "BEACON";
        break;
      }
    case REPLY:
      {
        os << "REPLY";
        break;
      }
    case REPLY_BACK:
      {
        os << "REPLY_BACK";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
      break;
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}


TypeHeader::MessageType
TypeHeader::GetMessageType () const
{
  return m_type;
}

bool
TypeHeader::IsMessageType (const MessageType type) const
{
  return m_type == type;
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}

NS_OBJECT_ENSURE_REGISTERED (SummaryVectorHeader);

SummaryVectorHeader::SummaryVectorHeader (size_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_packets.reserve (size);
}

SummaryVectorHeader::~SummaryVectorHeader ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
SummaryVectorHeader::GetTypeId (void)
{
  static TypeId tid =
    TypeId ("ns3::Epidemic::SummaryVectorHeader")
    .SetParent<Header> ()
    .AddConstructor<SummaryVectorHeader> ();
  return tid;
}

TypeId
SummaryVectorHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
SummaryVectorHeader::GetSerializedSize () const
{
  return sizeof(uint32_t) + (uint32_t) m_packets.size () * sizeof (uint32_t);
}

void
SummaryVectorHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtonU32 (m_packets.size ());

  for (std::vector<uint32_t>::const_iterator j = m_packets.begin ();
       j != m_packets.end (); ++j)
    {
      i.WriteHtonU32 (*j);
    }

}

uint32_t
SummaryVectorHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint32_t sm_length = i.ReadNtohU32 ();
  m_packets.reserve (sm_length);
  for (uint32_t j = 0; j < sm_length; ++j)
    {
      uint32_t tmp = i.ReadNtohU32 ();
      m_packets.push_back (tmp);
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());

  return dist;
}


std::ostream &
operator<< (std::ostream & os, SummaryVectorHeader const & packet)
{
  packet.Print (os);
  return os;
}


void
SummaryVectorHeader::Print (std::ostream &os) const
{
  os << " Summary_vector header with size: " << m_packets.size ()
  << "\nGlobal IDs:\n" << "NodeID:PacketID\n";
  for (std::vector<uint32_t>::const_iterator j = m_packets.begin ();
       j != m_packets.end (); ++j)
    {
      uint32_t a;
      uint32_t b;
      a = (uint32_t)((*j & 0xFFFF0000) >> 16);
      b = (uint32_t)(*j & 0xFFFF);
      Ipv4Address new_addr = Ipv4Address (a);
      os <<  new_addr <<  ":" << b << std::endl;
    }
}

void
SummaryVectorHeader::Add (const uint32_t pkt_ID)
{
  NS_LOG_FUNCTION (this << pkt_ID);
  m_packets.push_back (pkt_ID);
}

size_t
SummaryVectorHeader::Size (void) const
{
  return m_packets.size ();
}


bool
SummaryVectorHeader::Contains (const uint32_t pkt_ID) const
{
  bool contained = ( std::find (m_packets.begin (), m_packets.end (), pkt_ID)
                     != m_packets.end () );
  return contained;
}


NS_OBJECT_ENSURE_REGISTERED (EpidemicHeader);

EpidemicHeader::~EpidemicHeader ()
{
}


void
EpidemicHeader::SetPacketID (uint32_t pktID)
{
  NS_LOG_FUNCTION (this << pktID);
  m_packetID = pktID;
}

uint32_t
EpidemicHeader::GetPacketID  () const
{
  return m_packetID;
}


void
EpidemicHeader::SetHopCount (uint32_t floodCount)
{
  NS_LOG_FUNCTION (this << floodCount);
  m_hopCount = floodCount;
}

uint32_t
EpidemicHeader::GetHopCount () const
{

  return m_hopCount;
}


void
EpidemicHeader::SetTimeStamp (Time timeStamp)
{
  NS_LOG_FUNCTION (this << timeStamp);
  m_timeStamp = timeStamp;
}


Time EpidemicHeader::GetTimeStamp () const
{
  return m_timeStamp;
}


TypeId
EpidemicHeader::GetTypeId (void)
{
  static TypeId tid =
    TypeId ("ns3::Epidemic::EpidemicHeader")
    .SetParent<Header> ()
    .AddConstructor<EpidemicHeader> ();
  return tid;
}

TypeId
EpidemicHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
EpidemicHeader::GetSerializedSize () const
{

  return sizeof(m_packetID) + sizeof(m_hopCount) + sizeof(m_timeStamp);

}

void
EpidemicHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtonU32 (m_packetID);
  i.WriteHtonU32 (m_hopCount);
  i.WriteHtonU64 (m_timeStamp.GetNanoSeconds ());

}

uint32_t
EpidemicHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_packetID = i.ReadNtohU32 ();
  m_hopCount = i.ReadNtohU32 ();
  m_timeStamp = Time (i.ReadNtohU64 ());
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

std::ostream &
operator<< (std::ostream& os, const EpidemicHeader & header)
{
  header.Print (os);
  return os;
}

void
EpidemicHeader::Print (std::ostream &os) const
{
  os << " Packet ID: " << m_packetID << " Hop count: " << m_hopCount
  << " TimeStamp: " << m_timeStamp;

}
} //end namespace epidemic
} //end namespace ns3
