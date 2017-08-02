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

#ifndef EPIDEMIC_HELPER_H
#define EPIDEMIC_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"

/**
 * \file
 * \ingroup epidemic
 * ns3::EpidemicHelper declaration.
 */


namespace ns3 {

/**
 * \ingroup epidemic
 * \brief Helper class that adds Epidemic routing to nodes.
 */
class EpidemicHelper : public Ipv4RoutingHelper
{
public:
  /** Constructor */
  EpidemicHelper ();
  /** Destructor */
  ~EpidemicHelper ();
  // Inherited
  EpidemicHelper* Copy (void) const;
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
  /**
   * Set attributes by name.
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set.
   *
   * This method controls the attributes of "ns3::Epidemic::RoutingProtocol"
   */
  void Set (std::string name, const AttributeValue &value);

private:
  /** The factory to create Epidemic routing object */
  ObjectFactory m_agentFactory;
};

} //end namespace ns3

#endif /* EPIDEMIC_HELPER_H */
