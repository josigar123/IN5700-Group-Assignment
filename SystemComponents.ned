//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

import inet.mobility.contract.IMobility;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

// Channel which the system links will inherit from with associated class and relevant params
channel RealisticDelayChannel extends ned.DatarateChannel
{
    parameters:
        double baseLatency @unit(ms) = default(0ms);  // Base extra latency (time), fixed one-way delay
        double jitterPercentage = default(0.10); // Delay jitter% for link of baseLatency
        double propSpeed @unit(mps) = default(3e8mps); // Speed of light in air
        @class(RealisticDelayChannel);              // Link to C++ channel class
}

// This is the slow cellular link between the host and cloud, we assume public 5G which will go through the operator core
// to the cloud
channel SlowCellularLink extends RealisticDelayChannel {
    parameters:
        datarate = 50Mbps;
        baseLatency = 85ms;
        jitterPercentage = 0.20; // +-20%
        propSpeed = 3e8mps;
    
    @display("ls=red");
}

// This is the fast cellular link between a fog node (can) and host, we assume private 5G (MEC) for processing at the edge
channel FastCellularLink extends RealisticDelayChannel {
    parameters:
        datarate = 100Mbps;
        baseLatency = 10ms;
        jitterPercentage = 0.08; // +-8%
        propSpeed = 3e8mps;
        
    @display("ls=green");
}

// This is the fast Wi-Fi link between a fog node (can) and the cloud, assume an enterprise Wi-Fi 6 access point
channel FastWiFiLink extends RealisticDelayChannel{
    parameters:
        datarate = 200Mbps;
        baseLatency = 20ms;
        jitterPercentage = 0.08; // +-8%
        propSpeed = 3e8mps;
        
    @display("ls=blue");
}

// Simple module for turtle mob, extends the INETS TurtleMobility and asigngs a class with the module
simple TurtleMobility extends inet.mobility.single.TurtleMobility{
	@class(Extended::TurtleMobility);
}

// Defines the simple node module for the system
// With relevant parameters which are interesting to access in the program
simple Node
{
    parameters:
        double x;
        double y;
        double range = default(300);
        string rangeColor = default("green");
        int numGates = default(3);
        @display("p=$x,$y");
        @class(Node);

    gates:
        inout gate[numGates];
}

// Can giving its icon and assigning it a signal and associated class
simple CanNode extends Node {
    parameters:
        @class(CanNode);
        @display("i=block/bucket");
        @signal[garbageCollectedFromCan](type=bool);
}

// AnotherCan giving its icon and assigning it a signal and associated class
simple AnotherCanNode extends Node {
    parameters:
        @class(AnotherCanNode);
        @display("i=block/bucket");
        @signal[garbageCollectedFromAnotherCan](type=bool);
}

// The cloud with a server icon and class
simple CloudNode extends Node {

    parameters:
        @class(CloudNode);
        @display("i=device/server");
}

// Compound HostNode with a TurtleMobility submodule from the Extended namespace
module HostNode extends Node
{
    parameters:
        @class(HostNode);
        @display("i=block/wheelbarrow");

	// Assign the turtleScript the first leg of our xml
    submodules:
        mobility: TurtleMobility{
        	turtleScript = xmldoc("turtle.xml", "movements//movement[@id='1']");
        }
}

// Define the actual network
network GarbageCollectionSystem
{
    // Class for the system as well as num hosts
   	parameters:
   	   @class(GarbageCollectionSystem);
   	   int numHosts = 1;
   	        	
    @display("bgb=3000,1250");
	
	// Draw the inner and outer road
    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
    @figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	

	
    submodules:
        // Visualizer for drawing trail and velocity arrow on host
        visualizer: IntegratedCanvasVisualizer{
        	@display("p=1500,50");
        }
        // Init host
        host[numHosts]: HostNode {
            x = 1750;
            y = 300;
            range = 275;
        }
        // Init can
        can: CanNode  {
            x = 500;
            y = 150;
            range = 320;
            numGates = 2;
        }
        // Init anothercan
        anotherCan: AnotherCanNode {
            x = 573.885;
            y = 754.65;
        	range = 320;
        	numGates = 2;
        }
        // Init cloud
        cloud: CloudNode {
        	x = 1900;
        	y = 650;
        	range = 1600;
        }
	
	// Define the connections on the gates to the other system nodes, use inout gates for compactness, appropriate link is added as seen
    connections:
        host[0].gate[0] <--> FastCellularLink <--> can.gate[0];
        host[0].gate[1] <-->  FastCellularLink <--> anotherCan.gate[0];
        host[0].gate[2] <--> SlowCellularLink <--> cloud.gate[0];

        can.gate[1] <--> FastWiFiLink <--> cloud.gate[1];
        anotherCan.gate[1] <--> FastWiFiLink <--> cloud.gate[2];
}

