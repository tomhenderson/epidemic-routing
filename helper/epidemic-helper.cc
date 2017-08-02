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

#include "epidemic-helper.h"
#include "ns3/epidemic-routing-protocol.h"

/**
 * \file
 * \ingroup epidemic
 * ns3::EpidemicHelper implementation.
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EpidemicHelper");

EpidemicHelper::~EpidemicHelper ()
{
  NS_LOG_FUNCTION (this);
}

EpidemicHelper::EpidemicHelper () : Ipv4RoutingHelper ()
{
  NS_LOG_FUNCTION (this);
  m_agentFactory.SetTypeId ("ns3::Epidemic::RoutingProtocol");
}

EpidemicHelper* EpidemicHelper::Copy (void) const
{
  NS_LOG_FUNCTION (this);
  return new EpidemicHelper (*this);
}

Ptr<Ipv4RoutingProtocol> EpidemicHelper::Create (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node);
  Ptr<Epidemic::RoutingProtocol>
  agent = m_agentFactory.Create<Epidemic::RoutingProtocol> ();
  node->AggregateObject (agent);
  return agent;
}

void EpidemicHelper::Set (std::string name, const AttributeValue &value)
{
  NS_LOG_FUNCTION (this << name);
  m_agentFactory.Set (name, value);
}

} //end namespace ns3
