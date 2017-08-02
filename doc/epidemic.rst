.. include:: replace.txt
.. highlight:: cpp

Epidemic Routing
----------------

Epidemic Routing protocol is a controlled flooding routing protocol
designed specifically for use in multi-hop wireless ad hoc networks
of mobile nodes. This routing protocol is designed for intermittent 
or sparse networks. Packets are held in the source nodes' buffer until
the source node comes into communication range with another node.  
The packets are transmitted to the new node, which becomes a transit 
node for those packets. Transit nodes hold all packets received until 
they come into range with other nodes, at which point they each send 
the other any packets the other doesn't already have.  The name "epidemic" 
is drawn from the way packets spread virally by communication
contact between nodes.  At the source and all transit nodes, packets are
eventually dropped when the expire or the epidemic routing buffer 
becomes full, in which case the oldest packets are dropped. It is 
not useful for dense networks since the number of duplicate packets 
will be very large.

This model was developed by 
the `ResiliNets research group <http://www.ittc.ku.edu/resilinets>`_
at the University of Kansas.  


Epidemic Routing Routing Overview
*********************************

This implementation is based on the paper titled 'Epidemic Routing
for Partially-Connected Ad Hoc Networks.'[#Vahdat]_

In the original paper, the implementation was on top of top of
the Internet MANET Encapsulation Protocol (IMEP) layer,
which is  responsible for notifying the epidemic agent
when a new node comes into radio range. However, since IMEP
is not implemented in |ns3|, a beacon mechanism is added to the implementation.


Useful parameters
=================

Epidemic routing supports these options:

  +-----------------------+-----------------------------------+---------------+
  | Parameter             | Description                       | Default       |
  +=======================+===================================+===============+
  | HopCount              | Maximum number of hops a packet   | 64            |
  |                       | can be forwarded through.         |               | 
  |                       | HopCount serves a similar         |               |
  |                       | function to TTL, but the 8-bit    |               |
  |                       | range of TTL is too small, so we  |               |
  |                       | use a 32-bit field as in          |               |
  |                       | the paper.                        |               |
  +-----------------------+-----------------------------------+---------------+
  | QueueLength           | Maximum number of packets that    | 64            |
  |                       | can be stored in Epidemic buffer  |               |
  +-----------------------+-----------------------------------+---------------+
  | QueueEntryExpireTime  | Maximum time a packet can live    |               |
  |                       | since generated at the source.    | Seconds(100)  |
  |                       | Network-wide synchronization      |               |
  |                       | is assumed.                       |               |
  +-----------------------+-----------------------------------+---------------+
  | HostRecentPeriod      | Time in seconds for host recent   |               |
  |                       | period, in which hosts can not    | Seconds(10)   |
  |                       | re-exchange summary vectors.      |               |
  +-----------------------+-----------------------------------+---------------+
  | BeaconInterval        | Mean time interval between sending| Seconds(1)    |
  |                       | beacon packets.                   |               |
  +-----------------------+-----------------------------------+---------------+
  | BeaconRandomness      | Random number of milliseconds     | 100           |
  |                       | added at the beginning            |               |
  |                       | of the BeaconInterval to avoid    |               |
  |                       | collisions.                       |               |
  +-----------------------+-----------------------------------+---------------+


Dropping Packets
================
Packets, stored in buffers, are dropped if they exceed HopCount, they are
older than QueueEntryExpireTime, or the holding buffer exceed QueueLength.  


Helper
******

To have a node run Epidemic Routing Protocol, the easiest way would be to use 
the EpidemicHelper
in your simulation script. For instance (assuming ``mainNodes`` 
is a ``NodeContainer``)::

  EpidemicHelper epidemic;
  mainNodes.Install (epidemic, adhocNodes);

This will run the epidemic routing using the default values. To use 
different parameter values::

  EpidemicHelper epidemic;
  epidemic.Set ("HopCount", UintegerValue (20));
  epidemic.Set ("QueueLength", UintegerValue (100));
  epidemic.Set ("QueueEntryExpireTime", TimeValue (Seconds (60)));
  epidemic.Set ("BeaconInterval", UintegerValue (5));
  mainNodes.Install (epidemic, adhocNodes);


Examples
********
First example creates an N-node wireless network, which is set by 
default to 10 nodes. The mobility model can be either static Grid 
or Randomwaypoint, which by default is selected to be Grid.
The data traffic is generated using OnOff application and received 
by PacketSink. There is one source and one sink in this configuration.
One example can be found in ``src/epidemic/examples/epidemic-example.cc``  

This example creates the scenarios presented in the paper. 
[#Vahdat]_ We have 50 nodes in an area of 1500 m x 300 m. 
45 nodes are selected to send a packet of size 1KB to the other 44 nodes. 
The total packets are 45 * 44 = 1980 packets. The buffer size is 2000 
which can hold all packets in each simulation. The ranges for the transmission 
are from 10 m to 250 m while the default is set to 50 m. 
The example can be found in ``src/epidemic/examples/epidemic-benchmark.cc``


Validation
**********

This model has been tested as follows:

* Unit tests have been written to verify the internals of Epidemic. This can 
  be found in ``src/epidemic/test/epidemic-test-suite.cc``. These tests verify 
  whether the methods inside Epidemic module which deal with packet buffer, 
  headers work correctly.

LIMITATIONS 
***********
Epidemic does not work with more than one address per each interface.  Additional 
addresses are ignored. 

Epidemic routing protocol assume many beacons are exchanged in  the time nodes 
are in range. If not, packets will not be delivered.


References
**********

.. rubric:: Footnotes

.. [#Vahdat] Amin Vahdat and David Becker, "Epidemic Routing for
   Partially-Connected Ad Hoc Networks," Duke University, Technical
   Report CS-200006, http://issg.cs.duke.edu/epidemic/epidemic.pdf
