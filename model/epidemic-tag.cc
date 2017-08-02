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
#include "epidemic-tag.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/packet.h"

namespace ns3 {
namespace Epidemic {

NS_OBJECT_ENSURE_REGISTERED (ControlTag);

/// Get TypeId
TypeId ControlTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::Epidemic::ControlTag")
    .SetParent<Tag> ()
    .AddConstructor<ControlTag> ()
  ;
  return tid;
}


/// Get instanceTypeId
TypeId  ControlTag::GetInstanceTypeId () const
{
  return GetTypeId ();
}

/// Get epidemic tag
ControlTag::TagType ControlTag::GetTagType () const
{
  return m_tag;
}

bool
ControlTag::IsTagType (const TagType type) const
{
  return m_tag == type;
}

/// Set epidemic tag
void
ControlTag::SetTagType (const TagType tag)
{
  m_tag = tag;
}

/// Get size
uint32_t ControlTag::GetSerializedSize () const
{
  return sizeof(uint8_t);
}


/// Serialize
void  ControlTag::Serialize (TagBuffer i) const
{
  i.WriteU8 ((uint8_t) m_tag);
}



///Deserialize
void  ControlTag::Deserialize (TagBuffer i)
{
  uint8_t type = i.ReadU8 ();
  switch (type)
    {
    case CONTROL:
    case NOT_SET:
      {
        m_tag = (TagType) type;
        break;
      }
    default:
      break;
    }

}
/// Print
void  ControlTag::Print (std::ostream &os) const
{
  os << "ControlTag:" << m_tag;
}

} //end namespace epidemic
} //end namespace ns3
