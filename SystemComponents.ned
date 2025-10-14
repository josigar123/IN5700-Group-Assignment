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
import inet.mobility.single.TurtleMobility;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

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

module MobileNode {
    
    parameters:
        double x;
        double y;
        double range = default(300);
        int numGates = default(3);
        @display("i=block/wheelbarrow");
        
    gates:
        inout gate[numGates];
        
    submodules:
        mobility: TurtleMobility;
        node: Node {
            x = parent.x;
            y = parent.y;
            range = parent.range;
            numGates = parent.numGates;
        }
        
    connections:
        for i=0..numGates-1 {
            gate[i] <--> node.gate[i];
        }
}

network GarbageCollectionSystem
{
    @display("bgb=3000,1250");

    @figure[outerroad](type=polyline; points=1700,200,150,200,150,1100,1700,1100; lineColor=black; lineWidth=4);
    @figure[innerroad](type=polyline; points=1700,450,400,450,400,850,1700,850; lineColor=black; lineWidth=4);
	
    submodules:
        visualizer: IntegratedCanvasVisualizer{
        	@display("p=1500,50");
        }
        host: MobileNode {
            x = 1750;
            y = 300;
            range = 100;
        }
        can: Node  {
            x = 500;
            y = 150;
            range = 320;
            numGates = 2;
            @display("i=block/bucket");
        }
        anotherCan: Node {
            x = 573.885;
            y = 754.65;
        	range = 320;
        	numGates = 2;
        	@display("i=block/bucket");
        }
        cloud: Node {
        	x = 1900;
        	y = 650;
        	range = 1600;
        	@display("i=device/server");
        }
	
    connections:
        host.gate[0] <--> can.gate[0];
        host.gate[1] <--> anotherCan.gate[0];
        host.gate[2] <--> cloud.gate[0];
        
        can.gate[1] <--> cloud.gate[1];
        anotherCan.gate[1] <--> cloud.gate[2];
}

