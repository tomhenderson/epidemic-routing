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

#ifndef EPIDEMIC_TAG_H
#define EPIDEMIC_TAG_H


#include <iostream>
#include "ns3/log.h"
#include "ns3/tag.h"


namespace ns3 {
namespace Epidemic {
/**
 * Tag used by epidemic implementation
 *  \ingroup epidemic
 *  Tag used to differentiate between locally generated packets
 *  and incoming packets in RoutingProtocol::RouteInput() function.
 */



class ControlTag : public Tag
{

public:
  /// TagType enum, two types for epidemic tags
  enum TagType
  {
    CONTROL,           //!< Tagged as control message
    NOT_SET,           //!< Tag is not set
  };

  /// Constructor
  ControlTag (TagType o = NOT_SET) : Tag (),
                                     m_tag (o)
  {
  }

  /**
   *  Get the registered TypeId for this class.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId ();
  // Inherited
  TypeId  GetInstanceTypeId () const;

  /// \returns control tag type
  TagType GetTagType () const;

  /**
   * Check if this ControlTag is of the indicated TagType.
   *
   * \param type The expected TagType
   * \returns true if \p type matches the TagType of this ControlTag.
   */
  bool IsTagType (const TagType type) const;



  /**
   * \brief Seting epidemic tag.
   * \param tag TagType to be set.
   */
  void SetTagType (const TagType tag);
  // Inherited
  uint32_t GetSerializedSize () const;
  void  Serialize (TagBuffer i) const;
  void  Deserialize (TagBuffer i);
  void  Print (std::ostream &os) const;

private:
  /// epidemic tag
  TagType m_tag;
};





} //end namespace epidemic
} //end namespace ns3
#endif
